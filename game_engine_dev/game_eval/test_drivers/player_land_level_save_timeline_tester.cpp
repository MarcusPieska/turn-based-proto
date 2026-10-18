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
#include "player_land_level_save_timeline_mng.h"
#include "runtime_static_loader.h"
#include "tech_age_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "player-land-level-save-timeline";
static const char* G_PLOT_PY =
    "/home/w/Projects/rts-proto/game_engine_dev/game_eval/test_drivers/plot_player_land_level_save_timeline.py";
static const char* G_RT_LIB = "/home/w/Projects/rts-proto/game_engine_dev/data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "/home/w/Projects/rts-proto/game_engine_dev/";
static const u16 G_FIELD_N = 10;

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
    cstr path, u16 seat, u32 count, const EvalPaths& paths, const PlayerLandLevelSaveTimeline& tl) {
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "# seat=%u land=%u\n", static_cast<u32>(seat), count);
    std::fprintf(fp, "turn avg_level max_level\n");
    for (u16 si = 0; si < tl.save_n(); ++si) {
        const double avg = static_cast<double>(tl.avg_milli_at(si)) / 1000.0;
        std::fprintf(fp, "%u %.3f %u\n",
            paths.save_turn_at(si), avg, static_cast<u32>(tl.max_at(si)));
    }
    std::fclose(fp);
    return true;
}

static void pr_avg_tbl (
    cstr title, const EvalPaths& paths, const PlayerLandLevelSaveTimeline* const* tls, const u16* seats, u16 n) {
    if (n == 0 || tls == nullptr || seats == nullptr) {
        return;
    }
    std::printf("\n%s (avg)\n", title);
    std::printf("%4s", "turn");
    for (u16 k = 0; k < n; ++k) {
        std::printf("  s%02u", static_cast<u32>(seats[k]));
    }
    std::printf("\n");
    const u16 sn = tls[0]->save_n();
    for (u16 si = 0; si < sn; ++si) {
        std::printf("%4u", paths.save_turn_at(si));
        for (u16 k = 0; k < n; ++k) {
            const double avg = static_cast<double>(tls[k]->avg_milli_at(si)) / 1000.0;
            std::printf(" %5.2f", avg);
        }
        std::printf("\n");
    }
}

static void pr_max_tbl (
    cstr title, const EvalPaths& paths, const PlayerLandLevelSaveTimeline* const* tls, const u16* seats, u16 n) {
    if (n == 0 || tls == nullptr || seats == nullptr) {
        return;
    }
    std::printf("\n%s (max)\n", title);
    std::printf("%4s", "turn");
    for (u16 k = 0; k < n; ++k) {
        std::printf("  s%02u", static_cast<u32>(seats[k]));
    }
    std::printf("\n");
    const u16 sn = tls[0]->save_n();
    for (u16 si = 0; si < sn; ++si) {
        std::printf("%4u", paths.save_turn_at(si));
        for (u16 k = 0; k < n; ++k) {
            std::printf(" %5.2f", static_cast<double>(tls[k]->max_at(si)));
        }
        std::printf("\n");
    }
}

static void pr_final_dist (
    cstr title, const PlayerLandLevelSaveTimelineMng& mng, const u16* seats, u16 n) {
    if (n == 0 || seats == nullptr || mng.lvl_n() == 0u) {
        return;
    }
    std::printf("\n%s final level dist\n", title);
    const u16 ln = mng.lvl_n();
    for (u16 k = 0; k < n; ++k) {
        const u16 seat = seats[k];
        std::printf("Seat %u: [", static_cast<u32>(seat));
        for (u16 lv = 0; lv < ln; ++lv) {
            if (lv > 0u) {
                std::printf(", ");
            }
            std::printf("%u", mng.final_lvl_count(seat, lv));
        }
        std::printf("]\n");
    }
}

//================================================================================================================================
//=> - PlayerLandLevelSaveTimelineTester -
//================================================================================================================================

class PlayerLandLevelSaveTimelineTester : public EvalDriver {
public:
    PlayerLandLevelSaveTimelineTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

PlayerLandLevelSaveTimelineTester::PlayerLandLevelSaveTimelineTester ()
    : EvalDriver(make_need()) {
}

int PlayerLandLevelSaveTimelineTester::run () {
    const u16 player_n = paths().players();
    const u16 save_n = paths().save_turn_n();
    if (player_n == 0 || save_n < 2) {
        std::printf("player_land_level_save_timeline: bad sizes players=%u saves=%u\n",
            static_cast<u32>(player_n), static_cast<u32>(save_n));
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("player_land_level_save_timeline: cannot load runtime statics\n");
        return 1;
    }
    if (!TechAgeMng::setup(loader.statics())) {
        std::printf("player_land_level_save_timeline: TechAgeMng::setup failed\n");
        return 1;
    }
    std::printf("player_land_level_save_timeline scanning %u saves (units+players)\n",
        static_cast<u32>(save_n));
    PlayerLandLevelSaveTimelineMng mng;
    if (!mng.setup(player_n, save_n) || !mng.fill(paths(), loader.statics())) {
        std::printf("player_land_level_save_timeline: setup/fill failed\n");
        TechAgeMng::clear();
        return 1;
    }
    mng.sort();
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("player_land_level_save_timeline: bad out paths\n");
        TechAgeMng::clear();
        return 1;
    }
    char field_dir[512];
    char lucky_dir[512];
    if (std::snprintf(field_dir, sizeof(field_dir), "%s/field", data_dir) <= 0
        || std::snprintf(lucky_dir, sizeof(lucky_dir), "%s/lucky", data_dir) <= 0) {
        TechAgeMng::clear();
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(field_dir) || !mkdir_deep(lucky_dir)) {
        std::printf("player_land_level_save_timeline: cannot mkdir under %s\n", paths().out_root());
        TechAgeMng::clear();
        return 1;
    }

    const u16 pick_n = (mng.player_n() < G_FIELD_N) ? mng.player_n() : G_FIELD_N;
    const PlayerLandLevelSaveTimeline* field_tl[G_FIELD_N];
    u16 field_seat[G_FIELD_N];
    for (u16 k = 0; k < pick_n; ++k) {
        u16 idx = 0;
        if (pick_n == 1) {
            idx = 0;
        } else {
            idx = static_cast<u16>((static_cast<u32>(k) * (mng.player_n() - 1u)) / (pick_n - 1u));
        }
        char path[512];
        if (std::snprintf(path, sizeof(path), "%s/curve_%02u.txt", field_dir, static_cast<u32>(k)) <= 0) {
            TechAgeMng::clear();
            return 1;
        }
        if (!wr_curve(path, mng.seat(idx), mng.at(idx).count(), paths(), mng.at(idx))) {
            std::printf("player_land_level_save_timeline: write failed %s\n", path);
            TechAgeMng::clear();
            return 1;
        }
        field_tl[k] = &mng.at(idx);
        field_seat[k] = mng.seat(idx);
        std::printf("field wrote %s seat=%u land=%u rank_idx=%u\n",
            path, static_cast<u32>(mng.seat(idx)), mng.at(idx).count(), static_cast<u32>(idx));
    }

    const PlayerLandLevelSaveTimeline* lucky_tl[G_FIELD_N];
    u16 lucky_seat[G_FIELD_N];
    u16 lucky_w = 0;
    u16 lucky_tbl_n = 0;
    for (u16 i = 0; i < mng.player_n(); ++i) {
        const u16 seat = mng.seat(i);
        if (mng.lucky(seat) == 0u) {
            continue;
        }
        char path[512];
        if (std::snprintf(path, sizeof(path), "%s/curve_%02u.txt", lucky_dir, static_cast<u32>(lucky_w)) <= 0) {
            TechAgeMng::clear();
            return 1;
        }
        if (!wr_curve(path, seat, mng.at(i).count(), paths(), mng.at(i))) {
            std::printf("player_land_level_save_timeline: write failed %s\n", path);
            TechAgeMng::clear();
            return 1;
        }
        if (lucky_tbl_n < G_FIELD_N) {
            lucky_tl[lucky_tbl_n] = &mng.at(i);
            lucky_seat[lucky_tbl_n] = seat;
            lucky_tbl_n = static_cast<u16>(lucky_tbl_n + 1u);
        }
        std::printf("lucky wrote %s seat=%u land=%u\n",
            path, static_cast<u32>(seat), mng.at(i).count());
        lucky_w = static_cast<u16>(lucky_w + 1u);
    }
    if (lucky_w == 0u) {
        std::printf("player_land_level_save_timeline: no lucky seats\n");
        TechAgeMng::clear();
        return 1;
    }

    pr_avg_tbl("field", paths(), field_tl, field_seat, pick_n);
    pr_max_tbl("field", paths(), field_tl, field_seat, pick_n);
    pr_final_dist("field", mng, field_seat, pick_n);
    pr_avg_tbl("lucky", paths(), lucky_tl, lucky_seat, lucky_tbl_n);
    pr_max_tbl("lucky", paths(), lucky_tl, lucky_seat, lucky_tbl_n);
    pr_final_dist("lucky", mng, lucky_seat, lucky_tbl_n);

    std::fflush(stdout);
    char cmd[1024];
    if (std::snprintf(cmd, sizeof(cmd), "python3 '%s' '%s' '%s'", G_PLOT_PY, data_dir, eval_dir) <= 0) {
        TechAgeMng::clear();
        return 1;
    }
    const int rc = std::system(cmd);
    TechAgeMng::clear();
    if (rc != 0) {
        std::printf("player_land_level_save_timeline: plot script failed rc=%d\n", rc);
        return 1;
    }
    std::printf("player_land_level_save_timeline ok players=%u saves=%u field=%u lucky=%u\n",
        static_cast<u32>(player_n), static_cast<u32>(save_n),
        static_cast<u32>(pick_n), static_cast<u32>(lucky_w));
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    PlayerLandLevelSaveTimelineTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
