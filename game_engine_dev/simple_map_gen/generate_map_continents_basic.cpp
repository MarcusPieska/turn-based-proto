//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <algorithm>
#include <cstddef>
#include <limits>
#include <random>
#include <vector>

#include "generate_map_continents_basic.h"
#include "generate_terrain_combo_add.h"
#include "generate_terrain_cont_pn.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static u32 mix_seed (u32 base, u32 tag) {
    return base ^ (tag * 2654435769u);
}

static i32 layer_count_for_patch_size (u16 S) {
    i32 n = static_cast<i32>(S) / 80 + 1;
    if (n < 1) {
        n = 1;
    }
    if (n > 5) {
        n = 5;
    }
    return n;
}

static u64 rect_intersection_area (u16 ax, u16 ay, u16 aw, u16 ah, u16 bx, u16 by, u16 bw, u16 bh) {
    const i32 x0 = std::max<i32>(ax, bx);
    const i32 x1 = std::min<i32>(static_cast<i32>(ax) + static_cast<i32>(aw), static_cast<i32>(bx) + static_cast<i32>(bw));
    const i32 y0 = std::max<i32>(ay, by);
    const i32 y1 = std::min<i32>(static_cast<i32>(ay) + static_cast<i32>(ah), static_cast<i32>(by) + static_cast<i32>(bh));
    if (x0 >= x1 || y0 >= y1) {
        return 0;
    }
    return static_cast<u64>(x1 - x0) * static_cast<u64>(y1 - y0);
}

static u64 overlap_with_prior_patches (
    u16 cand_ax,
    u16 cand_ay,
    u16 cand_s,
    const std::vector<std::pair<u16, u16>>& anchors,
    const std::vector<u16>& sizes,
    size_t prior_count) {
    u64 sum = 0;
    for (size_t j = 0; j < prior_count; ++j) {
        const u16 sj = sizes[j];
        sum += rect_intersection_area(cand_ax, cand_ay, cand_s, cand_s, anchors[j].first, anchors[j].second, sj, sj);
    }
    return sum;
}

//================================================================================================================================
//=> - MapContinentsBasic -
//================================================================================================================================

Generate_MapContinentsBasic::Generate_MapContinentsBasic (const MapContinentsBasicParams& params) :
    m_valid_generation(false),
    m_params(params),
    m_canvas(nullptr),
    m_patches(),
    m_sizes(),
    m_anchors() {
    const u16 cw = m_params.m_canvas_w;
    const u16 ch = m_params.m_canvas_h;
    if (cw == 0 || ch == 0) {
        return;
    }
    if (m_params.m_continent_count == 0 && m_params.m_large_continent_count == 0) {
        return;
    }
    if (m_params.m_min_cont_size_perc == 0 || m_params.m_min_cont_size_perc > m_params.m_max_cont_size_perc) {
        return;
    }
    if (m_params.m_large_continent_count > 0 && m_params.m_large_continent_size_perc == 0) {
        return;
    }
    const u32 dim_ref = cw < ch ? static_cast<u32>(cw) : static_cast<u32>(ch);
    const u32 lo32 = dim_ref * static_cast<u32>(m_params.m_min_cont_size_perc) / 100u;
    const u32 hi32 = dim_ref * static_cast<u32>(m_params.m_max_cont_size_perc) / 100u;
    if (lo32 == 0 || lo32 > hi32 || hi32 > 65535u) {
        return;
    }
    const u16 lo = static_cast<u16>(lo32);
    const u16 hi = static_cast<u16>(hi32);
    const u32 large32 = dim_ref * static_cast<u32>(m_params.m_large_continent_size_perc) / 100u;
    const u16 large_s = static_cast<u16>(large32 > 65535u ? 65535u : large32);
    if (m_params.m_large_continent_count > 0 && large_s == 0) {
        return;
    }
    std::mt19937 rng(m_params.m_seed);
    std::uniform_int_distribution<int> dist_size(static_cast<int>(lo), static_cast<int>(hi));
    std::vector<u16> pool;
    pool.resize(static_cast<size_t>(m_params.m_continent_count));
    for (size_t k = 0; k < pool.size(); ++k) {
        pool[k] = static_cast<u16>(dist_size(rng));
    }
    std::sort(pool.begin(), pool.end(), std::greater<u16>());
    m_sizes.reserve(static_cast<size_t>(m_params.m_large_continent_count) + pool.size());
    for (u16 li = 0; li < m_params.m_large_continent_count; ++li) {
        m_sizes.push_back(large_s);
    }
    m_sizes.insert(m_sizes.end(), pool.begin(), pool.end());
    m_anchors.resize(m_sizes.size());
    m_patches.reserve(m_sizes.size());
    for (size_t li = 0; li < m_sizes.size(); ++li) {
        const u16 S = m_sizes[li];
        if (S == 0 || static_cast<u32>(S) > static_cast<u32>(cw) || static_cast<u32>(S) > static_cast<u32>(ch)) {
            for (MapArrayTerrain* p : m_patches) {
                delete p;
            }
            m_patches.clear();
            m_sizes.clear();
            m_anchors.clear();
            return;
        }
        std::uniform_int_distribution<int> ax_dist(0, static_cast<int>(cw) - static_cast<int>(S));
        std::uniform_int_distribution<int> ay_dist(0, static_cast<int>(ch) - static_cast<int>(S));
        u16 ax = 0;
        u16 ay = 0;
        const size_t large_n = static_cast<size_t>(m_params.m_large_continent_count);
        const bool is_large = li < large_n;
        if (is_large && li > 0) {
            u64 best_ov = std::numeric_limits<u64>::max();
            for (int trial = 0; trial < 10; ++trial) {
                const u16 tax = static_cast<u16>(ax_dist(rng));
                const u16 tay = static_cast<u16>(ay_dist(rng));
                const u64 ov = overlap_with_prior_patches(tax, tay, S, m_anchors, m_sizes, li);
                if (ov < best_ov) {
                    best_ov = ov;
                    ax = tax;
                    ay = tay;
                }
            }
        } else {
            ax = static_cast<u16>(ax_dist(rng));
            ay = static_cast<u16>(ay_dist(rng));
        }
        m_anchors[li] = std::pair<u16, u16>(ax, ay);
        const bool use_two_layer = static_cast<u32>(S) * 100u >= dim_ref * static_cast<u32>(m_params.m_patch_combo_min_size_perc);
        MapArrayTerrain* patch = nullptr;
        if (use_two_layer) {
            TerrainContPnParams p0;
            p0.m_seed = mix_seed(m_params.m_seed, static_cast<u32>(li * 2u));
            p0.m_width = S;
            p0.m_height = S;
            p0.m_layer_count = layer_count_for_patch_size(S);
            TerrainContPnParams p1 = p0;
            p1.m_seed = mix_seed(m_params.m_seed, static_cast<u32>(li * 2u + 1u));
            p1.m_inner_grad_limit = 0.00f;
            printf("1 p0.m_layer_count = %d (S=%u)\n", p0.m_layer_count, S);
            printf("1 p1.m_layer_count = %d (S=%u)\n", p1.m_layer_count, S);
            Generate_TerrainContPn ta(p0);
            Generate_TerrainContPn tb(p1);
            if (!ta.is_valid() || !tb.is_valid()) {
                for (MapArrayTerrain* p : m_patches) {
                    delete p;
                }
                m_patches.clear();
                m_sizes.clear();
                m_anchors.clear();
                return;
            }
            Generate_TerrainComboAdd comb;
            if (!comb.generate(ta.terrain(), tb.terrain(), 0, 0)) {
                for (MapArrayTerrain* p : m_patches) {
                    delete p;
                }
                m_patches.clear();
                m_sizes.clear();
                m_anchors.clear();
                return;
            }
            patch = comb.take_terrain();
        } else {
            TerrainContPnParams ps;
            ps.m_seed = mix_seed(m_params.m_seed, static_cast<u32>(li));
            ps.m_width = S;
            ps.m_height = S;
            ps.m_layer_count = layer_count_for_patch_size(S);
            printf("2 ps.m_layer_count = %d (S=%u)\n", ps.m_layer_count, S);
            ps.m_inner_grad_limit = 0.00f;
            Generate_TerrainContPn t(ps);
            if (!t.is_valid()) {
                for (MapArrayTerrain* p : m_patches) {
                    delete p;
                }
                m_patches.clear();
                m_sizes.clear();
                m_anchors.clear();
                return;
            }
            const MapArrayTerrain& tr = t.terrain();
            patch = new MapArrayTerrain();
            if (!patch->assign_copy(tr.width(), tr.height(), tr.data())) {
                delete patch;
                for (MapArrayTerrain* p : m_patches) {
                    delete p;
                }
                m_patches.clear();
                m_sizes.clear();
                m_anchors.clear();
                return;
            }
        }
        if (patch == nullptr) {
            for (MapArrayTerrain* p : m_patches) {
                delete p;
            }
            m_patches.clear();
            m_sizes.clear();
            m_anchors.clear();
            return;
        }
        m_patches.push_back(patch);
    }
    const u32 cn = static_cast<u32>(cw) * static_cast<u32>(ch);
    std::vector<u8> zeros(static_cast<size_t>(cn), TERR_NONE[0]);
    m_canvas = new MapArrayTerrain();
    if (!m_canvas->assign_copy(cw, ch, zeros.data())) {
        delete m_canvas;
        m_canvas = nullptr;
        for (MapArrayTerrain* p : m_patches) {
            delete p;
        }
        m_patches.clear();
        m_sizes.clear();
        m_anchors.clear();
        return;
    }
    for (size_t li = 0; li < m_patches.size(); ++li) {
        const u16 ax = m_anchors[li].first;
        const u16 ay = m_anchors[li].second;
        Generate_TerrainComboAdd layer;
        if (!layer.generate(*m_canvas, *m_patches[li], static_cast<i32>(ax), static_cast<i32>(ay))) {
            delete m_canvas;
            m_canvas = nullptr;
            for (MapArrayTerrain* p : m_patches) {
                delete p;
            }
            m_patches.clear();
            m_sizes.clear();
            m_anchors.clear();
            return;
        }
        MapArrayTerrain* next = layer.take_terrain();
        delete m_canvas;
        m_canvas = next;
    }
    {
        u8* cd = m_canvas->data_w();
        const u32 npx = static_cast<u32>(cw) * static_cast<u32>(ch);
        for (u32 i = 0; i < npx; ++i) {
            if (cd[i] == TERR_NONE[0]) {
                cd[i] = TERR_OCEAN[0];
            }
        }
    }
    m_valid_generation = true;
}

Generate_MapContinentsBasic::~Generate_MapContinentsBasic () {
    delete m_canvas;
    m_canvas = nullptr;
    for (MapArrayTerrain* p : m_patches) {
        delete p;
    }
    m_patches.clear();
}

bool Generate_MapContinentsBasic::is_valid () const {
    return m_valid_generation;
}

bool Generate_MapContinentsBasic::save_output (cstr path) const {
    if (path == nullptr || m_canvas == nullptr || !m_valid_generation) {
        return false;
    }
    return m_canvas->save(path);
}

u16 Generate_MapContinentsBasic::canvas_width () const {
    return m_canvas != nullptr ? m_canvas->width() : 0;
}

u16 Generate_MapContinentsBasic::canvas_height () const {
    return m_canvas != nullptr ? m_canvas->height() : 0;
}

const MapArrayTerrain& Generate_MapContinentsBasic::canvas () const {
    static MapArrayTerrain s_empty;
    if (m_canvas == nullptr) {
        return s_empty;
    }
    return *m_canvas;
}

size_t Generate_MapContinentsBasic::layer_count () const {
    return m_patches.size();
}

u16 Generate_MapContinentsBasic::layer_size (size_t i) const {
    return i < m_sizes.size() ? m_sizes[i] : 0;
}

void Generate_MapContinentsBasic::layer_anchor (size_t i, u16& out_ax, u16& out_ay) const {
    if (i < m_anchors.size()) {
        out_ax = m_anchors[i].first;
        out_ay = m_anchors[i].second;
    } else {
        out_ax = 0;
        out_ay = 0;
    }
}

const MapArrayTerrain& Generate_MapContinentsBasic::layer_patch (size_t i) const {
    static MapArrayTerrain s_empty;
    if (i >= m_patches.size() || m_patches[i] == nullptr) {
        return s_empty;
    }
    return *m_patches[i];
}

const MapContinentsBasicParams& Generate_MapContinentsBasic::params () const {
    return m_params;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
