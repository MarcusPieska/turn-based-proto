//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "lucky_sect_order_mng.h"

#include <cstdio>
#include <cstring>

#include "derived_statics_io.h"
#include "eval_log.h"
#include "eval_paths.h"
#include "game_array_simple.h"
#include "game_io.h"
#include "game_state.h"
#include "log_dbg/log_war_sect_begin.h"
#include "log_dbg/log_war_sect_exhaust.h"
#include "log_dbg/log_war_sect_fallback.h"
#include "log_dbg/log_war_sect_select.h"
#include "log_dbg/log_war_sect_skip.h"
#include "lucky_sect_order_snap.h"
#include "tech_age_mng.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void free_seats (PlayerState*& seats, u16& n) {
    if (seats == nullptr) {
        n = 0;
        return;
    }
    for (u16 i = 0; i < n; ++i) {
        delete[] seats[i].m_small_wonder_city;
        seats[i].m_small_wonder_city = nullptr;
        delete seats[i].m_explored_overlay;
        seats[i].m_explored_overlay = nullptr;
        delete seats[i].m_techs_researched;
        seats[i].m_techs_researched = nullptr;
        delete seats[i].m_civics_adopted;
        seats[i].m_civics_adopted = nullptr;
        delete seats[i].m_tech_age;
        seats[i].m_tech_age = nullptr;
        seats[i].m_res_ledger.clear();
    }
    delete[] seats;
    seats = nullptr;
    n = 0;
}

static i16 find_lucky (const u16* lucky, u16 lucky_n, u16 seat) {
    for (u16 i = 0; i < lucky_n; ++i) {
        if (lucky[i] == seat) {
            return static_cast<i16>(i);
        }
    }
    return -1;
}

static bool sec_centroid (
    const u16* paint,
    u16 w,
    u16 h,
    u16 sec_n,
    u16 sector,
    u16* ox,
    u16* oy) {
    if (paint == nullptr || ox == nullptr || oy == nullptr || sector >= sec_n) {
        return false;
    }
    const u16 tag = static_cast<u16>(sector + 1u);
    u32 sx = 0;
    u32 sy = 0;
    u32 sn = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            if (paint[i] != tag) {
                continue;
            }
            sx = sx + x;
            sy = sy + y;
            sn = sn + 1u;
        }
    }
    if (sn == 0u) {
        return false;
    }
    *ox = static_cast<u16>(sx / sn);
    *oy = static_cast<u16>(sy / sn);
    return true;
}

static bool wr_anno (
    cstr path,
    const u16* paint,
    u16 w,
    u16 h,
    u16 sec_n,
    const u16* order,
    u16 order_n,
    u16 begin_sec) {
    if (path == nullptr || paint == nullptr) {
        return false;
    }
    u16* rank = new u16[sec_n];
    if (rank == nullptr) {
        return false;
    }
    for (u16 i = 0; i < sec_n; ++i) {
        rank[i] = U16_KEY_NULL;
    }
    u16 used_n = 0;
    for (u16 i = 0; i < order_n; ++i) {
        const u16 s = order[i];
        if (s >= sec_n || rank[s] != U16_KEY_NULL) {
            continue;
        }
        rank[s] = used_n;
        used_n = static_cast<u16>(used_n + 1u);
    }
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        delete[] rank;
        return false;
    }
    std::fprintf(fp, "# lucky_sect_order labels\n");
    if (begin_sec < sec_n) {
        u16 bx = 0;
        u16 by = 0;
        if (sec_centroid(paint, w, h, sec_n, begin_sec, &bx, &by)) {
            std::fprintf(fp, "%u %u B\n", static_cast<u32>(bx), static_cast<u32>(by));
        }
    }
    for (u16 s = 0; s < sec_n; ++s) {
        if (rank[s] == U16_KEY_NULL) {
            continue;
        }
        u16 cx = 0;
        u16 cy = 0;
        if (!sec_centroid(paint, w, h, sec_n, s, &cx, &cy)) {
            continue;
        }
        std::fprintf(fp, "%u %u %u\n", static_cast<u32>(cx), static_cast<u32>(cy),
            static_cast<u32>(rank[s] + 1u));
    }
    std::fclose(fp);
    delete[] rank;
    return true;
}

//================================================================================================================================
//=> - LuckySectOrderMng -
//================================================================================================================================

LuckySectOrderMng::LuckySectOrderMng ()
    : m_lucky(nullptr),
      m_lucky_n(0),
      m_player_n(0),
      m_save_n(0),
      m_wrote_n(0),
      m_ord(nullptr),
      m_ord_n(nullptr),
      m_skip(nullptr),
      m_skip_n(nullptr),
      m_begin(nullptr) {
}

LuckySectOrderMng::~LuckySectOrderMng () {
    clr();
}

void LuckySectOrderMng::clr () {
    delete[] m_lucky;
    m_lucky = nullptr;
    delete[] m_ord;
    m_ord = nullptr;
    delete[] m_ord_n;
    m_ord_n = nullptr;
    delete[] m_skip;
    m_skip = nullptr;
    delete[] m_skip_n;
    m_skip_n = nullptr;
    delete[] m_begin;
    m_begin = nullptr;
    m_lucky_n = 0;
    m_player_n = 0;
    m_save_n = 0;
    m_wrote_n = 0;
}

bool LuckySectOrderMng::setup (u16 player_n, u16 save_n) {
    clr();
    if (player_n == 0 || save_n == 0) {
        return false;
    }
    m_player_n = player_n;
    m_save_n = save_n;
    return true;
}

bool LuckySectOrderMng::push_ord (u16 li, u16 sector) {
    if (m_ord == nullptr || m_ord_n == nullptr || li >= m_lucky_n) {
        return false;
    }
    const u16 n = m_ord_n[li];
    if (n >= k_ord_cap) {
        return false;
    }
    u16* row = m_ord + static_cast<u32>(li) * static_cast<u32>(k_ord_cap);
    for (u16 i = 0; i < n; ++i) {
        if (row[i] == sector) {
            return true;
        }
    }
    row[n] = sector;
    m_ord_n[li] = static_cast<u16>(n + 1u);
    return true;
}

bool LuckySectOrderMng::push_skip (u16 li, u16 sector) {
    if (m_skip == nullptr || m_skip_n == nullptr || li >= m_lucky_n) {
        return false;
    }
    const u16 n = m_skip_n[li];
    if (n >= k_ord_cap) {
        return false;
    }
    u16* row = m_skip + static_cast<u32>(li) * static_cast<u32>(k_ord_cap);
    for (u16 i = 0; i < n; ++i) {
        if (row[i] == sector) {
            return true;
        }
    }
    row[n] = sector;
    m_skip_n[li] = static_cast<u16>(n + 1u);
    return true;
}

bool LuckySectOrderMng::fill (const EvalPaths& paths, const EvalLog& log, cstr out_dir) {
    if (out_dir == nullptr || out_dir[0] == 0 || !paths.ok() || paths.save_turn_n() != m_save_n) {
        return false;
    }
    m_wrote_n = 0;
    const u32 turn0 = paths.save_turn_at(0);
    char players_p[512];
    if (!paths.players_path(turn0, players_p, sizeof(players_p))) {
        return false;
    }
    PlayerState* seats = nullptr;
    u16 seat_n = 0;
    if (!GameIo::load_players(players_p, seats, seat_n) || seat_n < m_player_n) {
        std::printf("lucky_sect_order: load_players failed %s seat_n=%u need=%u\n",
            players_p, static_cast<u32>(seat_n), static_cast<u32>(m_player_n));
        free_seats(seats, seat_n);
        return false;
    }
    u16 lucky_cap = 0;
    for (u16 p = 0; p < m_player_n; ++p) {
        if (seats[p].m_lucky != 0u) {
            lucky_cap = static_cast<u16>(lucky_cap + 1u);
        }
    }
    if (lucky_cap == 0) {
        std::printf("lucky_sect_order: no lucky seats in %s\n", players_p);
        free_seats(seats, seat_n);
        return false;
    }
    m_lucky = new u16[lucky_cap];
    m_ord = new u16[static_cast<u32>(lucky_cap) * static_cast<u32>(k_ord_cap)];
    m_ord_n = new u16[lucky_cap];
    m_skip = new u16[static_cast<u32>(lucky_cap) * static_cast<u32>(k_ord_cap)];
    m_skip_n = new u16[lucky_cap];
    m_begin = new u16[lucky_cap];
    if (m_lucky == nullptr || m_ord == nullptr || m_ord_n == nullptr
        || m_skip == nullptr || m_skip_n == nullptr || m_begin == nullptr) {
        free_seats(seats, seat_n);
        return false;
    }
    m_lucky_n = 0;
    for (u16 p = 0; p < m_player_n; ++p) {
        if (seats[p].m_lucky == 0u) {
            continue;
        }
        m_lucky[m_lucky_n] = p;
        m_ord_n[m_lucky_n] = 0;
        m_skip_n[m_lucky_n] = 0;
        m_begin[m_lucky_n] = U16_KEY_NULL;
        m_lucky_n = static_cast<u16>(m_lucky_n + 1u);
    }
    free_seats(seats, seat_n);

    u32 exhaust_n = 0;
    u32 fallback_n = 0;
    for (u32 li = 0; li < log.line_n(); ++li) {
        const char* line = log.line(li);
        unsigned a0 = 0;
        unsigned a1 = 0;
        unsigned a2 = 0;
        unsigned a3 = 0;
        if (LOG_WAR_SECT_BEGIN::PARSE(line, &a0, &a1, &a2)) {
            const i16 idx = find_lucky(m_lucky, m_lucky_n, static_cast<u16>(a0));
            if (idx >= 0) {
                m_begin[idx] = static_cast<u16>(a1);
            }
            continue;
        }
        if (LOG_WAR_SECT_SELECT::PARSE(line, &a0, &a1, &a2)) {
            const i16 idx = find_lucky(m_lucky, m_lucky_n, static_cast<u16>(a0));
            if (idx >= 0 && !push_ord(static_cast<u16>(idx), static_cast<u16>(a1))) {
                return false;
            }
            continue;
        }
        if (LOG_WAR_SECT_SKIP::PARSE(line, &a0, &a1, &a2, &a3)) {
            const i16 idx = find_lucky(m_lucky, m_lucky_n, static_cast<u16>(a0));
            if (idx >= 0 && !push_skip(static_cast<u16>(idx), static_cast<u16>(a1))) {
                return false;
            }
            continue;
        }
        if (LOG_WAR_SECT_EXHAUST::PARSE(line, &a0, &a1, &a2)) {
            if (find_lucky(m_lucky, m_lucky_n, static_cast<u16>(a0)) >= 0) {
                exhaust_n = exhaust_n + 1u;
            }
            continue;
        }
        if (LOG_WAR_SECT_FALLBACK::PARSE(line, &a0, &a1, &a2, &a3)) {
            if (find_lucky(m_lucky, m_lucky_n, static_cast<u16>(a0)) >= 0) {
                fallback_n = fallback_n + 1u;
            }
            continue;
        }
    }

    char der_p[512];
    if (std::snprintf(der_p, sizeof(der_p), "%s/derived_statics.bin", paths.out_root()) <= 0) {
        return false;
    }
    u16 pw = 0;
    u16 ph = 0;
    u16 sec_n = 0;
    u16* paint = nullptr;
    if (!DerivedStaticsIO::read(der_p, &pw, &ph, &sec_n, &paint) || paint == nullptr) {
        std::printf("lucky_sect_order: derived read failed %s\n", der_p);
        return false;
    }

    const u32 turn = paths.save_turn_at(static_cast<u16>(m_save_n - 1u));
    char map_p[512];
    if (!paths.map_path(turn, map_p, sizeof(map_p))) {
        delete[] paint;
        return false;
    }
    GameArraySimple map;
    if (!GameIo::load_map_tiles(map_p, map)) {
        delete[] paint;
        return false;
    }

    char dig_p[512];
    if (std::snprintf(dig_p, sizeof(dig_p), "%s/lucky_sect_order_digest.txt", out_dir) > 0) {
        std::FILE* df = std::fopen(dig_p, "w");
        if (df != nullptr) {
            std::fprintf(df, "# lucky_sect_order digest exhaust=%u fallback=%u\n",
                exhaust_n, fallback_n);
            for (u16 i = 0; i < m_lucky_n; ++i) {
                std::fprintf(df, "seat=%u begin=%u select_n=%u skip_n=%u\n",
                    static_cast<u32>(m_lucky[i]),
                    static_cast<u32>(m_begin[i]),
                    static_cast<u32>(m_ord_n[i]),
                    static_cast<u32>(m_skip_n[i]));
            }
            std::fclose(df);
        }
    }

    u8* skipped = new u8[sec_n];
    if (skipped == nullptr) {
        delete[] paint;
        return false;
    }
    for (u16 i = 0; i < m_lucky_n; ++i) {
        const u16 seat = m_lucky[i];
        for (u16 s = 0; s < sec_n; ++s) {
            skipped[s] = 0u;
        }
        const u16* skip_row = m_skip + static_cast<u32>(i) * static_cast<u32>(k_ord_cap);
        for (u16 k = 0; k < m_skip_n[i]; ++k) {
            if (skip_row[k] < sec_n) {
                skipped[skip_row[k]] = 1u;
            }
        }
        const u16* ord_row = m_ord + static_cast<u32>(i) * static_cast<u32>(k_ord_cap);
        for (u16 k = 0; k < m_ord_n[i]; ++k) {
            if (ord_row[k] < sec_n) {
                skipped[ord_row[k]] = 0u;
            }
        }
        char ppm_p[512];
        char anno_p[512];
        if (std::snprintf(ppm_p, sizeof(ppm_p), "%s/lucky_sect_order_seat_%03u.ppm",
                out_dir, static_cast<u32>(seat)) <= 0
            || std::snprintf(anno_p, sizeof(anno_p), "%s/anno_seat_%03u.txt",
                out_dir, static_cast<u32>(seat)) <= 0) {
            delete[] skipped;
            delete[] paint;
            return false;
        }
        if (!LuckySectOrderSnap::write(
                ppm_p, map, paint, pw, ph, sec_n, ord_row, m_ord_n[i], skipped)) {
            delete[] skipped;
            delete[] paint;
            return false;
        }
        if (!wr_anno(anno_p, paint, pw, ph, sec_n, ord_row, m_ord_n[i], m_begin[i])) {
            delete[] skipped;
            delete[] paint;
            return false;
        }
        m_wrote_n = static_cast<u16>(m_wrote_n + 1u);
        std::printf("wrote %s ord_n=%u skip_n=%u begin=%u\n",
            ppm_p,
            static_cast<u32>(m_ord_n[i]),
            static_cast<u32>(m_skip_n[i]),
            static_cast<u32>(m_begin[i]));
    }
    delete[] skipped;
    delete[] paint;
    std::printf("lucky_sect_order digest exhaust=%u fallback=%u\n", exhaust_n, fallback_n);
    return true;
}

u16 LuckySectOrderMng::wrote_n () const {
    return m_wrote_n;
}

u16 LuckySectOrderMng::lucky_n () const {
    return m_lucky_n;
}

u16 LuckySectOrderMng::lucky_at (u16 i) const {
    if (m_lucky == nullptr || i >= m_lucky_n) {
        return U16_KEY_NULL;
    }
    return m_lucky[i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
