//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_CONFIG_H
#define MAP_CONFIG_H

#include "game_primitives.h"

//================================================================================================================================
//=> - MapConfig -
//================================================================================================================================
//
//  Single-page aggregate of exposed map-generation tunables; defaults set in map_config_def only.
//
//================================================================================================================================

struct MapConfig {
    f32 m_perlin_lacunarity; // Step 03 noise perlin: octave lacunarity between frequency doublings
    f32 m_perlin_layer_freq_base; // Step 03 noise perlin: base frequency of layer 0
    f32 m_perlin_layer_weight; // Step 03 noise perlin: weight applied to each layer
    f32 m_perlin_layer_freq_step; // Step 03 noise perlin: frequency multiplier per layer
    i32 m_perlin_layer_count; // Step 03 noise perlin: number of summed layers (1–16)

    f32 m_shaped_radial_near; // Step 05 near shelf: land-depth radial weight cutoff (0 = coast only)
    u8 m_shaped_near_ocean; // Step 05 near shelf: target ocean class share (percent of shelf land)
    u8 m_shaped_near_sea; // Step 05 near shelf: target sea class share (percent of shelf land)
    u8 m_shaped_near_coastal; // Step 05 near shelf: target coastal class share (percent of shelf land)

    f32 m_shaped_radial_far; // Step 06 far shelf: land-depth radial weight cutoff (1 = full shelf)
    u8 m_shaped_far_ocean; // Step 06 far shelf: target ocean class share (percent of shelf land)
    u8 m_shaped_far_sea; // Step 06 far shelf: target sea class share (percent of shelf land)
    u8 m_shaped_far_coastal; // Step 06 far shelf: target coastal class share (percent of shelf land)

    f32 m_land_alt_lim_hills; // Step 23 land altitude: hills class cutoff on joint land score
    f32 m_land_alt_lim_mtn; // Step 23 land altitude: mountain class cutoff on joint land score

    u16 m_delta_flood_perc; // Step 35 delta swamps: target flooded land share (percent)
};

static inline MapConfig map_config_def () {
    MapConfig c;

    c.m_perlin_lacunarity = 2.0f; // Step 03 noise perlin: octave lacunarity between frequency doublings
    c.m_perlin_layer_freq_base = 0.5f; // Step 03 noise perlin: base frequency of layer 0
    c.m_perlin_layer_weight = 0.2f; // Step 03 noise perlin: weight applied to each layer
    c.m_perlin_layer_freq_step = 1.62f; // Step 03 noise perlin: frequency multiplier per layer
    c.m_perlin_layer_count = 5; // Step 03 noise perlin: number of summed layers (1–16)

    c.m_shaped_radial_near = 0.0f; // Step 05 near shelf: land-depth radial weight cutoff (0 = coast only)
    c.m_shaped_near_ocean = 18u; // Step 05 near shelf: target ocean class share (percent of shelf land)
    c.m_shaped_near_sea = 12u; // Step 05 near shelf: target sea class share (percent of shelf land)
    c.m_shaped_near_coastal = 8u; // Step 05 near shelf: target coastal class share (percent of shelf land)

    c.m_shaped_radial_far = 0.9f; // Step 06 far shelf: land-depth radial weight cutoff (1 = full shelf)
    c.m_shaped_far_ocean = 10u; // Step 06 far shelf: target ocean class share (percent of shelf land)
    c.m_shaped_far_sea = 20u; // Step 06 far shelf: target sea class share (percent of shelf land)
    c.m_shaped_far_coastal = 10u; // Step 06 far shelf: target coastal class share (percent of shelf land)

    c.m_land_alt_lim_hills = 0.30f; // Step 23 land altitude: hills class cutoff on joint land score
    c.m_land_alt_lim_mtn = 0.70f; // Step 23 land altitude: mountain class cutoff on joint land score

    c.m_delta_flood_perc = 3u; // Step 35 delta swamps: target flooded land share (percent)

    return c;
}

#endif // MAP_CONFIG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
