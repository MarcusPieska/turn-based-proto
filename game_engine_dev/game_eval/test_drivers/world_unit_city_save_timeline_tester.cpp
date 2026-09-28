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
#include "world_unit_city_save_timeline_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "world-unit-city-save-timeline";
static const char* G_PLOT_PY =
    "/home/w/Projects/rts-proto/game_engine_dev/game_eval/test_drivers/plot_world_unit_city_save_timeline.py";

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

static bool wr_series (cstr path, const EvalPaths& paths, const WorldUnitCitySaveTimeline& tl) {
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "turn units cities\n");
    for (u16 si = 0; si < tl.save_n(); ++si) {
        std::fprintf(fp, "%u %u %u\n",
            paths.save_turn_at(si), tl.units_at(si), tl.cities_at(si));
    }
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - WorldUnitCitySaveTimelineTester -
//================================================================================================================================

class WorldUnitCitySaveTimelineTester : public EvalDriver {
public:
    WorldUnitCitySaveTimelineTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

WorldUnitCitySaveTimelineTester::WorldUnitCitySaveTimelineTester ()
    : EvalDriver(make_need()) {
}

int WorldUnitCitySaveTimelineTester::run () {
    const u16 save_n = paths().save_turn_n();
    if (save_n < 2) {
        std::printf("world_unit_city_save_timeline: bad saves=%u\n", static_cast<u32>(save_n));
        return 1;
    }
    std::printf("world_unit_city_save_timeline scanning %u saves\n", static_cast<u32>(save_n));
    WorldUnitCitySaveTimelineMng mng;
    if (!mng.setup(save_n) || !mng.fill(paths())) {
        std::printf("world_unit_city_save_timeline: setup/fill failed\n");
        return 1;
    }
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("world_unit_city_save_timeline: bad out paths\n");
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(data_dir)) {
        std::printf("world_unit_city_save_timeline: cannot mkdir under %s\n", paths().out_root());
        return 1;
    }
    char path[512];
    if (std::snprintf(path, sizeof(path), "%s/totals.txt", data_dir) <= 0) {
        return 1;
    }
    if (!wr_series(path, paths(), mng.tl())) {
        std::printf("world_unit_city_save_timeline: write failed %s\n", path);
        return 1;
    }
    std::printf("wrote %s\n", path);
    std::fflush(stdout);
    char cmd[1024];
    if (std::snprintf(cmd, sizeof(cmd), "python3 '%s' '%s' '%s'", G_PLOT_PY, data_dir, eval_dir) <= 0) {
        return 1;
    }
    const int rc = std::system(cmd);
    if (rc != 0) {
        std::printf("world_unit_city_save_timeline: plot script failed rc=%d\n", rc);
        return 1;
    }
    std::printf("world_unit_city_save_timeline ok saves=%u\n", static_cast<u32>(save_n));
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    WorldUnitCitySaveTimelineTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
