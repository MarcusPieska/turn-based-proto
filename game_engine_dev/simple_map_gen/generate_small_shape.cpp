//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "generate_small_shape.h"

#include "generate_terrain_cont_outline.h"
#include "generate_terrain_rotation.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

static u16 max_u16 (u16 a, u16 b) {
    return a > b ? a : b;
}

static u16 canvas_sz (u16 want_w, u16 want_h) {
    static const u16 k_min = 48;
    u16 sz = static_cast<u16>(max_u16(want_w, want_h) * 2u);
    if (sz < k_min) {
        sz = k_min;
    }
    return sz;
}

static bool is_water_cls (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool save_bin_ppm (cstr path, const u8* ter, u16 w, u16 h) {
    if (path == nullptr || ter == nullptr || w == 0 || h == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        const u8 v = is_water_cls(ter[i]) ? 255u : 0u;
        std::fwrite(&v, 1, 1, fp);
        std::fwrite(&v, 1, 1, fp);
        std::fwrite(&v, 1, 1, fp);
    }
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - Generate_SmallShape -
//================================================================================================================================

Generate_SmallShape::Generate_SmallShape (const SmallShapeParams& params) :
    m_params(params),
    m_canvas_sz(0),
    m_valid_generation(false),
    m_terrain() {
}

bool Generate_SmallShape::generate () {
    m_valid_generation = false;
    m_terrain.clear();
    const u16 want_w = m_params.m_width;
    const u16 want_h = m_params.m_height;
    if (want_w == 0 || want_h == 0) {
        return false;
    }
    const u16 out_sz = canvas_sz(want_w, want_h);
    m_canvas_sz = out_sz;
    TerrainContOutlineParams op = {};
    op.m_seed = m_params.m_seed;
    op.m_width = out_sz;
    op.m_height = out_sz;
    op.m_fill_mode = TERR_OUTLINE_FILL_MODE_PERLIN_NOISE;
    op.m_outline_stretch_x = 1.f;
    op.m_outline_stretch_y = 1.f;
    if (want_w > want_h) {
        op.m_outline_stretch_y = static_cast<f32>(want_h) / static_cast<f32>(want_w);
    } else if (want_h > want_w) {
        op.m_outline_stretch_x = static_cast<f32>(want_w) / static_cast<f32>(want_h);
    }
    op.m_pn_params.m_inner_grad_limit = 0.65f;
    op.m_pn_params.m_layer_freq_base = 1.2f;
    op.m_pn_params.m_layer_count = 4;
    Generate_TerrainContOutline outline(op);
    if (!outline.generate() || !outline.is_valid()) {
        return false;
    }
    const u8* src = outline.terrain().data();
    if (src == nullptr) {
        return false;
    }
    MapArrayTerrain base;
    if (!base.assign_copy(out_sz, out_sz, src)) {
        return false;
    }
    Generate_TerrainRotation rot;
    if (!rot.generate(base, m_params.m_angle_deg) || !rot.is_valid()) {
        return false;
    }
    const u8* rd = rot.terrain().data();
    if (rd == nullptr) {
        return false;
    }
    if (!m_terrain.assign_copy(out_sz, out_sz, rd)) {
        return false;
    }
    m_valid_generation = true;
    return true;
}

bool Generate_SmallShape::is_valid () const {
    return m_valid_generation;
}

bool Generate_SmallShape::save_output (cstr path) const {
    if (!m_valid_generation || path == nullptr) {
        return false;
    }
    const u8* ter = m_terrain.data();
    if (ter == nullptr) {
        return false;
    }
    return save_bin_ppm(path, ter, m_terrain.width(), m_terrain.height());
}

u16 Generate_SmallShape::width () const {
    return m_terrain.width();
}

u16 Generate_SmallShape::height () const {
    return m_terrain.height();
}

u16 Generate_SmallShape::canvas_size () const {
    return m_canvas_sz;
}

const MapArrayTerrain& Generate_SmallShape::terrain () const {
    return m_terrain;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
