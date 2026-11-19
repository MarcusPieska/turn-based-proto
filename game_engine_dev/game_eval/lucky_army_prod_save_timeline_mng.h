//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LUCKY_ARMY_PROD_SAVE_TIMELINE_MNG_H
#define LUCKY_ARMY_PROD_SAVE_TIMELINE_MNG_H

#include "game_primitives.h"

class EvalPaths;
class RuntimeStatics; 

//================================================================================================================================
//=> - LuckyArmyProdSaveTimelineMng -
//================================================================================================================================
//
//  Per lucky seat: count new land-army unit indices between consecutive saves (set diff vs prior
//  save). Excludes workers/misc via UnitUtilityHelper::is_type_for_land_army.
//
//================================================================================================================================

class LuckyArmyProdSaveTimelineMng {
public:
    LuckyArmyProdSaveTimelineMng ();
    ~LuckyArmyProdSaveTimelineMng ();

    void clr ();
    bool setup (u16 player_n, u16 save_n);
    bool fill (const EvalPaths& paths, const RuntimeStatics& st);

    u16 player_n () const;
    u16 save_n () const;
    u16 lucky_n () const;
    u16 interval_n () const;
    u16 seat (u16 li) const;
    u32 turn_end (u16 ii) const;
    u32 new_n (u16 li, u16 ii) const;

private:
    LuckyArmyProdSaveTimelineMng (const LuckyArmyProdSaveTimelineMng&) = delete;
    LuckyArmyProdSaveTimelineMng& operator= (const LuckyArmyProdSaveTimelineMng&) = delete;

    u16* m_lucky;
    u32* m_turn_end;
    u32* m_new;
    u16 m_lucky_n;
    u16 m_player_n;
    u16 m_save_n;
    u16 m_iv_n;
};

#endif // LUCKY_ARMY_PROD_SAVE_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
