//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MISC_UNIT_STATE_SAVE_TIMELINE_MNG_H
#define MISC_UNIT_STATE_SAVE_TIMELINE_MNG_H

#include "game_primitives.h"
#include "player_unit_save_timeline.h"

class EvalPaths;
class RuntimeStatics;

//================================================================================================================================
//=> - MiscUnitSeries -
//================================================================================================================================

enum class MiscUnitSeries : u8 {
    LostField = 0, // Land combat, off city, not in campaign
    CampNoWar = 1, // Land combat, m_in_campaign, seat m_at_war == 0
    N = 2
};

//================================================================================================================================
//=> - MiscUnitStateSaveTimelineMng -
//================================================================================================================================
//
//  Niche per-seat unit-state series over the save sequence. fill() loads map dims once, then
//  units+cities+players per save. Seats stay in identity order. Captures m_lucky from first save.
//
//================================================================================================================================

class MiscUnitStateSaveTimelineMng {
public:
    MiscUnitStateSaveTimelineMng ();
    ~MiscUnitStateSaveTimelineMng ();

    void clr ();
    bool setup (u16 player_n, u16 save_n);
    bool fill (const EvalPaths& paths, const RuntimeStatics& st);

    u16 player_n () const;
    u16 save_n () const;
    u8 lucky (u16 seat) const;

    PlayerUnitSaveTimeline& at (MiscUnitSeries ser, u16 seat);
    const PlayerUnitSaveTimeline& at (MiscUnitSeries ser, u16 seat) const;

private:
    MiscUnitStateSaveTimelineMng (const MiscUnitStateSaveTimelineMng&) = delete;
    MiscUnitStateSaveTimelineMng& operator= (const MiscUnitStateSaveTimelineMng&) = delete;

    PlayerUnitSaveTimeline* m_tl[static_cast<u16>(MiscUnitSeries::N)];
    u8* m_lucky;
    u16 m_player_n;
    u16 m_save_n;
};

#endif // MISC_UNIT_STATE_SAVE_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
