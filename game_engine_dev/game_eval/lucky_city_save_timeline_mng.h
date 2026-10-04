//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LUCKY_CITY_SAVE_TIMELINE_MNG_H
#define LUCKY_CITY_SAVE_TIMELINE_MNG_H

#include "game_primitives.h"
#include "lucky_city_save_timeline.h"

class EvalPaths;

//================================================================================================================================
//=> - LuckyCitySaveTimelineMng -
//================================================================================================================================
//
//  One timeline per lucky seat (m_lucky from first players.bin). fill() loads cities per save and
//  counts owned cities. sort() by final-save city count descending.
//
//================================================================================================================================

class LuckyCitySaveTimelineMng {
public:
    LuckyCitySaveTimelineMng ();
    ~LuckyCitySaveTimelineMng ();

    void clr ();
    bool setup (u16 player_n, u16 save_n);
    bool fill (const EvalPaths& paths);
    void sort ();

    u16 lucky_n () const;
    u16 save_n () const;

    LuckyCitySaveTimeline& at (u16 i);
    const LuckyCitySaveTimeline& at (u16 i) const;
    u16 seat (u16 i) const;

private:
    LuckyCitySaveTimelineMng (const LuckyCitySaveTimelineMng&) = delete;
    LuckyCitySaveTimelineMng& operator= (const LuckyCitySaveTimelineMng&) = delete;

    LuckyCitySaveTimeline* m_tl;
    u16* m_seat;
    u16 m_lucky_n;
    u16 m_player_n;
    u16 m_save_n;
};

#endif // LUCKY_CITY_SAVE_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
