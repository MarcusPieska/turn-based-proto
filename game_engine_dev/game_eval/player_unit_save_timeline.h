//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_UNIT_SAVE_TIMELINE_H
#define PLAYER_UNIT_SAVE_TIMELINE_H

#include "game_primitives.h"

//================================================================================================================================
//=> - PlayerUnitSaveTimeline -
//================================================================================================================================
//
//  Per-seat army size at each save index. m_n[si] = unit count; m_count is final-save total (sort key).
//
//================================================================================================================================

class PlayerUnitSaveTimeline {
public:
    PlayerUnitSaveTimeline ();
    ~PlayerUnitSaveTimeline ();

    void clr ();
    bool setup (u16 save_n);
    bool set (u16 save_i, u32 n);
    void sync_count ();

    u16 save_n () const;
    u32 count () const;
    u32 at (u16 save_i) const;

    void swap (PlayerUnitSaveTimeline& o);

private:
    PlayerUnitSaveTimeline (const PlayerUnitSaveTimeline&) = delete;
    PlayerUnitSaveTimeline& operator= (const PlayerUnitSaveTimeline&) = delete;

    u32* m_n;
    u16 m_save_n;
    u32 m_count;
};

#endif // PLAYER_UNIT_SAVE_TIMELINE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
