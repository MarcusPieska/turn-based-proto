//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_BORDER_H
#define CITY_BORDER_H

#include "circular_tile_areas.h"
#include "game_primitives.h"

class GameArraySimple;

//================================================================================================================================
//=> - CityBorder -
//================================================================================================================================
//
//  Culture-facing border helper over CircularTileAreas. Culture maps to radius via ordered m_lims thresholds.
//  bind_map wires the active GameArraySimple for civ ownership claims when borders expand.
//
//================================================================================================================================

class CityBorder {
public:
    static void bind_map (GameArraySimple* map);
    static CircArea get (u16 radius);
    static bool will_expand (u16 old_culture, u16 new_culture);
    static u16 radius_for (u16 culture);
    static void claim_expand (u16 cx, u16 cy, u16 old_culture, u16 new_culture, u8 owner);

private:
    static const u16 m_lims[]; // Ordered culture ceilings that unlock the next radius
    static const u16 m_lims_n; // Length of m_lims
    static GameArraySimple* m_map; // Active match tile grid; null until bind_map
};

#endif // CITY_BORDER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
