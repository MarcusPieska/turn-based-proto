//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_FARM_SAVE_TIMELINE_MNG_H
#define PLAYER_FARM_SAVE_TIMELINE_MNG_H

#include "game_primitives.h"
#include "player_farm_save_timeline.h"

class EvalPaths;

//================================================================================================================================
//=> - PlayerFarmSaveTimelineMng -
//================================================================================================================================
//
//  One PlayerFarmSaveTimeline per seat. fill() loads one map blob at a time.
//  sort() by final-save farm count descending.
//
//================================================================================================================================

class PlayerFarmSaveTimelineMng {
public:
    PlayerFarmSaveTimelineMng ();
    ~PlayerFarmSaveTimelineMng ();

    void clr ();
    bool setup (u16 player_n, u16 save_n);
    bool fill (const EvalPaths& paths);
    void sort ();

    u16 player_n () const;
    u16 save_n () const;

    PlayerFarmSaveTimeline& at (u16 i);
    const PlayerFarmSaveTimeline& at (u16 i) const;
    u16 seat (u16 i) const;

private:
    PlayerFarmSaveTimelineMng (const PlayerFarmSaveTimelineMng&) = delete;
    PlayerFarmSaveTimelineMng& operator= (const PlayerFarmSaveTimelineMng&) = delete;

    PlayerFarmSaveTimeline* m_tl;
    u16* m_seat;
    u16 m_player_n;
    u16 m_save_n;
};

#endif // PLAYER_FARM_SAVE_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
