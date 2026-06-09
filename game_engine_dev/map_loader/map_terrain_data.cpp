//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "map_terrain_data.h"

#include "map_terrain_validate.h"

#include <cstdio>
#include <cstring>

//================================================================================================================================
//=> - MapTerrainData -
//================================================================================================================================

MapTerrainData::MapTerrainData () :
    m_w(0),
    m_h(0),
    m_rows(nullptr) {
}

MapTerrainData::~MapTerrainData () {
    clear();
}

u16 MapTerrainData::width () const {
    return m_w;
}

u16 MapTerrainData::height () const {
    return m_h;
}

const u8* MapTerrainData::data () const {
    return m_rows;
}

void MapTerrainData::clear () {
    delete[] m_rows;
    m_rows = nullptr;
    m_w = 0;
    m_h = 0;
}

bool MapTerrainData::assign_copy (u16 w, u16 h, const u8* src) {
    clear();
    if (src == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (!MapTerrainValidate::chk_classes(src, n)) {
        return false;
    }
    m_rows = new u8[n];
    std::memcpy(m_rows, src, static_cast<size_t>(n));
    m_w = w;
    m_h = h;
    return true;
}

bool MapTerrainData::assign_raw (u16 w, u16 h, const u8* src) {
    clear();
    if (src == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    m_rows = new u8[n];
    std::memcpy(m_rows, src, static_cast<size_t>(n));
    m_w = w;
    m_h = h;
    return true;
}

bool MapTerrainData::save_terrain_ppm (cstr path) const {
    if (path == nullptr || m_rows == nullptr || m_w == 0 || m_h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(m_w) * static_cast<u32>(m_h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        MapTerrainValidate::rgb_from_class(m_rows[i], &r, &g, &b);
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        delete[] rgb;
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)m_w, (unsigned)m_h);
    const size_t nbytes = static_cast<size_t>(n) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
