//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef INTF_LOOP_VIEW_H
#define INTF_LOOP_VIEW_H

#include "intf_snapshot_view.h"

//================================================================================================================================
//=> - Class: Intf_LoopView -
//================================================================================================================================
//
//  UI mk2 surface: snapshot reads plus an autonomous game loop (no player commands).
//  begin/step/end mirror GameLoop; map getters stay valid between begin and end.
//
//================================================================================================================================

class Intf_LoopView : public Intf_SnapshotView {
public:
    virtual ~Intf_LoopView ();

    virtual bool begin (cstr trace_path) = 0; // Bind/start match; trace_path may be null
    virtual bool step () = 0; // Advance one turn
    virtual void end () = 0; // Tear down loop bindings
};

#endif // INTF_LOOP_VIEW_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
