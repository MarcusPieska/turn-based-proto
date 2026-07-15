//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_MAKE_MAP_H
#define P1_MAKE_MAP_H

#include "game_primitives.h"
#include "p1_adj_coast_fertility.h"
#include "p1_adj_grassland_loess_tiles.h"
#include "p1_adj_land_altitude.h"
#include "p1_gen_climate.h"
#include "p1_gen_loess_boost.h"
#include "p1_gen_rich_coast_fertility.h"
#include "p1_gen_rain_orographic.h"
#include "p1_gen_shaped_outline.h"
#include "p1_gen_wind_pattern_adv.h"
#include "map_config.h"
#include "p1_map_size.h"
#include "p1_pipeline_steps.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - P1_MakeMapPrm -
//================================================================================================================================

struct P1_MakeMapPrm {
    P1_Gen_ShapedOutlinePrm m_shaped;
    P1_Adj_LandAltitudePrm m_lap;
    P1_Gen_WindPatternAdvPrm m_wind;
    P1_Gen_RainOrographicPrm m_rain;
    P1_Gen_ClimatePrm m_climate;
    P1_Gen_LoessBoostPrm m_loess;
    P1_Adj_GrasslandLoessTilesPrm m_grass_loess;
    P1_Gen_RichCoastFertilityPrm m_rich_coast;
    P1_Adj_CoastFertilityPrm m_coast_fert;
    u16 m_rain_finish;
    u16 m_slope_finish;
    u32 m_res_base_n;
};

static inline P1_MakeMapPrm p1_make_map_prm_from_cfg (const MapConfig& cfg) {
    P1_MakeMapPrm p;
    p.m_shaped = p1_gen_shaped_outline_prm_from_cfg(cfg);
    p.m_lap = p1_adj_land_altitude_prm_from_cfg(cfg);
    p.m_wind = p1_gen_wind_pattern_adv_prm_def();
    p.m_rain = p1_gen_rain_orographic_prm_def();
    p.m_climate = p1_gen_climate_prm_def();
    p.m_loess = p1_gen_loess_boost_prm_def();
    p.m_grass_loess = p1_adj_grassland_loess_tiles_prm_def();
    p.m_grass_loess.m_w_rain = static_cast<u16>(CLIMATE_WT_MAX);
    p.m_rich_coast = p1_gen_rich_coast_fertility_prm_def();
    p.m_coast_fert = p1_adj_coast_fertility_prm_def();
    p.m_rain_finish = 3;
    p.m_slope_finish = 100;
    p.m_res_base_n = 100u;
    return p;
}

static inline P1_MakeMapPrm p1_make_map_prm_def () {
    return p1_make_map_prm_from_cfg(map_config_def());
}

struct RuntimeStatics;

void p1_map_gen_set_statics (const RuntimeStatics* statics);
bool p1_map_gen_init (const RuntimeStatics* statics = nullptr);

static const u16 k_p1_step_coastal_mtn_limits = P1_STEP_COASTAL_MTN_LIMITS;
static const u16 k_p1_step_foothills = P1_STEP_ENSURE_MTN_FOOTHILLS;
static const u16 k_p1_step_wind = P1_STEP_WIND;
static const u16 k_p1_step_rain = P1_STEP_RAIN;
static const u16 k_p1_step_climate = P1_STEP_CLIMATE;
static const u16 k_p1_step_desert_cull = P1_STEP_DESERT_CULL;
static const u16 k_p1_step_loess = P1_STEP_LOESS;
static const u16 k_p1_step_grass_loess = P1_STEP_GRASS_LOESS;
static const u16 k_p1_step_core = 33u;
static const u16 k_p1_step_seed_export = P1_STEP_MAKE_MAP;
static const u16 k_p1_step_rich_coast_fert = P1_STEP_RICH_COAST_FERT;
static const u16 k_p1_step_coast_fert_adj = P1_STEP_COAST_FERT_ADJ;
static const u16 k_p1_step_ensure_adj = P1_STEP_ENSURE_ADJ;
static const u16 k_p1_step_forest_overlay = P1_STEP_FOREST_OVERLAY;
static const u16 k_p1_step_delta_swamps = P1_STEP_DELTA_SWAMPS;
static const u16 k_p1_step_resources = P1_STEP_RESOURCES;

static const char* G_RT_LIB = "../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../";

struct P1_MakeMapRslt {
    u16 m_w;
    u16 m_h;
    u8* m_terrain;
    u8* m_climate;
    u8* m_rivers;
    u16* m_wshed;
    u8* m_wind_dir;
    u8* m_wind_str;
    u8* m_loess;
    u8* m_rain;
    u8* m_overlay;
    u16* m_resources;
};

//================================================================================================================================
//=> - P1_MakeMap -
//================================================================================================================================

class P1_MakeMap {
public:
    explicit P1_MakeMap (const P1_RunPrm& prm, const MapConfig& cfg = map_config_def());
    P1_MakeMap (const P1_RunPrm& prm, const MapConfig& cfg, const P1_MakeMapPrm& mp);
    ~P1_MakeMap ();

    bool generate (u16 last_step = k_p1_step_seed_export);
    bool is_valid () const;
    const P1_MakeMapRslt& result () const;
    bool save_terrain_ppm (cstr path) const;
    bool save_climate_ppm (cstr path) const;
    bool save_rivers_ppm (cstr path) const;
    bool save_overlay_ppm (cstr path) const;
    bool save_resources_ppm (cstr path) const;
    bool save_seed_export () const;
    static void free_rslt (P1_MakeMapRslt* rslt);
    static bool copy_rslt (P1_MakeMapRslt* dst, const P1_MakeMapRslt& src);

private:
    P1_MakeMap (const P1_MakeMap& other) = delete;
    P1_MakeMap (P1_MakeMap&& other) = delete;

    P1_RunPrm m_prm;
    MapConfig m_cfg;
    P1_MakeMapPrm m_mp;
    bool m_valid_generation;
    P1_MakeMapRslt m_rslt;
};

#endif // P1_MAKE_MAP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
