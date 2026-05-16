//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "cont_indexer.h"

#include <algorithm>
#include <cstdio>

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static void cont_index_fill_color (u32 rid, u8* rgb) {
    rgb[0] = static_cast<u8>((rid * 47u + 13u) & 255u);
    rgb[1] = static_cast<u8>((rid * 17u + 7u) & 255u);
    rgb[2] = static_cast<u8>((rid * 31u + 23u) & 255u);
}

static u32 flood_land_from_seed (
    u32 seed,
    u16 wi,
    u16 hi,
    std::vector<i32>& comp,
    u32 rid,
    u8* out_rgb,
    const u8 fill_rgb[3]) {
    std::vector<u32> st;
    st.push_back(seed);
    u32 cnt = 0;
    while (!st.empty()) {
        const u32 i = st.back();
        st.pop_back();
        if (comp[i] != -2) {
            continue;
        }
        comp[i] = static_cast<i32>(rid);
        cnt++;
        u8* op = out_rgb + i * 3u;
        op[0] = fill_rgb[0];
        op[1] = fill_rgb[1];
        op[2] = fill_rgb[2];
        const u32 wi32 = static_cast<u32>(wi);
        const u16 py = static_cast<u16>(i / wi32);
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * wi32);
        if (px > 0) {
            const u32 j = i - 1u;
            if (comp[j] == -2) {
                st.push_back(j);
            }
        }
        if (static_cast<u32>(px) + 1u < static_cast<u32>(wi)) {
            const u32 j = i + 1u;
            if (comp[j] == -2) {
                st.push_back(j);
            }
        }
        if (py > 0) {
            const u32 j = i - wi32;
            if (comp[j] == -2) {
                st.push_back(j);
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(hi)) {
            const u32 j = i + wi32;
            if (comp[j] == -2) {
                st.push_back(j);
            }
        }
    }
    return cnt;
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 wi, u16 hi) {
    if (path == nullptr || rgb == nullptr) {
        return false;
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)wi, (unsigned)hi);
    const size_t nbytes = static_cast<size_t>(wi) * static_cast<size_t>(hi) * 3u;
    std::fwrite(rgb, 1, nbytes, fp);
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - ContIndexer -
//================================================================================================================================

ContIndexer::ContIndexer (const WaterLandOverlay& overlay) :
    m_ok(false),
    m_w(0),
    m_h(0),
    m_regions(),
    m_rgb() {
    if (!overlay.is_valid()) {
        return;
    }
    const u16 w = overlay.width();
    const u16 h = overlay.height();
    const u8* wl = overlay.water_land_gray();
    if (wl == nullptr || w == 0 || h == 0) {
        return;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::vector<i32> comp(static_cast<size_t>(n));
    for (u32 i = 0; i < n; ++i) {
        const u8 g = wl[i];
        comp[i] = (g == WL_OVERLAY_LAND_GRAY) ? -2 : -1;
    }
    m_rgb.assign(static_cast<size_t>(n) * 3u, 0);
    m_regions.clear();
    u32 rid = 0;
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            if (comp[i] != -2) {
                continue;
            }
            u8 fill[3];
            cont_index_fill_color(rid, fill);
            const u32 area = flood_land_from_seed(i, w, h, comp, rid, m_rgb.data(), fill);
            ContRegionRec rec;
            rec.m_sx = px;
            rec.m_sy = py;
            rec.m_area_px = area;
            rec.m_r = fill[0];
            rec.m_g = fill[1];
            rec.m_b = fill[2];
            m_regions.push_back(rec);
            rid++;
        }
    }
    m_w = w;
    m_h = h;
    m_ok = true;
}

bool ContIndexer::is_valid () const {
    return m_ok;
}

u16 ContIndexer::width () const {
    return m_w;
}

u16 ContIndexer::height () const {
    return m_h;
}

u32 ContIndexer::region_count () const {
    return static_cast<u32>(m_regions.size());
}

const std::vector<ContRegionRec>& ContIndexer::regions () const {
    return m_regions;
}

const u8* ContIndexer::count_map_rgb () const {
    return m_rgb.empty() ? nullptr : m_rgb.data();
}

bool ContIndexer::save_count_map_ppm (cstr path) const {
    if (!m_ok || path == nullptr) {
        return false;
    }
    return save_rgb_ppm(path, m_rgb.data(), m_w, m_h);
}

void ContIndexer::print_regions_by_area_desc () const {
    if (!m_ok) {
        return;
    }
    std::vector<ContRegionRec> sorted = m_regions;
    std::sort(sorted.begin(), sorted.end(), [] (const ContRegionRec& a, const ContRegionRec& b) {
        return a.m_area_px > b.m_area_px;
    });
    for (size_t k = 0; k < sorted.size(); ++k) {
        const ContRegionRec& r = sorted[k];
        std::printf(
            "%zu  area=%u  start=(%u,%u)  rgb=(%u,%u,%u)\n",
            k + 1u,
            static_cast<unsigned>(r.m_area_px),
            static_cast<unsigned>(r.m_sx),
            static_cast<unsigned>(r.m_sy),
            static_cast<unsigned>(r.m_r),
            static_cast<unsigned>(r.m_g),
            static_cast<unsigned>(r.m_b));
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
