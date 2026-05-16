//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generator_constants.h"

#include "perlin_noise.h"

#include <cstdio>
#include <cstring>

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static const u8* terr_rgb_from_class (u8 c) {
    static const u8* const k_rows[] = {
        TERR_NONE,
        TERR_OCEAN,
        TERR_SEA,
        TERR_COASTAL,
        TERR_PLAINS,
        TERR_HILLS,
        TERR_MOUNTAINS};
    for (unsigned i = 0; i < sizeof(k_rows) / sizeof(k_rows[0]); ++i) {
        if (k_rows[i][0] == c) {
            return k_rows[i] + 1;
        }
    }
    return TERR_NONE + 1;
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
//=> - MapArrayClimate -
//================================================================================================================================

MapArrayClimate::MapArrayClimate () :
    m_w(0),
    m_h(0),
    m_d(nullptr) {
}

MapArrayClimate::~MapArrayClimate () {
    delete[] m_d;
}

u16 MapArrayClimate::width () const {
    return m_w;
}

u16 MapArrayClimate::height () const {
    return m_h;
}

const u8* MapArrayClimate::data () const {
    return m_d;
}

u8* MapArrayClimate::data_w () {
    return m_d;
}

bool MapArrayClimate::save (cstr path) const {
    (void)path;
    return false;
}

//================================================================================================================================
//=> - MapArrayTerrain -
//================================================================================================================================

MapArrayTerrain::MapArrayTerrain () :
    m_w(0),
    m_h(0),
    m_d(nullptr) {
}

MapArrayTerrain::~MapArrayTerrain () {
    clear();
}

u16 MapArrayTerrain::width () const {
    return m_w;
}

u16 MapArrayTerrain::height () const {
    return m_h;
}

const u8* MapArrayTerrain::data () const {
    return m_d;
}

u8* MapArrayTerrain::data_w () {
    return m_d;
}

void MapArrayTerrain::clear () {
    delete[] m_d;
    m_d = nullptr;
    m_w = 0;
    m_h = 0;
}

bool MapArrayTerrain::assign_copy (u16 w, u16 h, const u8* src) {
    clear();
    if (src == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    m_d = new u8[n];
    std::memcpy(m_d, src, static_cast<size_t>(n));
    m_w = w;
    m_h = h;
    return true;
}

bool MapArrayTerrain::save (cstr path) const {
    if (path == nullptr || m_d == nullptr || m_w == 0 || m_h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(m_w) * static_cast<u32>(m_h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    for (u32 i = 0; i < n; ++i) {
        const u8* px = terr_rgb_from_class(m_d[i]);
        std::memcpy(rgb + i * 3u, px, 3u);
    }
    const bool ok = save_rgb_ppm(path, rgb, m_w, m_h);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - MapArrayOverlay -
//================================================================================================================================

MapArrayOverlay::MapArrayOverlay () :
    m_w(0),
    m_h(0),
    m_d(nullptr) {
}

MapArrayOverlay::~MapArrayOverlay () {
    clear();
}

u16 MapArrayOverlay::width () const {
    return m_w;
}

u16 MapArrayOverlay::height () const {
    return m_h;
}

const u8* MapArrayOverlay::data () const {
    return m_d;
}

u8* MapArrayOverlay::data_w () {
    return m_d;
}

void MapArrayOverlay::clear () {
    delete[] m_d;
    m_d = nullptr;
    m_w = 0;
    m_h = 0;
}

bool MapArrayOverlay::resize (u16 w, u16 h) {
    clear();
    if (w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    m_d = new u8[n];
    m_w = w;
    m_h = h;
    return true;
}

bool MapArrayOverlay::assign_copy (u16 w, u16 h, const u8* src) {
    if (!resize(w, h) || src == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memcpy(m_d, src, static_cast<size_t>(n));
    return true;
}

bool MapArrayOverlay::save (cstr path) const {
    if (path == nullptr || m_d == nullptr || m_w == 0 || m_h == 0) {
        return false;
    }
    return save_perlin_gray_pgm(path, m_d, m_w, m_h);
}

//================================================================================================================================
//=> - MapArrayDistance -
//================================================================================================================================

static const u16 MAP_DIST_U16_INF = 0xFFFFu;

MapArrayDistance::MapArrayDistance () :
    m_w(0),
    m_h(0),
    m_d(nullptr) {
}

MapArrayDistance::~MapArrayDistance () {
    clear();
}

u16 MapArrayDistance::width () const {
    return m_w;
}

u16 MapArrayDistance::height () const {
    return m_h;
}

const u16* MapArrayDistance::data () const {
    return m_d;
}

u16* MapArrayDistance::data_w () {
    return m_d;
}

void MapArrayDistance::clear () {
    delete[] m_d;
    m_d = nullptr;
    m_w = 0;
    m_h = 0;
}

bool MapArrayDistance::resize (u16 w, u16 h) {
    clear();
    if (w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    m_d = new u16[n];
    for (u32 i = 0; i < n; ++i) {
        m_d[i] = MAP_DIST_U16_INF;
    }
    m_w = w;
    m_h = h;
    return true;
}

bool MapArrayDistance::assign_copy (u16 w, u16 h, const u16* src) {
    if (!resize(w, h) || src == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memcpy(m_d, src, static_cast<size_t>(n) * sizeof(u16));
    return true;
}

bool MapArrayDistance::save (cstr path) const {
    if (path == nullptr || m_d == nullptr || m_w == 0 || m_h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(m_w) * static_cast<u32>(m_h);
    u32 max_vis = 0;
    for (u32 i = 0; i < n; ++i) {
        const u16 d = m_d[i];
        if (d == MAP_DIST_U16_INF) {
            continue;
        }
        const u32 dv = static_cast<u32>(d > 255u ? 255u : static_cast<u32>(d));
        if (dv > max_vis) {
            max_vis = dv;
        }
    }
    if (max_vis == 0u) {
        max_vis = 1u;
    }
    u8* gray = new u8[n];
    for (u32 i = 0; i < n; ++i) {
        const u16 d = m_d[i];
        if (d == MAP_DIST_U16_INF) {
            gray[i] = 0;
        } else {
            const u32 dv = static_cast<u32>(d > 255u ? 255u : static_cast<u32>(d));
            gray[i] = static_cast<u8>((dv * 255u) / max_vis);
        }
    }
    const bool ok = save_perlin_gray_pgm(path, gray, m_w, m_h);
    delete[] gray;
    return ok;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
