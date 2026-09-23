//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_FARM_SAVE_TIMELINE_H
#define PLAYER_FARM_SAVE_TIMELINE_H

#include "game_primitives.h"

//================================================================================================================================
//=> - PlayerFarmSaveTimeline -
//================================================================================================================================
//
//  Per-seat farm count at each save index. m_n[si] = farms on owned tiles; m_count is final-save total (sort key).
//
//================================================================================================================================

class PlayerFarmSaveTimeline {
public:
    PlayerFarmSaveTimeline ();
    ~PlayerFarmSaveTimeline ();

    void clr ();
    bool setup (u16 save_n);
    bool set (u16 save_i, u32 n);
    void sync_count ();

    u16 save_n () const;
    u32 count () const;
    u32 at (u16 save_i) const;

    void swap (PlayerFarmSaveTimeline& o);

private:
    PlayerFarmSaveTimeline (const PlayerFarmSaveTimeline&) = delete;
    PlayerFarmSaveTimeline& operator= (const PlayerFarmSaveTimeline&) = delete;

    u32* m_n;
    u16 m_save_n;
    u32 m_count;
};

#endif // PLAYER_FARM_SAVE_TIMELINE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
