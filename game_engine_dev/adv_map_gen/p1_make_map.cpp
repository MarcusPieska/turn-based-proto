//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_make_map.h"

#include "p1_adj_grassland_loess_tiles.h"
#include "p1_adj_coast_fertility.h"
#include "p1_adj_ensure_adj_rules.h"
#include "p1_adj_ensure_coasts.h"
#include "p1_adj_ensure_mtn_foothills.h"
#include "p1_adj_ensure_river_valleys.h"
#include "p1_adj_ensure_seas.h"
#include "p1_adj_land_altitude.h"
#include "p1_adj_outline_fill.h"
#include "p1_gen_shaped_outline.h"
#include "p1_adj_coastal_mtn_rivers.h"
#include "p1_adj_river_inlets.h"
#include "p1_adj_river_lakes.h"
#include "p1_gen_climate.h"
#include "p1_gen_desert_river_cull.h"
#include "p1_gen_distance_to_river.h"
#include "p1_gen_land_depth.h"
#include "p1_gen_loess_boost.h"
#include "p1_gen_nearness_to_watershed_mtn.h"
#include "p1_gen_noise_perlin.h"
#include "p1_gen_cont_outlines.h"
#include "p1_gen_rich_coast_fertility.h"
#include "p1_gen_rain_orographic.h"
#include "p1_gen_river_lines.h"
#include "p1_gen_river_dist.h"
#include "p1_gen_river_network.h"
#include "p1_gen_river_pts.h"
#include "p1_gen_river_sectors.h"
#include "p1_gen_coastal_mtn_limits.h"
#include "p1_gen_watershed_mountains.h"
#include "p1_gen_watershed_mountain_line_sets.h"
#include "p1_gen_wind_pattern_adv.h"
#include "map_terrain_data.h"
#include "p1_tester_util.h"
#include "p1_wb_util.h"

#include <cstdio>
#include <ctime>
#include <cstring>

//================================================================================================================================
//=> - Private timing helpers -
//================================================================================================================================

#ifdef P1_ENABLE_TIMING

static void p1_mk_print_time (cstr step, double sec) {
    char lbl[128];
    std::snprintf(lbl, sizeof(lbl), "P1_MakeMap %s ", step);
    const size_t ll = std::strlen(lbl);
    const size_t col_time = 59;
    std::printf("%s", lbl);
    for (size_t p = ll; p < col_time; ++p) {
        std::printf("-");
    }
    const double ms = sec * 1000.0;
    if (ms < 1000.0) {
        std::printf(" %.2f ms\n", ms);
    } else {
        std::printf(" %.2f s\n", sec);
    }
}

struct P1_MkTimer {
    clock_t m_t0;
    cstr m_name;
    P1_MkTimer (cstr name) : m_t0(clock()), m_name(name) {}
    ~P1_MkTimer () {
        const clock_t t1 = clock();
        const double sec = static_cast<double>(t1 - m_t0) / static_cast<double>(CLOCKS_PER_SEC);
        p1_mk_print_time(m_name, sec);
    }
};

#define P1_MK_TIME(step) P1_MkTimer _p1_mk_t_##__LINE__(step)

#else

#define P1_MK_TIME(step)

#endif

static bool copy_u8_own (u8** dst, const u8* src, u32 n) {
    if (dst == nullptr) {
        return false;
    }
    delete[] *dst;
    *dst = nullptr;
    if (src == nullptr || n == 0u) {
        return false;
    }
    *dst = new u8[n];
    if (*dst == nullptr) {
        return false;
    }
    std::memcpy(*dst, src, static_cast<size_t>(n));
    return true;
}

static bool copy_u16_own (u16** dst, const u16* src, u32 n) {
    if (dst == nullptr) {
        return false;
    }
    delete[] *dst;
    *dst = nullptr;
    if (src == nullptr || n == 0u) {
        return false;
    }
    *dst = new u16[n];
    if (*dst == nullptr) {
        return false;
    }
    std::memcpy(*dst, src, static_cast<size_t>(n) * sizeof(u16));
    return true;
}

static void free_mk_early (u8* terrain, u8* river, u16* wshed) {
    delete[] wshed;
    delete[] river;
    delete[] terrain;
}

//================================================================================================================================
//=> - P1_MakeMap -
//================================================================================================================================

P1_MakeMap::P1_MakeMap (const P1_RunPrm& prm, const P1_MakeMapPrm& mp) :
    m_prm(prm),
    m_mp(mp),
    m_valid_generation(false) {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_terrain = nullptr;
    m_rslt.m_climate = nullptr;
    m_rslt.m_rivers = nullptr;
    m_rslt.m_wshed = nullptr;
    m_rslt.m_wind_dir = nullptr;
    m_rslt.m_wind_str = nullptr;
    m_rslt.m_loess = nullptr;
    m_rslt.m_rain = nullptr;
}

void P1_MakeMap::free_rslt (P1_MakeMapRslt* rslt) {
    if (rslt == nullptr) {
        return;
    }
    delete[] rslt->m_terrain;
    delete[] rslt->m_climate;
    delete[] rslt->m_rivers;
    delete[] rslt->m_wshed;
    delete[] rslt->m_wind_dir;
    delete[] rslt->m_wind_str;
    delete[] rslt->m_loess;
    delete[] rslt->m_rain;
    rslt->m_terrain = nullptr;
    rslt->m_climate = nullptr;
    rslt->m_rivers = nullptr;
    rslt->m_wshed = nullptr;
    rslt->m_wind_dir = nullptr;
    rslt->m_wind_str = nullptr;
    rslt->m_loess = nullptr;
    rslt->m_rain = nullptr;
    rslt->m_w = 0;
    rslt->m_h = 0;
}

bool P1_MakeMap::copy_rslt (P1_MakeMapRslt* dst, const P1_MakeMapRslt& src) {
    if (dst == nullptr || src.m_w == 0 || src.m_h == 0) {
        return false;
    }
    const u32 npx = static_cast<u32>(src.m_w) * static_cast<u32>(src.m_h);
    free_rslt(dst);
    if (!copy_u8_own(&dst->m_terrain, src.m_terrain, npx) || !copy_u8_own(&dst->m_rivers, src.m_rivers, npx)) {
        free_rslt(dst);
        return false;
    }
    if (src.m_wshed != nullptr && !copy_u16_own(&dst->m_wshed, src.m_wshed, npx)) {
        free_rslt(dst);
        return false;
    }
    if (src.m_climate != nullptr && !copy_u8_own(&dst->m_climate, src.m_climate, npx)) {
        free_rslt(dst);
        return false;
    }
    if (src.m_wind_dir != nullptr && !copy_u8_own(&dst->m_wind_dir, src.m_wind_dir, npx)) {
        free_rslt(dst);
        return false;
    }
    if (src.m_wind_str != nullptr && !copy_u8_own(&dst->m_wind_str, src.m_wind_str, npx)) {
        free_rslt(dst);
        return false;
    }
    if (src.m_loess != nullptr && !copy_u8_own(&dst->m_loess, src.m_loess, npx)) {
        free_rslt(dst);
        return false;
    }
    if (src.m_rain != nullptr && !copy_u8_own(&dst->m_rain, src.m_rain, npx)) {
        free_rslt(dst);
        return false;
    }
    dst->m_w = src.m_w;
    dst->m_h = src.m_h;
    return true;
}

bool P1_MakeMap::generate (u16 last_step) {
    m_valid_generation = false;
    free_rslt(&m_rslt);
    if (!p1_run_prm_ok(m_prm)) {
        return false;
    }
    const u16 w = m_prm.m_w;
    const u16 h = m_prm.m_h;
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    if (WhiteboardMng::tile_n() == 0u) {
        p1_wb_init(w, h);
    }
    u8* terrain = new u8[npx];
    if (terrain == nullptr) {
        return false;
    }
    u8* river = nullptr;
    u16* wshed = nullptr;
    u8* climate_copy = nullptr;
    bool ok = true;
    f32* perlin_f32 = nullptr;
    {
        const u8* noise = nullptr;
        const u8* ov = nullptr;
        const u16* land_depth = nullptr;
        P1_Gen_NoisePerlin noise_gen(m_prm);
        MapArrayOverlay ov_map;
        P1_Gen_LandDepth depth_gen(m_prm);
        P1_Gen_DistanceToRiver dist_gen(m_prm);
        P1_Gen_NearnessToWatershedMtn near_gen(m_prm);
        P1_Gen_RiverLines lin_gen(m_prm);
        P1_Gen_RiverNetwork net_gen(m_prm);
        const P1_Gen_NoisePerlinPrm nprm = p1_gen_noise_perlin_prm_from_cfg(p1_map_config_def(), w, h);
        perlin_f32 = new f32[npx];
        if (perlin_f32 == nullptr) {
            delete[] terrain;
            return false;
        }
    {
        P1_MK_TIME("03 noise_perlin");
        ok = noise_gen.generate_with_combo(m_prm.m_seed, nprm, perlin_f32) && noise_gen.is_valid();
        if (ok) {
            noise = noise_gen.result().m_ov.data();
            ok = noise != nullptr;
        }
    }
    if (!ok) {
        delete[] perlin_f32;
        delete[] terrain;
        return false;
    }
    {
        P1_MK_TIME("01 cont_outline");
        ok = p1_gen_step01_ov(m_prm, &ov_map);
        if (ok) {
            ov = ov_map.data();
            ok = ov != nullptr && ov_map.width() == w && ov_map.height() == h;
        }
    }
    if (!ok) {
        delete[] perlin_f32;
        delete[] terrain;
        return false;
    }
    {
        P1_MK_TIME("04 land_depth");
        ok = depth_gen.generate(ov, w, h) && depth_gen.is_valid();
        if (ok) {
            land_depth = depth_gen.result().m_dist.data();
            ok = land_depth != nullptr;
        }
    }
    if (!ok) {
        delete[] perlin_f32;
        delete[] terrain;
        return false;
    }
    {
        P1_MK_TIME("02 outline_fill");
        P1_Adj_OutlineFill fill(m_prm);
        ok = fill.adjust(terrain, w, h, ov) && fill.is_valid();
    }
    if (!ok) {
        delete[] perlin_f32;
        delete[] terrain;
        return false;
    }
    {
        const u32 npx_sh = static_cast<u32>(w) * static_cast<u32>(h);
        u8* near_ter = new u8[npx_sh];
        u8* far_ter = new u8[npx_sh];
        if (near_ter == nullptr || far_ter == nullptr) {
            delete[] far_ter;
            delete[] near_ter;
            delete[] perlin_f32;
            delete[] terrain;
            return false;
        }
        std::memcpy(near_ter, terrain, static_cast<size_t>(npx_sh));
        std::memcpy(far_ter, terrain, static_cast<size_t>(npx_sh));
        P1_Gen_ShapedOutline shaped(m_prm);
        shaped.bind_perlin_field(perlin_f32, w, h);
        {
            P1_MK_TIME("05 shaped_outline_near");
            ok = shaped.generate_layer(near_ter, w, h, ov, land_depth, m_mp.m_shaped.m_radial_near, m_mp.m_shaped.m_shelf_near)
                && shaped.is_valid();
        }
        if (!ok) {
            delete[] far_ter;
            delete[] near_ter;
            delete[] perlin_f32;
            delete[] terrain;
            return false;
        }
        {
            P1_MK_TIME("06 shaped_outline_far");
            ok = shaped.generate_layer(far_ter, w, h, ov, land_depth, m_mp.m_shaped.m_radial_far, m_mp.m_shaped.m_shelf_far)
                && shaped.is_valid();
        }
        if (!ok) {
            delete[] far_ter;
            delete[] near_ter;
            delete[] perlin_f32;
            delete[] terrain;
            return false;
        }
        {
            P1_MK_TIME("07 shaped_outline_merge");
            ok = shaped.merge_layers(terrain, w, h, ov, land_depth, near_ter, far_ter)
                && shaped.is_valid();
        }
        delete[] far_ter;
        delete[] near_ter;
    }
    delete[] perlin_f32;
    if (!ok) {
        delete[] terrain;
        return false;
    }
    P1_Gen_RiverPts pts_gen(m_prm);
    P1_Gen_RiverSectors sec_gen(m_prm);
    P1_Gen_CoastalMtnLimits coast_lim(m_prm);
    {
        P1_MK_TIME("08 river_pts");
        ok = pts_gen.generate(terrain, w, h) && pts_gen.is_valid();
    }
    if (!ok) {
        delete[] terrain;
        return false;
    }
    {
        P1_MK_TIME("09 river_sectors");
        ok = sec_gen.generate(terrain, w, h, pts_gen.result()) && sec_gen.is_valid();
    }
    if (!ok) {
        delete[] terrain;
        return false;
    }
    {
        P1_MK_TIME("10 coastal_mtn_limits");
        ok = coast_lim.generate(terrain, w, h, sec_gen.result()) && coast_lim.is_valid();
    }
    if (!ok) {
        delete[] terrain;
        return false;
    }
    {
        P1_MK_TIME("11 river_network");
        ok = net_gen.generate(terrain, w, h, sec_gen.result(), coast_lim.result()) && net_gen.is_valid();
    }
    if (!ok) {
        delete[] terrain;
        return false;
    }
    wshed = new u16[npx];
    if (wshed == nullptr) {
        delete[] terrain;
        return false;
    }
    std::memcpy(wshed, net_gen.result().m_ov, static_cast<size_t>(npx) * sizeof(u16));
    {
        P1_MK_TIME("11 river_lines");
        ok = lin_gen.generate(terrain, w, h, sec_gen.result(), net_gen.result())
            && lin_gen.is_valid();
    }
    if (!ok) {
        free_mk_early(terrain, nullptr, wshed);
        return false;
    }
    river = new u8[npx];
    if (river == nullptr) {
        free_mk_early(terrain, nullptr, wshed);
        return false;
    }
    std::memcpy(river, lin_gen.result().m_ov, static_cast<size_t>(npx));
    {
        P1_MK_TIME("12 coastal_mtn_rivers");
        P1_Adj_CoastalMtnRivers cmr(m_prm);
        ok = cmr.adjust(terrain, w, h, river, sec_gen.result(), coast_lim.result()) && cmr.is_valid();
    }
    if (!ok) {
        free_mk_early(terrain, river, wshed);
        return false;
    }
    {
        P1_MK_TIME("13 river_lakes");
        P1_Adj_RiverLakes lakes(m_prm);
        ok = lakes.adjust(terrain, w, h, river) && lakes.is_valid();
    }
    if (!ok) {
        free_mk_early(terrain, river, wshed);
        return false;
    }
    {
        P1_MK_TIME("14 river_inlets");
        P1_Adj_RiverInlets inlets(m_prm);
        ok = inlets.adjust(terrain, w, h, river, lin_gen.result()) && inlets.is_valid();
    }
    if (!ok) {
        free_mk_early(terrain, river, wshed);
        return false;
    }
    u8* dist_riv = nullptr;
    {
        P1_MK_TIME("15 distance_to_river");
        ok = dist_gen.generate(terrain, w, h, river) && dist_gen.is_valid();
        if (ok) {
            dist_riv = const_cast<u8*>(dist_gen.result().m_ov.data());
            ok = dist_riv != nullptr;
        }
    }
    if (!ok) {
        free_mk_early(terrain, river, wshed);
        return false;
    }
    const u8* near_mtn = nullptr;
    P1_Gen_WatershedMountains border_gen(m_prm);
    P1_Gen_WatershedMountainLineSets line_gen(m_prm);
    {
        P1_MK_TIME("14 watershed_mountains");
        ok = border_gen.generate(terrain, w, h, net_gen.result(), noise) && border_gen.is_valid();
    }
    if (!ok) {
        free_mk_early(terrain, river, wshed);
        return false;
    }
    {
        P1_MK_TIME("14 watershed_mountain_line_sets");
        ok = line_gen.generate(border_gen.result()) && line_gen.is_valid();
    }
    if (!ok) {
        free_mk_early(terrain, river, wshed);
        return false;
    }
    {
        P1_MK_TIME("16 nearness_watershed_mtn");
        ok = near_gen.generate(terrain, w, h, line_gen.result(), coast_lim.result()) && near_gen.is_valid();
        if (ok) {
            near_mtn = near_gen.result().m_ov.data();
            ok = near_mtn != nullptr;
        }
    }
    if (!ok) {
        free_mk_early(terrain, river, wshed);
        return false;
    }
    {
        P1_MK_TIME("17 land_altitude");
        P1_Adj_LandAltitude alt_gen(m_prm, m_mp.m_lap);
        ok = alt_gen.adjust(terrain, w, h, noise, dist_riv, near_mtn) && alt_gen.is_valid();
    }
    if (!ok) {
        free_mk_early(terrain, river, wshed);
        return false;
    }
    {
        P1_MK_TIME("18 ensure_coasts");
        P1_Adj_EnsureCoasts coasts(m_prm);
        ok = coasts.adjust(terrain, w, h) && coasts.is_valid();
    }
    if (!ok) {
        free_mk_early(terrain, river, wshed);
        return false;
    }
    {
        P1_MK_TIME("19 ensure_seas");
        P1_Adj_EnsureSeas seas(m_prm);
        ok = seas.adjust(terrain, w, h) && seas.is_valid();
    }
    if (!ok) {
        free_mk_early(terrain, river, wshed);
        return false;
    }
    {
        P1_MK_TIME("20 ensure_river_valleys");
        P1_Gen_RiverDist riv_dist_gen(m_prm);
        ok = riv_dist_gen.generate(terrain, w, h, river, sec_gen.result(), net_gen.result())
            && riv_dist_gen.is_valid();
        const u16* dist_dn = nullptr;
        if (ok) {
            dist_dn = riv_dist_gen.result().m_dn.data();
            ok = dist_dn != nullptr;
        }
        P1_Adj_EnsureRiverValleys valleys(m_prm);
        if (ok) {
            ok = valleys.adjust(terrain, w, h, river, dist_dn) && valleys.is_valid();
        }
    }
    if (!ok) {
        free_mk_early(terrain, river, wshed);
        return false;
    }
    {
        P1_MK_TIME("21 ensure_mtn_foothills");
        P1_Adj_EnsureMtnFoothills foothills(m_prm);
        ok = foothills.adjust(terrain, w, h) && foothills.is_valid();
    }
    if (!ok) {
        free_mk_early(terrain, river, wshed);
        return false;
    }
    }
    m_rslt.m_w = w;
    m_rslt.m_h = h;
    m_rslt.m_terrain = terrain;
    m_rslt.m_rivers = river;
    m_rslt.m_wshed = wshed;
    if (last_step <= k_p1_step_foothills) {
        m_valid_generation = true;
        return true;
    }
    if (last_step >= k_p1_step_wind) {
        P1_MK_TIME("22 wind_pattern_adv");
        P1_Gen_WindPatternAdv wind_gen(m_prm, m_mp.m_wind);
        ok = wind_gen.generate(terrain, w, h) && wind_gen.is_valid();
        if (ok) {
            const u8* wdir = wind_gen.result().m_dir.data();
            const u8* wstr = wind_gen.result().m_str.data();
            ok = wdir != nullptr && wstr != nullptr
                && copy_u8_own(&m_rslt.m_wind_dir, wdir, npx)
                && copy_u8_own(&m_rslt.m_wind_str, wstr, npx);
        }
    }
    if (!ok) {
        free_rslt(&m_rslt);
        return false;
    }
    if (last_step <= k_p1_step_wind) {
        m_valid_generation = true;
        return true;
    }
    if (last_step >= k_p1_step_rain) {
        P1_MK_TIME("23 rain_orographic");
        P1_Gen_RainOrographic rain_gen(m_prm, m_mp.m_rain_finish, m_mp.m_slope_finish, m_mp.m_rain);
        ok = rain_gen.generate(terrain, m_rslt.m_wind_dir, m_rslt.m_wind_str, w, h) && rain_gen.is_valid();
        if (ok) {
            const u8* rov = rain_gen.result().m_rain.data();
            ok = rov != nullptr && copy_u8_own(&m_rslt.m_rain, rov, npx);
        }
    }
    if (!ok) {
        free_rslt(&m_rslt);
        return false;
    }
    if (last_step <= k_p1_step_rain) {
        m_valid_generation = true;
        return true;
    }
    const u8* climate = nullptr;
    if (last_step >= k_p1_step_climate) {
        P1_MK_TIME("24 climate");
        P1_Gen_Climate climate_gen(m_prm, m_mp.m_climate);
        ok = climate_gen.generate(terrain, w, h, river, m_rslt.m_rain) && climate_gen.is_valid();
        if (ok) {
            climate = climate_gen.result().m_ov.data();
            ok = climate != nullptr;
        }
    }
    if (!ok) {
        free_rslt(&m_rslt);
        return false;
    }
    if (last_step <= k_p1_step_climate) {
        climate_copy = new u8[npx];
        if (climate_copy == nullptr) {
            free_rslt(&m_rslt);
            return false;
        }
        std::memcpy(climate_copy, climate, static_cast<size_t>(npx));
        m_rslt.m_climate = climate_copy;
        m_valid_generation = true;
        return true;
    }
    if (last_step >= k_p1_step_desert_cull) {
        P1_MK_TIME("25 desert_river_cull");
        P1_Gen_DesertRiverCull cull_gen(m_prm);
        ok = cull_gen.generate(river, w, h, terrain, climate, false) && cull_gen.is_valid();
    }
    if (!ok) {
        free_rslt(&m_rslt);
        return false;
    }
    if (last_step <= k_p1_step_desert_cull) {
        climate_copy = new u8[npx];
        if (climate_copy == nullptr) {
            free_rslt(&m_rslt);
            return false;
        }
        std::memcpy(climate_copy, climate, static_cast<size_t>(npx));
        m_rslt.m_climate = climate_copy;
        m_valid_generation = true;
        return true;
    }
    climate_copy = new u8[npx];
    if (climate_copy == nullptr) {
        free_rslt(&m_rslt);
        return false;
    }
    std::memcpy(climate_copy, climate, static_cast<size_t>(npx));
    m_rslt.m_climate = climate_copy;
    if (last_step >= k_p1_step_loess) {
        P1_MK_TIME("26 loess_boost");
        P1_Gen_LoessBoost loess_gen(m_prm, m_mp.m_loess);
        ok = loess_gen.generate(climate_copy, m_rslt.m_wind_dir, m_rslt.m_wind_str, w, h) && loess_gen.is_valid();
        if (ok) {
            const u8* lov = loess_gen.result().m_ov.data();
            ok = lov != nullptr && copy_u8_own(&m_rslt.m_loess, lov, npx);
        }
    }
    if (!ok) {
        free_rslt(&m_rslt);
        return false;
    }
    if (last_step <= k_p1_step_loess) {
        m_valid_generation = true;
        return true;
    }
    if (last_step >= k_p1_step_grass_loess) {
        P1_MK_TIME("27 grassland_loess_tiles");
        P1_Adj_GrasslandLoessTiles grass_adj(m_prm, m_mp.m_grass_loess);
        ok = grass_adj.adjust(terrain, climate_copy, m_rslt.m_loess, m_rslt.m_rain, w, h) && grass_adj.is_valid();
    }
    if (!ok) {
        free_rslt(&m_rslt);
        return false;
    }
    if (last_step <= k_p1_step_grass_loess && last_step != k_p1_step_seed_export) {
        m_valid_generation = true;
        return true;
    }
    if (last_step >= k_p1_step_rich_coast_fert || last_step == k_p1_step_seed_export) {
        P1_MK_TIME("30 rich_coast_fertility");
        P1_Gen_RichCoastFertility fert_gen(m_prm, m_mp.m_rich_coast);
        ok = fert_gen.generate(terrain, w, h) && fert_gen.is_valid();
        const u16* coast_fert = ok ? fert_gen.result().m_ov : nullptr;
        if (ok && (last_step >= k_p1_step_coast_fert_adj || last_step == k_p1_step_seed_export)) {
            P1_MK_TIME("31 coast_fertility_adj");
            P1_Adj_CoastFertility cf_adj(m_prm, m_mp.m_coast_fert);
            ok = coast_fert != nullptr
                && cf_adj.adjust(climate_copy, w, h, coast_fert) && cf_adj.is_valid();
        }
    }
    if (!ok) {
        free_rslt(&m_rslt);
        return false;
    }
    if (last_step <= k_p1_step_coast_fert_adj && last_step != k_p1_step_seed_export) {
        m_valid_generation = true;
        return true;
    }
    if (last_step >= k_p1_step_ensure_adj || last_step == k_p1_step_seed_export) {
        P1_MK_TIME("32 ensure_adj_rules");
        P1_Adj_EnsureAdjRules adj_rules(m_prm);
        ok = adj_rules.adjust(terrain, climate_copy, river, w, h) && adj_rules.is_valid();
    }
    if (!ok) {
        free_rslt(&m_rslt);
        return false;
    }
    if (last_step <= k_p1_step_ensure_adj && last_step != k_p1_step_seed_export) {
        m_valid_generation = true;
        return true;
    }
    m_valid_generation = true;
    if (last_step >= k_p1_step_seed_export) {
        P1_MK_TIME("29 seed_export");
        if (!save_seed_export()) {
            free_rslt(&m_rslt);
            m_valid_generation = false;
            return false;
        }
    }
    return true;
}

bool P1_MakeMap::is_valid () const {
    return m_valid_generation;
}

const P1_MakeMapRslt& P1_MakeMap::result () const {
    return m_rslt;
}

bool P1_MakeMap::save_terrain_ppm (cstr path) const {
    if (!m_valid_generation || path == nullptr || m_rslt.m_terrain == nullptr) {
        return false;
    }
    MapTerrainData map;
    if (!map.assign_copy(m_rslt.m_w, m_rslt.m_h, m_rslt.m_terrain)) {
        return false;
    }
    return map.save_terrain_ppm(path);
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 wi, u16 hi) {
    if (path == nullptr || rgb == nullptr || wi == 0 || hi == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)wi, (unsigned)hi);
    const size_t nbytes = static_cast<size_t>(wi) * static_cast<size_t>(hi) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

bool P1_MakeMap::save_climate_ppm (cstr path) const {
    if (!m_valid_generation || path == nullptr || m_rslt.m_climate == nullptr) {
        return false;
    }
    const u16 w = m_rslt.m_w;
    const u16 h = m_rslt.m_h;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        climate_to_rgb(m_rslt.m_climate[i], &r, &g, &b);
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

bool P1_MakeMap::save_rivers_ppm (cstr path) const {
    if (!m_valid_generation || path == nullptr || m_rslt.m_rivers == nullptr) {
        return false;
    }
    const u16 w = m_rslt.m_w;
    const u16 h = m_rslt.m_h;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        const u8 v = m_rslt.m_rivers[i] != 0 ? 255 : 0;
        rgb[i * 3u + 0] = v;
        rgb[i * 3u + 1] = v;
        rgb[i * 3u + 2] = v;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

bool P1_MakeMap::save_seed_export () const {
    if (!m_valid_generation) {
        return false;
    }
    char terr_path[320];
    char clim_path[320];
    char riv_path[320];
    if (!p1_make_seed_export_path(m_prm.m_seed, "terrain", terr_path, sizeof(terr_path))
        || !p1_make_seed_export_path(m_prm.m_seed, "climate", clim_path, sizeof(clim_path))
        || !p1_make_seed_export_path(m_prm.m_seed, "rivers", riv_path, sizeof(riv_path))) {
        return false;
    }
    if (!save_terrain_ppm(terr_path)) {
        std::printf("failed to save seed terrain: %s\n", terr_path);
        return false;
    }
    std::printf("saved: %s\n", terr_path);
    if (!save_climate_ppm(clim_path)) {
        std::printf("failed to save seed climate: %s\n", clim_path);
        return false;
    }
    std::printf("saved: %s\n", clim_path);
    if (!save_rivers_ppm(riv_path)) {
        std::printf("failed to save seed rivers: %s\n", riv_path);
        return false;
    }
    std::printf("saved: %s\n", riv_path);
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
