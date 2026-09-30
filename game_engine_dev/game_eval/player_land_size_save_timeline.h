//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_LAND_SIZE_SAVE_TIMELINE_H
#define PLAYER_LAND_SIZE_SAVE_TIMELINE_H

#include "game_primitives.h"

//================================================================================================================================
//=> - PlayerLandSizeSaveTimeline -
//================================================================================================================================
//
//  Per-seat land-unit size series at each save index. Avg/max store effective size (1 + raw).
//  m_count is final-save land unit count (sort key).
//
//================================================================================================================================

class PlayerLandSizeSaveTimeline {
public:
    PlayerLandSizeSaveTimeline ();
    ~PlayerLandSizeSaveTimeline ();

    void clr ();
    bool setup (u16 save_n);
    bool set (u16 save_i, u32 land_n, u32 avg_milli, u16 max_sz);
    void sync_count ();

    u16 save_n () const;
    u32 count () const;
    u32 land_at (u16 save_i) const;
    u32 avg_milli_at (u16 save_i) const;
    u16 max_at (u16 save_i) const;

    void swap (PlayerLandSizeSaveTimeline& o);

private:
    PlayerLandSizeSaveTimeline (const PlayerLandSizeSaveTimeline&) = delete;
    PlayerLandSizeSaveTimeline& operator= (const PlayerLandSizeSaveTimeline&) = delete;

    u32* m_land;
    u32* m_avg_milli;
    u16* m_max;
    u16 m_save_n;
    u32 m_count;
};

#endif // PLAYER_LAND_SIZE_SAVE_TIMELINE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
