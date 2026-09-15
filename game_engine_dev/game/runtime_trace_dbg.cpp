//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "runtime_trace_dbg.h"
#include "trace_sink.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

//================================================================================================================================
//=> - TraceSink -
//================================================================================================================================

static FILE* g_fp = nullptr;
static bool g_auto_flush = false;

bool TraceSink::open (cstr path) {
    close();
    if (path == nullptr) {
        return false;
    }
    g_fp = std::fopen(path, "w");
    return g_fp != nullptr;
}

void TraceSink::close () {
    if (g_fp == nullptr) {
        return;
    }
    std::fclose(g_fp);
    g_fp = nullptr;
}

bool TraceSink::ok () {
    return g_fp != nullptr;
}

void TraceSink::set_auto_flush (bool on) {
    g_auto_flush = on;
}

void TraceSink::printf (cstr fmt, ...) {
    if (fmt == nullptr) {
        return;
    }
    FILE* out = (g_fp != nullptr) ? g_fp : stdout;
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(out, fmt, ap);
    va_end(ap);
    if (g_auto_flush || g_fp == nullptr) {
        std::fflush(out);
    }
}

void TraceSink::flush () {
    if (g_fp != nullptr) {
        std::fflush(g_fp);
    }
    std::fflush(stdout);
}

//================================================================================================================================
//=> - Printing functions -
//================================================================================================================================

#if defined(RUNTIME_TRACE_DBG)

void trace_setup(cstr label) {
    TraceSink::close();
    if (label != nullptr) {
        TraceSink::open(label);
    }
    #if defined(ENABLE_FLUSH_AFTER_PRINT)
        TraceSink::set_auto_flush(true);
    #endif
}

void trace_new_turn(u16 turn) {
    if (!TraceSink::ok()) {
        return;
    }
    TraceSink::printf("NEW_TURN:%u\n", turn);
}

void trace_explore_discover (u16 x, u16 y, u16 player) {
    if (!TraceSink::ok()) {
        return;
    }
    TraceSink::printf("EXPLORE_DISCOVER:%u:%u:%u\n", x, y, player);
}

void trace_path_failure (cstr msg) {
    if (!TraceSink::ok()) {
        return;
    }
    TraceSink::printf("PATH_FAILURE:%s\n", (msg != nullptr) ? msg : "");
}

#if defined(ENABLED_TRACE_P2P_MK4_OVL)

void trace_p2p_mk4_enter (u16 ux, u16 uy, u16 vx, u16 vy, u16 ct, u16 nt) {
    if (!TraceSink::ok()) {
        return;
    }
    TraceSink::printf("P2P_ENTER:%u:%u:%u:%u:%u:%u\n",
        static_cast<unsigned>(ux), static_cast<unsigned>(uy),
        static_cast<unsigned>(vx), static_cast<unsigned>(vy),
        static_cast<unsigned>(ct), static_cast<unsigned>(nt));
}

void trace_p2p_mk4_leave (u16 sx, u16 sy, u16 st) {
    if (!TraceSink::ok()) {
        return;
    }
    TraceSink::printf("P2P_LEAVE:%u:%u:%u\n",
        static_cast<unsigned>(sx), static_cast<unsigned>(sy),
        static_cast<unsigned>(st));
}

void trace_p2p_mk4_block (u16 vx, u16 vy, u16 d) {
    if (!TraceSink::ok()) {
        return;
    }
    TraceSink::printf("P2P_BLOCK:%u:%u:%u\n",
        static_cast<unsigned>(vx), static_cast<unsigned>(vy),
        static_cast<unsigned>(d));
}

void trace_p2p_mk4_skip (u16 sx, u16 sy, u16 st) {
    if (!TraceSink::ok()) {
        return;
    }
    TraceSink::printf("P2P_SKIP:%u:%u:%u\n",
        static_cast<unsigned>(sx), static_cast<unsigned>(sy),
        static_cast<unsigned>(st));
}

#endif

#if defined(ENABLED_TRACE_P2P_MK4_WALK)

void trace_p2p_mk4_walk_step (
    u16 ux,
    u16 uy,
    u16 vx,
    u16 vy,
    u16 cost,
    u16 mp,
    u32 turn) {
    if (!TraceSink::ok()) {
        return;
    }
    TraceSink::printf("P2P_MK4_WALK_STEP:%u:%u:%u:%u:%u:%u:%u\n",
        static_cast<unsigned>(ux), static_cast<unsigned>(uy),
        static_cast<unsigned>(vx), static_cast<unsigned>(vy),
        static_cast<unsigned>(cost), static_cast<unsigned>(mp),
        static_cast<unsigned>(turn));
}

void trace_p2p_mk4_walk_stall (u16 x, u16 y, u16 mp, u32 turn) {
    if (!TraceSink::ok()) {
        return;
    }
    TraceSink::printf("P2P_MK4_WALK_STALL:%u:%u:%u:%u\n",
        static_cast<unsigned>(x), static_cast<unsigned>(y),
        static_cast<unsigned>(mp), static_cast<unsigned>(turn));
}

void trace_p2p_mk4_walk_done (u16 x, u16 y, u32 steps, u32 turns) {
    if (!TraceSink::ok()) {
        return;
    }
    TraceSink::printf("P2P_MK4_WALK_DONE:%u:%u:%u:%u\n",
        static_cast<unsigned>(x), static_cast<unsigned>(y),
        static_cast<unsigned>(steps), static_cast<unsigned>(turns));
}

#endif

#if defined(ENABLED_TRACE_P2P_MK3_WALK)

void trace_p2p_mk3_walk_step (
    u16 ux,
    u16 uy,
    u16 vx,
    u16 vy,
    u16 cost,
    u16 mp,
    u32 turn) {
    if (!TraceSink::ok()) {
        return;
    }
    TraceSink::printf("P2P_MK3_WALK_STEP:%u:%u:%u:%u:%u:%u:%u\n",
        static_cast<unsigned>(ux), static_cast<unsigned>(uy),
        static_cast<unsigned>(vx), static_cast<unsigned>(vy),
        static_cast<unsigned>(cost), static_cast<unsigned>(mp),
        static_cast<unsigned>(turn));
}

void trace_p2p_mk3_walk_stall (u16 x, u16 y, u16 mp, u32 turn) {
    if (!TraceSink::ok()) {
        return;
    }
    TraceSink::printf("P2P_MK3_WALK_STALL:%u:%u:%u:%u\n",
        static_cast<unsigned>(x), static_cast<unsigned>(y),
        static_cast<unsigned>(mp), static_cast<unsigned>(turn));
}

void trace_p2p_mk3_walk_done (u16 x, u16 y, u32 steps, u32 turns) {
    if (!TraceSink::ok()) {
        return;
    }
    TraceSink::printf("P2P_MK3_WALK_DONE:%u:%u:%u:%u\n",
        static_cast<unsigned>(x), static_cast<unsigned>(y),
        static_cast<unsigned>(steps), static_cast<unsigned>(turns));
}

#endif

#if defined(ENABLED_MAP_ARRAY_ACCESS_CHK)

void check_map_array_access (u16 w, u16 h, u16 x, u16 y) {
    if (x < w && y < h) {
        return;
    }
    if (TraceSink::ok()) {
        TraceSink::printf("MAP_ARRAY_ACCESS:%u:%u:%u:%u\n",
            static_cast<unsigned>(w),
            static_cast<unsigned>(h),
            static_cast<unsigned>(x),
            static_cast<unsigned>(y));
    }
    std::fprintf(stderr, "map array access: (%u,%u) outside %ux%u\n",
        static_cast<unsigned>(x),
        static_cast<unsigned>(y),
        static_cast<unsigned>(w),
        static_cast<unsigned>(h));
    std::fflush(stdout);
    std::fflush(stderr);
    TraceSink::flush();
    std::abort();
}

#endif

// [INJECTION_TAG]

//================================================================================================================================
//=> - End of printing functions -
//================================================================================================================================

#endif // RUNTIME_TRACE_DBG

//================================================================================================================================
//=> - End -
//================================================================================================================================
