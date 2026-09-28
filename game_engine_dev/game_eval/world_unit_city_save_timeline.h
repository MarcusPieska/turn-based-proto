//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WORLD_UNIT_CITY_SAVE_TIMELINE_H
#define WORLD_UNIT_CITY_SAVE_TIMELINE_H

#include "game_primitives.h"

//================================================================================================================================
//=> - WorldUnitCitySaveTimeline -
//================================================================================================================================
//
//  Global unit and city totals at each save index (all seats summed).
//
//================================================================================================================================

class WorldUnitCitySaveTimeline {
public:
    WorldUnitCitySaveTimeline ();
    ~WorldUnitCitySaveTimeline ();

    void clr ();
    bool setup (u16 save_n);
    bool set (u16 save_i, u32 units, u32 cities);

    u16 save_n () const;
    u32 units_at (u16 save_i) const;
    u32 cities_at (u16 save_i) const;

private:
    WorldUnitCitySaveTimeline (const WorldUnitCitySaveTimeline&) = delete;
    WorldUnitCitySaveTimeline& operator= (const WorldUnitCitySaveTimeline&) = delete;

    u32* m_units;
    u32* m_cities;
    u16 m_save_n;
};

#endif // WORLD_UNIT_CITY_SAVE_TIMELINE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
