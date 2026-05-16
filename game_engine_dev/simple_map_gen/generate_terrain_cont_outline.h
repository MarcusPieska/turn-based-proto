//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_TERRAIN_CONT_OUTLINE_H
#define GENERATE_TERRAIN_CONT_OUTLINE_H

#include <vector>

#include "game_primitives.h"
#include "generate_terrain_cont_pn.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - TerrainContOutlineParams -
//================================================================================================================================

#define TERR_OUTLINE_FILL_MODE_FULL 0
#define TERR_OUTLINE_FILL_MODE_PERLIN_NOISE 1
#define TERR_OUTLINE_FILL_MODE_PN_MIX 2

struct TerrainContOutlineParams {
    u32 m_seed = 0;
    u16 m_width = 1000;
    u16 m_height = 1000;

    u8 m_fill_mode = TERR_OUTLINE_FILL_MODE_FULL;
    u8 m_mix_mode_corner_count = 3;
    TerrainContPnParams m_pn_params;
    
    bool m_debug = false;
};

//================================================================================================================================
//=> - TerrainContOutline -
//================================================================================================================================

class Generate_TerrainContOutline {
public:
    explicit Generate_TerrainContOutline (const TerrainContOutlineParams& params);
    explicit Generate_TerrainContOutline (u32 seed, u16 w = 1000, u16 h = 1000);

    bool generate ();
    bool is_valid () const;
    bool save_output (cstr path) const;
    bool save_land_to_water_distance (cstr path) const;
    bool save_interior_grad_mask (cstr path) const;

    u16 width () const;
    u16 height () const;
    const MapArrayTerrain& terrain () const;
    const MapArrayDistance& land_to_water_distance () const;
    bool has_land_to_water_distance () const;

private:
    Generate_TerrainContOutline (const Generate_TerrainContOutline& other) = delete;
    Generate_TerrainContOutline (Generate_TerrainContOutline&& other) = delete;

    TerrainContOutlineParams m_params;
    bool m_valid_generation;
    bool m_has_land_to_water;
    MapArrayTerrain m_terrain;
    MapArrayDistance m_land_to_water;
    std::vector<u8> m_interior_mask;
};

#endif // GENERATE_TERRAIN_CONT_OUTLINE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
