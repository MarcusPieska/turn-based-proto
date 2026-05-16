//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_TERRAIN_COMBO_MIN_H
#define GENERATE_TERRAIN_COMBO_MIN_H

#include "game_primitives.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - TerrainComboMin -
//================================================================================================================================

class Generate_TerrainComboMin {
public:
    explicit Generate_TerrainComboMin ();
    ~Generate_TerrainComboMin ();

    bool generate (const MapArrayTerrain& a, const MapArrayTerrain& b);
    bool is_valid () const;
    void save_output (cstr path) const;

    u16 width () const;
    u16 height () const;
    const u8* terrain_class () const;

private:
    Generate_TerrainComboMin (const Generate_TerrainComboMin& other) = delete;
    Generate_TerrainComboMin (Generate_TerrainComboMin&& other) = delete;

    bool m_valid_generation;
    MapArrayTerrain* m_terrain;
};

#endif // GENERATE_TERRAIN_COMBO_MIN_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
