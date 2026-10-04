//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LUCKY_COMMERCE_SAVE_TIMELINE_H
#define LUCKY_COMMERCE_SAVE_TIMELINE_H

#include "game_primitives.h"

//================================================================================================================================
//=> - LuckyCommerceSaveTimeline -
//================================================================================================================================
//
//  Per-seat commerce treasury (PlayerState::m_commerce) at each save index. m_count is final-save
//  value (sort key).
//
//================================================================================================================================

class LuckyCommerceSaveTimeline {
public:
    LuckyCommerceSaveTimeline ();
    ~LuckyCommerceSaveTimeline ();

    void clr ();
    bool setup (u16 save_n);
    bool set (u16 save_i, u32 n);
    void sync_count ();

    u16 save_n () const;
    u32 count () const;
    u32 at (u16 save_i) const;

    void swap (LuckyCommerceSaveTimeline& o);

private:
    LuckyCommerceSaveTimeline (const LuckyCommerceSaveTimeline&) = delete;
    LuckyCommerceSaveTimeline& operator= (const LuckyCommerceSaveTimeline&) = delete;

    u32* m_n;
    u16 m_save_n;
    u32 m_count;
};

#endif // LUCKY_COMMERCE_SAVE_TIMELINE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
