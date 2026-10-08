//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef INTF_MK2_LOOP_VIEW_H
#define INTF_MK2_LOOP_VIEW_H

#include "intf_loop_view.h"
#include "intf_mk1_snapshot_view.h"

//================================================================================================================================
//=> - Class: IntfMk2_LoopView -
//================================================================================================================================
//
//  Concrete UI mk2 boilerplate. Reuses IntfMk1_SnapshotView for map reads; begin/step/end are
//  stubs until GameLoop/GameSetup wiring is added under game/.
//
//================================================================================================================================

class IntfMk2_LoopView : public Intf_LoopView {
public:
    IntfMk2_LoopView ();
    ~IntfMk2_LoopView () override;

    IntfMk1_SnapshotView& snapshot ();
    const IntfMk1_SnapshotView& snapshot () const;

    bool ready () const override;
    u16 width () const override;
    u16 height () const override;
    u8 get_terrain (u16 x, u16 y) const override;
    u8 get_climate (u16 x, u16 y) const override;
    u8 get_river (u16 x, u16 y) const override;

    bool begin (cstr trace_path) override;
    bool step () override;
    void end () override;

private:
    IntfMk1_SnapshotView m_snap; // Shared map-read implementation for mk2/mk3
    bool m_running; // True between successful begin and end
};

#endif // INTF_MK2_LOOP_VIEW_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
