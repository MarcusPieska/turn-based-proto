//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "sector_network.h"

#include "game_map_defs.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool terr_walk_land (u8 terr) {
    if (overlay_is_water_terr(terr)) {
        return false;
    }
    if (terr == TERR_MOUNTAINS[0] || terr == TERR_NONE[0]) {
        return false;
    }
    return true;
}

static bool terr_walk_water (u8 terr) {
    return overlay_is_water_terr(terr);
}

//================================================================================================================================
//=> - SectorNetwork -
//================================================================================================================================

SectorNetwork::SectorNetwork () :
    m_ok(false),
    m_w(0),
    m_h(0),
    m_n(0),
    m_cols(0),
    m_rows(0),
    m_sec(nullptr) {
}

SectorNetwork::~SectorNetwork () {
    clear();
}

void SectorNetwork::clear () {
    delete[] m_sec;
    m_sec = nullptr;
    m_w = 0;
    m_h = 0;
    m_n = 0;
    m_cols = 0;
    m_rows = 0;
    m_ok = false;
}

i32 SectorNetwork::cen_x (i32 col) {
    return static_cast<i32>(SN_ORIGIN) + col * static_cast<i32>(SN_STEP);
}

i32 SectorNetwork::cen_y (i32 col, i32 row) {
    const i32 base = static_cast<i32>(SN_ORIGIN) + row * static_cast<i32>(SN_STEP);
    if ((col & 1) == 0) {
        return base - SN_OFF_EVEN;
    }
    return base + SN_OFF_ODD;
}

u16 SectorNetwork::id_of (i32 col, i32 row) const {
    if (col < 0 || row < 0 || col >= static_cast<i32>(m_cols) || row >= static_cast<i32>(m_rows)) {
        return U16_KEY_NULL;
    }
    return static_cast<u16>(static_cast<u32>(col) * static_cast<u32>(m_rows) + static_cast<u32>(row));
}

bool SectorNetwork::tile_pass (const u8* terr, u16 x, u16 y, bool water) const {
    if (terr == nullptr || x >= m_w || y >= m_h) {
        return false;
    }
    const u8 t = terr[static_cast<u32>(y) * static_cast<u32>(m_w) + static_cast<u32>(x)];
    if (water) {
        return terr_walk_water(t);
    }
    return terr_walk_land(t);
}

void SectorNetwork::flood_sec (u16 id, const u8* terr, bool water, u16* wd, u8* qx, u8* qy) {
    Sector& S = m_sec[id];
    u8* mask = water ? &S.m_wmask : &S.m_mask;
    *mask = 0;
    if (!tile_pass(terr, S.m_x, S.m_y, water)) {
        return;
    }
    const i32 sc = static_cast<i32>(id / m_rows);
    const i32 sr = static_cast<i32>(id % m_rows);
    i32 ndc[6];
    i32 ndr[6];
    if ((sc & 1) == 0) {
        ndc[0] = 0; ndr[0] = -1;
        ndc[1] = 0; ndr[1] = 1;
        ndc[2] = -1; ndr[2] = -1;
        ndc[3] = 1; ndr[3] = -1;
        ndc[4] = -1; ndr[4] = 0;
        ndc[5] = 1; ndr[5] = 0;
    } else {
        ndc[0] = 0; ndr[0] = -1;
        ndc[1] = 0; ndr[1] = 1;
        ndc[2] = -1; ndr[2] = 0;
        ndc[3] = 1; ndr[3] = 0;
        ndc[4] = -1; ndr[4] = 1;
        ndc[5] = 1; ndr[5] = 1;
    }
    u16 nbid[6];
    u16 nbx[6];
    u16 nby[6];
    u32 need = 0;
    for (u32 b = 0; b < 6u; ++b) {
        nbid[b] = id_of(sc + ndc[b], sr + ndr[b]);
        if (nbid[b] == U16_KEY_NULL) {
            nbx[b] = U16_KEY_NULL;
            nby[b] = U16_KEY_NULL;
            continue;
        }
        nbx[b] = m_sec[nbid[b]].m_x;
        nby[b] = m_sec[nbid[b]].m_y;
        ++need;
    }
    if (need == 0) {
        return;
    }
    for (u32 i = 0; i < SN_WIN_N; ++i) {
        wd[i] = U16_KEY_NULL;
    }
    const i32 half = static_cast<i32>(SN_FLOOD_R);
    const i32 ox = static_cast<i32>(S.m_x) - half;
    const i32 oy = static_cast<i32>(S.m_y) - half;
    const u32 sidx = static_cast<u32>(half) * SN_WIN + static_cast<u32>(half);
    wd[sidx] = 0;
    u32 qn = 0;
    qx[qn] = static_cast<u8>(half);
    qy[qn] = static_cast<u8>(half);
    ++qn;
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    u32 hit = 0;
    for (u32 qh = 0; qh < qn && hit < need; ++qh) {
        const u8 lx = qx[qh];
        const u8 ly = qy[qh];
        const u16 d0 = wd[static_cast<u32>(ly) * SN_WIN + static_cast<u32>(lx)];
        const i32 wx = ox + static_cast<i32>(lx);
        const i32 wy = oy + static_cast<i32>(ly);
        if (wx >= 0 && wy >= 0 && wx < static_cast<i32>(m_w) && wy < static_cast<i32>(m_h)) {
            const u16 ux = static_cast<u16>(wx);
            const u16 uy = static_cast<u16>(wy);
            for (u32 b = 0; b < 6u; ++b) {
                if (nbid[b] == U16_KEY_NULL) {
                    continue;
                }
                if ((*mask & static_cast<u8>(1u << b)) != 0) {
                    continue;
                }
                if (ux == nbx[b] && uy == nby[b]) {
                    *mask = static_cast<u8>(*mask | static_cast<u8>(1u << b));
                    ++hit;
                }
            }
        }
        if (d0 >= SN_FLOOD_R) {
            continue;
        }
        for (i32 dir = 0; dir < 4; ++dir) {
            const i32 nlx = static_cast<i32>(lx) + dx4[dir];
            const i32 nly = static_cast<i32>(ly) + dy4[dir];
            if (nlx < 0 || nly < 0 || nlx >= static_cast<i32>(SN_WIN) || nly >= static_cast<i32>(SN_WIN)) {
                continue;
            }
            const u32 nidx = static_cast<u32>(nly) * SN_WIN + static_cast<u32>(nlx);
            if (wd[nidx] != U16_KEY_NULL) {
                continue;
            }
            const i32 nx = ox + nlx;
            const i32 ny = oy + nly;
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(m_w) || ny >= static_cast<i32>(m_h)) {
                continue;
            }
            if (!tile_pass(terr, static_cast<u16>(nx), static_cast<u16>(ny), water)) {
                continue;
            }
            wd[nidx] = static_cast<u16>(d0 + 1u);
            qx[qn] = static_cast<u8>(nlx);
            qy[qn] = static_cast<u8>(nly);
            ++qn;
        }
    }
}

void SectorNetwork::wire (const u8* terr) {
    u16 wd[SN_WIN_N];
    u8 qx[SN_WIN_N];
    u8 qy[SN_WIN_N];
    for (u16 i = 0; i < m_n; ++i) {
        flood_sec(i, terr, false, wd, qx, qy);
    }
    for (u16 i = 0; i < m_n; ++i) {
        flood_sec(i, terr, true, wd, qx, qy);
    }
}

bool SectorNetwork::begin (u16 w, u16 h, const u8* terr) {
    clear();
    if (w <= SN_ORIGIN || h <= SN_ORIGIN || terr == nullptr) {
        return false;
    }
    const u16 cols = static_cast<u16>((w - SN_ORIGIN - 1u) / SN_STEP + 1u);
    const u16 rows = static_cast<u16>((h - SN_ORIGIN + SN_OFF_EVEN - 1u) / SN_STEP + 1u);
    const u32 n32 = static_cast<u32>(cols) * static_cast<u32>(rows);
    if (cols == 0 || rows == 0 || n32 == 0 || n32 > 65535u) {
        return false;
    }
    for (u16 c = 0; c < cols; ++c) {
        const i32 cx = cen_x(static_cast<i32>(c));
        if (cx < 0 || cx >= static_cast<i32>(w)) {
            return false;
        }
        for (u16 r = 0; r < rows; ++r) {
            const i32 cy = cen_y(static_cast<i32>(c), static_cast<i32>(r));
            if (cy < 0 || cy >= static_cast<i32>(h)) {
                return false;
            }
        }
    }
    m_sec = new Sector[n32];
    m_cols = cols;
    m_rows = rows;
    m_w = w;
    m_h = h;
    m_n = static_cast<u16>(n32);
    for (u16 c = 0; c < cols; ++c) {
        for (u16 r = 0; r < rows; ++r) {
            const u16 id = static_cast<u16>(static_cast<u32>(c) * static_cast<u32>(rows) + static_cast<u32>(r));
            m_sec[id].m_x = static_cast<u16>(cen_x(static_cast<i32>(c)));
            m_sec[id].m_y = static_cast<u16>(cen_y(static_cast<i32>(c), static_cast<i32>(r)));
            m_sec[id].m_mask = 0;
            m_sec[id].m_wmask = 0;
        }
    }
    wire(terr);
    m_ok = true;
    return true;
}

u16 SectorNetwork::id_at (u16 x, u16 y) const {
    if (!m_ok || x >= m_w || y >= m_h) {
        return U16_KEY_NULL;
    }
    i32 c0 = (static_cast<i32>(x) - static_cast<i32>(SN_ORIGIN)) / static_cast<i32>(SN_STEP);
    if (c0 < 0) {
        c0 = 0;
    }
    if (c0 >= static_cast<i32>(m_cols)) {
        c0 = static_cast<i32>(m_cols) - 1;
    }
    i32 r0 = 0;
    if ((c0 & 1) == 0) {
        r0 = (static_cast<i32>(y) - (static_cast<i32>(SN_ORIGIN) - SN_OFF_EVEN)) / static_cast<i32>(SN_STEP);
    } else {
        r0 = (static_cast<i32>(y) - (static_cast<i32>(SN_ORIGIN) + SN_OFF_ODD)) / static_cast<i32>(SN_STEP);
    }
    if (r0 < 0) {
        r0 = 0;
    }
    if (r0 >= static_cast<i32>(m_rows)) {
        r0 = static_cast<i32>(m_rows) - 1;
    }
    u16 best = U16_KEY_NULL;
    u32 best_d = 0xffffffffu;
    for (i32 dc = -2; dc <= 2; ++dc) {
        for (i32 dr = -2; dr <= 2; ++dr) {
            const u16 id = id_of(c0 + dc, r0 + dr);
            if (id == U16_KEY_NULL) {
                continue;
            }
            const Sector& S = m_sec[id];
            const i32 dx = static_cast<i32>(S.m_x) - static_cast<i32>(x);
            const i32 dy = static_cast<i32>(S.m_y) - static_cast<i32>(y);
            const u32 d = static_cast<u32>(dx * dx + dy * dy);
            if (best == U16_KEY_NULL || d < best_d) {
                best = id;
                best_d = d;
            }
        }
    }
    return best;
}

u16 SectorNetwork::nbr (u16 id, u8 bit) const {
    if (!m_ok || id >= m_n || bit > 5u) {
        return U16_KEY_NULL;
    }
    if ((m_sec[id].m_mask & static_cast<u8>(1u << bit)) == 0) {
        return U16_KEY_NULL;
    }
    const i32 c = static_cast<i32>(id / m_rows);
    const i32 r = static_cast<i32>(id % m_rows);
    i32 dc = 0;
    i32 dr = 0;
    if ((c & 1) == 0) {
        const i32 edc[6] = {0, 0, -1, 1, -1, 1};
        const i32 edr[6] = {-1, 1, -1, -1, 0, 0};
        dc = edc[bit];
        dr = edr[bit];
    } else {
        const i32 odc[6] = {0, 0, -1, 1, -1, 1};
        const i32 odr[6] = {-1, 1, 0, 0, 1, 1};
        dc = odc[bit];
        dr = odr[bit];
    }
    return id_of(c + dc, r + dr);
}

u16 SectorNetwork::wnbr (u16 id, u8 bit) const {
    if (!m_ok || id >= m_n || bit > 5u) {
        return U16_KEY_NULL;
    }
    if ((m_sec[id].m_wmask & static_cast<u8>(1u << bit)) == 0) {
        return U16_KEY_NULL;
    }
    const i32 c = static_cast<i32>(id / m_rows);
    const i32 r = static_cast<i32>(id % m_rows);
    i32 dc = 0;
    i32 dr = 0;
    if ((c & 1) == 0) {
        const i32 edc[6] = {0, 0, -1, 1, -1, 1};
        const i32 edr[6] = {-1, 1, -1, -1, 0, 0};
        dc = edc[bit];
        dr = edr[bit];
    } else {
        const i32 odc[6] = {0, 0, -1, 1, -1, 1};
        const i32 odr[6] = {-1, 1, 0, 0, 1, 1};
        dc = odc[bit];
        dr = odr[bit];
    }
    return id_of(c + dc, r + dr);
}

const Sector* SectorNetwork::get (u16 id) const {
    if (!m_ok || id >= m_n) {
        return nullptr;
    }
    return &m_sec[id];
}

bool SectorNetwork::is_valid () const {
    return m_ok;
}

u16 SectorNetwork::sector_n () const {
    return m_n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
