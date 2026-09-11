//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "sector_support.h"

#include <cstring>

#include "build_adds_array.h"
#include "city.h"
#include "game_state.h"
#include "gen_land_sectors.h"
#include "land_sector_network.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

const GameState* SectorSupport::m_st = nullptr;
const Whiteboard_2B* SectorSupport::m_sec = nullptr;
const LandSectorSeeds* SectorSupport::m_seeds = nullptr;
const LandSectorNetwork* SectorSupport::m_net = nullptr;

//================================================================================================================================
//=> - SectorSupport -
//================================================================================================================================

bool SectorSupport::bind (
    const GameState* st,
    const Whiteboard_2B* sec,
    const LandSectorSeeds* seeds,
    const LandSectorNetwork* net)
{
    clr();
    if (st == nullptr || sec == nullptr || seeds == nullptr || net == nullptr) {
        return false;
    }
    if (!sec->ok() || !net->ok() || seeds->m_pts == nullptr || seeds->m_n == 0u) {
        return false;
    }
    if (st->m_player_n == 0u || st->m_player_n > k_seat_cap) {
        return false;
    }
    m_st = st;
    m_sec = sec;
    m_seeds = seeds;
    m_net = net;
    return true;
}

void SectorSupport::clr () {
    m_st = nullptr;
    m_sec = nullptr;
    m_seeds = nullptr;
    m_net = nullptr;
}

bool SectorSupport::ready () {
    return m_st != nullptr && m_sec != nullptr && m_seeds != nullptr && m_net != nullptr
        && m_sec->ok() && m_net->ok() && m_seeds->m_pts != nullptr && m_seeds->m_n > 0u
        && m_st->m_player_n > 0u && m_st->m_player_n <= k_seat_cap;
}

SectorPresence SectorSupport::sector_presence (u16 player, u16 sector) {
    SectorPresence out = {};
    out.m_rival = U16_KEY_NULL;
    if (!ready() || player >= m_st->m_player_n || sector >= m_seeds->m_n) {
        return out;
    }
    const u16 w = m_sec->w();
    const u16 h = m_sec->h();
    const u32 wi = static_cast<u32>(w);
    const u16 tag = static_cast<u16>(sector + 1u);
    const u16 sx = m_seeds->m_pts[sector].m_x;
    const u16 sy = m_seeds->m_pts[sector].m_y;
    if (sx >= w || sy >= h || m_sec->rd(sx, sy) != tag) {
        return out;
    }
    Whiteboard_1B wb_vis("SectorSupport", "vis", 0u);
    Whiteboard_4B wb_q("SectorSupport", "q", 0u);
    if (!wb_vis.ok() || !wb_q.ok()) {
        return out;
    }
    const u32 n = WhiteboardMng::tile_n();
    std::memset(wb_vis.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u8));
    u32* q = wb_q.get_iter_ptr();
    u32 qn = 0u;
    const u32 s0 = static_cast<u32>(sy) * wi + static_cast<u32>(sx);
    wb_vis.wr_i(s0, 1u);
    q[qn++] = s0;
    u16 cnt[k_seat_cap];
    std::memset(cnt, 0, static_cast<size_t>(m_st->m_player_n) * sizeof(u16));
    static const i32 dx4[4] = {-1, 1, 0, 0};
    static const i32 dy4[4] = {0, 0, -1, 1};
    const GameArraySimple& map = m_st->m_map;
    for (u32 qh = 0u; qh < qn; ++qh) {
        const u32 ti = q[qh];
        const u32 py = ti / wi;
        const u32 px = ti - py * wi;
        const u16 x = static_cast<u16>(px);
        const u16 y = static_cast<u16>(py);
        if (map.get_add_typ(x, y) == BUILD_ADD_CITY) {
            const u16 cidx = map.get_add_idx(x, y);
            const City* c = m_st->m_cities.get_city(cidx);
            if (c != nullptr) {
                const u16 own = c->get_owner();
                if (own < m_st->m_player_n) {
                    cnt[own] = static_cast<u16>(cnt[own] + 1u);
                }
            }
        }
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(px) + dx4[d];
            const i32 ny = static_cast<i32>(py) + dy4[d];
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            const u32 ni = static_cast<u32>(uy) * wi + static_cast<u32>(ux);
            if (wb_vis.rd_i(ni) != 0u || m_sec->rd_i(ni) != tag) {
                continue;
            }
            wb_vis.wr_i(ni, 1u);
            q[qn++] = ni;
        }
    }
    out.m_own = cnt[player];
    u16 other = 0u;
    u16 rival = U16_KEY_NULL;
    u16 best = 0u;
    for (u16 p = 0; p < m_st->m_player_n; ++p) {
        if (p == player || cnt[p] == 0u) {
            continue;
        }
        other = static_cast<u16>(other + cnt[p]);
        if (rival == U16_KEY_NULL || cnt[p] > best) {
            rival = p;
            best = cnt[p];
        }
    }
    out.m_other = other;
    out.m_rival = rival;
    return out;
}

bool SectorSupport::tally_all (u16 player, u16* own, u16* oth) {
    if (!ready() || own == nullptr || oth == nullptr || player >= m_st->m_player_n) {
        return false;
    }
    const u16 sn = m_seeds->m_n;
    std::memset(own, 0, static_cast<size_t>(sn) * sizeof(u16));
    std::memset(oth, 0, static_cast<size_t>(sn) * sizeof(u16));
    const u16 cn = m_st->m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = m_st->m_cities.get_city(i);
        if (c == nullptr) {
            continue;
        }
        const u16 x = c->get_x();
        const u16 y = c->get_y();
        if (x >= m_sec->w() || y >= m_sec->h()) {
            continue;
        }
        const u16 tag = m_sec->rd(x, y);
        if (tag == GLS_IDX_NONE || tag > sn) {
            continue;
        }
        const u16 si = static_cast<u16>(tag - 1u);
        if (c->get_owner() == player) {
            own[si] = static_cast<u16>(own[si] + 1u);
        } else {
            oth[si] = static_cast<u16>(oth[si] + 1u);
        }
    }
    return true;
}

u16 SectorSupport::best_integral_sector (u16 player) {
    if (!ready() || player >= m_st->m_player_n) {
        return U16_KEY_NULL;
    }
    const u16 sn = m_seeds->m_n;
    if (static_cast<u32>(sn) > WhiteboardMng::tile_n()) {
        return U16_KEY_NULL;
    }
    Whiteboard_2B wb_own("SectorSupport", "own", 0u);
    Whiteboard_2B wb_oth("SectorSupport", "oth", 0u);
    Whiteboard_1B wb_full("SectorSupport", "full", 0u);
    Whiteboard_1B wb_cand("SectorSupport", "cand", 0u);
    if (!wb_own.ok() || !wb_oth.ok() || !wb_full.ok() || !wb_cand.ok()) {
        return U16_KEY_NULL;
    }
    u16* own = wb_own.get_iter_ptr();
    u16* oth = wb_oth.get_iter_ptr();
    u8* full = wb_full.get_iter_ptr();
    u8* cand = wb_cand.get_iter_ptr();
    if (!tally_all(player, own, oth)) {
        return U16_KEY_NULL;
    }
    std::memset(full, 0, static_cast<size_t>(sn));
    std::memset(cand, 0, static_cast<size_t>(sn));
    for (u16 s = 0; s < sn; ++s) {
        if (own[s] >= 1u && oth[s] == 0u) {
            full[s] = 1u;
        }
    }
    const u16 ln = m_net->link_n();
    for (u16 i = 0; i < ln; ++i) {
        const LandSectorLink* L = m_net->get(i);
        if (L == nullptr) {
            continue;
        }
        if (full[L->m_a] != 0u && full[L->m_b] == 0u && oth[L->m_b] >= 1u) {
            cand[L->m_b] = 1u;
        }
        if (full[L->m_b] != 0u && full[L->m_a] == 0u && oth[L->m_a] >= 1u) {
            cand[L->m_a] = 1u;
        }
    }
    i32 best_sc = 0;
    u32 best_yld = 0u;
    u16 best = U16_KEY_NULL;
    bool have = false;
    for (u16 s = 0; s < sn; ++s) {
        if (cand[s] == 0u || (own[s] + oth[s]) == 0u) {
            continue;
        }
        i32 sc = 0;
        for (u16 i = 0; i < ln; ++i) {
            const LandSectorLink* L = m_net->get(i);
            if (L == nullptr) {
                continue;
            }
            u16 oth_s = U16_KEY_NULL;
            if (L->m_a == s) {
                oth_s = L->m_b;
            } else if (L->m_b == s) {
                oth_s = L->m_a;
            } else {
                continue;
            }
            if (full[oth_s] != 0u) {
                ++sc;
            } else {
                --sc;
            }
        }
        const u32 yld = m_seeds->m_pts[s].m_yields;
        if (!have || sc > best_sc || (sc == best_sc && yld > best_yld)) {
            have = true;
            best_sc = sc;
            best_yld = yld;
            best = s;
        }
    }
    return best;
}

u16 SectorSupport::best_defensible_sector (u16 player) {
    if (!ready() || player >= m_st->m_player_n) {
        return U16_KEY_NULL;
    }
    const u16 sn = m_seeds->m_n;
    if (static_cast<u32>(sn) > WhiteboardMng::tile_n()) {
        return U16_KEY_NULL;
    }
    Whiteboard_2B wb_own("SectorSupport", "own", 0u);
    Whiteboard_2B wb_oth("SectorSupport", "oth", 0u);
    Whiteboard_1B wb_full("SectorSupport", "full", 0u);
    Whiteboard_1B wb_cand("SectorSupport", "cand", 0u);
    if (!wb_own.ok() || !wb_oth.ok() || !wb_full.ok() || !wb_cand.ok()) {
        return U16_KEY_NULL;
    }
    u16* own = wb_own.get_iter_ptr();
    u16* oth = wb_oth.get_iter_ptr();
    u8* full = wb_full.get_iter_ptr();
    u8* cand = wb_cand.get_iter_ptr();
    if (!tally_all(player, own, oth)) {
        return U16_KEY_NULL;
    }
    std::memset(full, 0, static_cast<size_t>(sn));
    std::memset(cand, 0, static_cast<size_t>(sn));
    for (u16 s = 0; s < sn; ++s) {
        if (own[s] >= 1u && oth[s] == 0u) {
            full[s] = 1u;
        }
    }
    const u16 ln = m_net->link_n();
    for (u16 i = 0; i < ln; ++i) {
        const LandSectorLink* L = m_net->get(i);
        if (L == nullptr) {
            continue;
        }
        if (full[L->m_a] != 0u && full[L->m_b] == 0u && oth[L->m_b] >= 1u) {
            cand[L->m_b] = 1u;
        }
        if (full[L->m_b] != 0u && full[L->m_a] == 0u && oth[L->m_a] >= 1u) {
            cand[L->m_a] = 1u;
        }
    }
    u32 best_sc = 0u;
    u32 best_yld = 0u;
    u16 best = U16_KEY_NULL;
    bool have = false;
    for (u16 s = 0; s < sn; ++s) {
        if (cand[s] == 0u || (own[s] + oth[s]) == 0u) {
            continue;
        }
        u32 sc = 0u;
        for (u16 i = 0; i < ln; ++i) {
            const LandSectorLink* L = m_net->get(i);
            if (L == nullptr) {
                continue;
            }
            u16 oth_s = U16_KEY_NULL;
            if (L->m_a == s) {
                oth_s = L->m_b;
            } else if (L->m_b == s) {
                oth_s = L->m_a;
            } else {
                continue;
            }
            if (full[oth_s] != 0u) {
                sc = static_cast<u32>(sc + static_cast<u32>(L->m_len));
            }
        }
        const u32 yld = m_seeds->m_pts[s].m_yields;
        if (!have || sc > best_sc || (sc == best_sc && yld > best_yld)) {
            have = true;
            best_sc = sc;
            best_yld = yld;
            best = s;
        }
    }
    return best;
}

u16 SectorSupport::get_shared_sector (u16 player) {
    if (!ready() || player >= m_st->m_player_n) {
        return U16_KEY_NULL;
    }
    const u16 sn = m_seeds->m_n;
    if (static_cast<u32>(sn) > WhiteboardMng::tile_n()) {
        return U16_KEY_NULL;
    }
    Whiteboard_2B wb_own("SectorSupport", "own", 0u);
    Whiteboard_2B wb_oth("SectorSupport", "oth", 0u);
    if (!wb_own.ok() || !wb_oth.ok()) {
        return U16_KEY_NULL;
    }
    u16* own = wb_own.get_iter_ptr();
    u16* oth = wb_oth.get_iter_ptr();
    if (!tally_all(player, own, oth)) {
        return U16_KEY_NULL;
    }
    for (u16 s = 0; s < sn; ++s) {
        if (own[s] >= 1u && oth[s] >= 1u) {
            return s;
        }
    }
    return U16_KEY_NULL;
}

u16 SectorSupport::enemy_cities (u16 sector, u16 enemy, u16* tgts, u16 cap) {
    if (!ready() || tgts == nullptr || cap == 0u || sector >= m_seeds->m_n || enemy >= m_st->m_player_n) {
        return 0u;
    }
    const u16 tag = static_cast<u16>(sector + 1u);
    u16 n = 0u;
    const u16 cn = m_st->m_cities.get_city_count();
    for (u16 i = 0; i < cn && n < cap; ++i) {
        const City* c = m_st->m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != enemy) {
            continue;
        }
        const u16 x = c->get_x();
        const u16 y = c->get_y();
        if (x >= m_sec->w() || y >= m_sec->h()) {
            continue;
        }
        if (m_sec->rd(x, y) != tag) {
            continue;
        }
        tgts[n++] = i;
    }
    return n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
