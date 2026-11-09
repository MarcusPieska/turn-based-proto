//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "sector_support.h"

#include <cstring>

#include "build_adds_array.h"
#include "city.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "gen_land_sectors.h"
#include "land_sector_network.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static const i8 k_ss_dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const i8 k_ss_dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

static bool ss_walk_ok (const GameState& st, u16 x, u16 y) {
    const u8 t = st.m_map.get_terrain(x, y);
    if (t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0]) {
        return false;
    }
    if (t == TERR_INLAND_SEA[0] || t == TERR_INLAND_LAKE[0]) {
        return false;
    }
    if (t == TERR_MOUNTAINS[0] || t == TERR_VOLCANO[0]) {
        return false;
    }
    return true;
}

static bool ss_foe_ok (const GameState& st, u16 seat, u16 owner) {
    if (owner == seat || owner >= st.m_player_n || st.m_player_states == nullptr) {
        return false;
    }
    if (st.m_player_states[owner].m_is_active == 0u || st.m_player_states[owner].m_lucky != 0u) {
        return false;
    }
    return true;
}

static bool ss_probe_pass (const GameState& st, u16 seat, u16 x, u16 y) {
    if (!ss_walk_ok(st, x, y)) {
        return false;
    }
    const u8 o = st.m_map.get_civ_owner(x, y);
    if (o == U8_KEY_NULL || o == static_cast<u8>(seat)) {
        return true;
    }
    if (o >= st.m_player_n || st.m_player_states == nullptr) {
        return false;
    }
    return st.m_player_states[o].m_lucky == 0u;
}

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

bool SectorSupport::has_own_city (u16 seat, u16 sector) {
    if (!ready() || seat >= m_st->m_player_n || sector >= m_seeds->m_n) {
        return false;
    }
    const u16 tag = static_cast<u16>(sector + 1u);
    const u16 w = m_sec->w();
    const u16 h = m_sec->h();
    const u16 cn = m_st->m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = m_st->m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        const u16 x = c->get_x();
        const u16 y = c->get_y();
        if (x < w && y < h && m_sec->rd(x, y) == tag) {
            return true;
        }
    }
    return false;
}

bool SectorSupport::probe_adj (u16 seat, u16 src, u16 tgt, u16* ox, u16* oy) {
    if (!ready() || ox == nullptr || oy == nullptr || seat >= m_st->m_player_n) {
        return false;
    }
    if (src >= m_seeds->m_n || tgt >= m_seeds->m_n || src == tgt) {
        return false;
    }
    if (!has_own_city(seat, src)) {
        return false;
    }
    const u16 w = m_st->m_map.width();
    const u16 h = m_st->m_map.height();
    if (WhiteboardMng::width() != w || WhiteboardMng::height() != h) {
        return false;
    }
    const u16 stag = static_cast<u16>(src + 1u);
    const u16 ttag = static_cast<u16>(tgt + 1u);
    Whiteboard_1B seen("SectorSupport", "pr_seen", 0u);
    Whiteboard_2B cit("SectorSupport", "pr_cit", 0u);
    Whiteboard_4B que("SectorSupport", "pr_que", 0u);
    if (!seen.ok() || !cit.ok() || !que.ok()) {
        return false;
    }
    const u32 tn = m_st->m_map.tile_n();
    std::memset(seen.raw(), 0, static_cast<size_t>(tn));
    for (u32 i = 0; i < tn; ++i) {
        cit.wr_i(i, U16_KEY_NULL);
    }
    const u16 cn = m_st->m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = m_st->m_cities.get_city(i);
        if (c == nullptr || !ss_foe_ok(*m_st, seat, c->get_owner())) {
            continue;
        }
        const u16 cx = c->get_x();
        const u16 cy = c->get_y();
        if (cx >= w || cy >= h || m_sec->rd(cx, cy) != ttag) {
            continue;
        }
        cit.wr(cx, cy, i);
    }
    u32 qh = 0u;
    u32 qt = 0u;
    for (u16 i = 0; i < cn; ++i) {
        const City* c = m_st->m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        const u16 x = c->get_x();
        const u16 y = c->get_y();
        if (x >= w || y >= h || m_sec->rd(x, y) != stag) {
            continue;
        }
        if (!ss_probe_pass(*m_st, seat, x, y) || seen.rd(x, y) != 0u) {
            continue;
        }
        seen.wr(x, y, 1u);
        que.wr_i(qt++, static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x));
    }
    while (qh < qt) {
        const u32 i = que.rd_i(qh++);
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        if (m_sec->rd(x, y) == ttag && cit.rd(x, y) != U16_KEY_NULL) {
            *ox = x;
            *oy = y;
            return true;
        }
        for (u8 d = 0; d < 8u; ++d) {
            const i32 nx = static_cast<i32>(x) + static_cast<i32>(k_ss_dx[d]);
            const i32 ny = static_cast<i32>(y) + static_cast<i32>(k_ss_dy[d]);
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (seen.rd(ux, uy) != 0u) {
                continue;
            }
            const u16 tag = m_sec->rd(ux, uy);
            if (tag != stag && tag != ttag) {
                continue;
            }
            if (!ss_probe_pass(*m_st, seat, ux, uy)) {
                continue;
            }
            seen.wr(ux, uy, 1u);
            que.wr_i(qt++, static_cast<u32>(uy) * static_cast<u32>(w) + static_cast<u32>(ux));
        }
    }
    return false;
}

bool SectorSupport::find_own_seed (u16 seat, u16 sector, u16* ox, u16* oy) {
    if (!ready() || ox == nullptr || oy == nullptr || seat >= m_st->m_player_n || sector >= m_seeds->m_n) {
        return false;
    }
    const u16 tag = static_cast<u16>(sector + 1u);
    const u16 w = m_sec->w();
    const u16 h = m_sec->h();
    const u16 cn = m_st->m_cities.get_city_count();
    const u16 sx = m_seeds->m_pts[sector].m_x;
    const u16 sy = m_seeds->m_pts[sector].m_y;
    u32 best = 0xFFFFFFFFu;
    u16 bx = 0u;
    u16 by = 0u;
    bool found = false;
    for (u16 i = 0; i < cn; ++i) {
        const City* c = m_st->m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        const u16 x = c->get_x();
        const u16 y = c->get_y();
        if (x >= w || y >= h || m_sec->rd(x, y) != tag || !ss_walk_ok(*m_st, x, y)) {
            continue;
        }
        const u32 adx = sx > x ? static_cast<u32>(sx - x) : static_cast<u32>(x - sx);
        const u32 ady = sy > y ? static_cast<u32>(sy - y) : static_cast<u32>(y - sy);
        const u32 d = adx + ady;
        if (!found || d < best) {
            best = d;
            bx = x;
            by = y;
            found = true;
        }
    }
    if (found) {
        *ox = bx;
        *oy = by;
        return true;
    }
    const u16 ln = m_net->link_n();
    for (u16 li = 0; li < ln; ++li) {
        const LandSectorLink* lk = m_net->get(li);
        if (lk == nullptr) {
            continue;
        }
        u16 nbr = U16_KEY_NULL;
        if (lk->m_a == sector) {
            nbr = lk->m_b;
        } else if (lk->m_b == sector) {
            nbr = lk->m_a;
        } else {
            continue;
        }
        if (probe_adj(seat, nbr, sector, ox, oy)) {
            return true;
        }
    }
    return false;
}

u16 SectorSupport::flood_foreign (u16 seat, u16 sector, u16 sx, u16 sy, u16* out, u16 cap) {
    if (!ready() || out == nullptr || cap == 0u || seat >= m_st->m_player_n || sector >= m_seeds->m_n) {
        return 0u;
    }
    const u16 w = m_st->m_map.width();
    const u16 h = m_st->m_map.height();
    const u16 tag = static_cast<u16>(sector + 1u);
    if (sx >= w || sy >= h || !ss_walk_ok(*m_st, sx, sy) || m_sec->rd(sx, sy) != tag) {
        return 0u;
    }
    if (WhiteboardMng::width() != w || WhiteboardMng::height() != h) {
        return 0u;
    }
    Whiteboard_1B seen("SectorSupport", "ff_seen", 0u);
    Whiteboard_2B cit("SectorSupport", "ff_cit", 0u);
    Whiteboard_4B que("SectorSupport", "ff_que", 0u);
    if (!seen.ok() || !cit.ok() || !que.ok()) {
        return 0u;
    }
    const u32 tn = m_st->m_map.tile_n();
    std::memset(seen.raw(), 0, static_cast<size_t>(tn));
    for (u32 i = 0; i < tn; ++i) {
        cit.wr_i(i, U16_KEY_NULL);
    }
    const u16 cn = m_st->m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = m_st->m_cities.get_city(i);
        if (c == nullptr || !ss_foe_ok(*m_st, seat, c->get_owner())) {
            continue;
        }
        const u16 cx = c->get_x();
        const u16 cy = c->get_y();
        if (cx >= w || cy >= h || m_sec->rd(cx, cy) != tag) {
            continue;
        }
        cit.wr(cx, cy, i);
    }
    u32 qh = 0u;
    u32 qt = 0u;
    que.wr_i(qt++, static_cast<u32>(sy) * static_cast<u32>(w) + static_cast<u32>(sx));
    seen.wr(sx, sy, 1u);
    u16 n = 0u;
    while (qh < qt && n < cap) {
        const u32 i = que.rd_i(qh++);
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        const u16 cidx = cit.rd(x, y);
        if (cidx != U16_KEY_NULL) {
            out[n++] = cidx;
            if (n >= cap) {
                return n;
            }
        }
        for (u8 d = 0; d < 8u; ++d) {
            const i32 nx = static_cast<i32>(x) + static_cast<i32>(k_ss_dx[d]);
            const i32 ny = static_cast<i32>(y) + static_cast<i32>(k_ss_dy[d]);
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (seen.rd(ux, uy) != 0u || m_sec->rd(ux, uy) != tag || !ss_walk_ok(*m_st, ux, uy)) {
                continue;
            }
            seen.wr(ux, uy, 1u);
            que.wr_i(qt++, static_cast<u32>(uy) * static_cast<u32>(w) + static_cast<u32>(ux));
        }
    }
    return n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
