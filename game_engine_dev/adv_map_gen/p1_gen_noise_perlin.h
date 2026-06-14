//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_NOISE_PERLIN_H
#define P1_GEN_NOISE_PERLIN_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Gen_NoisePerlinPrm -
//================================================================================================================================

struct P1_Gen_NoisePerlinPrm {
    u16 m_w;
    u16 m_h;
    f32 m_lacunarity;
    f32 m_layer_freq_base;
    f32 m_layer_weight;
    f32 m_layer_freq_step;
    i32 m_layer_count;
};

//================================================================================================================================
//=> - P1_Gen_NoisePerlinRslt -
//================================================================================================================================

struct P1_Gen_NoisePerlinRslt {
    u16 m_w;
    u16 m_h;
    P1_Gen_NoisePerlinPrm m_prm;
    MapArrayOverlay m_ov;
};

//================================================================================================================================
//=> - P1_Gen_NoisePerlin -
//================================================================================================================================

class P1_Gen_NoisePerlin {
public:
    explicit P1_Gen_NoisePerlin (const P1_RunPrm& prm);

    bool generate ();
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
