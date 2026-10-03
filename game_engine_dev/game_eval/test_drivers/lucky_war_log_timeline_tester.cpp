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
#include "lucky_war_log_timeline_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "lucky-war-log-timeline";
static const char* G_PLOT_PY =
    "/home/w/Projects/rts-proto/game_engine_dev/game_eval/test_drivers/plot_lucky_war_log_timeline.py";
static const u16 G_SEAT_MAX = 10;

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

static char kind_ch (LuckyWarEv k) {
    switch (k) {
    case LuckyWarEv::Start:
        return 'S';
    case LuckyWarEv::End:
        return 'E';
    case LuckyWarEv::Muster:
        return 'M';
    case LuckyWarEv::Capture:
        return 'C';
    case LuckyWarEv::Army:
        return 'A';
    case LuckyWarEv::Loss:
        return 'L';
    case LuckyWarEv::Targets:
        return 'T';
    case LuckyWarEv::FormFail:
        return 'F';
    case LuckyWarEv::DeclFail:
        return 'D';
    case LuckyWarEv::AssaultFail:
        return 'X';
    default:
        return '?';
    }
}

static bool wr_seat (cstr path, u16 seat, const LuckyWarLogTimeline& tl) {
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "# seat=%u events=%u\n", static_cast<u32>(seat), static_cast<u32>(tl.n()));
    std::fprintf(fp, "turn kind a b\n");
    for (u16 i = 0; i < tl.n(); ++i) {
        const LuckyWarEvt& e = tl.at(i);
        std::fprintf(fp, "%u %c %u %u\n",
            static_cast<u32>(e.m_turn),
            kind_ch(e.m_kind),
            static_cast<u32>(e.m_a),
            static_cast<u32>(e.m_b));
    }
    std::fclose(fp);
    return true;
}

static void wr_ev_line (std::FILE* fp, const LuckyWarEvt& e) {
    const u32 t = static_cast<u32>(e.m_turn);
    const u32 a = static_cast<u32>(e.m_a);
    const u32 b = static_cast<u32>(e.m_b);
    switch (e.m_kind) {
    case LuckyWarEv::Start:
        std::fprintf(fp, "  t%-4u war enemy=%u\n", t, a);
        break;
    case LuckyWarEv::End:
        std::fprintf(fp, "  t%-4u peace enemy=%u\n", t, a);
        break;
    case LuckyWarEv::Muster:
        std::fprintf(fp, "  t%-4u muster units=%u size=%u\n", t, a, b);
        break;
    case LuckyWarEv::Capture:
        std::fprintf(fp, "  t%-4u capture city=(%u,%u)\n", t, a, b);
        break;
    case LuckyWarEv::Army:
        std::fprintf(fp, "  t%-4u army units=%u size=%u\n", t, a, b);
        break;
    case LuckyWarEv::Loss:
        std::fprintf(fp, "  t%-4u loss units=%u\n", t, a);
        break;
    case LuckyWarEv::Targets:
        std::fprintf(fp, "  t%-4u targets n=%u\n", t, a);
        break;
    case LuckyWarEv::FormFail:
        std::fprintf(fp, "  t%-4u form_fail enemy=%u\n", t, a);
        break;
    case LuckyWarEv::DeclFail:
        std::fprintf(fp, "  t%-4u decl_fail reason=%u enemy=%u\n", t, a, b);
        break;
    case LuckyWarEv::AssaultFail:
        std::fprintf(fp, "  t%-4u assault_fail city=(%u,%u)\n", t, a, b);
        break;
    default:
        std::fprintf(fp, "  t%-4u ? a=%u b=%u\n", t, a, b);
        break;
    }
}

static bool wr_digest (cstr path, const LuckyWarLogTimelineMng& mng, u16 pick_n) {
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "lucky_war_log_digest max_turn=%u seats=%u\n",
        static_cast<u32>(mng.max_turn()), static_cast<u32>(pick_n));
    for (u16 i = 0; i < pick_n; ++i) {
        const LuckyWarLogTimeline& tl = mng.at(i);
        std::fprintf(fp, "\nseat=%u events=%u\n",
            static_cast<u32>(mng.seat(i)), static_cast<u32>(tl.n()));
        for (u16 j = 0; j < tl.n(); ++j) {
            wr_ev_line(fp, tl.at(j));
        }
    }
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - LuckyWarLogTimelineTester -
//================================================================================================================================

class LuckyWarLogTimelineTester : public EvalDriver {
public:
    LuckyWarLogTimelineTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.trace();
    n.logs().m_war_muster = 1;
    n.logs().m_war_peace = 1;
    n.logs().m_war_peace_mock = 1;
    n.logs().m_war_army_size = 1;
    n.logs().m_war_city_capture = 1;
    n.logs().m_war_army_cant_fight = 1;
    n.logs().m_war_form_army_fail = 1;
    n.logs().m_war_declare_fail = 1;
    n.logs().m_war_assault_fail_stop = 1;
    return n;
}

LuckyWarLogTimelineTester::LuckyWarLogTimelineTester ()
    : EvalDriver(make_need()) {
}

int LuckyWarLogTimelineTester::run () {
    LuckyWarLogTimelineMng mng;
    if (!mng.fill(log())) {
        std::printf("lucky_war_log_timeline: no war seats in trace\n");
        return 1;
    }
    mng.sort();
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("lucky_war_log_timeline: bad out paths\n");
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(data_dir)) {
        std::printf("lucky_war_log_timeline: cannot mkdir\n");
        return 1;
    }
    {
        char meta[512];
        if (std::snprintf(meta, sizeof(meta), "%s/meta.txt", data_dir) > 0) {
            std::FILE* fp = std::fopen(meta, "w");
            if (fp != nullptr) {
                std::fprintf(fp, "max_turn=%u\n", static_cast<u32>(mng.max_turn()));
                std::fprintf(fp, "seat_n=%u\n", static_cast<u32>(mng.seat_n()));
                std::fclose(fp);
            }
        }
    }
    const u16 pick_n = (mng.seat_n() < G_SEAT_MAX) ? mng.seat_n() : G_SEAT_MAX;
    for (u16 i = 0; i < pick_n; ++i) {
        char path[512];
        if (std::snprintf(path, sizeof(path), "%s/seat_%02u.txt", data_dir, static_cast<u32>(i)) <= 0) {
            return 1;
        }
        if (!wr_seat(path, mng.seat(i), mng.at(i))) {
            std::printf("lucky_war_log_timeline: write failed %s\n", path);
            return 1;
        }
        std::printf("wrote %s seat=%u events=%u\n",
            path, static_cast<u32>(mng.seat(i)), static_cast<u32>(mng.at(i).n()));
    }
    {
        char dig[512];
        if (std::snprintf(dig, sizeof(dig), "%s/timeline_digest.txt", data_dir) <= 0
            || !wr_digest(dig, mng, pick_n)) {
            std::printf("lucky_war_log_timeline: digest write failed\n");
            return 1;
        }
        std::printf("wrote %s\n", dig);
        char dig2[512];
        if (std::snprintf(dig2, sizeof(dig2), "%s/lucky_war_log_timeline.txt", eval_dir) > 0) {
            (void)wr_digest(dig2, mng, pick_n);
            std::printf("wrote %s\n", dig2);
        }
    }
    std::fflush(stdout);
    char cmd[1024];
    if (std::snprintf(cmd, sizeof(cmd), "python3 '%s' '%s' '%s'", G_PLOT_PY, data_dir, eval_dir) <= 0) {
        return 1;
    }
    const int rc = std::system(cmd);
    if (rc != 0) {
        std::printf("lucky_war_log_timeline: plot failed rc=%d\n", rc);
        return 1;
    }
    std::printf("lucky_war_log_timeline ok seats=%u max_turn=%u\n",
        static_cast<u32>(pick_n), static_cast<u32>(mng.max_turn()));
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    LuckyWarLogTimelineTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
