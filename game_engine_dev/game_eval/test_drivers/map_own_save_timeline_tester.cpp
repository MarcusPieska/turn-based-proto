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
#include "map_own_save_timeline_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "map-own-save-timeline";

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

//================================================================================================================================
//=> - MapOwnSaveTimelineTester -
//================================================================================================================================

class MapOwnSaveTimelineTester : public EvalDriver {
public:
    MapOwnSaveTimelineTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

MapOwnSaveTimelineTester::MapOwnSaveTimelineTester ()
    : EvalDriver(make_need()) {
}

int MapOwnSaveTimelineTester::run () {
    const u16 save_n = paths().save_turn_n();
    if (save_n < 2) {
        std::printf("map_own_save_timeline: bad sizes saves=%u\n", static_cast<u32>(save_n));
        return 1;
    }
    std::printf("map_own_save_timeline writing %u ownership snaps (map only)\n",
        static_cast<u32>(save_n));
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("map_own_save_timeline: bad out paths\n");
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(data_dir)) {
        std::printf("map_own_save_timeline: cannot mkdir under %s\n", paths().out_root());
        return 1;
    }
    MapOwnSaveTimelineMng mng;
    if (!mng.setup(save_n) || !mng.fill(paths(), data_dir)) {
        std::printf("map_own_save_timeline: setup/fill failed\n");
        return 1;
    }
    std::printf("map_own_save_timeline ok saves=%u wrote=%u dir=%s\n",
        static_cast<u32>(save_n), static_cast<u32>(mng.wrote_n()), data_dir);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    MapOwnSaveTimelineTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
