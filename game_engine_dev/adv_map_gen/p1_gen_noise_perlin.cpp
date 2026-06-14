//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>

#include "p1_gen_noise_perlin.h"
#include "perlin_noise.h"

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static u32 derived_layer_seed (u32 base_seed, i32 layer_idx) {
    return base_seed ^ (static_cast<u32>(layer_idx + 1) * 2654435769u);
}

static void default_prm (P1_Gen_NoisePerlinPrm* p, u16 map_w, u16 map_h) {
    p->m_w = map_w;
    p->m_h = map_h;
    p->m_lacunarity = 2.f;
    p->m_layer_freq_base = 0.5f;
    p->m_layer_weight = 0.2f;
    p->m_layer_freq_step = 1.62f;
    p->m_layer_count = 5;
}

static bool build_layered_perlin_gray (
    u32 seed,
    const P1_Gen_NoisePerlinPrm& prm,
    u8* out_gray) {
    const u16 w = prm.m_w;
    const u16 h = prm.m_h;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (n == 0 || out_gray == nullptr) {
        return false;
    }
    i32 lc = prm.m_layer_count;
    if (lc < 1) {
        lc = 1;
    }
    f32* combo = new f32[n];
    if (combo == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        combo[i] = 0.f;
    }
    PerlinImgParams layer_params;
    layer_params.m_w = w;
    layer_params.m_h = h;
    layer_params.m_lacunarity = prm.m_lacunarity;
    f32 freq = prm.m_layer_freq_base;
    for (i32 k = 0; k < lc; ++k) {
        layer_params.m_frequency = freq;
        const u32 lay_seed = derived_layer_seed(seed, k);
        if (!accumulate_perlin_field_f32(combo, prm.m_layer_weight, layer_params, lay_seed)) {
            delete[] combo;
            return false;
        }
        freq *= prm.m_layer_freq_step;
    }
    f32 vmin = combo[0];
    f32 vmax = combo[0];
    for (u32 i = 1; i < n; ++i) {
        if (combo[i] < vmin) {
            vmin = combo[i];
        }
        if (combo[i] > vmax) {
            vmax = combo[i];
        }
    }
    f32 denom = vmax - vmin;
    if (denom < 1e-12f) {
        denom = 1.f;
    }
    for (u32 i = 0; i < n; ++i) {
        f32 t = (combo[i] - vmin) / denom;
        if (t < 0.f) {
            t = 0.f;
        }
        if (t > 1.f) {
            t = 1.f;
        }
        out_gray[i] = static_cast<u8>(std::lrint(static_cast<f64>(t) * 255.0));
    }
    delete[] combo;
    return true;
}

//================================================================================================================================
//=> - P1_Gen_NoisePerlin -
//================================================================================================================================

P1_Gen_NoisePerlin::P1_Gen_NoisePerlin (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    default_prm(&m_rslt.m_prm, prm.m_w, prm.m_h);
}

bool P1_Gen_NoisePerlin::generate () {
    m_valid_generation = false;
    m_rslt.m_ov.clear();
    if (!p1_run_prm_ok(m_prm)) {
        return false;
    }
    default_prm(&m_rslt.m_prm, m_prm.m_w, m_prm.m_h);
    m_rslt.m_w = m_rslt.m_prm.m_w;
    m_rslt.m_h = m_rslt.m_prm.m_h;
    if (!m_rslt.m_ov.resize(m_rslt.m_w, m_rslt.m_h)) {
        return false;
    }
    if (!build_layered_perlin_gray(m_prm.m_seed, m_rslt.m_prm, m_rslt.m_ov.data_w())) {
        return false;
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_NoisePerlin::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_NoisePerlinRslt& P1_Gen_NoisePerlin::result () const {
    return m_rslt;
}

void P1_Gen_NoisePerlin::save_output (cstr path) const {
    if (!m_valid_generation || path == nullptr) {
        return;
    }
    m_rslt.m_ov.save(path);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
