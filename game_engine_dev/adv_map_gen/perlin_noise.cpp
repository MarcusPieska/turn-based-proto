//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>
#include <cstdio>
#include <new>

#include "perlin_noise.h"
#include "p1_step_log.h"

//================================================================================================================================
//=> - Internal helpers -
//================================================================================================================================

static inline f32 fade (f32 t) {
    return t * t * t * (t * (t * 6.f - 15.f) + 10.f);
}

static inline f32 lerp (f32 a, f32 b, f32 t) {
    return a + t * (b - a);
}

static inline i32 fast_floor_i32 (f32 x) {
    const i32 i = static_cast<i32>(x);
    return i - static_cast<i32>(x < static_cast<f32>(i));
}

static const f32 GRAD2[16][2] = {
    {1.f, 1.f}, {-1.f, 1.f}, {1.f, -1.f}, {-1.f, -1.f},
    {1.f, 0.f}, {-1.f, 0.f}, {1.f, 0.f}, {-1.f, 0.f},
    {0.f, 1.f}, {0.f, -1.f}, {0.f, 1.f}, {0.f, -1.f},
    {1.f, 1.f}, {0.f, -1.f}, {-1.f, 1.f}, {0.f, -1.f}
};

static inline f32 grad2_tbl (i32 hash, f32 x, f32 y) {
    const f32* g = GRAD2[hash & 15];
    return g[0] * x + g[1] * y;
}

static const i32 OCTAVES = 9;
static const f32 PERSISTENCE = 0.5f;
static const f32 SAMPLE_SCALE = 8.f;

struct FbmOctaveSetup {
    f32 m_dcx[OCTAVES];
    f32 m_dcy[OCTAVES];
    f32 m_amp[OCTAVES];
};

static void fbm_octave_init (FbmOctaveSetup* s, const PerlinImgParams& p) {
    const f32 invw = 1.f / static_cast<f32>(p.m_w);
    const f32 invh = 1.f / static_cast<f32>(p.m_h);
    const f32 k = p.m_frequency * SAMPLE_SCALE;
    f32 lacp = 1.f;
    f32 a = 1.f;
    for (i32 o = 0; o < OCTAVES; ++o) {
        s->m_dcx[o] = invw * k * lacp;
        s->m_dcy[o] = invh * k * lacp;
        s->m_amp[o] = a;
        lacp *= p.m_lacunarity;
        a *= PERSISTENCE;
    }
}

static inline f32 fbm_sum_oct9 (const PerlinNoise& gen, const f32* cx, const f32* cy, const FbmOctaveSetup& setup) {
    return gen.noise2(cx[0], cy[0]) * setup.m_amp[0]
        + gen.noise2(cx[1], cy[1]) * setup.m_amp[1]
        + gen.noise2(cx[2], cy[2]) * setup.m_amp[2]
        + gen.noise2(cx[3], cy[3]) * setup.m_amp[3]
        + gen.noise2(cx[4], cy[4]) * setup.m_amp[4]
        + gen.noise2(cx[5], cy[5]) * setup.m_amp[5]
        + gen.noise2(cx[6], cy[6]) * setup.m_amp[6]
        + gen.noise2(cx[7], cy[7]) * setup.m_amp[7]
        + gen.noise2(cx[8], cy[8]) * setup.m_amp[8];
}

static inline f32 fbm_sum_octaves (PerlinNoise& gen, const f32* cx, const f32* cy, const FbmOctaveSetup& setup) {
    return fbm_sum_oct9(gen, cx, cy, setup);
}

static void perlin_gray_fill (const f32* src, u32 n, f32 vmin, f32 vmax, u8* out_gray) {
    f32 denom = vmax - vmin;
    if (denom < 1e-12f) {
        denom = 1.f;
    }
    for (u32 i = 0; i < n; ++i) {
        f32 t = (src[i] - vmin) / denom;
        if (t < 0.f) {
            t = 0.f;
        }
        if (t > 1.f) {
            t = 1.f;
        }
        out_gray[i] = static_cast<u8>(std::lrint(t * 255.f));
    }
}

//================================================================================================================================
//=> - PerlinImgParams -
//================================================================================================================================

PerlinImgParams::PerlinImgParams () :
    m_w(1000),
    m_h(1000),
    m_frequency(5.f),
    m_lacunarity(2.f) {
}

//================================================================================================================================
//=> - PerlinNoise -
//================================================================================================================================

PerlinNoise::PerlinNoise (u32 seed) {
    for (u16 i = 0; i < 256; ++i) {
        m_perm[i] = static_cast<u8>(i);
    }
    for (u16 i = 255; i > 0; --i) {
        seed = seed * 1664525u + 1013904223u;
        u16 j = static_cast<u16>(seed % static_cast<u32>(i + 1));
        u8 tmp = m_perm[i];
        m_perm[i] = m_perm[j];
        m_perm[j] = tmp;
    }
    for (u16 i = 0; i < 256; ++i) {
        m_perm[i + 256] = m_perm[i];
    }
}

f32 PerlinNoise::noise2 (f32 x, f32 y) const {
    const i32 xi0 = fast_floor_i32(x);
    const i32 yi0 = fast_floor_i32(y);
    const i32 xi = xi0 & 255;
    const i32 yi = yi0 & 255;
    const f32 xf = x - static_cast<f32>(xi0);
    const f32 yf = y - static_cast<f32>(yi0);
    const f32 u = fade(xf);
    const f32 v = fade(yf);
    const i32 aa = m_perm[xi + m_perm[yi]];
    const i32 ba = m_perm[xi + 1 + m_perm[yi]];
    const i32 ab = m_perm[xi + m_perm[yi + 1]];
    const i32 bb = m_perm[xi + 1 + m_perm[yi + 1]];
    const f32 x1 = lerp(grad2_tbl(aa, xf, yf), grad2_tbl(ba, xf - 1.f, yf), u);
    const f32 x2 = lerp(grad2_tbl(ab, xf, yf - 1.f), grad2_tbl(bb, xf - 1.f, yf - 1.f), u);
    return lerp(x1, x2, v);
}

//================================================================================================================================
//=> - Render / IO -
//================================================================================================================================

bool render_perlin_gray_u8 (u8* out_row_major, const PerlinImgParams& params, u32 seed) {
    if (out_row_major == nullptr || params.m_w == 0 || params.m_h == 0) {
        return false;
    }
    FbmOctaveSetup setup;
    fbm_octave_init(&setup, params);
    PerlinNoise gen(seed);
    const u32 n = static_cast<u32>(params.m_w) * static_cast<u32>(params.m_h);
    f32* acc = new f32[n];
    f32 vmin = 0.f;
    f32 vmax = 0.f;
    bool first = true;
    f32 cx[OCTAVES];
    f32 cy[OCTAVES];
    for (u16 py = 0; py < params.m_h; ++py) {
        const f32 pyf = static_cast<f32>(py) + 0.5f;
        for (i32 o = 0; o < OCTAVES; ++o) {
            cy[o] = pyf * setup.m_dcy[o];
        }
        for (i32 o = 0; o < OCTAVES; ++o) {
            cx[o] = 0.5f * setup.m_dcx[o];
        }
        for (u16 px = 0; px < params.m_w; ++px) {
            const f32 sum = fbm_sum_octaves(gen, cx, cy, setup);
            const u32 idx = static_cast<u32>(py) * static_cast<u32>(params.m_w) + static_cast<u32>(px);
            acc[idx] = sum;
            if (first) {
                vmin = vmax = sum;
                first = false;
            } else {
                if (sum < vmin) {
                    vmin = sum;
                }
                if (sum > vmax) {
                    vmax = sum;
                }
            }
            for (i32 o = 0; o < OCTAVES; ++o) {
                cx[o] += setup.m_dcx[o];
            }
        }
    }
    f32 denom = vmax - vmin;
    if (denom < 1e-12f) {
        denom = 1.f;
    }
    for (u32 i = 0; i < n; ++i) {
        f32 t = (acc[i] - vmin) / denom;
        if (t < 0.f) {
            t = 0.f;
        }
        if (t > 1.f) {
            t = 1.f;
        }
        out_row_major[i] = static_cast<u8>(std::lrint(static_cast<f64>(t) * 255.0));
    }
    delete[] acc;
    return true;
}

bool render_perlin_field_f32 (f32* out_row_major, const PerlinImgParams& params, u32 seed) {
    if (out_row_major == nullptr || params.m_w == 0 || params.m_h == 0) {
        return false;
    }
    FbmOctaveSetup setup;
    fbm_octave_init(&setup, params);
    PerlinNoise gen(seed);
    f32 cx[OCTAVES];
    f32 cy[OCTAVES];
    for (u16 py = 0; py < params.m_h; ++py) {
        const f32 pyf = static_cast<f32>(py) + 0.5f;
        for (i32 o = 0; o < OCTAVES; ++o) {
            cy[o] = pyf * setup.m_dcy[o];
        }
        for (i32 o = 0; o < OCTAVES; ++o) {
            cx[o] = 0.5f * setup.m_dcx[o];
        }
        for (u16 px = 0; px < params.m_w; ++px) {
            const u32 idx = static_cast<u32>(py) * static_cast<u32>(params.m_w) + static_cast<u32>(px);
            out_row_major[idx] = fbm_sum_octaves(gen, cx, cy, setup);
            for (i32 o = 0; o < OCTAVES; ++o) {
                cx[o] += setup.m_dcx[o];
            }
        }
    }
    return true;
}

bool accumulate_perlin_field_f32 (f32* acc_row_major, f32 weight, const PerlinImgParams& params, u32 seed) {
    if (acc_row_major == nullptr || params.m_w == 0 || params.m_h == 0) {
        return false;
    }
    FbmOctaveSetup setup;
    fbm_octave_init(&setup, params);
    PerlinNoise gen(seed);
    f32 cx[OCTAVES];
    f32 cy[OCTAVES];
    for (u16 py = 0; py < params.m_h; ++py) {
        const f32 pyf = static_cast<f32>(py) + 0.5f;
        for (i32 o = 0; o < OCTAVES; ++o) {
            cy[o] = pyf * setup.m_dcy[o];
        }
        for (i32 o = 0; o < OCTAVES; ++o) {
            cx[o] = 0.5f * setup.m_dcx[o];
        }
        for (u16 px = 0; px < params.m_w; ++px) {
            const u32 idx = static_cast<u32>(py) * static_cast<u32>(params.m_w) + static_cast<u32>(px);
            acc_row_major[idx] += weight * fbm_sum_octaves(gen, cx, cy, setup);
            for (i32 o = 0; o < OCTAVES; ++o) {
                cx[o] += setup.m_dcx[o];
            }
        }
    }
    return true;
}

static const i32 k_fused_layer_max = 16;

bool render_perlin_layers_f32 (
    f32* out_row_major,
    u8* out_gray_row_major,
    u16 w,
    u16 h,
    f32 lacunarity,
    const PerlinLayerSpec* layers,
    i32 layer_n) 
{
    if (out_row_major == nullptr || layers == nullptr || w == 0 || h == 0 || layer_n <= 0) {
        P1_STEP_ABORT("P1_Gen_NoisePerlin", "render_perlin invalid args");
    }
    if (layer_n > k_fused_layer_max) {
        P1_STEP_ABORT("P1_Gen_NoisePerlin", "render_perlin layer_n out of range");
    }
    const i32 lc = layer_n;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    alignas(PerlinNoise) u8 gen_buf[k_fused_layer_max][sizeof(PerlinNoise)];
    PerlinNoise* gen[k_fused_layer_max];
    FbmOctaveSetup setup[k_fused_layer_max];
    f32 wt[k_fused_layer_max];
    for (i32 k = 0; k < lc; ++k) {
        gen[k] = new (&gen_buf[k]) PerlinNoise(layers[k].m_seed);
        PerlinImgParams lp;
        lp.m_w = w;
        lp.m_h = h;
        lp.m_frequency = layers[k].m_frequency;
        lp.m_lacunarity = lacunarity;
        fbm_octave_init(&setup[k], lp);
        wt[k] = layers[k].m_weight;
    }
    f32 cx[k_fused_layer_max][OCTAVES];
    f32 cy[k_fused_layer_max][OCTAVES];
    f32 vmin = 0.f;
    f32 vmax = 0.f;
    bool have_rng = false;
    for (u16 py = 0; py < h; ++py) {
        const f32 pyf = static_cast<f32>(py) + 0.5f;
        for (i32 k = 0; k < lc; ++k) {
            for (i32 o = 0; o < OCTAVES; ++o) {
                cy[k][o] = pyf * setup[k].m_dcy[o];
                cx[k][o] = 0.5f * setup[k].m_dcx[o];
            }
        }
        for (u16 px = 0; px < w; ++px) {
            const u32 idx = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            f32 sum = 0.f;
            for (i32 k = 0; k < lc; ++k) {
                sum += wt[k] * fbm_sum_oct9(*gen[k], cx[k], cy[k], setup[k]);
                for (i32 o = 0; o < OCTAVES; ++o) {
                    cx[k][o] += setup[k].m_dcx[o];
                }
            }
            out_row_major[idx] = sum;
            if (out_gray_row_major != nullptr) {
                if (!have_rng) {
                    vmin = vmax = sum;
                    have_rng = true;
                } else {
                    if (sum < vmin) {
                        vmin = sum;
                    }
                    if (sum > vmax) {
                        vmax = sum;
                    }
                }
            }
        }
    }
    if (out_gray_row_major != nullptr) {
        perlin_gray_fill(out_row_major, n, vmin, vmax, out_gray_row_major);
    }
    for (i32 k = 0; k < lc; ++k) {
        gen[k]->~PerlinNoise();
    }
    return true;
}

bool save_perlin_gray_pgm (cstr path, const u8* pix, u16 w, u16 h) {
    if (path == nullptr || pix == nullptr) {
        return false;
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P5\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    std::fwrite(pix, 1, static_cast<size_t>(w) * static_cast<size_t>(h), fp);
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
