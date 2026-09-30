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
#include "player_land_size_save_timeline_mng.h"
#include "runtime_static_loader.h"
#include "tech_age_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "player-land-size-save-timeline";
static const char* G_PLOT_PY =
    "/home/w/Projects/rts-proto/game_engine_dev/game_eval/test_drivers/plot_player_land_size_save_timeline.py";
static const char* G_RT_LIB = "/home/w/Projects/rts-proto/game_engine_dev/data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "/home/w/Projects/rts-proto/game_engine_dev/";
static const u16 G_CURVE_N = 5;

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
    cstr path, u16 seat, u32 count, const EvalPaths& paths, const PlayerLandSizeSaveTimeline& tl) {
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "# seat=%u land=%u\n", static_cast<u32>(seat), count);
    std::fprintf(fp, "turn avg_size max_size\n");
    for (u16 si = 0; si < tl.save_n(); ++si) {
        const double avg = static_cast<double>(tl.avg_milli_at(si)) / 1000.0;
        std::fprintf(fp, "%u %.3f %u\n",
            paths.save_turn_at(si), avg, static_cast<u32>(tl.max_at(si)));
    }
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - PlayerLandSizeSaveTimelineTester -
//================================================================================================================================

class PlayerLandSizeSaveTimelineTester : public EvalDriver {
public:
    PlayerLandSizeSaveTimelineTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

PlayerLandSizeSaveTimelineTester::PlayerLandSizeSaveTimelineTester ()
    : EvalDriver(make_need()) {
}

int PlayerLandSizeSaveTimelineTester::run () {
    const u16 player_n = paths().players();
    const u16 save_n = paths().save_turn_n();
    if (player_n == 0 || save_n < 2) {
        std::printf("player_land_size_save_timeline: bad sizes players=%u saves=%u\n",
            static_cast<u32>(player_n), static_cast<u32>(save_n));
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("player_land_size_save_timeline: cannot load runtime statics\n");
        return 1;
    }
    if (!TechAgeMng::setup(loader.statics())) {
        std::printf("player_land_size_save_timeline: TechAgeMng::setup failed\n");
        return 1;
    }
    std::printf("player_land_size_save_timeline scanning %u saves (units+players)\n",
        static_cast<u32>(save_n));
    PlayerLandSizeSaveTimelineMng mng;
    if (!mng.setup(player_n, save_n) || !mng.fill(paths(), loader.statics())) {
        std::printf("player_land_size_save_timeline: setup/fill failed\n");
        TechAgeMng::clear();
        return 1;
    }
    mng.sort();
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("player_land_size_save_timeline: bad out paths\n");
        TechAgeMng::clear();
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(data_dir)) {
        std::printf("player_land_size_save_timeline: cannot mkdir under %s\n", paths().out_root());
        TechAgeMng::clear();
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
            TechAgeMng::clear();
            return 1;
        }
        if (!wr_curve(path, mng.seat(idx), mng.at(idx).count(), paths(), mng.at(idx))) {
            std::printf("player_land_size_save_timeline: write failed %s\n", path);
            TechAgeMng::clear();
            return 1;
        }
        std::printf("wrote %s seat=%u land=%u rank_idx=%u\n",
            path, static_cast<u32>(mng.seat(idx)), mng.at(idx).count(), static_cast<u32>(idx));
    }
    std::fflush(stdout);
    char cmd[1024];
    if (std::snprintf(cmd, sizeof(cmd), "python3 '%s' '%s' '%s'", G_PLOT_PY, data_dir, eval_dir) <= 0) {
        TechAgeMng::clear();
        return 1;
    }
    const int rc = std::system(cmd);
    TechAgeMng::clear();
    if (rc != 0) {
        std::printf("player_land_size_save_timeline: plot script failed rc=%d\n", rc);
        return 1;
    }
    std::printf("player_land_size_save_timeline ok players=%u saves=%u curves=%u\n",
        static_cast<u32>(player_n), static_cast<u32>(save_n), static_cast<u32>(pick_n));
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    PlayerLandSizeSaveTimelineTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
