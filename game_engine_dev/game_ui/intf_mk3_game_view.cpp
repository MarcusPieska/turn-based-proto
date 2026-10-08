//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "intf_mk3_game_view.h"

//================================================================================================================================
//=> - IntfMk3_GameView -
//================================================================================================================================

IntfMk3_GameView::IntfMk3_GameView () {
}

IntfMk3_GameView::~IntfMk3_GameView () {
}

IntfMk2_LoopView& IntfMk3_GameView::loop () {
    return m_loop;
}

const IntfMk2_LoopView& IntfMk3_GameView::loop () const {
    return m_loop;
}

bool IntfMk3_GameView::ready () const {
    return m_loop.ready();
}

u16 IntfMk3_GameView::width () const {
    return m_loop.width();
}

u16 IntfMk3_GameView::height () const {
    return m_loop.height();
}

u8 IntfMk3_GameView::get_terrain (u16 x, u16 y) const {
    return m_loop.get_terrain(x, y);
}

u8 IntfMk3_GameView::get_climate (u16 x, u16 y) const {
    return m_loop.get_climate(x, y);
}

u8 IntfMk3_GameView::get_river (u16 x, u16 y) const {
    return m_loop.get_river(x, y);
}

bool IntfMk3_GameView::begin (cstr trace_path) {
    return m_loop.begin(trace_path);
}

bool IntfMk3_GameView::step () {
    return m_loop.step();
}

void IntfMk3_GameView::end () {
    m_loop.end();
}

bool IntfMk3_GameView::submit_cmd (cstr cmd, cstr args) {
    (void)cmd;
    (void)args;
    return false;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
