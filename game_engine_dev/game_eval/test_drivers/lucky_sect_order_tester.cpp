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
#include "lucky_sect_order_mng.h"
#include "runtime_static_loader.h"
#include "tech_age_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "lucky-sect-order";
static const char* G_RT_LIB = "/home/w/Projects/rts-proto/game_engine_dev/data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "/home/w/Projects/rts-proto/game_engine_dev/";
static const char* G_ANNO = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/test_drivers/annotate_map.py";

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

static bool run_annotate (cstr img, cstr anno, cstr out) {
    char cmd[1536];
    if (std::snprintf(cmd, sizeof(cmd),
            "python3 \"%s\" \"%s\" \"%s\" \"%s\" --font-size 12",
            G_ANNO, img, anno, out) <= 0) {
        return false;
    }
    return std::system(cmd) == 0;
}

//================================================================================================================================
//=> - LuckySectOrderTester -
//================================================================================================================================

class LuckySectOrderTester : public EvalDriver {
public:
    LuckySectOrderTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.trace();
    n.save_seq();
    n.logs().m_war_sect_begin = 1;
    n.logs().m_war_sect_select = 1;
    n.logs().m_war_sect_skip = 1;
    n.logs().m_war_sect_exhaust = 1;
    n.logs().m_war_sect_fallback = 1;
    n.logs().m_war_sect_done = 1;
    return n;
}

LuckySectOrderTester::LuckySectOrderTester ()
    : EvalDriver(make_need()) {
}

int LuckySectOrderTester::run () {
    const u16 save_n = paths().save_turn_n();
    const u16 player_n = paths().players();
    if (save_n < 1 || player_n == 0) {
        std::printf("lucky_sect_order: bad sizes saves=%u players=%u\n",
            static_cast<u32>(save_n), static_cast<u32>(player_n));
        return 1;
    }
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("lucky_sect_order: bad out paths\n");
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(data_dir)) {
        std::printf("lucky_sect_order: cannot mkdir under %s\n", paths().out_root());
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("lucky_sect_order: cannot load runtime statics\n");
        return 1;
    }
    if (!TechAgeMng::setup(loader.statics())) {
        std::printf("lucky_sect_order: TechAgeMng::setup failed\n");
        return 1;
    }
    LuckySectOrderMng mng;
    if (!mng.setup(player_n, save_n) || !mng.fill(paths(), log(), data_dir)) {
        std::printf("lucky_sect_order: setup/fill failed\n");
        TechAgeMng::clear();
        return 1;
    }
    char ppm_p[512];
    char anno_p[512];
    char png_p[512];
    for (u16 i = 0; i < mng.lucky_n(); ++i) {
        const u16 seat = mng.lucky_at(i);
        if (std::snprintf(ppm_p, sizeof(ppm_p), "%s/lucky_sect_order_seat_%03u.ppm",
                data_dir, static_cast<u32>(seat)) <= 0
            || std::snprintf(anno_p, sizeof(anno_p), "%s/anno_seat_%03u.txt",
                data_dir, static_cast<u32>(seat)) <= 0
            || std::snprintf(png_p, sizeof(png_p), "%s/lucky_sect_order_seat_%03u.png",
                data_dir, static_cast<u32>(seat)) <= 0) {
            TechAgeMng::clear();
            return 1;
        }
        if (!run_annotate(ppm_p, anno_p, png_p)) {
            std::printf("lucky_sect_order: annotate failed seat=%u\n", static_cast<u32>(seat));
            TechAgeMng::clear();
            return 1;
        }
        std::remove(ppm_p);
        std::remove(anno_p);
    }
    TechAgeMng::clear();
    std::printf("lucky_sect_order ok lucky=%u wrote=%u dir=%s\n",
        static_cast<u32>(mng.lucky_n()), static_cast<u32>(mng.wrote_n()), data_dir);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    LuckySectOrderTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
