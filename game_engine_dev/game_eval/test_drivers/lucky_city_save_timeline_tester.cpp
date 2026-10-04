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
#include "lucky_city_save_timeline_mng.h"
#include "runtime_static_loader.h"
#include "tech_age_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "lucky-city-save-timeline";
static const char* G_PLOT_PY =
    "/home/w/Projects/rts-proto/game_engine_dev/game_eval/test_drivers/plot_lucky_city_save_timeline.py";
static const char* G_RT_LIB = "/home/w/Projects/rts-proto/game_engine_dev/data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "/home/w/Projects/rts-proto/game_engine_dev/";

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
    cstr path, u16 seat, u32 count, const EvalPaths& paths, const LuckyCitySaveTimeline& tl) {
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "# seat=%u cities=%u\n", static_cast<u32>(seat), count);
    std::fprintf(fp, "turn cities\n");
    for (u16 si = 0; si < tl.save_n(); ++si) {
        std::fprintf(fp, "%u %u\n", paths.save_turn_at(si), tl.at(si));
    }
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - LuckyCitySaveTimelineTester -
//================================================================================================================================

class LuckyCitySaveTimelineTester : public EvalDriver {
public:
    LuckyCitySaveTimelineTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

LuckyCitySaveTimelineTester::LuckyCitySaveTimelineTester ()
    : EvalDriver(make_need()) {
}

int LuckyCitySaveTimelineTester::run () {
    const u16 player_n = paths().players();
    const u16 save_n = paths().save_turn_n();
    if (player_n == 0 || save_n < 2) {
        std::printf("lucky_city_save_timeline: bad sizes players=%u saves=%u\n",
            static_cast<u32>(player_n), static_cast<u32>(save_n));
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("lucky_city_save_timeline: cannot load runtime statics\n");
        return 1;
    }
    if (!TechAgeMng::setup(loader.statics())) {
        std::printf("lucky_city_save_timeline: TechAgeMng::setup failed\n");
        return 1;
    }
    std::printf("lucky_city_save_timeline scanning %u saves (players+cities)\n",
        static_cast<u32>(save_n));
    LuckyCitySaveTimelineMng mng;
    if (!mng.setup(player_n, save_n) || !mng.fill(paths())) {
        std::printf("lucky_city_save_timeline: setup/fill failed\n");
        TechAgeMng::clear();
        return 1;
    }
    mng.sort();
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("lucky_city_save_timeline: bad out paths\n");
        TechAgeMng::clear();
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(data_dir)) {
        std::printf("lucky_city_save_timeline: cannot mkdir under %s\n", paths().out_root());
        TechAgeMng::clear();
        return 1;
    }
    for (u16 k = 0; k < mng.lucky_n(); ++k) {
        char path[512];
        if (std::snprintf(path, sizeof(path), "%s/curve_%02u.txt", data_dir, static_cast<u32>(k)) <= 0) {
            TechAgeMng::clear();
            return 1;
        }
        if (!wr_curve(path, mng.seat(k), mng.at(k).count(), paths(), mng.at(k))) {
            std::printf("lucky_city_save_timeline: write failed %s\n", path);
            TechAgeMng::clear();
            return 1;
        }
        std::printf("wrote %s seat=%u cities=%u\n",
            path, static_cast<u32>(mng.seat(k)), mng.at(k).count());
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
        std::printf("lucky_city_save_timeline: plot script failed rc=%d\n", rc);
        return 1;
    }
    std::printf("lucky_city_save_timeline ok lucky=%u saves=%u\n",
        static_cast<u32>(mng.lucky_n()), static_cast<u32>(save_n));
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    LuckyCitySaveTimelineTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
