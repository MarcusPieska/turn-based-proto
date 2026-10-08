//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "intf_mk2_loop_view.h"

//================================================================================================================================
//=> - IntfMk2_LoopView -
//================================================================================================================================

IntfMk2_LoopView::IntfMk2_LoopView () : m_running(false) {
}

IntfMk2_LoopView::~IntfMk2_LoopView () {
    end();
}

IntfMk1_SnapshotView& IntfMk2_LoopView::snapshot () {
    return m_snap;
}

const IntfMk1_SnapshotView& IntfMk2_LoopView::snapshot () const {
    return m_snap;
}

bool IntfMk2_LoopView::ready () const {
    return m_snap.ready();
}

u16 IntfMk2_LoopView::width () const {
    return m_snap.width();
}

u16 IntfMk2_LoopView::height () const {
    return m_snap.height();
}

u8 IntfMk2_LoopView::get_terrain (u16 x, u16 y) const {
    return m_snap.get_terrain(x, y);
}

u8 IntfMk2_LoopView::get_climate (u16 x, u16 y) const {
    return m_snap.get_climate(x, y);
}

u8 IntfMk2_LoopView::get_river (u16 x, u16 y) const {
    return m_snap.get_river(x, y);
}

bool IntfMk2_LoopView::begin (cstr trace_path) {
    (void)trace_path;
    m_running = false;
    return false;
}

bool IntfMk2_LoopView::step () {
    if (!m_running) {
        return false;
    }
    return false;
}

void IntfMk2_LoopView::end () {
    m_running = false;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
