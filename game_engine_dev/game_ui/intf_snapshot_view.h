//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef INTF_SNAPSHOT_VIEW_H
#define INTF_SNAPSHOT_VIEW_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Class: Intf_SnapshotView -
//================================================================================================================================
//
//  Read-only map surface for UI mk1. Exposes tile geography the view can sample in-frame.
//  Mk2/Mk3 interfaces extend this so a renderer can keep depending on Intf_SnapshotView alone.
//
//================================================================================================================================

class Intf_SnapshotView {
public:
    virtual ~Intf_SnapshotView ();

    virtual bool ready () const = 0; // True when a map grid is bound or loaded
    virtual u16 width () const = 0; // Grid width in tiles
    virtual u16 height () const = 0; // Grid height in tiles
    virtual u8 get_terrain (u16 x, u16 y) const = 0; // Terrain class at tile
    virtual u8 get_climate (u16 x, u16 y) const = 0; // Climate class at tile
    virtual u8 get_river (u16 x, u16 y) const = 0; // River flag at tile
};

#endif // INTF_SNAPSHOT_VIEW_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
