//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <algorithm>
#include <vector>

#include "p1_adj_grassland_loess_tiles.h"
#include "game_map_defs.h"
#include "p1_wb_util.h"
#include "wb_que_xy.h"

//================================================================================================================================
//=> - Private adjustment helpers -
//================================================================================================================================

struct GrassCand {
    u16 m_x;
    u16 m_y;
    u16 m_eff;
};

static bool grass_cand_gt (const GrassCand& a, const GrassCand& b) {
    if (a.m_eff != b.m_eff) {
        return a.m_eff > b.m_eff;
    }
    if (a.m_y != b.m_y) {
        return a.m_y < b.m_y;
    }
    return a.m_x < b.m_x;
}

static bool is_land_terr (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0] || cls == TERR_MOUNTAINS[0];
}

static bool is_water_terr (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0]
        || cls == TERR_INLAND_SEA[0] || cls == TERR_INLAND_LAKE[0];
}

static bool grass_inner (
    const u8* terrain,
    const u8* climate,
    u16 w,
    u16 h,
    u16 px,
    u16 py) 
{
    for (i32 oy = -1; oy <= 1; ++oy) {
        for (i32 ox = -1; ox <= 1; ++ox) {
            if (ox == 0 && oy == 0) {
                continue;
            }
            const i32 nx = static_cast<i32>(px) + ox;
            const i32 ny = static_cast<i32>(py) + oy;
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                return false;
            }
            const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
            if (climate[j] == CLIMATE_GRASSLAND) {
                continue;
            }
            if (is_water_terr(terrain[j])) {
                continue;
            }
            return false;
        }
    }
    return true;
}

static u16 rain_scaled (u8 rain, u16 wt) {
    const u32 v = (static_cast<u32>(rain) * static_cast<u32>(wt) + static_cast<u32>(CLIMATE_WT_MAX) / 2u)
        / static_cast<u32>(CLIMATE_WT_MAX);
    return static_cast<u16>(v > 255u ? 255u : v);
}

static u16 loess_net (u8 loess, u16 adj) {
    const u16 lv = static_cast<u16>(loess);
    if (lv == 0u) {
        return 0u;
    }
    if (lv <= adj) {
        return 1u;
    }
    return static_cast<u16>(lv - adj);
}

static void que_to_vec (const WB_QueXY& que, const u16* eff, u16 w, std::vector<GrassCand>* out) {
    if (out == nullptr || eff == nullptr) {
        return;
    }
    const u32 qn = que.count();
    out->reserve(out->size() + static_cast<size_t>(qn));
    for (u32 k = 0; k < qn; ++k) {
        const u16 px = que.x_at(k);
        const u16 py = que.y_at(k);
        const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
        GrassCand c;
        c.m_x = px;
        c.m_y = py;
        c.m_eff = eff[i];
        out->push_back(c);
    }
}

static u32 pick_vec (const std::vector<GrassCand>& vec, u32 cap, u32 done, u8* climate, u16 w) {
    for (size_t k = 0; k < vec.size() && done < cap; ++k) {
        const GrassCand& c = vec[k];
        const u32 i = static_cast<u32>(c.m_y) * static_cast<u32>(w) + static_cast<u32>(c.m_x);
        climate[i] = CLIMATE_BLACK_SOIL;
        ++done;
    }
    return done;
}

static bool apply_grass_loess (
    u8* terrain,
    u8* climate,
    const u8* loess,
    const u8* rain,
    u16 w,
    u16 h,
    u16 rain_wt,
    u8 land_pct,
    u32* out_pick) 
{
    if (terrain == nullptr || climate == nullptr || loess == nullptr || rain == nullptr
        || out_pick == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_2B wb_adj("P1_Adj_GrasslandLoessTiles", "adj", 0u);
    P1_WB_CHK(wb_adj);
    Whiteboard_2B wb_eff("P1_Adj_GrasslandLoessTiles", "eff", 0u);
    P1_WB_CHK(wb_eff);
    WB_QueXY que_in;
    WB_QueXY que_out;
    if (!que_in.ok() || !que_out.ok()) {
        return false;
    }
    u16* adj = wb_adj.get_iter_ptr();
    u16* eff = wb_eff.get_iter_ptr();
    u32 land_n = 0;
    for (u32 i = 0; i < n; ++i) {
        adj[i] = rain_scaled(rain[i], rain_wt);
        eff[i] = loess_net(loess[i], adj[i]);
        if (is_land_terr(terrain[i])) {
            ++land_n;
        }
    }
    u32 cap = 0;
    if (land_pct > 0) {
        cap = (land_n * static_cast<u32>(land_pct) + 50u) / 100u;
    }
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            if (climate[i] != CLIMATE_GRASSLAND || eff[i] == 0u) {
                continue;
            }
            if (grass_inner(terrain, climate, w, h, px, py)) {
                if (!que_in.push(px, py)) {
                    return false;
                }
            } else if (!que_out.push(px, py)) {
                return false;
            }
        }
    }
    std::vector<GrassCand> vec_in;
    std::vector<GrassCand> vec_out;
    vec_in.reserve(static_cast<size_t>(que_in.count()));
    vec_out.reserve(static_cast<size_t>(que_out.count()));
    que_to_vec(que_in, eff, w, &vec_in);
    que_to_vec(que_out, eff, w, &vec_out);
    std::sort(vec_in.begin(), vec_in.end(), grass_cand_gt);
    std::sort(vec_out.begin(), vec_out.end(), grass_cand_gt);
    u32 done = pick_vec(vec_in, cap, 0u, climate, w);
    done = pick_vec(vec_out, cap, done, climate, w);
    *out_pick = done;
    return true;
}

//================================================================================================================================
//=> - P1_Adj_GrasslandLoessTiles -
//================================================================================================================================

P1_Adj_GrasslandLoessTiles::P1_Adj_GrasslandLoessTiles (
    const P1_RunPrm& prm,
    const P1_Adj_GrasslandLoessTilesPrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_valid_adjust(false),
    m_pick_n(0u) {
}

bool P1_Adj_GrasslandLoessTiles::adjust (
    u8* terrain,
    u8* climate,
    const u8* loess,
    const u8* rain,
    u16 w,
    u16 h) 
{
    m_valid_adjust = false;
    m_pick_n = 0u;
    if (!p1_run_prm_ok(m_prm) || terrain == nullptr || climate == nullptr || loess == nullptr
        || rain == nullptr || w == 0 || h == 0) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    u32 pick_n = 0;
    if (!apply_grass_loess(terrain, climate, loess, rain, w, h, m_sp.m_w_rain, m_sp.m_land_pct, &pick_n)) {
        return false;
    }
    m_pick_n = pick_n;
    m_valid_adjust = true;
    return true;
}

bool P1_Adj_GrasslandLoessTiles::is_valid () const {
    return m_valid_adjust;
}

u32 P1_Adj_GrasslandLoessTiles::picked_n () const {
    return m_pick_n;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
