//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_ROAD_SAVE_TIMELINE_MNG_H
#define MAP_ROAD_SAVE_TIMELINE_MNG_H

#include "game_primitives.h"
#include "map_road_save_timeline.h"

class EvalPaths;

//================================================================================================================================
//=> - MapRoadSaveTimelineMng -
//================================================================================================================================
//
//  Map-wide series: road intents (mtn_pass, fort) plus ROAD_PATH/COBBLE/ASPHALT/RAIL/VIRTUAL.
//  fill() loads one map blob at a time and tallies all series in one pass.
//
//================================================================================================================================

class MapRoadSaveTimelineMng {
public:
    static const u16 SERIES_INTENT_MTN = 0;
    static const u16 SERIES_INTENT_FORT = 1;
    static const u16 SERIES_PATH = 2;
    static const u16 SERIES_COBBLE = 3;
    static const u16 SERIES_ASPHALT = 4;
    static const u16 SERIES_RAIL = 5;
    static const u16 SERIES_VIRTUAL = 6;
    static const u16 SERIES_N = 7;

    MapRoadSaveTimelineMng ();
    ~MapRoadSaveTimelineMng ();

    void clr ();
    bool setup (u16 save_n);
    bool fill (const EvalPaths& paths);

    u16 series_n () const;
    u16 save_n () const;
    cstr label (u16 i) const;

    MapRoadSaveTimeline& at (u16 i);
    const MapRoadSaveTimeline& at (u16 i) const;

private:
    MapRoadSaveTimelineMng (const MapRoadSaveTimelineMng&) = delete;
    MapRoadSaveTimelineMng& operator= (const MapRoadSaveTimelineMng&) = delete;

    MapRoadSaveTimeline* m_tl; // One timeline per series
    u16 m_save_n; // Save slot count
};

#endif // MAP_ROAD_SAVE_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
