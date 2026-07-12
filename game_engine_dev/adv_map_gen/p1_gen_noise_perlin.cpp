//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>

#include "p1_gen_noise_perlin.h"
#include "perlin_noise.h"
#include "p1_step_log.h"
#include "p1_wb_util.h" 

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static u32 derived_layer_seed (u32 base_seed, i32 layer_idx) {
    return base_seed ^ (static_cast<u32>(layer_idx + 1) * 2654435769u);
}

bool p1_build_perlin_field_f32 (u32 seed, const P1_Gen_NoisePerlinPrm& prm, f32* combo, u8* gray) {
    const u16 w = prm.m_w;
    const u16 h = prm.m_h;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (n == 0 || combo == nullptr) {
        P1_STEP_ABORT("P1_Gen_NoisePerlin", "null combo or zero pixels");
    }
    i32 lc = prm.m_layer_count;
    if (lc < 1) {
        lc = 1;
    }
    if (lc > 16) {
        P1_STEP_ABORT("P1_Gen_NoisePerlin", "layer_count out of range");
    }
    PerlinLayerSpec layers[16];
    f32 freq = prm.m_layer_freq_base;
    for (i32 k = 0; k < lc; ++k) {
        layers[k].m_weight = prm.m_layer_weight;
        layers[k].m_frequency = freq;
        layers[k].m_seed = derived_layer_seed(seed, k);
        freq *= prm.m_layer_freq_step;
    }
    return render_perlin_layers_f32(combo, gray, w, h, prm.m_lacunarity, layers, lc);
}

void p1_perlin_field_to_gray (const f32* combo, u32 n, u8* out_gray) {
    if (combo == nullptr || out_gray == nullptr || n == 0) {
        return;
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
    m_rslt.m_prm = p1_gen_noise_perlin_prm_def(prm.m_w, prm.m_h);
}

bool P1_Gen_NoisePerlin::generate () {
    m_valid_generation = false;
    m_rslt.m_ov.clear();
    if (!p1_run_prm_ok(m_prm)) {
        P1_STEP_ABORT("P1_Gen_NoisePerlin", "invalid run prm");
    }
    m_rslt.m_prm = p1_gen_noise_perlin_prm_def(m_prm.m_w, m_prm.m_h);
    m_rslt.m_w = m_rslt.m_prm.m_w;
    m_rslt.m_h = m_rslt.m_prm.m_h;
    const u32 n = static_cast<u32>(m_rslt.m_w) * static_cast<u32>(m_rslt.m_h);
    if (n == 0 || !m_rslt.m_ov.resize(m_rslt.m_w, m_rslt.m_h)) {
        P1_STEP_ABORT("P1_Gen_NoisePerlin", "overlay resize failed");
    }
    Whiteboard_4B wb_combo("P1_Gen_NoisePerlin", "combo", m_prm.m_seed);
    P1_WB_CHK(wb_combo);
    f32* combo = reinterpret_cast<f32*>(wb_combo.get_iter_ptr());
    u8* gray = m_rslt.m_ov.data_w();
    if (!p1_build_perlin_field_f32(m_prm.m_seed, m_rslt.m_prm, combo, gray)) {
        P1_STEP_ABORT("P1_Gen_NoisePerlin", "perlin field build failed");
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_NoisePerlin::generate_with_combo (u32 seed, const P1_Gen_NoisePerlinPrm& prm, f32* combo) {
    m_valid_generation = false;
    m_rslt.m_ov.clear();
    if (!p1_run_prm_ok(m_prm) || combo == nullptr) {
        P1_STEP_ABORT("P1_Gen_NoisePerlin", "generate_with_combo invalid prm or null combo");
    }
    m_rslt.m_prm = prm;
    m_rslt.m_w = prm.m_w;
    m_rslt.m_h = prm.m_h;
    const u32 n = static_cast<u32>(m_rslt.m_w) * static_cast<u32>(m_rslt.m_h);
    if (n == 0 || !m_rslt.m_ov.resize(m_rslt.m_w, m_rslt.m_h)) {
        P1_STEP_ABORT("P1_Gen_NoisePerlin", "overlay resize failed");
    }
    if (!p1_build_perlin_field_f32(seed, prm, combo, m_rslt.m_ov.data_w())) {
        P1_STEP_ABORT("P1_Gen_NoisePerlin", "generate_with_combo field build failed");
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_NoisePerlin::generate_from_field (const f32* combo, const P1_Gen_NoisePerlinPrm& prm) {
    m_valid_generation = false;
    m_rslt.m_ov.clear();
    if (!p1_run_prm_ok(m_prm) || combo == nullptr) {
        P1_STEP_ABORT("P1_Gen_NoisePerlin", "generate_from_field invalid prm or null combo");
    }
    m_rslt.m_prm = prm;
    m_rslt.m_w = prm.m_w;
    m_rslt.m_h = prm.m_h;
    const u32 n = static_cast<u32>(m_rslt.m_w) * static_cast<u32>(m_rslt.m_h);
    if (n == 0 || !m_rslt.m_ov.resize(m_rslt.m_w, m_rslt.m_h)) {
        P1_STEP_ABORT("P1_Gen_NoisePerlin", "overlay resize failed");
    }
    p1_perlin_field_to_gray(combo, n, m_rslt.m_ov.data_w());
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
