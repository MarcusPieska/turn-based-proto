//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef INTF_MK3_GAME_VIEW_H
#define INTF_MK3_GAME_VIEW_H

#include "intf_game_view.h"
#include "intf_mk2_loop_view.h"

//================================================================================================================================
//=> - Class: IntfMk3_GameView -
//================================================================================================================================
//
//  Concrete UI mk3 boilerplate. Delegates loop/snapshot to IntfMk2_LoopView; submit_cmd is a
//  stub until interactive command routing exists under game/.
//
//================================================================================================================================

class IntfMk3_GameView : public Intf_GameView {
public:
    IntfMk3_GameView ();
    ~IntfMk3_GameView () override;

    IntfMk2_LoopView& loop ();
    const IntfMk2_LoopView& loop () const;

    bool ready () const override;
    u16 width () const override;
    u16 height () const override;
    u8 get_terrain (u16 x, u16 y) const override;
    u8 get_climate (u16 x, u16 y) const override;
    u8 get_river (u16 x, u16 y) const override;

    bool begin (cstr trace_path) override;
    bool step () override;
    void end () override;
    bool submit_cmd (cstr cmd, cstr args) override;

private:
    IntfMk2_LoopView m_loop; // Loop + snapshot implementation reused by mk3
};

#endif // INTF_MK3_GAME_VIEW_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
