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
#include "log_dbg/log_sector_path_grads.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_PLOT_PY = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/test_drivers/plot_spg_log_timeline.py";
static const u32 G_PT_CAP = 8192u;
static const u16 G_INST_N = 2u;

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

static i16 inst_of (u32 seat, u32* map, u16* map_n) {
    for (u16 i = 0; i < *map_n; ++i) {
        if (map[i] == seat) {
            return static_cast<i16>(i);
        }
    }
    if (*map_n >= G_INST_N) {
        return -1;
    }
    map[*map_n] = seat;
    const i16 id = static_cast<i16>(*map_n);
    *map_n = static_cast<u16>(*map_n + 1u);
    return id;
}

static bool wr_curve (
    cstr path,
    const u32* turns,
    const u32* inst,
    const u32* chunks,
    const u32* wb,
    u32 n) {
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "turn inst chunks wb\n");
    for (u32 i = 0; i < n; ++i) {
        std::fprintf(fp, "%u %u %u %u\n", turns[i], inst[i], chunks[i], wb[i]);
    }
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - SpgLogTimelineTester -
//================================================================================================================================

class SpgLogTimelineTester : public EvalDriver {
public:
    SpgLogTimelineTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.trace();
    n.logs().m_sector_path_grads = 1;
    return n;
}

SpgLogTimelineTester::SpgLogTimelineTester ()
    : EvalDriver(make_need()) {
}

int SpgLogTimelineTester::run () {
    u32 turns[G_PT_CAP];
    u32 inst[G_PT_CAP];
    u32 chunks[G_PT_CAP];
    u32 wb[G_PT_CAP];
    u32 n = 0u;
    u32 seat_map[G_INST_N];
    u16 seat_n = 0u;
    const EvalLog& lg = log();
    for (u32 i = 0; i < lg.line_n(); ++i) {
        unsigned turn = 0u;
        unsigned seat = 0u;
        unsigned ch = 0u;
        unsigned boards = 0u;
        if (!LOG_SECTOR_PATH_GRADS::PARSE(lg.line(i), &turn, &seat, &ch, &boards)) {
            continue;
        }
        const i16 id = inst_of(seat, seat_map, &seat_n);
        if (id < 0) {
            continue;
        }
        if (n >= G_PT_CAP) {
            break;
        }
        turns[n] = turn;
        inst[n] = static_cast<u32>(id);
        chunks[n] = ch;
        wb[n] = boards;
        n = n + 1u;
    }
    if (n == 0u) {
        std::printf("spg_log_timeline: no spg lines in trace\n");
        return 1;
    }
    char eval_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))) {
        std::printf("spg_log_timeline: bad eval_dir\n");
        return 1;
    }
    if (!mkdir_deep(eval_dir)) {
        std::printf("spg_log_timeline: cannot mkdir %s\n", eval_dir);
        return 1;
    }
    char curve_p[512];
    if (std::snprintf(curve_p, sizeof(curve_p), "%s/spg_log_timeline.txt", eval_dir) <= 0) {
        return 1;
    }
    if (!wr_curve(curve_p, turns, inst, chunks, wb, n)) {
        std::printf("spg_log_timeline: write failed %s\n", curve_p);
        return 1;
    }
    std::printf("wrote %s points=%u seats=%u", curve_p, n, SC_U32(seat_n));
    for (u16 i = 0; i < seat_n; ++i) {
        std::printf(" inst%u=seat%u", SC_U32(i), seat_map[i]);
    }
    std::printf("\n");
    std::fflush(stdout);
    char cmd[1024];
    if (std::snprintf(cmd, sizeof(cmd), "python3 '%s' '%s'", G_PLOT_PY, eval_dir) <= 0) {
        return 1;
    }
    const int rc = std::system(cmd);
    if (rc != 0) {
        std::printf("spg_log_timeline: plot failed rc=%d\n", rc);
        return 1;
    }
    std::printf("spg_log_timeline ok points=%u\n", n);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    SpgLogTimelineTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
