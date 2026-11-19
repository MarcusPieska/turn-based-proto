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
#include "lucky_army_prod_save_timeline_mng.h"
#include "runtime_static_loader.h"
#include "tech_age_mng.h"
#include "unit_utility_helper.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "lucky-army-prod-save-timeline";
static const char* G_PLOT_PY =
    "/home/w/Projects/rts-proto/game_engine_dev/game_eval/test_drivers/plot_lucky_army_prod_save_timeline.py";
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

static bool wr_curve (cstr path, u16 seat, const LuckyArmyProdSaveTimelineMng& mng, u16 li) {
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    u32 sum = 0u;
    for (u16 ii = 0; ii < mng.interval_n(); ++ii) {
        sum = sum + mng.new_n(li, ii);
    }
    std::fprintf(fp, "# seat=%u sum=%u\n", static_cast<u32>(seat), sum);
    std::fprintf(fp, "turn_end new_n\n");
    for (u16 ii = 0; ii < mng.interval_n(); ++ii) {
        std::fprintf(fp, "%u %u\n", mng.turn_end(ii), mng.new_n(li, ii));
    }
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - LuckyArmyProdSaveTimelineTester -
//================================================================================================================================

class LuckyArmyProdSaveTimelineTester : public EvalDriver {
public:
    LuckyArmyProdSaveTimelineTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

LuckyArmyProdSaveTimelineTester::LuckyArmyProdSaveTimelineTester ()
    : EvalDriver(make_need()) {
}

int LuckyArmyProdSaveTimelineTester::run () {
    const u16 player_n = paths().players();
    const u16 save_n = paths().save_turn_n();
    if (player_n == 0u || save_n < 2u) {
        std::printf("lucky_army_prod_save_timeline: bad sizes players=%u saves=%u\n",
            static_cast<u32>(player_n), static_cast<u32>(save_n));
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("lucky_army_prod_save_timeline: cannot load runtime statics\n");
        return 1;
    }
    if (!TechAgeMng::setup(loader.statics())) {
        std::printf("lucky_army_prod_save_timeline: TechAgeMng::setup failed\n");
        return 1;
    }
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("lucky_army_prod_save_timeline: bad out paths\n");
        TechAgeMng::clear();
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(data_dir)) {
        std::printf("lucky_army_prod_save_timeline: cannot mkdir under %s\n", paths().out_root());
        TechAgeMng::clear();
        return 1;
    }
    std::printf("lucky_army_prod_save_timeline scanning %u saves (units+players)\n",
        static_cast<u32>(save_n));
    LuckyArmyProdSaveTimelineMng mng;
    if (!mng.setup(player_n, save_n) || !mng.fill(paths(), loader.statics())) {
        std::printf("lucky_army_prod_save_timeline: setup/fill failed\n");
        UnitUtilityHelper::clear();
        TechAgeMng::clear();
        return 1;
    }
    UnitUtilityHelper::clear();
    TechAgeMng::clear();
    for (u16 li = 0; li < mng.lucky_n(); ++li) {
        const u16 seat = mng.seat(li);
        char path[512];
        if (std::snprintf(path, sizeof(path), "%s/curve_%03u.txt", data_dir, static_cast<u32>(seat)) <= 0) {
            return 1;
        }
        if (!wr_curve(path, seat, mng, li)) {
            std::printf("lucky_army_prod_save_timeline: write failed %s\n", path);
            return 1;
        }
        std::printf("wrote %s seat=%u\n", path, static_cast<u32>(seat));
    }
    std::fflush(stdout);
    char cmd[1024];
    if (std::snprintf(cmd, sizeof(cmd), "python3 '%s' '%s' '%s'", G_PLOT_PY, data_dir, eval_dir) <= 0) {
        return 1;
    }
    const int rc = std::system(cmd);
    if (rc != 0) {
        std::printf("lucky_army_prod_save_timeline: plot script failed rc=%d\n", rc);
        return 1;
    }
    std::printf("lucky_army_prod_save_timeline ok players=%u saves=%u lucky=%u iv=%u\n",
        static_cast<u32>(player_n), static_cast<u32>(save_n),
        static_cast<u32>(mng.lucky_n()), static_cast<u32>(mng.interval_n()));
    return 0; 
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    LuckyArmyProdSaveTimelineTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
