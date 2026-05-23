//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_terrain_rotation.h"

#include <cmath>

//================================================================================================================================
//=> - TerrainRotation -
//================================================================================================================================

Generate_TerrainRotation::Generate_TerrainRotation () :
    m_valid_generation(false),
    m_terrain(nullptr) {
}

Generate_TerrainRotation::~Generate_TerrainRotation () {
    delete m_terrain;
    m_terrain = nullptr;
}

i32 Generate_TerrainRotation::norm_deg_cw (i32 degrees_cw) {
    i32 d = degrees_cw % 360;
    if (d < 0) {
        d += 360;
    }
    return d;
}

bool Generate_TerrainRotation::rot_nn_cw (const MapArrayTerrain& src, i32 degrees_cw) {
    const u16 w = src.width();
    const u16 h = src.height();
    const u8* sd = src.data();
    if (w == 0 || h == 0 || sd == nullptr) {
        return false;
    }
    const i32 d = norm_deg_cw(degrees_cw);
    if (d == 0) {
        if (m_terrain == nullptr) {
            m_terrain = new MapArrayTerrain();
        }
        return m_terrain->assign_copy(w, h, sd);
    }
    const f32 rad = static_cast<f32>(d) * static_cast<f32>(M_PI) / 180.f;
    const f32 co = std::cos(rad);
    const f32 si = std::sin(rad);
    const f32 cx = static_cast<f32>(w - 1u) * 0.5f;
    const f32 cy = static_cast<f32>(h - 1u) * 0.5f;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* buf = new u8[n];
    const u8 pad = TERR_OCEAN[0];
    for (u16 dy = 0; dy < h; ++dy) {
        for (u16 dx = 0; dx < w; ++dx) {
            const f32 rx = static_cast<f32>(dx) - cx;
            const f32 ry = static_cast<f32>(dy) - cy;
            const f32 sx = co * rx + si * ry + cx;
            const f32 sy = -si * rx + co * ry + cy;
            const i32 ix = static_cast<i32>(std::lrint(sx));
            const i32 iy = static_cast<i32>(std::lrint(sy));
            u8 v = pad;
            if (ix >= 0 && iy >= 0 && ix < static_cast<i32>(w) && iy < static_cast<i32>(h)) {
                v = sd[static_cast<u32>(iy) * static_cast<u32>(w) + static_cast<u32>(ix)];
            }
            buf[static_cast<u32>(dy) * static_cast<u32>(w) + static_cast<u32>(dx)] = v;
        }
    }
    if (m_terrain == nullptr) {
        m_terrain = new MapArrayTerrain();
    }
    const bool ok = m_terrain->assign_copy(w, h, buf);
    delete[] buf;
    return ok;
}

bool Generate_TerrainRotation::generate (const MapArrayTerrain& src, i32 degrees_cw) {
    m_valid_generation = false;
    const bool ok = rot_nn_cw(src, degrees_cw);
    m_valid_generation = ok;
    return ok;
}

bool Generate_TerrainRotation::is_valid () const {
    return m_valid_generation;
}

bool Generate_TerrainRotation::save_output (cstr path) const {
    if (path == nullptr) {
        return false;
    }
    if (!m_valid_generation || m_terrain == nullptr) {
        return false;
    }
    return m_terrain->save(path);
}

u16 Generate_TerrainRotation::width () const {
    return m_terrain != nullptr ? m_terrain->width() : 0;
}

u16 Generate_TerrainRotation::height () const {
    return m_terrain != nullptr ? m_terrain->height() : 0;
}

const MapArrayTerrain& Generate_TerrainRotation::terrain () const {
    return *m_terrain;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
