//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LUCKY_ARMY_FIELD_SAVE_TIMELINE_MNG_H
#define LUCKY_ARMY_FIELD_SAVE_TIMELINE_MNG_H

#include "game_primitives.h"

class EvalPaths;
class RuntimeStatics;

//================================================================================================================================
//=> - LuckyArmyFieldSaveTimelineMng -
//================================================================================================================================
//
//  Per lucky seat / save: land-army empire totals (+ canBarrage), and the m_in_army/m_in_campaign
//  subset (+ canBarrage). Counts group tails (null tile xy) so formed groups are not undercounted.
//
//================================================================================================================================

class LuckyArmyFieldSaveTimelineMng {
public:
    LuckyArmyFieldSaveTimelineMng ();
    ~LuckyArmyFieldSaveTimelineMng ();

    void clr ();
    bool setup (u16 player_n, u16 save_n);
    bool fill (const EvalPaths& paths, const RuntimeStatics& st);

    u16 player_n () const;
    u16 save_n () const;
    u16 lucky_n () const;
    u16 seat (u16 li) const;
    u32 turn_at (u16 si) const;
    u32 tot_n (u16 li, u16 si) const;
    u32 tot_arty_n (u16 li, u16 si) const;
    u32 field_n (u16 li, u16 si) const;
    u32 field_arty_n (u16 li, u16 si) const;

private:
    LuckyArmyFieldSaveTimelineMng (const LuckyArmyFieldSaveTimelineMng&) = delete;
    LuckyArmyFieldSaveTimelineMng& operator= (const LuckyArmyFieldSaveTimelineMng&) = delete;

    u16* m_lucky;
    u32* m_turn;
    u32* m_tot;
    u32* m_tot_arty;
    u32* m_field;
    u32* m_field_arty;
    u16 m_lucky_n;
    u16 m_player_n;
    u16 m_save_n;
};

#endif // LUCKY_ARMY_FIELD_SAVE_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
