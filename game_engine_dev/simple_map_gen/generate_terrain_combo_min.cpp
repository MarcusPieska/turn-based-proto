//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstddef>
#include <vector>

#include "generate_terrain_combo_min.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static u8 pick_min (u8 a, u8 b) {
    return a < b ? a : b;
}

//================================================================================================================================
//=> - TerrainComboMin -
//================================================================================================================================

Generate_TerrainComboMin::Generate_TerrainComboMin () :
    m_valid_generation(false),
    m_terrain(nullptr) {
}

Generate_TerrainComboMin::~Generate_TerrainComboMin () {
    delete m_terrain;
    m_terrain = nullptr;
}

bool Generate_TerrainComboMin::generate (const MapArrayTerrain& a, const MapArrayTerrain& b) {
    m_valid_generation = false;
    delete m_terrain;
    m_terrain = nullptr;
    const u16 w = a.width();
    const u16 h = a.height();
    const u8* ad = a.data();
    const u8* bd = b.data();
    if (w == 0 || h == 0 || ad == nullptr || bd == nullptr) {
        return false;
    }
    if (b.width() != w || b.height() != h || bd == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::vector<u8> out(static_cast<size_t>(n));
    for (u32 i = 0; i < n; ++i) {
        out[i] = pick_min(ad[i], bd[i]);
    }
    m_terrain = new MapArrayTerrain();
    if (!m_terrain->assign_copy(w, h, out.data())) {
        delete m_terrain;
        m_terrain = nullptr;
        return false;
    }
    m_valid_generation = true;
    return true;
}

bool Generate_TerrainComboMin::is_valid () const {
    return m_valid_generation;
}

void Generate_TerrainComboMin::save_output (cstr path) const {
    if (path == nullptr) {
        return;
    }
    if (!m_valid_generation || m_terrain == nullptr) {
        return;
    }
    m_terrain->save(path);
}

u16 Generate_TerrainComboMin::width () const {
    return m_terrain != nullptr ? m_terrain->width() : 0;
}

u16 Generate_TerrainComboMin::height () const {
    return m_terrain != nullptr ? m_terrain->height() : 0;
}

const u8* Generate_TerrainComboMin::terrain_class () const {
    return m_terrain != nullptr ? m_terrain->data() : nullptr;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
