//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_tracer.h"

#ifdef CITY_TRACER_ENABLE

#include <cstdio>
#include <cstdlib>
#include <cstring>

//================================================================================================================================
//=> - Limits -
//================================================================================================================================

static const u16 k_name_n = 64u;
static const u32 k_line_n = 256u;
static const u32 k_buf_init = 1u << 20;

//================================================================================================================================
//=> - Scratch line -
//================================================================================================================================

struct CityLine {
    u16 m_city; // City index for this line
    u16 m_owner; // Player owner index
    u32 m_turn; // Game turn for this line
    u16 m_pop; // Population at commit
    i16 m_san; // Net sanitation at commit
    u16 m_food; // Food yield added this turn
    u16 m_prod; // Production yield added this turn
    u16 m_com; // Commerce yield added this turn
    u16 m_cult; // Culture yield added this turn
    u16 m_sci; // Science yield added this turn
    u16 m_rel; // Religion yield added this turn
    u8 m_start; // 1 if a build/train was started
    u8 m_finish; // 1 if a build/train finished
    char m_name[k_name_n]; // Catalog name; Wealth for accumulate; empty if none
};

//================================================================================================================================
//=> - Module state -
//================================================================================================================================

static FILE* g_fp = nullptr;
static char* g_buf = nullptr;
static u32 g_buf_n = 0;
static u32 g_buf_cap = 0;
static CityLine g_line = {};
static u8 g_have = 0;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void clr_line () {
    g_line.m_city = 0;
    g_line.m_turn = 0;
    g_line.m_pop = 0;
    g_line.m_san = 0;
    g_line.m_food = 0;
    g_line.m_prod = 0;
    g_line.m_com = 0;
    g_line.m_cult = 0;
    g_line.m_sci = 0;
    g_line.m_rel = 0;
    g_line.m_start = 0;
    g_line.m_finish = 0;
    g_line.m_name[0] = '\0';
    g_have = 0;
}

static bool grow_buf (u32 need) {
    if (g_buf_n + need <= g_buf_cap) {
        return true;
    }
    u32 cap = g_buf_cap == 0 ? k_buf_init : g_buf_cap;
    while (cap < g_buf_n + need) {
        cap = cap << 1;
    }
    char* nbuf = static_cast<char*>(std::realloc(g_buf, cap));
    if (nbuf == nullptr) {
        return false;
    }
    g_buf = nbuf;
    g_buf_cap = cap;
    return true;
}

static void set_name (cstr name) {
    if (name == nullptr || name[0] == '\0') {
        return;
    }
    std::snprintf(g_line.m_name, k_name_n, "%s", name);
}

//================================================================================================================================
//=> - CityTracer -
//================================================================================================================================

void CityTracer::setup (cstr path) {
    clear();
    if (path == nullptr || path[0] == '\0') {
        return;
    }
    g_fp = std::fopen(path, "w");
    if (g_fp == nullptr) {
        return;
    }
    std::fprintf(g_fp, "# city:owner:turn:pop:san:food:prod:com:cult:sci:rel:name:start:finish\n");
    std::fflush(g_fp);
}

void CityTracer::clear () {
    if (g_fp != nullptr) {
        std::fclose(g_fp);
        g_fp = nullptr;
    }
    std::free(g_buf);
    g_buf = nullptr;
    g_buf_n = 0;
    g_buf_cap = 0;
    clr_line();
}

void CityTracer::begin_city (u16 city_idx, u16 owner, u32 turn) {
    clr_line();
    g_line.m_city = city_idx;
    g_line.m_owner = owner;
    g_line.m_turn = turn;
    g_have = 1;
}

void CityTracer::log_pop (u16 pop) {
    if (g_have == 0) {
        return;
    }
    g_line.m_pop = pop;
}

void CityTracer::log_san (i16 san) {
    if (g_have == 0) {
        return;
    }
    g_line.m_san = san;
}

void CityTracer::log_food (u16 yield) {
    if (g_have == 0) {
        return;
    }
    g_line.m_food = yield;
}

void CityTracer::log_prod (u16 yield) {
    if (g_have == 0) {
        return;
    }
    g_line.m_prod = yield;
}

void CityTracer::log_com (u16 yield) {
    if (g_have == 0) {
        return;
    }
    g_line.m_com = yield;
}

void CityTracer::log_cult (u16 yield) {
    if (g_have == 0) {
        return;
    }
    g_line.m_cult = yield;
}

void CityTracer::log_sci (u16 yield) {
    if (g_have == 0) {
        return;
    }
    g_line.m_sci = yield;
}

void CityTracer::log_rel (u16 yield) {
    if (g_have == 0) {
        return;
    }
    g_line.m_rel = yield;
}

void CityTracer::log_build (cstr name, u8 start, u8 finish) {
    if (g_have == 0) {
        return;
    }
    if (finish != 0) {
        g_line.m_finish = 1;
        set_name(name);
    }
    if (start != 0) {
        g_line.m_start = 1;
        if (g_line.m_finish == 0) {
            set_name(name);
        }
    }
}

void CityTracer::commit () {
    if (g_have == 0) {
        return;
    }
    char line[k_line_n];
    const int n = std::snprintf(
        line, sizeof(line),
        "%u:%u:%u:%u:%d:%u:%u:%u:%u:%u:%u:%s:%u:%u\n",
        static_cast<unsigned>(g_line.m_city),
        static_cast<unsigned>(g_line.m_owner),
        g_line.m_turn,
        static_cast<unsigned>(g_line.m_pop),
        static_cast<int>(g_line.m_san),
        static_cast<unsigned>(g_line.m_food),
        static_cast<unsigned>(g_line.m_prod),
        static_cast<unsigned>(g_line.m_com),
        static_cast<unsigned>(g_line.m_cult),
        static_cast<unsigned>(g_line.m_sci),
        static_cast<unsigned>(g_line.m_rel),
        g_line.m_name,
        static_cast<unsigned>(g_line.m_start),
        static_cast<unsigned>(g_line.m_finish)
    );
    clr_line();
    if (n <= 0) {
        return;
    }
    const u32 len = static_cast<u32>(n);
    if (!grow_buf(len)) {
        return;
    }
    std::memcpy(g_buf + g_buf_n, line, len);
    g_buf_n = g_buf_n + len;
}

void CityTracer::flush_turn () {
    if (g_fp == nullptr || g_buf_n == 0) {
        g_buf_n = 0;
        return;
    }
    std::fwrite(g_buf, 1, g_buf_n, g_fp);
    std::fflush(g_fp);
    g_buf_n = 0;
}

#endif

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
