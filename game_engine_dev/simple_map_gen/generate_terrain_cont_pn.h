//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_TERRAIN_CONT_PN_H
#define GENERATE_TERRAIN_CONT_PN_H

#include <vector>

#include "game_primitives.h"
#include "generator_constants.h"
#include "perlin_noise.h"

//================================================================================================================================
//=> - TerrainContPnParams -
//================================================================================================================================

struct TerrainContPnParams {
    u32 m_seed = 0;
    u16 m_width = 1000;
    u16 m_height = 1000;
    f32 m_lacunarity = 2.f;
    f32 m_layer_freq_base = 0.5f;
    f32 m_layer_weight = 0.2f;
    f32 m_layer_freq_step = 1.62f;
    i32 m_layer_count = 5;
    f32 m_inner_grad_limit = 0.8f;

    f64 m_terr_lim_ocean = 0.7;
    f64 m_terr_lim_sea = 0.85;
    f64 m_terr_lim_coastal = 0.88;
    f64 m_terr_lim_plains = 0.94;
    f64 m_terr_lim_hills = 0.99;
    f64 m_terr_lim_mountains = 1.0;

    bool m_debug = false;
};

//================================================================================================================================
//=> - TerrainContPn -
//================================================================================================================================

class Generate_TerrainContPn {
public:
    explicit Generate_TerrainContPn (const TerrainContPnParams& params);

    bool generate ();
    bool is_valid () const;
    void save_output (cstr path) const;

    u16 width () const;
    u16 height () const;
    const u8* height_gray () const;
    const u8* terrain_class () const;
    const MapArrayTerrain& terrain () const;
    bool save_height_gray (cstr path) const;
    bool save_terrain_rgb (cstr path) const;

private:
    Generate_TerrainContPn (const Generate_TerrainContPn& other) = delete;
    Generate_TerrainContPn (Generate_TerrainContPn&& other) = delete;

    bool m_valid_generation;
    TerrainContPnParams m_params;
    u16 m_w;
    u16 m_h;
    std::vector<u8> m_height_gray;
    MapArrayTerrain m_terrain;
};

#endif // GENERATE_TERRAIN_CONT_PN_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
