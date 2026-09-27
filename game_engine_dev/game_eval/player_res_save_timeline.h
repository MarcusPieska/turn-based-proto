//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_RES_SAVE_TIMELINE_H
#define PLAYER_RES_SAVE_TIMELINE_H

#include "game_primitives.h"

//================================================================================================================================
//=> - PlayerResSaveTimeline -
//================================================================================================================================
//
//  Per-seat ledger amount for one resource at each save index. m_n[si] = amount; m_count is sum over saves (sort key).
//
//================================================================================================================================

class PlayerResSaveTimeline {
public:
    PlayerResSaveTimeline ();
    ~PlayerResSaveTimeline ();

    void clr ();
    bool setup (u16 save_n);
    bool set (u16 save_i, u32 n);
    void sync_count ();

    u16 save_n () const;
    u64 count () const;
    u32 at (u16 save_i) const;

    void swap (PlayerResSaveTimeline& o);

private:
    PlayerResSaveTimeline (const PlayerResSaveTimeline&) = delete;
    PlayerResSaveTimeline& operator= (const PlayerResSaveTimeline&) = delete;

    u32* m_n;
    u16 m_save_n;
    u64 m_count;
};

#endif // PLAYER_RES_SAVE_TIMELINE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
