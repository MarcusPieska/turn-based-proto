//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WORLD_UNIT_CITY_SAVE_TIMELINE_MNG_H
#define WORLD_UNIT_CITY_SAVE_TIMELINE_MNG_H

#include "game_primitives.h"
#include "world_unit_city_save_timeline.h"

class EvalPaths;

//================================================================================================================================
//=> - WorldUnitCitySaveTimelineMng -
//================================================================================================================================
//
//  Loads one units+cities save pair at a time; records global totals per save index.
//
//================================================================================================================================

class WorldUnitCitySaveTimelineMng {
public:
    WorldUnitCitySaveTimelineMng ();
    ~WorldUnitCitySaveTimelineMng ();

    void clr ();
    bool setup (u16 save_n);
    bool fill (const EvalPaths& paths);

    u16 save_n () const;
    WorldUnitCitySaveTimeline& tl ();
    const WorldUnitCitySaveTimeline& tl () const;

private:
    WorldUnitCitySaveTimelineMng (const WorldUnitCitySaveTimelineMng&) = delete;
    WorldUnitCitySaveTimelineMng& operator= (const WorldUnitCitySaveTimelineMng&) = delete;

    WorldUnitCitySaveTimeline m_tl;
    u16 m_save_n;
};

#endif // WORLD_UNIT_CITY_SAVE_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
