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
#include "map_own_lucky_end_mng.h"
#include "runtime_static_loader.h"
#include "tech_age_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "map-own-lucky-end";
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

//================================================================================================================================
//=> - MapOwnLuckyEndTester -
//================================================================================================================================

class MapOwnLuckyEndTester : public EvalDriver {
public:
    MapOwnLuckyEndTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

MapOwnLuckyEndTester::MapOwnLuckyEndTester ()
    : EvalDriver(make_need()) {
}

int MapOwnLuckyEndTester::run () {
    const u16 save_n = paths().save_turn_n();
    const u16 player_n = paths().players();
    if (save_n < 1 || player_n == 0) {
        std::printf("map_own_lucky_end: bad sizes saves=%u players=%u\n",
            static_cast<u32>(save_n), static_cast<u32>(player_n));
        return 1;
    }
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("map_own_lucky_end: bad out paths\n");
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(data_dir)) {
        std::printf("map_own_lucky_end: cannot mkdir under %s\n", paths().out_root());
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("map_own_lucky_end: cannot load runtime statics\n");
        return 1;
    }
    if (!TechAgeMng::setup(loader.statics())) {
        std::printf("map_own_lucky_end: TechAgeMng::setup failed\n");
        return 1;
    }
    MapOwnLuckyEndMng mng;
    if (!mng.setup(player_n, save_n) || !mng.fill(paths(), data_dir)) {
        std::printf("map_own_lucky_end: setup/fill failed\n");
        TechAgeMng::clear();
        return 1;
    }
    TechAgeMng::clear();
    std::printf("map_own_lucky_end ok lucky=%u wrote=%u dir=%s\n",
        static_cast<u32>(mng.lucky_n()), static_cast<u32>(mng.wrote_n()), data_dir);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    MapOwnLuckyEndTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
