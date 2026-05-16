//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_TERRAIN_COMBO_ADD_H
#define GENERATE_TERRAIN_COMBO_ADD_H

#include "game_primitives.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - TerrainComboAdd -
//================================================================================================================================

class Generate_TerrainComboAdd {
public:
    explicit Generate_TerrainComboAdd ();
    ~Generate_TerrainComboAdd ();

    bool generate (const MapArrayTerrain& base, const MapArrayTerrain& add, i32 offset_x = 0, i32 offset_y = 0);
    bool is_valid () const;
    void save_output (cstr path) const;

    u16 width () const;
    u16 height () const;
    const u8* terrain_class () const;
    MapArrayTerrain* take_terrain ();

private:
    Generate_TerrainComboAdd (const Generate_TerrainComboAdd& other) = delete;
    Generate_TerrainComboAdd (Generate_TerrainComboAdd&& other) = delete;

    bool m_valid_generation;
    MapArrayTerrain* m_terrain;
};

#endif // GENERATE_TERRAIN_COMBO_ADD_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
