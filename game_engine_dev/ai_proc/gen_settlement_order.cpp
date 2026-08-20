//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "gen_settlement_order.h"

#include <cstring>

#include "assert_log.h"
#include "game_array_simple.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Local -
//================================================================================================================================

static const i8 k_dx4[4] = {0, 1, 0, -1};
static const i8 k_dy4[4] = {-1, 0, 1, 0};
static const u16 k_none = 0xFFFFu;

static u32 tix (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool is_land (const GameArraySimple& map, u16 x, u16 y) {
    return !overlay_is_water_terr(map.get_terrain(x, y));
}

static u32 spot_n (const GameArraySimple& map) {
    u32 n = 0;
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_planned_city(x, y) != 0u) {
                n += 1u;
            }
        }
    }
    return n;
}

static void add_pt (
    Whiteboard_2B& xs,
    Whiteboard_2B& ys,
    u32* wr,
    const u32* cap,
    u16 p,
    u16 x,
    u16 y)
{
    GAME_EXPECT(wr != nullptr, "GenSettlementOrder add_pt got nullptr wr");
    GAME_EXPECT(cap != nullptr, "GenSettlementOrder add_pt got nullptr cap");
    if (wr[p] >= cap[p]) {
        return;
    }
    const u32 k = wr[p];
    xs.wr_i(k, x);
    ys.wr_i(k, y);
    wr[p] = k + 1u;
}

static bool flood (
    const GameArraySimple& map,
    const SpgCoordPair* starts,
    u32 start_n,
    Whiteboard_2B& own,
    Whiteboard_4B& que,
    Whiteboard_2B& xs,
    Whiteboard_2B& ys,
    u32* wr,
    const u32* cap)
{
    GAME_EXPECT(starts != nullptr, "GenSettlementOrder flood got nullptr starts");
    GAME_EXPECT(wr != nullptr, "GenSettlementOrder flood got nullptr wr");
    GAME_EXPECT(cap != nullptr, "GenSettlementOrder flood got nullptr cap");
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 tn = map.tile_n();
    if (w == 0 || h == 0 || tn == 0 || starts == nullptr || start_n == 0u) {
        return false;
    }
    if (!own.ok() || !que.ok() || !xs.ok() || !ys.ok() || wr == nullptr || cap == nullptr) {
        return false;
    }
    std::memset(own.raw(), 0xFF, static_cast<size_t>(tn) * sizeof(u16));
    u32 qt = 0;
    for (u32 p = 0; p < start_n; ++p) {
        const u16 x = starts[p].x;
        const u16 y = starts[p].y;
        if (x >= w || y >= h) {
            continue;
        }
        if (!is_land(map, x, y)) {
            continue;
        }
        const u32 i = tix(w, x, y);
        if (own.rd_i(i) != k_none) {
            continue;
        }
        own.wr_i(i, static_cast<u16>(p));
        que.wr_i(qt, i);
        qt += 1u;
        if (map.get_planned_city(x, y) == 0u) {
            continue;
        }
        add_pt(xs, ys, wr, cap, static_cast<u16>(p), x, y);
    }
    u32 qh = 0;
    while (qh < qt) {
        const u32 i = que.rd_i(qh);
        qh += 1u;
        const u16 p = own.rd_i(i);
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        for (u32 k = 0; k < 4u; ++k) {
            const int nx = static_cast<int>(x) + k_dx4[k];
            const int ny = static_cast<int>(y) + k_dy4[k];
            if (nx < 0 || ny < 0 || nx >= static_cast<int>(w) || ny >= static_cast<int>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (!is_land(map, ux, uy)) {
                continue;
            }
            const u32 ni = tix(w, ux, uy);
            if (own.rd_i(ni) != k_none) {
                continue;
            }
            own.wr_i(ni, p);
            que.wr_i(qt, ni);
            qt += 1u;
            if (map.get_planned_city(ux, uy) == 0u) {
                continue;
            }
            add_pt(xs, ys, wr, cap, p, ux, uy);
        }
    }
    return true;
}

//================================================================================================================================
//=> - GenSettlementOrder -
//================================================================================================================================

GenSettlementOrder::GenSettlementOrder () :
    m_xs("GenSettlementOrder", "xs", 0u),
    m_ys("GenSettlementOrder", "ys", 0u),
    m_pn(0),
    m_ptn(0) {
    for (u16 i = 0; i < GSO_MAX_PN; ++i) {
        m_st[i] = 0;
        m_en[i] = 0;
        m_hd[i] = 0;
        m_tl[i] = 0;
    }
}

void GenSettlementOrder::clr () {
    m_pn = 0;
    m_ptn = 0;
    for (u16 i = 0; i < GSO_MAX_PN; ++i) {
        m_st[i] = 0;
        m_en[i] = 0;
        m_hd[i] = 0;
        m_tl[i] = 0;
    }
}

bool GenSettlementOrder::ok () const {
    return m_xs.ok() && m_ys.ok();
}

u16 GenSettlementOrder::pn () const {
    return m_pn;
}

u32 GenSettlementOrder::n (u16 p) const {
    if (p >= m_pn) {
        return 0;
    }
    return m_en[p] - m_st[p];
}

u32 GenSettlementOrder::st (u16 p) const {
    if (p >= m_pn) {
        return 0;
    }
    return m_st[p];
}

u32 GenSettlementOrder::en (u16 p) const {
    if (p >= m_pn) {
        return 0;
    }
    return m_en[p];
}

u32 GenSettlementOrder::hd (u16 p) const {
    if (p >= m_pn) {
        return 0;
    }
    return m_hd[p];
}

u32 GenSettlementOrder::tl (u16 p) const {
    if (p >= m_pn) {
        return 0;
    }
    return m_tl[p];
}

SpgCoordPair GenSettlementOrder::at (u16 p, u32 i) const {
    SpgCoordPair z = {0, 0};
    if (p >= m_pn || !ok()) {
        return z;
    }
    if (i >= n(p)) {
        return z;
    }
    const u32 k = m_st[p] + i;
    z.x = m_xs.rd_i(k);
    z.y = m_ys.rd_i(k);
    return z;
}

bool GenSettlementOrder::gen_excl (const GameArraySimple& map, const SpgCoordPair* starts, u32 start_n) {
    clr();
    if (!ok()) {
        return false;
    }
    if (starts == nullptr || start_n == 0u || start_n > GSO_MAX_PN) {
        return false;
    }
    const u32 spots = spot_n(map);
    const u32 tn = map.tile_n();
    m_pn = static_cast<u16>(start_n);
    const bool fit = spots == 0u || start_n <= tn / spots;
    u32 last = 0;
    if (fit) {
        m_ptn = start_n * spots;
        last = m_ptn;
        for (u16 p = 0; p < m_pn; ++p) {
            m_st[p] = static_cast<u32>(p) * spots;
        }
    } else {
        m_ptn = tn;
        last = tn;
        for (u16 p = 0; p < m_pn; ++p) {
            m_st[p] = static_cast<u32>((static_cast<u64>(p) * tn) / start_n);
        }
    }
    u32 wr[GSO_MAX_PN];
    u32 cap[GSO_MAX_PN];
    for (u16 p = 0; p < m_pn; ++p) {
        cap[p] = (p + 1u < m_pn) ? m_st[p + 1u] : last;
        wr[p] = m_st[p];
        m_en[p] = m_st[p];
        m_hd[p] = m_st[p];
        m_tl[p] = m_st[p];
    }
    Whiteboard_2B own("GenSettlementOrder", "own", 0u);
    Whiteboard_4B que("GenSettlementOrder", "que", 0u);
    if (!flood(map, starts, start_n, own, que, m_xs, m_ys, wr, cap)) {
        clr();
        return false;
    }
    for (u16 p = 0; p < m_pn; ++p) {
        m_en[p] = wr[p];
        m_hd[p] = m_st[p];
        m_tl[p] = m_en[p];
    }
    return true;
}

bool GenSettlementOrder::gen_all (const GameArraySimple& map, u16 sx, u16 sy) {
    SpgCoordPair s = {sx, sy};
    return gen_excl(map, &s, 1u);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
