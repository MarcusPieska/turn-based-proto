//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_ROAD_SAVE_TIMELINE_H
#define MAP_ROAD_SAVE_TIMELINE_H

#include "game_primitives.h"

//================================================================================================================================
//=> - MapRoadSaveTimeline -
//================================================================================================================================
//
//  One map-wide series over save indices. m_n[si] = count at that save; m_count is final-save total.
//
//================================================================================================================================

class MapRoadSaveTimeline {
public:
    MapRoadSaveTimeline ();
    ~MapRoadSaveTimeline ();

    void clr ();
    bool setup (u16 save_n);
    bool set (u16 save_i, u32 n);
    void sync_count ();

    u16 save_n () const;
    u32 count () const;
    u32 at (u16 save_i) const;

private:
    MapRoadSaveTimeline (const MapRoadSaveTimeline&) = delete;
    MapRoadSaveTimeline& operator= (const MapRoadSaveTimeline&) = delete;

    u32* m_n; // Per-save counts
    u16 m_save_n; // Save slot count
    u32 m_count; // Final-save total
};

#endif // MAP_ROAD_SAVE_TIMELINE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
