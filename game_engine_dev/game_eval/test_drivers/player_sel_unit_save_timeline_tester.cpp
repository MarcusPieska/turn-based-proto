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
#include "player_sel_unit_save_timeline_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "player-sel-unit-save-timeline";
static const char* G_PLOT_PY =
    "/home/w/Projects/rts-proto/game_engine_dev/game_eval/test_drivers/plot_player_sel_unit_save_timeline.py";
static const u16 G_CURVE_N = 10;
static const u16 G_WORKER_TYP = 1;

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

static bool wr_curve (
    cstr path, u16 seat, u32 count, u16 unit_typ, const EvalPaths& paths, const PlayerSelUnitSaveTimeline& tl) {
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "# seat=%u count=%u unit_typ=%u\n",
        static_cast<u32>(seat), count, static_cast<u32>(unit_typ));
    std::fprintf(fp, "turn units\n");
    for (u16 si = 0; si < tl.save_n(); ++si) {
        std::fprintf(fp, "%u %u\n", paths.save_turn_at(si), tl.at(si));
    }
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - PlayerSelUnitSaveTimelineTester -
//================================================================================================================================

class PlayerSelUnitSaveTimelineTester : public EvalDriver {
public:
    PlayerSelUnitSaveTimelineTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

PlayerSelUnitSaveTimelineTester::PlayerSelUnitSaveTimelineTester ()
    : EvalDriver(make_need()) {
}

int PlayerSelUnitSaveTimelineTester::run () {
    const u16 player_n = paths().players();
    const u16 save_n = paths().save_turn_n();
    if (player_n == 0 || save_n < 2) {
        std::printf("player_sel_unit_save_timeline: bad sizes players=%u saves=%u\n",
            static_cast<u32>(player_n), static_cast<u32>(save_n));
        return 1;
    }
    std::printf("player_sel_unit_save_timeline scanning %u saves typ=%u (units only)\n",
        static_cast<u32>(save_n), static_cast<u32>(G_WORKER_TYP));
    PlayerSelUnitSaveTimelineMng mng;
    if (!mng.setup(player_n, save_n) || !mng.fill(paths(), G_WORKER_TYP)) {
        std::printf("player_sel_unit_save_timeline: setup/fill failed\n");
        return 1;
    }
    mng.sort();
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("player_sel_unit_save_timeline: bad out paths\n");
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(data_dir)) {
        std::printf("player_sel_unit_save_timeline: cannot mkdir under %s\n", paths().out_root());
        return 1;
    }
    const u16 pick_n = (mng.player_n() < G_CURVE_N) ? mng.player_n() : G_CURVE_N;
    for (u16 k = 0; k < pick_n; ++k) {
        u16 idx = 0;
        if (pick_n == 1) {
            idx = 0;
        } else {
            idx = static_cast<u16>((static_cast<u32>(k) * (mng.player_n() - 1u)) / (pick_n - 1u));
        }
        char path[512];
        if (std::snprintf(path, sizeof(path), "%s/curve_%02u.txt", data_dir, static_cast<u32>(k)) <= 0) {
            return 1;
        }
        if (!wr_curve(path, mng.seat(idx), mng.at(idx).count(), mng.unit_typ(), paths(), mng.at(idx))) {
            std::printf("player_sel_unit_save_timeline: write failed %s\n", path);
            return 1;
        }
        std::printf("wrote %s seat=%u count=%u rank_idx=%u\n",
            path, static_cast<u32>(mng.seat(idx)), mng.at(idx).count(), static_cast<u32>(idx));
    }
    std::fflush(stdout);
    char cmd[1024];
    if (std::snprintf(cmd, sizeof(cmd), "python3 '%s' '%s' '%s'", G_PLOT_PY, data_dir, eval_dir) <= 0) {
        return 1;
    }
    const int rc = std::system(cmd);
    if (rc != 0) {
        std::printf("player_sel_unit_save_timeline: plot script failed rc=%d\n", rc);
        return 1;
    }
    std::printf("player_sel_unit_save_timeline ok players=%u saves=%u curves=%u typ=%u\n",
        static_cast<u32>(player_n), static_cast<u32>(save_n), static_cast<u32>(pick_n),
        static_cast<u32>(mng.unit_typ()));
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    PlayerSelUnitSaveTimelineTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
