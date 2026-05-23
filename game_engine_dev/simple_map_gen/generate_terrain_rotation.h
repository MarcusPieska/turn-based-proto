//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_TERRAIN_ROTATION_H
#define GENERATE_TERRAIN_ROTATION_H

#include "game_primitives.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - TerrainRotation -
//================================================================================================================================

class Generate_TerrainRotation {
public:
    explicit Generate_TerrainRotation ();
    ~Generate_TerrainRotation ();

    bool generate (const MapArrayTerrain& src, i32 degrees_cw);
    bool is_valid () const;
    bool save_output (cstr path) const;

    u16 width () const;
    u16 height () const;
    const MapArrayTerrain& terrain () const;

private:
    Generate_TerrainRotation (const Generate_TerrainRotation& other) = delete;
    Generate_TerrainRotation (Generate_TerrainRotation&& other) = delete;

    static i32 norm_deg_cw (i32 degrees_cw);
    bool rot_nn_cw (const MapArrayTerrain& src, i32 degrees_cw);

    bool m_valid_generation;
    MapArrayTerrain* m_terrain;
};

#endif // GENERATE_TERRAIN_ROTATION_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
