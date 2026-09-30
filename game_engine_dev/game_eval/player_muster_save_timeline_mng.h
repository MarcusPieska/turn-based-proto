//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_MUSTER_SAVE_TIMELINE_MNG_H
#define PLAYER_MUSTER_SAVE_TIMELINE_MNG_H

#include "game_primitives.h"
#include "player_unit_save_timeline.h"

class EvalPaths;
class RuntimeStatics;

//================================================================================================================================
//=> - PlayerMusterSaveTimelineMng -
//================================================================================================================================
//
//  Per-seat dry muster size (sum of 1+m_unit_size over muster_collect_depart picks at owned cities).
//  fill() loads map+units+cities per save. sort() by final-save muster descending.
//
//================================================================================================================================

class PlayerMusterSaveTimelineMng {
public:
    PlayerMusterSaveTimelineMng ();
    ~PlayerMusterSaveTimelineMng ();

    void clr ();
    bool setup (u16 player_n, u16 save_n);
    bool fill (const EvalPaths& paths, const RuntimeStatics& st);
    void sort ();

    u16 player_n () const;
    u16 save_n () const;

    PlayerUnitSaveTimeline& at (u16 i);
    const PlayerUnitSaveTimeline& at (u16 i) const;
    u16 seat (u16 i) const;

private:
    PlayerMusterSaveTimelineMng (const PlayerMusterSaveTimelineMng&) = delete;
    PlayerMusterSaveTimelineMng& operator= (const PlayerMusterSaveTimelineMng&) = delete;

    PlayerUnitSaveTimeline* m_tl;
    u16* m_seat;
    u16 m_player_n;
    u16 m_save_n;
};

#endif // PLAYER_MUSTER_SAVE_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
