//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_area_index.h"

#include "generator_whiteboard.h"

#include <algorithm>
#include <vector>

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static const u16 k_wb_excl = 0;
static const u16 k_wb_open = 1;

static void mark_mask_eq (u16* wb, const u8* terrain, u32 n, u8 terr_idx) {
    for (u32 i = 0; i < n; ++i) {
        wb[i] = (terrain[i] == terr_idx) ? k_wb_open : k_wb_excl;
    }
}

static void mark_mask_ge (u16* wb, const u8* terrain, u32 n, u8 terr_idx) {
    for (u32 i = 0; i < n; ++i) {
        wb[i] = (terrain[i] >= terr_idx) ? k_wb_open : k_wb_excl;
    }
}

static void mark_mask_le (u16* wb, const u8* terrain, u32 n, u8 terr_idx) {
    for (u32 i = 0; i < n; ++i) {
        wb[i] = (terrain[i] <= terr_idx) ? k_wb_open : k_wb_excl;
    }
}

static u32 flood_from_seed (
    u16 w,
    u16 h,
    u16* wb,
    u32 seed,
    std::vector<u32>& st) 
{
    st.clear();
    st.push_back(seed);
    u32 cnt = 0;
    const u32 wi = static_cast<u32>(w);
    while (!st.empty()) {
        const u32 i = st.back();
        st.pop_back();
        if (wb[i] != k_wb_open) {
            continue;
        }
        wb[i] = k_wb_excl;
        cnt++;
        const u16 py = static_cast<u16>(i / wi);
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * wi);
        if (px > 0) {
            const u32 j = i - 1u;
            if (wb[j] == k_wb_open) {
                st.push_back(j);
            }
        }
        if (static_cast<u32>(px) + 1u < wi) {
            const u32 j = i + 1u;
            if (wb[j] == k_wb_open) {
                st.push_back(j);
            }
        }
        if (py > 0) {
            const u32 j = i - wi;
            if (wb[j] == k_wb_open) {
                st.push_back(j);
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
            const u32 j = i + wi;
            if (wb[j] == k_wb_open) {
                st.push_back(j);
            }
        }
    }
    return cnt;
}

static AreaIdxResult* collect_areas (u16 w, u16 h, u16* wb) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::vector<AreaIdxEntry> recs;
    recs.reserve(256);
    std::vector<u32> st;
    st.reserve(static_cast<std::size_t>(n / 4u + 64u));
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            if (wb[i] != k_wb_open) {
                continue;
            }
            const u32 area = flood_from_seed(w, h, wb, i, st);
            AreaIdxEntry e = {};
            e.x = px;
            e.y = py;
            e.size = area;
            recs.push_back(e);
        }
    }
    AreaIdxResult* out = new AreaIdxResult();
    if (out == nullptr) {
        return nullptr;
    }
    out->area_n = static_cast<u32>(recs.size());
    if (out->area_n == 0) {
        out->areas = nullptr;
        return out;
    }
    out->areas = new AreaIdxEntry[out->area_n];
    if (out->areas == nullptr) {
        delete out;
        return nullptr;
    }
    for (u32 k = 0; k < out->area_n; ++k) {
        out->areas[k] = recs[k];
    }
    std::sort(out->areas, out->areas + out->area_n, [](const AreaIdxEntry& a, const AreaIdxEntry& b) {
        if (a.size != b.size) {
            return a.size > b.size;
        }
        if (a.y != b.y) {
            return a.y < b.y;
        }
        return a.x < b.x;
    });
    return out;
}

static AreaIdxResult* finish_index (u16 w, u16 h, u16* wb) {
    if (wb == nullptr) {
        return nullptr;
    }
    AreaIdxResult* out = collect_areas(w, h, wb);
    GeneratorWhiteboard::release(wb);
    return out;
}

static u16* begin_index (const u8* terrain, u16 w, u16 h) {
    if (terrain == nullptr || w == 0 || h == 0) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    return GeneratorWhiteboard::alloc(static_cast<i32>(n));
}

//================================================================================================================================
//=> - Generate_AreaIndex -
//================================================================================================================================

AreaIdxResult* Generate_AreaIndex::generate_eq (const u8* terrain, u16 w, u16 h, u8 terr_idx) {
    u16* wb = begin_index(terrain, w, h);
    if (wb == nullptr) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    mark_mask_eq(wb, terrain, n, terr_idx);
    return finish_index(w, h, wb);
}

AreaIdxResult* Generate_AreaIndex::generate_ge (const u8* terrain, u16 w, u16 h, u8 terr_idx) {
    u16* wb = begin_index(terrain, w, h);
    if (wb == nullptr) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    mark_mask_ge(wb, terrain, n, terr_idx);
    return finish_index(w, h, wb);
}

AreaIdxResult* Generate_AreaIndex::generate_le (const u8* terrain, u16 w, u16 h, u8 terr_idx) {
    u16* wb = begin_index(terrain, w, h);
    if (wb == nullptr) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    mark_mask_le(wb, terrain, n, terr_idx);
    return finish_index(w, h, wb);
}

void Generate_AreaIndex::free_result (AreaIdxResult* res) {
    if (res == nullptr) {
        return;
    }
    delete[] res->areas;
    delete res;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
