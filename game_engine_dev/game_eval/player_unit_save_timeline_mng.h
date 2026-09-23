//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_UNIT_SAVE_TIMELINE_MNG_H
#define PLAYER_UNIT_SAVE_TIMELINE_MNG_H

#include "game_primitives.h"
#include "player_unit_save_timeline.h"

class EvalPaths;

//================================================================================================================================
//=> - PlayerUnitSaveTimelineMng -
//================================================================================================================================
//
//  One PlayerUnitSaveTimeline per seat. fill() loads one units blob at a time (never keeps maps).
//  sort() by final-save army size descending.
//
//================================================================================================================================

class PlayerUnitSaveTimelineMng {
public:
    PlayerUnitSaveTimelineMng ();
    ~PlayerUnitSaveTimelineMng ();

    void clr ();
    bool setup (u16 player_n, u16 save_n);
    bool fill (const EvalPaths& paths);
    void sort ();

    u16 player_n () const;
    u16 save_n () const;

    PlayerUnitSaveTimeline& at (u16 i);
    const PlayerUnitSaveTimeline& at (u16 i) const;
    u16 seat (u16 i) const;

private:
    PlayerUnitSaveTimelineMng (const PlayerUnitSaveTimelineMng&) = delete;
    PlayerUnitSaveTimelineMng& operator= (const PlayerUnitSaveTimelineMng&) = delete;

    PlayerUnitSaveTimeline* m_tl;
    u16* m_seat;
    u16 m_player_n;
    u16 m_save_n;
};

#endif // PLAYER_UNIT_SAVE_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
