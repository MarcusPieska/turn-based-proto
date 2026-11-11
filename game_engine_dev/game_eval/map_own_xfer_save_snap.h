//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_OWN_XFER_SAVE_SNAP_H
#define MAP_OWN_XFER_SAVE_SNAP_H

#include "game_primitives.h"

class GameArraySimple;

//================================================================================================================================
//=> - MapOwnXferSaveSnap -
//================================================================================================================================
//
//  Bleached climate (+ rivers) base, ownership shade, roads/cities, then mountains last with a
//  slight alpha so ranges sit on top of transfer stripes. write() shades someone→someone changes
//  between prev and cur (none→someone ignored). write_seats() is the all-xfer composite path.
//
//================================================================================================================================

class MapOwnXferSaveSnap {
public:
    MapOwnXferSaveSnap () = delete;

    static bool write (cstr path, const GameArraySimple& cur, const GameArraySimple& prev);
    static bool write_seats (cstr path, const GameArraySimple& base, const u8* seats);
};

#endif // MAP_OWN_XFER_SAVE_SNAP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
