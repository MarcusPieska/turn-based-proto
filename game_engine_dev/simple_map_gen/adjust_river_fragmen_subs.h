//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef ADJUST_RIVER_FRAGMEN_SUBS_H
#define ADJUST_RIVER_FRAGMEN_SUBS_H

#include "game_primitives.h"

//================================================================================================================================
//=> - RiverFragmenSubs -
//================================================================================================================================

class Adjust_RiverFragmenSubs {
public:
    explicit Adjust_RiverFragmenSubs (u32 seed);

    bool adjust (u8* terrain, const u8* riv, u16 w, u16 h);
    bool is_valid () const;

private:
    Adjust_RiverFragmenSubs (const Adjust_RiverFragmenSubs& other) = delete;
    Adjust_RiverFragmenSubs (Adjust_RiverFragmenSubs&& other) = delete;

    u32 m_seed;
    bool m_valid_adjust;
};

#endif // ADJUST_RIVER_FRAGMEN_SUBS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
