//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "runtime_trace_dbg.h"

#if defined(RUNTIME_TRACE_DBG)

#include <cstdio>
#include <cstdlib>

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static FILE* s_trace_file = nullptr;

//================================================================================================================================
//=> - Printing functions -
//================================================================================================================================

void trace_setup(cstr label) {
    if (s_trace_file != nullptr) {
        std::fclose(s_trace_file);
        s_trace_file = nullptr;
    }
    if (label != nullptr) {
        s_trace_file = std::fopen(label, "w");
    }
}

void trace_city_foundation(u16 x, u16 y, u16 player) {
    if (s_trace_file == nullptr) {
        return;
    }
    std::fprintf(s_trace_file, "CITY_FOUNDATION:%u:%u:%u\n", x, y, player);
    #if defined(ENABLE_FLUSH_AFTER_PRINT)
        std::fflush(s_trace_file);
    #endif
}

void trace_civ_spawn_pt (u16 x, u16 y, u16 civ_idx) {
    if (s_trace_file == nullptr) {
        return;
    }
    std::fprintf(s_trace_file, "CIV_SPAWN_PT:%u:%u:%u\n", x, y, civ_idx);
    #if defined(ENABLE_FLUSH_AFTER_PRINT)
        std::fflush(s_trace_file);
    #endif
}

void trace_unit_spawn (u16 typ_idx, u16 civ_idx, u16 x, u16 y) {
    if (s_trace_file == nullptr) {
        return;
    }
    std::fprintf(s_trace_file, "UNIT_SPAWN:%u:%u:%u:%u\n", typ_idx, civ_idx, x, y);
    #if defined(ENABLE_FLUSH_AFTER_PRINT)
        std::fflush(s_trace_file);
    #endif
}

void trace_new_turn(u16 turn) {
    if (s_trace_file == nullptr) {
        return;
    }
    std::fprintf(s_trace_file, "NEW_TURN:%u\n", turn);
    #if defined(ENABLE_FLUSH_AFTER_PRINT)
        std::fflush(s_trace_file);
    #endif
}

void trace_explore_discover (u16 x, u16 y, u16 player) {
    if (s_trace_file == nullptr) {
        return;
    }
    std::fprintf(s_trace_file, "EXPLORE_DISCOVER:%u:%u:%u\n", x, y, player);
    #if defined(ENABLE_FLUSH_AFTER_PRINT)
        std::fflush(s_trace_file);
    #endif
}

void trace_path_failure (cstr msg) {
    if (s_trace_file == nullptr) {
        return;
    }
    std::fprintf(s_trace_file, "PATH_FAILURE:%s\n", (msg != nullptr) ? msg : "");
    #if defined(ENABLE_FLUSH_AFTER_PRINT)
        std::fflush(s_trace_file);
    #endif
}

#if defined(ENABLED_TRACE_P2P_MK4_OVL)

void trace_p2p_mk4_enter (u16 ux, u16 uy, u16 vx, u16 vy, u16 ct, u16 nt) {
    if (s_trace_file == nullptr) {
        return;
    }
    std::fprintf(s_trace_file, "P2P_ENTER:%u:%u:%u:%u:%u:%u\n",
        static_cast<unsigned>(ux), static_cast<unsigned>(uy),
        static_cast<unsigned>(vx), static_cast<unsigned>(vy),
        static_cast<unsigned>(ct), static_cast<unsigned>(nt));
    #if defined(ENABLE_FLUSH_AFTER_PRINT)
        std::fflush(s_trace_file);
    #endif
}

void trace_p2p_mk4_leave (u16 sx, u16 sy, u16 st) {
    if (s_trace_file == nullptr) {
        return;
    }
    std::fprintf(s_trace_file, "P2P_LEAVE:%u:%u:%u\n",
        static_cast<unsigned>(sx), static_cast<unsigned>(sy),
        static_cast<unsigned>(st));
    #if defined(ENABLE_FLUSH_AFTER_PRINT)
        std::fflush(s_trace_file);
    #endif
}

void trace_p2p_mk4_block (u16 vx, u16 vy, u16 d) {
    if (s_trace_file == nullptr) {
        return;
    }
    std::fprintf(s_trace_file, "P2P_BLOCK:%u:%u:%u\n",
        static_cast<unsigned>(vx), static_cast<unsigned>(vy),
        static_cast<unsigned>(d));
    #if defined(ENABLE_FLUSH_AFTER_PRINT)
        std::fflush(s_trace_file);
    #endif
}

void trace_p2p_mk4_skip (u16 sx, u16 sy, u16 st) {
    if (s_trace_file == nullptr) {
        return;
    }
    std::fprintf(s_trace_file, "P2P_SKIP:%u:%u:%u\n",
        static_cast<unsigned>(sx), static_cast<unsigned>(sy),
        static_cast<unsigned>(st));
    #if defined(ENABLE_FLUSH_AFTER_PRINT)
        std::fflush(s_trace_file);
    #endif
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
    if (s_trace_file == nullptr) {
        return;
    }
    std::fprintf(s_trace_file, "P2P_MK4_WALK_STEP:%u:%u:%u:%u:%u:%u:%u\n",
        static_cast<unsigned>(ux), static_cast<unsigned>(uy),
        static_cast<unsigned>(vx), static_cast<unsigned>(vy),
        static_cast<unsigned>(cost), static_cast<unsigned>(mp),
        static_cast<unsigned>(turn));
    #if defined(ENABLE_FLUSH_AFTER_PRINT)
        std::fflush(s_trace_file);
    #endif
}

void trace_p2p_mk4_walk_stall (u16 x, u16 y, u16 mp, u32 turn) {
    if (s_trace_file == nullptr) {
        return;
    }
    std::fprintf(s_trace_file, "P2P_MK4_WALK_STALL:%u:%u:%u:%u\n",
        static_cast<unsigned>(x), static_cast<unsigned>(y),
        static_cast<unsigned>(mp), static_cast<unsigned>(turn));
    #if defined(ENABLE_FLUSH_AFTER_PRINT)
        std::fflush(s_trace_file);
    #endif
}

void trace_p2p_mk4_walk_done (u16 x, u16 y, u32 steps, u32 turns) {
    if (s_trace_file == nullptr) {
        return;
    }
    std::fprintf(s_trace_file, "P2P_MK4_WALK_DONE:%u:%u:%u:%u\n",
        static_cast<unsigned>(x), static_cast<unsigned>(y),
        static_cast<unsigned>(steps), static_cast<unsigned>(turns));
    #if defined(ENABLE_FLUSH_AFTER_PRINT)
        std::fflush(s_trace_file);
    #endif
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
    if (s_trace_file == nullptr) {
        return;
    }
    std::fprintf(s_trace_file, "P2P_MK3_WALK_STEP:%u:%u:%u:%u:%u:%u:%u\n",
        static_cast<unsigned>(ux), static_cast<unsigned>(uy),
        static_cast<unsigned>(vx), static_cast<unsigned>(vy),
        static_cast<unsigned>(cost), static_cast<unsigned>(mp),
        static_cast<unsigned>(turn));
    #if defined(ENABLE_FLUSH_AFTER_PRINT)
        std::fflush(s_trace_file);
    #endif
}

void trace_p2p_mk3_walk_stall (u16 x, u16 y, u16 mp, u32 turn) {
    if (s_trace_file == nullptr) {
        return;
    }
    std::fprintf(s_trace_file, "P2P_MK3_WALK_STALL:%u:%u:%u:%u\n",
        static_cast<unsigned>(x), static_cast<unsigned>(y),
        static_cast<unsigned>(mp), static_cast<unsigned>(turn));
    #if defined(ENABLE_FLUSH_AFTER_PRINT)
        std::fflush(s_trace_file);
    #endif
}

void trace_p2p_mk3_walk_done (u16 x, u16 y, u32 steps, u32 turns) {
    if (s_trace_file == nullptr) {
        return;
    }
    std::fprintf(s_trace_file, "P2P_MK3_WALK_DONE:%u:%u:%u:%u\n",
        static_cast<unsigned>(x), static_cast<unsigned>(y),
        static_cast<unsigned>(steps), static_cast<unsigned>(turns));
    #if defined(ENABLE_FLUSH_AFTER_PRINT)
        std::fflush(s_trace_file);
    #endif
}

#endif

#if defined(ENABLED_MAP_ARRAY_ACCESS_CHK)

void check_map_array_access (u16 w, u16 h, u16 x, u16 y) {
    if (x < w && y < h) {
        return;
    }
    if (s_trace_file != nullptr) {
        std::fprintf(s_trace_file, "MAP_ARRAY_ACCESS:%u:%u:%u:%u\n",
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
    if (s_trace_file != nullptr) {
        std::fflush(s_trace_file);
    }
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

