//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_RES_SAVE_TIMELINE_MNG_H
#define PLAYER_RES_SAVE_TIMELINE_MNG_H

#include "game_primitives.h"
#include "player_res_save_timeline.h"

class EvalPaths;

//================================================================================================================================
//=> - PlayerResSaveTimelineMng -
//================================================================================================================================
//
//  One PlayerResSaveTimeline per seat. fill() loads players.bin one save at a time for a fixed res index.
//  sort() by ledger-sum over saves descending (u64).
//
//================================================================================================================================

class PlayerResSaveTimelineMng {
public:
    PlayerResSaveTimelineMng ();
    ~PlayerResSaveTimelineMng ();

    void clr ();
    bool setup (u16 player_n, u16 save_n);
    bool fill (const EvalPaths& paths, u16 res_idx);
    void sort ();

    u16 player_n () const;
    u16 save_n () const;
    u16 res_idx () const;

    PlayerResSaveTimeline& at (u16 i);
    const PlayerResSaveTimeline& at (u16 i) const;
    u16 seat (u16 i) const;

private:
    PlayerResSaveTimelineMng (const PlayerResSaveTimelineMng&) = delete;
    PlayerResSaveTimelineMng& operator= (const PlayerResSaveTimelineMng&) = delete;

    PlayerResSaveTimeline* m_tl;
    u16* m_seat;
    u16 m_player_n;
    u16 m_save_n;
    u16 m_res_idx;
};

#endif // PLAYER_RES_SAVE_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
