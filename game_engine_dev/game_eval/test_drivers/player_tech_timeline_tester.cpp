//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>

#include "eval_driver.h"
#include "eval_need.h"
#include "player_tech_timeline_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "player-tech-timeline";
static const char* G_PLOT_PY =
    "/home/w/Projects/rts-proto/game_engine_dev/game_eval/test_drivers/plot_player_tech_timeline.py";
static const u16 G_CURVE_N = 10;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool mkdir_p (cstr path) {
    if (path == nullptr) {
        return false;
    }
    return ::mkdir(path, 0755) == 0 || errno == EEXIST;
}

static bool mkdir_deep (cstr path) {
    if (path == nullptr || path[0] == 0) {
        return false;
    }
    char tmp[512];
    if (std::snprintf(tmp, sizeof(tmp), "%s", path) <= 0) {
        return false;
    }
    const u32 n = static_cast<u32>(std::strlen(tmp));
    for (u32 i = 1; i < n; ++i) {
        if (tmp[i] != '/') {
            continue;
        }
        tmp[i] = 0;
        if (!mkdir_p(tmp)) {
            return false;
        }
        tmp[i] = '/';
    }
    return mkdir_p(tmp);
}

static u16 probe_tech_n (const EvalLog& log) {
    u16 mx = 0;
    bool any = false;
    for (u32 i = 0; i < log.line_n(); ++i) {
        u16 player = 0;
        u16 tech = 0;
        if (!log.parse_tech_discover(i, &player, &tech)) {
            continue;
        }
        any = true;
        if (tech > mx) {
            mx = tech;
        }
    }
    if (!any || mx == UINT16_MAX) {
        return 0;
    }
    return static_cast<u16>(mx + 1u);
}

static bool wr_curve (cstr path, u16 seat, u16 count, const u16* y, u16 turn_n) {
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "# seat=%u count=%u\n", static_cast<u32>(seat), static_cast<u32>(count));
    std::fprintf(fp, "turn unlocks\n");
    for (u16 t = 0; t < turn_n; ++t) {
        std::fprintf(fp, "%u %u\n", static_cast<u32>(t), static_cast<u32>(y[t]));
    }
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - PlayerTechTimelineTester -
//================================================================================================================================

class PlayerTechTimelineTester : public EvalDriver {
public:
    PlayerTechTimelineTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.logs().m_new_turn = 1;
    n.logs().m_player_tech_discover = 1;
    return n;
}

PlayerTechTimelineTester::PlayerTechTimelineTester ()
    : EvalDriver(make_need()) {
}

int PlayerTechTimelineTester::run () {
    const u16 player_n = paths().players();
    const u16 tech_n = probe_tech_n(log());
    if (player_n == 0 || tech_n == 0) {
        std::printf("player_tech_timeline: bad sizes players=%u tech_n=%u\n",
            static_cast<u32>(player_n), static_cast<u32>(tech_n));
        return 1;
    }
    PlayerTechTimelineMng mng;
    if (!mng.setup(player_n, tech_n) || !mng.fill(log())) {
        std::printf("player_tech_timeline: setup/fill failed\n");
        return 1;
    }
    mng.sort();
    const u16 turn_n = mng.turn_n();
    if (turn_n == 0 || turn_n == UINT16_MAX) {
        std::printf("player_tech_timeline: bad turn_n\n");
        return 1;
    }
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("player_tech_timeline: bad out paths\n");
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(data_dir)) {
        std::printf("player_tech_timeline: cannot mkdir eval/data under %s\n", paths().out_root());
        return 1;
    }
    u16* y = new u16[turn_n];
    const u16 pick_n = (mng.player_n() < G_CURVE_N) ? mng.player_n() : G_CURVE_N;
    for (u16 k = 0; k < pick_n; ++k) {
        u16 idx = 0;
        if (pick_n == 1) {
            idx = 0;
        } else {
            idx = static_cast<u16>((static_cast<u32>(k) * (mng.player_n() - 1u)) / (pick_n - 1u));
        }
        if (!mng.at(idx).cum(y, turn_n)) {
            delete[] y;
            std::printf("player_tech_timeline: cum failed idx=%u\n", static_cast<u32>(idx));
            return 1;
        }
        char path[512];
        if (std::snprintf(path, sizeof(path), "%s/curve_%02u.txt", data_dir, static_cast<u32>(k)) <= 0) {
            delete[] y;
            return 1;
        }
        if (!wr_curve(path, mng.seat(idx), mng.at(idx).count(), y, turn_n)) {
            delete[] y;
            std::printf("player_tech_timeline: write failed %s\n", path);
            return 1;
        }
        std::printf("wrote %s seat=%u count=%u rank_idx=%u\n",
            path, static_cast<u32>(mng.seat(idx)), static_cast<u32>(mng.at(idx).count()),
            static_cast<u32>(idx));
    }
    delete[] y;
    std::fflush(stdout);
    char cmd[1024];
    if (std::snprintf(cmd, sizeof(cmd), "python3 '%s' '%s' '%s'", G_PLOT_PY, data_dir, eval_dir) <= 0) {
        return 1;
    }
    const int rc = std::system(cmd);
    if (rc != 0) {
        std::printf("player_tech_timeline: plot script failed rc=%d\n", rc);
        return 1;
    }
    std::printf("player_tech_timeline ok players=%u tech_n=%u turn_n=%u curves=%u\n",
        static_cast<u32>(player_n), static_cast<u32>(tech_n), static_cast<u32>(turn_n),
        static_cast<u32>(pick_n));
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    PlayerTechTimelineTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
