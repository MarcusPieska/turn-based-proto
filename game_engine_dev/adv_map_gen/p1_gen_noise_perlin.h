//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_NOISE_PERLIN_H
#define P1_GEN_NOISE_PERLIN_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_map_config.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Gen_NoisePerlinPrm -
//================================================================================================================================

struct P1_Gen_NoisePerlinPrm {
    u16 m_w;
    u16 m_h;
    f32 m_lacunarity; // Low cost: octave spacing at setup only; 9 octaves per layer either way
    f32 m_layer_freq_base; // Low cost: layer-0 frequency at setup only; no extra per-pixel work
    f32 m_layer_weight; // Low cost: one multiply per layer per pixel
    f32 m_layer_freq_step; // Low cost: layer frequency list at setup only; no extra per-pixel work
    i32 m_layer_count; // High cost: linear cost; each layer adds 9 noise2() calls per pixel
};

static inline P1_Gen_NoisePerlinPrm p1_gen_noise_perlin_prm_from_cfg (const P1_MapConfig& cfg, u16 w, u16 h) {
    P1_Gen_NoisePerlinPrm p;
    p.m_w = w;
    p.m_h = h;
    p.m_lacunarity = cfg.m_perlin_lacunarity;
    p.m_layer_freq_base = cfg.m_perlin_layer_freq_base;
    p.m_layer_weight = cfg.m_perlin_layer_weight;
    p.m_layer_freq_step = cfg.m_perlin_layer_freq_step;
    p.m_layer_count = cfg.m_perlin_layer_count;
    if (p.m_layer_count < 1) {
        p.m_layer_count = 1;
    }
    if (p.m_layer_count > 16) {
        p.m_layer_count = 16;
    }
    return p;
}

static inline P1_Gen_NoisePerlinPrm p1_gen_noise_perlin_prm_def (u16 w, u16 h) {
    return p1_gen_noise_perlin_prm_from_cfg(p1_map_config_def(), w, h);
}

//================================================================================================================================
//=> - P1_Gen_NoisePerlinRslt -
//================================================================================================================================

struct P1_Gen_NoisePerlinRslt {
    u16 m_w;
    u16 m_h;
    P1_Gen_NoisePerlinPrm m_prm;
    MapArrayOverlay m_ov;
};

bool p1_build_perlin_field_f32 (u32 seed, const P1_Gen_NoisePerlinPrm& prm, f32* combo, u8* gray = nullptr);
void p1_perlin_field_to_gray (const f32* combo, u32 n, u8* out_gray);

//================================================================================================================================
//=> - P1_Gen_NoisePerlin -
//================================================================================================================================
//
//  Multi-layer Perlin field; gray overlay is normalized combo for view and downstream gens.
//
//================================================================================================================================

class P1_Gen_NoisePerlin {
public:
    explicit P1_Gen_NoisePerlin (const P1_RunPrm& prm);

    bool generate ();
    bool generate_from_field (const f32* combo, const P1_Gen_NoisePerlinPrm& prm);
    bool generate_with_combo (u32 seed, const P1_Gen_NoisePerlinPrm& prm, f32* combo);
    bool is_valid () const;
    const P1_Gen_NoisePerlinRslt& result () const;
    void save_output (cstr path) const;

private:
    P1_Gen_NoisePerlin (const P1_Gen_NoisePerlin& other) = delete;
    P1_Gen_NoisePerlin (P1_Gen_NoisePerlin&& other) = delete;

    P1_RunPrm m_prm;
    bool m_valid_generation;
    P1_Gen_NoisePerlinRslt m_rslt;
};

#endif // P1_GEN_NOISE_PERLIN_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
