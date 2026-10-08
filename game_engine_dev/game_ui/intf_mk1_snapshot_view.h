//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef INTF_MK1_SNAPSHOT_VIEW_H
#define INTF_MK1_SNAPSHOT_VIEW_H

#include "game_array_simple.h"
#include "intf_snapshot_view.h"

//================================================================================================================================
//=> - Class: IntfMk1_SnapshotView -
//================================================================================================================================
//
//  Concrete UI mk1 session. Owns a GameArraySimple for PPM/tiles loads, or borrows an external
//  grid via bind_map for later mk2/mk3 reuse. Forwards terrain/climate/river reads only.
//
//================================================================================================================================

class IntfMk1_SnapshotView : public Intf_SnapshotView {
public:
    IntfMk1_SnapshotView ();
    ~IntfMk1_SnapshotView () override;

    bool load_map_ppms (cstr terr_path, cstr clim_path, cstr riv_path, cstr ov_path = nullptr);
    bool load_map_tiles (cstr path);
    bool bind_map (const GameArraySimple* map);
    void clear ();

    bool ready () const override;
    u16 width () const override;
    u16 height () const override;
    u8 get_terrain (u16 x, u16 y) const override;
    u8 get_climate (u16 x, u16 y) const override;
    u8 get_river (u16 x, u16 y) const override;

private:
    const GameArraySimple* src () const;

    GameArraySimple m_map; // Owned grid filled by load_*
    const GameArraySimple* m_ext; // Borrowed grid from bind_map; null if unused
};

#endif // INTF_MK1_SNAPSHOT_VIEW_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
