//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LUCKY_CITY_SAVE_TIMELINE_H
#define LUCKY_CITY_SAVE_TIMELINE_H

#include "game_primitives.h"

//================================================================================================================================
//=> - LuckyCitySaveTimeline -
//================================================================================================================================
//
//  Per-seat owned city count at each save index. m_count is final-save total (sort key).
//
//================================================================================================================================

class LuckyCitySaveTimeline {
public:
    LuckyCitySaveTimeline ();
    ~LuckyCitySaveTimeline ();

    void clr ();
    bool setup (u16 save_n);
    bool set (u16 save_i, u32 n);
    void sync_count ();

    u16 save_n () const;
    u32 count () const;
    u32 at (u16 save_i) const;

    void swap (LuckyCitySaveTimeline& o);

private:
    LuckyCitySaveTimeline (const LuckyCitySaveTimeline&) = delete;
    LuckyCitySaveTimeline& operator= (const LuckyCitySaveTimeline&) = delete;

    u32* m_n;
    u16 m_save_n;
    u32 m_count;
};

#endif // LUCKY_CITY_SAVE_TIMELINE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
