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
#include "map_road_save_timeline_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "map-road-save-timeline";
static const char* G_PLOT_PY =
    "/home/w/Projects/rts-proto/game_engine_dev/game_eval/test_drivers/plot_map_road_save_timeline.py";

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
    cstr path, cstr label, u32 count, const EvalPaths& paths, const MapRoadSaveTimeline& tl) {
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "# series=%s count=%u\n", label, count);
    std::fprintf(fp, "turn count\n");
    for (u16 si = 0; si < tl.save_n(); ++si) {
        std::fprintf(fp, "%u %u\n", paths.save_turn_at(si), tl.at(si));
    }
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - MapRoadSaveTimelineTester -
//================================================================================================================================

class MapRoadSaveTimelineTester : public EvalDriver {
public:
    MapRoadSaveTimelineTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

MapRoadSaveTimelineTester::MapRoadSaveTimelineTester ()
    : EvalDriver(make_need()) {
}

int MapRoadSaveTimelineTester::run () {
    const u16 save_n = paths().save_turn_n();
    if (save_n < 2) {
        std::printf("map_road_save_timeline: bad sizes saves=%u\n", static_cast<u32>(save_n));
        return 1;
    }
    std::printf("map_road_save_timeline scanning %u saves (map only, one at a time)\n",
        static_cast<u32>(save_n));
    MapRoadSaveTimelineMng mng;
    if (!mng.setup(save_n) || !mng.fill(paths())) {
        std::printf("map_road_save_timeline: setup/fill failed\n");
        return 1;
    }
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("map_road_save_timeline: bad out paths\n");
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(data_dir)) {
        std::printf("map_road_save_timeline: cannot mkdir under %s\n", paths().out_root());
        return 1;
    }
    for (u16 k = 0; k < mng.series_n(); ++k) {
        char path[512];
        if (std::snprintf(path, sizeof(path), "%s/curve_%02u.txt", data_dir, static_cast<u32>(k)) <= 0) {
            return 1;
        }
        if (!wr_curve(path, mng.label(k), mng.at(k).count(), paths(), mng.at(k))) {
            std::printf("map_road_save_timeline: write failed %s\n", path);
            return 1;
        }
        std::printf("wrote %s series=%s count=%u\n",
            path, mng.label(k), mng.at(k).count());
    }
    std::fflush(stdout);
    char cmd[1024];
    if (std::snprintf(cmd, sizeof(cmd), "python3 '%s' '%s' '%s'", G_PLOT_PY, data_dir, eval_dir) <= 0) {
        return 1;
    }
    const int rc = std::system(cmd);
    if (rc != 0) {
        std::printf("map_road_save_timeline: plot script failed rc=%d\n", rc);
        return 1;
    }
    std::printf("map_road_save_timeline ok saves=%u series=%u\n",
        static_cast<u32>(save_n), static_cast<u32>(mng.series_n()));
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    MapRoadSaveTimelineTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
