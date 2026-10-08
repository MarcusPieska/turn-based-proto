//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "intf_mk1_snapshot_view.h"

//================================================================================================================================
//=> - IntfMk1_SnapshotView -
//================================================================================================================================

IntfMk1_SnapshotView::IntfMk1_SnapshotView () : m_ext(nullptr) {
}

IntfMk1_SnapshotView::~IntfMk1_SnapshotView () {
    clear();
}

bool IntfMk1_SnapshotView::bind_map (const GameArraySimple* map) {
    if (map == nullptr || map->width() == 0 || map->height() == 0 || map->tile_n() == 0) {
        return false;
    }
    m_map.clear();
    m_ext = map;
    return true;
}

void IntfMk1_SnapshotView::clear () {
    m_ext = nullptr;
    m_map.clear();
}

const GameArraySimple* IntfMk1_SnapshotView::src () const {
    if (m_ext != nullptr) {
        return m_ext;
    }
    if (m_map.tile_n() == 0) {
        return nullptr;
    }
    return &m_map;
}

bool IntfMk1_SnapshotView::ready () const {
    return src() != nullptr;
}

u16 IntfMk1_SnapshotView::width () const {
    const GameArraySimple* m = src();
    return m != nullptr ? m->width() : 0;
}

u16 IntfMk1_SnapshotView::height () const {
    const GameArraySimple* m = src();
    return m != nullptr ? m->height() : 0;
}

u8 IntfMk1_SnapshotView::get_terrain (u16 x, u16 y) const {
    const GameArraySimple* m = src();
    return m != nullptr ? m->get_terrain(x, y) : 0;
}

u8 IntfMk1_SnapshotView::get_climate (u16 x, u16 y) const {
    const GameArraySimple* m = src();
    return m != nullptr ? m->get_climate(x, y) : 0;
}

u8 IntfMk1_SnapshotView::get_river (u16 x, u16 y) const {
    const GameArraySimple* m = src();
    return m != nullptr ? m->get_river(x, y) : 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
