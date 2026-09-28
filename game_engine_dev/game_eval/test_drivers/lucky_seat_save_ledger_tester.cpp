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
#include "lucky_seat_save_ledger_mng.h"
#include "runtime_static_loader.h"
#include "tech_age_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "lucky-seat-save-ledger";
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
//=> - LuckySeatSaveLedgerTester -
//================================================================================================================================

class LuckySeatSaveLedgerTester : public EvalDriver {
public:
    LuckySeatSaveLedgerTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

LuckySeatSaveLedgerTester::LuckySeatSaveLedgerTester ()
    : EvalDriver(make_need()) {
}

int LuckySeatSaveLedgerTester::run () {
    const u16 player_n = paths().players();
    const u16 save_n = paths().save_turn_n();
    if (player_n == 0 || save_n < 2) {
        std::printf("lucky_seat_save_ledger: bad sizes players=%u saves=%u\n",
            static_cast<u32>(player_n), static_cast<u32>(save_n));
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("lucky_seat_save_ledger: cannot load runtime statics\n");
        return 1;
    }
    if (!TechAgeMng::setup(loader.statics())) {
        std::printf("lucky_seat_save_ledger: TechAgeMng::setup failed\n");
        return 1;
    }
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("lucky_seat_save_ledger: bad out paths\n");
        TechAgeMng::clear();
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(data_dir)) {
        std::printf("lucky_seat_save_ledger: cannot mkdir under %s\n", paths().out_root());
        TechAgeMng::clear();
        return 1;
    }
    std::printf("lucky_seat_save_ledger scanning %u saves (players+units+cities)\n",
        static_cast<u32>(save_n));
    LuckySeatSaveLedgerMng mng;
    if (!mng.setup(player_n, save_n) || !mng.fill(paths(), data_dir, loader.statics())) {
        std::printf("lucky_seat_save_ledger: setup/fill failed\n");
        TechAgeMng::clear();
        return 1;
    }
    TechAgeMng::clear();
    std::printf("lucky_seat_save_ledger ok players=%u saves=%u lucky=%u dir=%s\n",
        static_cast<u32>(player_n), static_cast<u32>(save_n),
        static_cast<u32>(mng.lucky_n()), data_dir);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    LuckySeatSaveLedgerTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
