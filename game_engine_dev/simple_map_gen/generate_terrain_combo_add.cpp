//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_terrain_combo_add.h"

#include <cstddef>
#include <vector>

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static u8 pick_class (u8 a, u8 b) {
    return a > b ? a : b;
}

//================================================================================================================================
//=> - TerrainComboAdd -
//================================================================================================================================

Generate_TerrainComboAdd::Generate_TerrainComboAdd () :
    m_valid_generation(false),
    m_terrain(nullptr) {
}

Generate_TerrainComboAdd::~Generate_TerrainComboAdd () {
    delete m_terrain;
    m_terrain = nullptr;
}

bool Generate_TerrainComboAdd::generate (
    const MapArrayTerrain& base,
    const MapArrayTerrain& add,
    i32 offset_x,
    i32 offset_y) {
    m_valid_generation = false;
    delete m_terrain;
    m_terrain = nullptr;
    const u16 bw = base.width();
    const u16 bh = base.height();
    const u8* bd = base.data();
    const u16 aw = add.width();
    const u16 ah = add.height();
    const u8* ad = add.data();
    const u32 bn = static_cast<u32>(bw) * static_cast<u32>(bh);
    if (bn == 0 || bd == nullptr) {
        return false;
    }
    if (aw == 0 || ah == 0 || ad == nullptr) {
        return false;
    }
    std::vector<u8> out(static_cast<size_t>(bn));
    for (u16 by = 0; by < bh; ++by) {
        for (u16 bx = 0; bx < bw; ++bx) {
            const u32 bi = static_cast<u32>(by) * static_cast<u32>(bw) + static_cast<u32>(bx);
            u8 v = bd[bi];
            const i32 sx = static_cast<i32>(bx) - offset_x;
            const i32 sy = static_cast<i32>(by) - offset_y;
            if (sx >= 0 && sy >= 0 && sx < static_cast<i32>(aw) && sy < static_cast<i32>(ah)) {
                const u32 ai = static_cast<u32>(sy) * static_cast<u32>(aw) + static_cast<u32>(sx);
                v = pick_class(v, ad[ai]);
            }
            out[bi] = v;
        }
    }
    m_terrain = new MapArrayTerrain();
    if (!m_terrain->assign_copy(bw, bh, out.data())) {
        delete m_terrain;
        m_terrain = nullptr;
        return false;
    }
    m_valid_generation = true;
    return true;
}

bool Generate_TerrainComboAdd::is_valid () const {
    return m_valid_generation;
}

void Generate_TerrainComboAdd::save_output (cstr path) const {
    if (path == nullptr) {
        return;
    }
    if (!m_valid_generation || m_terrain == nullptr) {
        return;
    }
    m_terrain->save(path);
}

u16 Generate_TerrainComboAdd::width () const {
    return m_terrain != nullptr ? m_terrain->width() : 0;
}

u16 Generate_TerrainComboAdd::height () const {
    return m_terrain != nullptr ? m_terrain->height() : 0;
}

const u8* Generate_TerrainComboAdd::terrain_class () const {
    return m_terrain != nullptr ? m_terrain->data() : nullptr;
}

MapArrayTerrain* Generate_TerrainComboAdd::take_terrain () {
    MapArrayTerrain* p = m_terrain;
    m_terrain = nullptr;
    m_valid_generation = false;
    return p;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
