//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LUCKY_COMMERCE_SAVE_TIMELINE_MNG_H
#define LUCKY_COMMERCE_SAVE_TIMELINE_MNG_H

#include "game_primitives.h"
#include "lucky_commerce_save_timeline.h"

class EvalPaths;

//================================================================================================================================
//=> - LuckyCommerceSaveTimelineMng -
//================================================================================================================================
//
//  One timeline per lucky seat (m_lucky from first players.bin). fill() loads players per save and
//  records m_commerce. sort() by final-save commerce descending.
//
//================================================================================================================================

class LuckyCommerceSaveTimelineMng {
public:
    LuckyCommerceSaveTimelineMng ();
    ~LuckyCommerceSaveTimelineMng ();

    void clr ();
    bool setup (u16 player_n, u16 save_n);
    bool fill (const EvalPaths& paths);
    void sort ();

    u16 lucky_n () const;
    u16 save_n () const;

    LuckyCommerceSaveTimeline& at (u16 i);
    const LuckyCommerceSaveTimeline& at (u16 i) const;
    u16 seat (u16 i) const;

private:
    LuckyCommerceSaveTimelineMng (const LuckyCommerceSaveTimelineMng&) = delete;
    LuckyCommerceSaveTimelineMng& operator= (const LuckyCommerceSaveTimelineMng&) = delete;

    LuckyCommerceSaveTimeline* m_tl;
    u16* m_seat;
    u16 m_lucky_n;
    u16 m_player_n;
    u16 m_save_n;
};

#endif // LUCKY_COMMERCE_SAVE_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
