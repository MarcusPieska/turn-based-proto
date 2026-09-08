//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "continent_size_indexer.h"

#include <cstring>

#include "game_map_defs.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_land (u8 cls) {
    if (cls == TERR_NONE[0]) {
        return false;
    }
    return !overlay_is_water_terr(cls);
}

static u32 tidx (u16 w, u32 x, u32 y) {
    return y * static_cast<u32>(w) + x;
}

static u32 flood_comp (const u8* terr, u16 w, u16 h, u16 idx, u32 seed_i, u16* ov, u32* q) {
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    u32 qn = 0u;
    u32 tiles = 0u;
    ov[seed_i] = idx;
    q[qn++] = seed_i;
    for (u32 qh = 0u; qh < qn; ++qh) {
        const u32 i = q[qh];
        ++tiles;
        const u32 py = i / wi;
        const u32 px = i - py * wi;
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(px) + dx4[d];
            const i32 ny = static_cast<i32>(py) + dy4[d];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                continue;
            }
            const u32 ni = tidx(w, static_cast<u32>(nx), static_cast<u32>(ny));
            if (ov[ni] != 0u || !is_land(terr[ni])) {
                continue;
            }
            ov[ni] = idx;
            q[qn++] = ni;
        }
    }
    return tiles;
}

static void half_pal (u16 i, u8* r, u8* g, u8* b) {
    static const u8 k[ContSizeList::k_cap][3] = {
        {160u, 40u, 40u}, {160u, 100u, 40u}, {160u, 160u, 40u}, {100u, 160u, 40u},
        {40u, 160u, 40u}, {40u, 160u, 100u}, {40u, 160u, 160u}, {40u, 100u, 160u},
        {40u, 40u, 160u}, {100u, 40u, 160u}, {160u, 40u, 160u}, {160u, 40u, 100u},
        {120u, 60u, 40u}, {60u, 120u, 40u}, {40u, 120u, 100u}, {40u, 60u, 120u},
        {100u, 40u, 120u}, {120u, 40u, 60u}, {80u, 80u, 40u}, {80u, 40u, 80u}};
    const u16 j = static_cast<u16>(i % ContSizeList::k_cap);
    *r = k[j][0];
    *g = k[j][1];
    *b = k[j][2];
}

static u32 pack_rgb (u8 r, u8 g, u8 b) {
    return (static_cast<u32>(r) << 16) | (static_cast<u32>(g) << 8) | static_cast<u32>(b);
}

static void push_top (ContSizeList* top, u16 id, u32 tiles) {
    if (top == nullptr || id == 0u || tiles == 0u) {
        return;
    }
    u16 at = 0u;
    if (top->m_n < ContSizeList::k_cap) {
        at = top->m_n;
        top->m_e[at].m_id = id;
        top->m_e[at].m_tiles = tiles;
        top->m_n = static_cast<u16>(top->m_n + 1u);
    } else if (tiles <= top->m_e[ContSizeList::k_cap - 1u].m_tiles) {
        return;
    } else {
        at = static_cast<u16>(ContSizeList::k_cap - 1u);
        top->m_e[at].m_id = id;
        top->m_e[at].m_tiles = tiles;
    }
    while (at > 0u && top->m_e[at].m_tiles > top->m_e[at - 1u].m_tiles) {
        const ContSizeEnt t = top->m_e[at - 1u];
        top->m_e[at - 1u] = top->m_e[at];
        top->m_e[at] = t;
        --at;
    }
}

//================================================================================================================================
//=> - ContinentSizeIndexer -
//================================================================================================================================

bool ContinentSizeIndexer::index (
    const u8* terr,
    u16 w,
    u16 h,
    ContSizeList* out,
    Whiteboard_4B& out_rgb,
    Whiteboard_2B& out_idx)
{
    if (terr == nullptr || out == nullptr || w == 0u || h == 0u) {
        return false;
    }
    if (!out_rgb.ok() || out_rgb.w() != w || out_rgb.h() != h) {
        return false;
    }
    if (!out_idx.ok() || out_idx.w() != w || out_idx.h() != h) {
        return false;
    }
    if (WhiteboardMng::width() != w || WhiteboardMng::height() != h) {
        return false;
    }
    out->m_n = 0u;
    for (u16 i = 0; i < ContSizeList::k_cap; ++i) {
        out->m_e[i].m_id = 0u;
        out->m_e[i].m_tiles = 0u;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_2B wb_ov("ContinentSizeIndexer", "ov", 0u);
    Whiteboard_4B wb_q("ContinentSizeIndexer", "flood_q", 0u);
    if (!wb_ov.ok() || !wb_q.ok()) {
        return false;
    }
    u16* ov = wb_ov.get_iter_ptr();
    u32* q = wb_q.get_iter_ptr();
    u32* rgb = out_rgb.get_iter_ptr();
    u16* idx_out = out_idx.get_iter_ptr();
    if (ov == nullptr || q == nullptr || rgb == nullptr || idx_out == nullptr) {
        return false;
    }
    std::memset(ov, 0, static_cast<size_t>(n) * sizeof(u16));
    std::memset(rgb, 0, static_cast<size_t>(n) * sizeof(u32));
    std::memset(idx_out, 0, static_cast<size_t>(n) * sizeof(u16));
    u16 idx = 0u;
    for (u32 i = 0; i < n; ++i) {
        if (!is_land(terr[i]) || ov[i] != 0u) {
            continue;
        }
        if (idx == U16_KEY_NULL) {
            return false;
        }
        idx = static_cast<u16>(idx + 1u);
        const u32 tiles = flood_comp(terr, w, h, idx, i, ov, q);
        push_top(out, idx, tiles);
    }
    u16 remap_n = 0u;
    u16 remap_id[ContSizeList::k_cap];
    for (u16 i = 0; i < out->m_n; ++i) {
        remap_id[i] = out->m_e[i].m_id;
        ++remap_n;
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 id = ov[i];
        if (id == 0u) {
            continue;
        }
        u16 rank = U16_KEY_NULL;
        for (u16 r = 0; r < remap_n; ++r) {
            if (remap_id[r] == id) {
                rank = r;
                break;
            }
        }
        if (rank == U16_KEY_NULL) {
            continue;
        }
        u8 cr = 0u;
        u8 cg = 0u;
        u8 cb = 0u;
        half_pal(rank, &cr, &cg, &cb);
        rgb[i] = pack_rgb(cr, cg, cb);
        idx_out[i] = static_cast<u16>(rank + 1u);
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
