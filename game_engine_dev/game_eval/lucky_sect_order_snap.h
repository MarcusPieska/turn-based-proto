//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LUCKY_SECT_ORDER_SNAP_H
#define LUCKY_SECT_ORDER_SNAP_H

#include "game_primitives.h"

class GameArraySimple;

//================================================================================================================================
//=> - LuckySectOrderSnap -
//================================================================================================================================
//
//  One PPM: bleached terrain, green->blue fill by select order, black 1px rim on selected,
//  medium-gray rim on never-selected. Optional skipped[] paints reserved red (no rim change).
//
//================================================================================================================================

class LuckySectOrderSnap {
public:
    LuckySectOrderSnap () = delete;

    static bool write (
        cstr path,
        const GameArraySimple& map,
        const u16* paint,
        u16 pw,
        u16 ph,
        u16 sec_n,
        const u16* order,
        u16 order_n,
        const u8* skipped);
};

#endif // LUCKY_SECT_ORDER_SNAP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
