//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include "generate_terrain_cont_pn.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static u32 derived_layer_seed (u32 base_seed, i32 layer_idx) {
    return base_seed ^ (static_cast<u32>(layer_idx + 1) * 2654435769u);
}

static bool save_field_f32_gray_pgm (cstr path, const f32* f, u16 wi, u16 hi) {
    const u32 n = static_cast<u32>(wi) * static_cast<u32>(hi);
    if (n == 0 || f == nullptr) {
        return false;
    }
    f32 vmin = f[0];
    f32 vmax = f[0];
    for (u32 i = 1; i < n; ++i) {
        if (f[i] < vmin) {
            vmin = f[i];
        }
        if (f[i] > vmax) {
            vmax = f[i];
        }
    }
    f32 d = vmax - vmin;
    if (d < 1e-12f) {
        d = 1.f;
    }
    u8* pix = new u8[n];
    for (u32 i = 0; i < n; ++i) {
        f32 t = (f[i] - vmin) / d;
        if (t < 0.f) {
            t = 0.f;
        }
        if (t > 1.f) {
            t = 1.f;
        }
        pix[i] = static_cast<u8>(std::lrint(static_cast<f64>(t) * 255.0));
    }
    bool ok = save_perlin_gray_pgm(path, pix, wi, hi);
    delete[] pix;
    return ok;
}

static bool save_unit_mask_gray_pgm (cstr path, const f32* m, u16 wi, u16 hi) {
    const u32 n = static_cast<u32>(wi) * static_cast<u32>(hi);
    if (n == 0 || m == nullptr) {
        return false;
    }
    u8* pix = new u8[n];
    for (u32 i = 0; i < n; ++i) {
        f32 t = m[i];
        if (t < 0.f) {
            t = 0.f;
        }
        if (t > 1.f) {
            t = 1.f;
        }
        pix[i] = static_cast<u8>(std::lrint(static_cast<f64>(t) * 255.0));
    }
    bool ok = save_perlin_gray_pgm(path, pix, wi, hi);
    delete[] pix;
    return ok;
}

static void fill_radial_mask_raw (f32* mask, u16 w, u16 h, f32 inner_lim) {
    const f32 cx = 0.5f * static_cast<f32>(w - 1);
    const f32 cy = 0.5f * static_cast<f32>(h - 1);
    const u16 wm = w > h ? w : h;
    const f32 r = 0.5f * static_cast<f32>(wm);
    const f32 r_in = r * inner_lim;
    const f32 band = r - r_in;
    const f32 r2 = r * r;
    const f32 r_in2 = r_in * r_in;
    for (u16 py = 0; py < h; ++py) {
        const f32 dy = static_cast<f32>(py) - cy;
        const f32 dy2 = dy * dy;
        for (u16 px = 0; px < w; ++px) {
            const f32 dx = static_cast<f32>(px) - cx;
            const f32 dist2 = dx * dx + dy2;
            u32 idx = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            if (dist2 >= r2) {
                mask[idx] = 0.f;
            } else if (dist2 <= r_in2) {
                mask[idx] = 1.f;
            } else if (band > 1e-12f) {
                const f32 dist = std::sqrt(dist2);
                mask[idx] = 1.f - (dist - r_in) / band;
            } else {
                mask[idx] = 0.f;
            }
        }
    }
}

static size_t sorted_frac_index (u32 n, f64 cum_pop_end, f64 lim_mountains) {
    if (n == 0u) {
        return 0u;
    }
    if (cum_pop_end <= 0.0) {
        return 0u;
    }
    if (cum_pop_end >= lim_mountains) {
        return static_cast<size_t>(n - 1u);
    }
    const size_t j = static_cast<size_t>(std::floor(cum_pop_end * static_cast<f64>(n)));
    return j >= static_cast<size_t>(n) ? static_cast<size_t>(n - 1u) : j;
}

static u8 terrain_class_percentile (f64 v, const f64 thr[5]) {
    if (v <= thr[0]) {
        return TERR_OCEAN[0];
    }
    if (v <= thr[1]) {
        return TERR_SEA[0];
    }
    if (v <= thr[2]) {
        return TERR_COASTAL[0];
    }
    if (v <= thr[3]) {
        return TERR_PLAINS[0];
    }
    if (v <= thr[4]) {
        return TERR_HILLS[0];
    }
    return TERR_MOUNTAINS[0];
}

static bool build_from_params (
    const TerrainContPnParams& p,
    u16& out_w,
    u16& out_h,
    std::vector<u8>& out_gray,
    MapArrayTerrain& out_terrain) {
    const u16 w = p.m_width;
    const u16 h = p.m_height;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (n == 0) {
        return false;
    }
    const i32 lc = p.m_layer_count < 1 ? 1 : p.m_layer_count;
    f32* combo = new f32[n];
    f32* scratch = nullptr;
    if (p.m_debug) {
        scratch = new f32[n];
    }
    f32* radial_dbg = nullptr;
    for (u32 i = 0; i < n; ++i) {
        combo[i] = 0.f;
    }
    if (p.m_debug) {
        radial_dbg = new f32[n];
        fill_radial_mask_raw(radial_dbg, w, h, p.m_inner_grad_limit);
        char db[120];
        std::snprintf(db, sizeof(db), "debug_continent_radial_mask_%u.pgm", p.m_seed);
        if (!save_unit_mask_gray_pgm(db, radial_dbg, w, h)) {
            delete[] combo;
            delete[] scratch;
            delete[] radial_dbg;
            return false;
        }
    }
    PerlinImgParams layer_params;
    layer_params.m_w = w;
    layer_params.m_h = h;
    layer_params.m_lacunarity = p.m_lacunarity;
    f32 freq = p.m_layer_freq_base;
    for (int k = 0; k < lc; ++k) {
        layer_params.m_frequency = freq;
        const u32 lay_seed = derived_layer_seed(p.m_seed, k);
        if (p.m_debug) {
            if (!render_perlin_field_f32(scratch, layer_params, lay_seed)) {
                delete[] combo;
                delete[] scratch;
                delete[] radial_dbg;
                return false;
            }
            char lay_path[120];
            std::snprintf(lay_path, sizeof(lay_path), "debug_continent_layer_%02d_%u.pgm", k + 1, p.m_seed);
            if (!save_field_f32_gray_pgm(lay_path, scratch, w, h)) {
                delete[] combo;
                delete[] scratch;
                delete[] radial_dbg;
                return false;
            }
            const f32 lw = p.m_layer_weight;
            for (u32 i = 0; i < n; ++i) {
                combo[i] += lw * scratch[i];
            }
        } else {
            if (!accumulate_perlin_field_f32(combo, p.m_layer_weight, layer_params, lay_seed)) {
                delete[] combo;
                delete[] scratch;
                delete[] radial_dbg;
                return false;
            }
        }
        freq *= p.m_layer_freq_step;
    }
    delete[] scratch;
    if (p.m_debug && radial_dbg != nullptr) {
        for (u32 i = 0; i < n; ++i) {
            radial_dbg[i] = 1.f - radial_dbg[i];
        }
        char db[120];
        std::snprintf(db, sizeof(db), "debug_continent_radial_inverted_%u.pgm", p.m_seed);
        if (!save_unit_mask_gray_pgm(db, radial_dbg, w, h)) {
            delete[] combo;
            delete[] radial_dbg;
            return false;
        }
        delete[] radial_dbg;
        radial_dbg = nullptr;
    }
    const f32 cx = 0.5f * static_cast<f32>(w - 1);
    const f32 cy = 0.5f * static_cast<f32>(h - 1);
    const u16 wm = w > h ? w : h;
    const f32 r = 0.5f * static_cast<f32>(wm);
    const f32 r_in = r * p.m_inner_grad_limit;
    const f32 band = r - r_in;
    const f32 r2 = r * r;
    const f32 r_in2 = r_in * r_in;
    f32 vmin = 0.f;
    f32 vmax = 0.f;
    bool first = true;
    for (u16 py = 0; py < h; ++py) {
        const f32 dy = static_cast<f32>(py) - cy;
        const f32 dy2 = dy * dy;
        for (u16 px = 0; px < w; ++px) {
            const u32 idx = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            const f32 dx = static_cast<f32>(px) - cx;
            const f32 dist2 = dx * dx + dy2;
            f32 sub = 0.f;
            if (dist2 >= r2) {
                sub = 1.f;
            } else if (dist2 <= r_in2) {
                sub = 0.f;
            } else if (band > 1e-12f) {
                const f32 dist = std::sqrt(dist2);
                sub = (dist - r_in) / band;
            }
            combo[idx] -= sub;
            const f32 v = combo[idx];
            if (first) {
                vmin = vmax = v;
                first = false;
            } else {
                if (v < vmin) {
                    vmin = v;
                }
                if (v > vmax) {
                    vmax = v;
                }
            }
        }
    }
    f32 denom = vmax - vmin;
    if (denom < 1e-12f) {
        denom = 1.f;
    }
    std::vector<f32> norm(static_cast<size_t>(n));
    for (u32 i = 0; i < n; ++i) {
        norm[i] = (combo[i] - vmin) / denom;
    }
    delete[] combo;
    const f64 lim_cap = p.m_terr_lim_mountains;
    const size_t j0 = sorted_frac_index(n, p.m_terr_lim_ocean, lim_cap);
    const size_t j1 = sorted_frac_index(n, p.m_terr_lim_sea, lim_cap);
    const size_t j2 = sorted_frac_index(n, p.m_terr_lim_coastal, lim_cap);
    const size_t j3 = sorted_frac_index(n, p.m_terr_lim_plains, lim_cap);
    const size_t j4 = sorted_frac_index(n, p.m_terr_lim_hills, lim_cap);
    std::vector<f32> work(norm);
    std::nth_element(work.begin(), work.begin() + static_cast<std::ptrdiff_t>(j0), work.end());
    f64 thr[5];
    thr[0] = static_cast<f64>(work[j0]);
    std::nth_element(work.begin(), work.begin() + static_cast<std::ptrdiff_t>(j1), work.end());
    thr[1] = static_cast<f64>(work[j1]);
    std::nth_element(work.begin(), work.begin() + static_cast<std::ptrdiff_t>(j2), work.end());
    thr[2] = static_cast<f64>(work[j2]);
    std::nth_element(work.begin(), work.begin() + static_cast<std::ptrdiff_t>(j3), work.end());
    thr[3] = static_cast<f64>(work[j3]);
    std::nth_element(work.begin(), work.begin() + static_cast<std::ptrdiff_t>(j4), work.end());
    thr[4] = static_cast<f64>(work[j4]);
    out_gray.resize(static_cast<size_t>(n));
    std::vector<u8> out_class(static_cast<size_t>(n));
    for (u32 i = 0; i < n; ++i) {
        f64 vd = static_cast<f64>(norm[i]);
        if (vd < 0.0) {
            vd = 0.0;
        }
        if (vd > 1.0) {
            vd = 1.0;
        }
        out_gray[i] = static_cast<u8>(std::lrint(vd * 255.0));
        out_class[i] = terrain_class_percentile(vd, thr);
    }
    if (!out_terrain.assign_copy(w, h, out_class.data())) {
        return false;
    }
    out_w = w;
    out_h = h;
    return true;
}

//================================================================================================================================
//=> - TerrainContPn -
//================================================================================================================================

Generate_TerrainContPn::Generate_TerrainContPn (const TerrainContPnParams& params) :
    m_valid_generation(false),
    m_params(params),
    m_w(0),
    m_h(0),
    m_height_gray(),
    m_terrain() {
    generate();
}

bool Generate_TerrainContPn::generate () {
    m_valid_generation = build_from_params(m_params, m_w, m_h, m_height_gray, m_terrain);
    return m_valid_generation;
}

bool Generate_TerrainContPn::is_valid () const {
    return m_valid_generation;
}

void Generate_TerrainContPn::save_output (cstr path) const {
    if (path == nullptr) {
        return;
    }
    save_terrain_rgb(path);
}

u16 Generate_TerrainContPn::width () const {
    return m_w;
}

u16 Generate_TerrainContPn::height () const {
    return m_h;
}

const u8* Generate_TerrainContPn::height_gray () const {
    return m_height_gray.empty() ? nullptr : m_height_gray.data();
}

const u8* Generate_TerrainContPn::terrain_class () const {
    return m_terrain.data();
}

const MapArrayTerrain& Generate_TerrainContPn::terrain () const {
    return m_terrain;
}

bool Generate_TerrainContPn::save_height_gray (cstr path) const {
    if (!m_valid_generation || path == nullptr) {
        return false;
    }
    return save_perlin_gray_pgm(path, m_height_gray.data(), m_w, m_h);
}

bool Generate_TerrainContPn::save_terrain_rgb (cstr path) const {
    if (!m_valid_generation || path == nullptr) {
        return false;
    }
    return m_terrain.save(path);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
