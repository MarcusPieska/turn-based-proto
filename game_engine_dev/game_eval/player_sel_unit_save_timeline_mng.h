//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_SEL_UNIT_SAVE_TIMELINE_MNG_H
#define PLAYER_SEL_UNIT_SAVE_TIMELINE_MNG_H

#include "game_primitives.h"
#include "player_sel_unit_save_timeline.h"

class EvalPaths;

//================================================================================================================================
//=> - PlayerSelUnitSaveTimelineMng -
//================================================================================================================================
//
//  One PlayerSelUnitSaveTimeline per seat. fill(paths, unit_typ_idx) loads one units blob at a time.
//  Counts only units whose m_unit_typ_idx matches. sort() by final-save count descending.
//
//================================================================================================================================

class PlayerSelUnitSaveTimelineMng {
public:
    PlayerSelUnitSaveTimelineMng ();
    ~PlayerSelUnitSaveTimelineMng ();

    void clr ();
    bool setup (u16 player_n, u16 save_n);
    bool fill (const EvalPaths& paths, u16 unit_typ_idx);
    void sort ();

    u16 player_n () const;
    u16 save_n () const;
    u16 unit_typ () const;

    PlayerSelUnitSaveTimeline& at (u16 i);
    const PlayerSelUnitSaveTimeline& at (u16 i) const;
    u16 seat (u16 i) const;

private:
    PlayerSelUnitSaveTimelineMng (const PlayerSelUnitSaveTimelineMng&) = delete;
    PlayerSelUnitSaveTimelineMng& operator= (const PlayerSelUnitSaveTimelineMng&) = delete;

    PlayerSelUnitSaveTimeline* m_tl;
    u16* m_seat;
    u16 m_player_n;
    u16 m_save_n;
    u16 m_unit_typ;
};

#endif // PLAYER_SEL_UNIT_SAVE_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
