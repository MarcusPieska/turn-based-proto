//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>

#include "eval_driver.h"
#include "eval_need.h"
#include "player_res_save_timeline_mng.h"
#include "resource_static_data.h"
#include "resource_static_key.h"
#include "runtime_static_loader.h"
#include "tech_age_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "player-res-save-timeline";
static const char* G_PLOT_PY =
    "/home/w/Projects/rts-proto/game_engine_dev/game_eval/test_drivers/plot_player_res_save_timeline.py";
static const char* G_RT_LIB =
    "/home/w/Projects/rts-proto/game_engine_dev/data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "/home/w/Projects/rts-proto/game_engine_dev/";
static const u16 G_CURVE_N = 10;

static u16 g_res_idx = U16_KEY_NULL;
static cstr g_res_name = nullptr;

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

static bool name_eq_ci (cstr a, cstr b) {
    if (a == nullptr || b == nullptr) {
        return false;
    }
    while (*a != 0 && *b != 0) {
        const int ca = std::tolower(static_cast<unsigned char>(*a));
        const int cb = std::tolower(static_cast<unsigned char>(*b));
        if (ca != cb) {
            return false;
        }
        ++a;
        ++b;
    }
    return *a == 0 && *b == 0;
}

static void print_res_names (const ResourceStaticData& rs) {
    const u16 n = rs.get_item_count();
    std::printf("resources (%u):\n", static_cast<u32>(n));
    for (u16 i = 0; i < n; ++i) {
        cstr nm = rs.get_name(ResourceStaticDataKey::from_raw(i));
        std::printf("  %s\n", nm != nullptr ? nm : "?");
    }
}

static u16 find_res (const ResourceStaticData& rs, cstr arg) {
    if (arg == nullptr || arg[0] == 0) {
        return U16_KEY_NULL;
    }
    const u16 n = rs.get_item_count();
    for (u16 i = 0; i < n; ++i) {
        cstr nm = rs.get_name(ResourceStaticDataKey::from_raw(i));
        if (name_eq_ci(nm, arg)) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static bool wr_curve (
    cstr path, u16 seat, u64 sum, cstr res_name, const EvalPaths& paths, const PlayerResSaveTimeline& tl) {
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "# seat=%u sum=%llu res=%s\n",
        static_cast<u32>(seat), static_cast<unsigned long long>(sum), res_name != nullptr ? res_name : "?");
    std::fprintf(fp, "turn amount\n");
    for (u16 si = 0; si < tl.save_n(); ++si) {
        std::fprintf(fp, "%u %u\n", paths.save_turn_at(si), tl.at(si));
    }
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - PlayerResSaveTimelineTester -
//================================================================================================================================

class PlayerResSaveTimelineTester : public EvalDriver {
public:
    PlayerResSaveTimelineTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

PlayerResSaveTimelineTester::PlayerResSaveTimelineTester ()
    : EvalDriver(make_need()) {
}

int PlayerResSaveTimelineTester::run () {
    const u16 player_n = paths().players();
    const u16 save_n = paths().save_turn_n();
    if (player_n == 0 || save_n < 2 || g_res_idx == U16_KEY_NULL || g_res_name == nullptr) {
        std::printf("player_res_save_timeline: bad sizes/res players=%u saves=%u res=%u\n",
            static_cast<u32>(player_n), static_cast<u32>(save_n), static_cast<u32>(g_res_idx));
        return 1;
    }
    std::printf("player_res_save_timeline scanning %u saves res=%s idx=%u (players only)\n",
        static_cast<u32>(save_n), g_res_name, static_cast<u32>(g_res_idx));
    PlayerResSaveTimelineMng mng;
    if (!mng.setup(player_n, save_n) || !mng.fill(paths(), g_res_idx)) {
        std::printf("player_res_save_timeline: setup/fill failed\n");
        return 1;
    }
    mng.sort();
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("player_res_save_timeline: bad out paths\n");
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(data_dir)) {
        std::printf("player_res_save_timeline: cannot mkdir under %s\n", paths().out_root());
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
        if (!wr_curve(path, mng.seat(idx), mng.at(idx).count(), g_res_name, paths(), mng.at(idx))) {
            std::printf("player_res_save_timeline: write failed %s\n", path);
            return 1;
        }
        std::printf("wrote %s seat=%u sum=%llu rank_idx=%u\n",
            path, static_cast<u32>(mng.seat(idx)),
            static_cast<unsigned long long>(mng.at(idx).count()), static_cast<u32>(idx));
    }
    std::fflush(stdout);
    char cmd[1024];
    if (std::snprintf(cmd, sizeof(cmd), "python3 '%s' '%s' '%s' '%s'",
            G_PLOT_PY, data_dir, eval_dir, g_res_name) <= 0) {
        return 1;
    }
    const int rc = std::system(cmd);
    if (rc != 0) {
        std::printf("player_res_save_timeline: plot script failed rc=%d\n", rc);
        return 1;
    }
    std::printf("player_res_save_timeline ok players=%u saves=%u curves=%u res=%s\n",
        static_cast<u32>(player_n), static_cast<u32>(save_n), static_cast<u32>(pick_n), g_res_name);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    if (argc < 2 || argv[1] == nullptr || argv[1][0] == 0) {
        std::printf("usage: player_res_save_timeline_tester <resource>\n");
        std::printf("example: player_res_save_timeline_tester iron\n");
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("player_res_save_timeline: cannot load runtime statics\n");
        return 1;
    }
    const ResourceStaticData& rs = loader.statics().resource();
    const u16 idx = find_res(rs, argv[1]);
    if (idx == U16_KEY_NULL) {
        std::printf("player_res_save_timeline: unknown resource '%s'\n", argv[1]);
        print_res_names(rs);
        return 1;
    }
    if (!TechAgeMng::setup(loader.statics())) {
        std::printf("player_res_save_timeline: TechAgeMng::setup failed\n");
        return 1;
    }
    g_res_idx = idx;
    g_res_name = rs.get_name(ResourceStaticDataKey::from_raw(idx));
    PlayerResSaveTimelineTester t;
    const int rc = t.go(G_PATHS);
    TechAgeMng::clear();
    return rc;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
