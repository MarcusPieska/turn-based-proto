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
#include "misc_unit_state_save_timeline_mng.h"
#include "runtime_static_loader.h"
#include "tech_age_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "misc-unit-state-save-timeline";
static const char* G_PLOT_PY = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/test_drivers/plot_misc_unit_state_save_timeline.py";
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

static bool wr_save_snap (
    cstr path,
    u32 turn,
    MiscUnitSeries ser,
    const MiscUnitStateSaveTimelineMng& mng,
    u16 save_i) {
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "# turn=%u players=%u\n", turn, static_cast<u32>(mng.player_n()));
    std::fprintf(fp, "seat n\n");
    for (u16 p = 0; p < mng.player_n(); ++p) {
        std::fprintf(fp, "%u %u\n", static_cast<u32>(p), mng.at(ser, p).at(save_i));
    }
    std::fclose(fp);
    return true;
}

static bool wr_lucky (cstr path, const MiscUnitStateSaveTimelineMng& mng) {
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "seat lucky\n");
    for (u16 p = 0; p < mng.player_n(); ++p) {
        std::fprintf(fp, "%u %u\n", static_cast<u32>(p), static_cast<u32>(mng.lucky(p)));
    }
    std::fclose(fp);
    return true;
}

static bool wr_all_saves (
    MiscUnitStateSaveTimelineMng& mng,
    const EvalPaths& paths,
    cstr data_dir,
    cstr eval_dir) {
    const u16 save_n = mng.save_n();
    for (u16 si = 0; si < save_n; ++si) {
        const u32 turn = paths.save_turn_at(si);
        char lost_p[512];
        char camp_p[512];
        if (std::snprintf(lost_p, sizeof(lost_p), "%s/save_%04u_lost.txt", data_dir, turn) <= 0
            || std::snprintf(camp_p, sizeof(camp_p), "%s/save_%04u_in_campaign.txt", data_dir, turn) <= 0) {
            return false;
        }
        if (!wr_save_snap(lost_p, turn, MiscUnitSeries::LostField, mng, si)
            || !wr_save_snap(camp_p, turn, MiscUnitSeries::CampNoWar, mng, si)) {
            return false;
        }
    }
    char lucky_p[512];
    if (std::snprintf(lucky_p, sizeof(lucky_p), "%s/lucky.txt", data_dir) <= 0 || !wr_lucky(lucky_p, mng)) {
        return false;
    }
    std::printf("wrote %u save pairs + %s\n", static_cast<u32>(save_n), lucky_p);
    char cmd[1024];
    if (std::snprintf(cmd, sizeof(cmd), "python3 '%s' '%s' '%s'", G_PLOT_PY, data_dir, eval_dir) <= 0) {
        return false;
    }
    std::fflush(stdout);
    return std::system(cmd) == 0;
}

//================================================================================================================================
//=> - MiscUnitStateSaveTimelineTester -
//================================================================================================================================

class MiscUnitStateSaveTimelineTester : public EvalDriver {
public:
    MiscUnitStateSaveTimelineTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

MiscUnitStateSaveTimelineTester::MiscUnitStateSaveTimelineTester ()
    : EvalDriver(make_need()) {
}

int MiscUnitStateSaveTimelineTester::run () {
    const u16 player_n = paths().players();
    const u16 save_n = paths().save_turn_n();
    if (player_n == 0 || save_n < 2) {
        std::printf("misc_unit_state_save_timeline: bad sizes players=%u saves=%u\n",
            static_cast<u32>(player_n), static_cast<u32>(save_n));
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("misc_unit_state_save_timeline: cannot load runtime statics\n");
        return 1;
    }
    if (!TechAgeMng::setup(loader.statics())) {
        std::printf("misc_unit_state_save_timeline: TechAgeMng::setup failed\n");
        return 1;
    }
    std::printf("misc_unit_state_save_timeline scanning %u saves (units+cities+players)\n",
        static_cast<u32>(save_n));
    MiscUnitStateSaveTimelineMng mng;
    if (!mng.setup(player_n, save_n) || !mng.fill(paths(), loader.statics())) {
        std::printf("misc_unit_state_save_timeline: setup/fill failed\n");
        TechAgeMng::clear();
        return 1;
    }
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("misc_unit_state_save_timeline: bad out paths\n");
        TechAgeMng::clear();
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(data_dir)) {
        std::printf("misc_unit_state_save_timeline: cannot mkdir under %s\n", paths().out_root());
        TechAgeMng::clear();
        return 1;
    }
    if (!wr_all_saves(mng, paths(), data_dir, eval_dir)) {
        std::printf("misc_unit_state_save_timeline: write/plot failed\n");
        TechAgeMng::clear();
        return 1;
    }
    TechAgeMng::clear();
    std::printf("misc_unit_state_save_timeline ok players=%u saves=%u\n",
        static_cast<u32>(player_n), static_cast<u32>(save_n));
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    MiscUnitStateSaveTimelineTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
