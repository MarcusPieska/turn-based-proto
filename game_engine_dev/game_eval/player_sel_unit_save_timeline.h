//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_SEL_UNIT_SAVE_TIMELINE_H
#define PLAYER_SEL_UNIT_SAVE_TIMELINE_H

#include "game_primitives.h"

//================================================================================================================================
//=> - PlayerSelUnitSaveTimeline -
//================================================================================================================================
//
//  Per-seat selected-unit count at each save index. m_n[si] = count; m_count is final-save total (sort key).
//
//================================================================================================================================

class PlayerSelUnitSaveTimeline {
public:
    PlayerSelUnitSaveTimeline ();
    ~PlayerSelUnitSaveTimeline ();

    void clr ();
    bool setup (u16 save_n);
    bool set (u16 save_i, u32 n);
    void sync_count ();

    u16 save_n () const;
    u32 count () const;
    u32 at (u16 save_i) const;

    void swap (PlayerSelUnitSaveTimeline& o);

private:
    PlayerSelUnitSaveTimeline (const PlayerSelUnitSaveTimeline&) = delete;
    PlayerSelUnitSaveTimeline& operator= (const PlayerSelUnitSaveTimeline&) = delete;

    u32* m_n;
    u16 m_save_n;
    u32 m_count;
};

#endif // PLAYER_SEL_UNIT_SAVE_TIMELINE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
