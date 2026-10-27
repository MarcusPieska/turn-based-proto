//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "dia_save_map.h"

#include <cstdio>
#include <cstring>

//================================================================================================================================
//=> - Class: DiaSaveMap -
//================================================================================================================================

DiaSaveMap::DiaSaveMap () : m_w(0), m_h(0) {}

DiaSaveMap::~DiaSaveMap () {}

size_t DiaSaveMap::ix (int r, int c) const {
    return (size_t)r * (size_t)m_w + (size_t)c;
}

int DiaSaveMap::w () const { return m_w; }
int DiaSaveMap::h () const { return m_h; }

uint8_t DiaSaveMap::terr (int r, int c) const { return m_terr[ix (r, c)]; }
uint8_t DiaSaveMap::clim (int r, int c) const { return m_clim[ix (r, c)]; }
uint16_t DiaSaveMap::ov (int r, int c) const { return m_ov[ix (r, c)]; }
uint8_t DiaSaveMap::riv (int r, int c) const { return m_riv[ix (r, c)]; }
uint8_t DiaSaveMap::road (int r, int c) const { return m_road[ix (r, c)]; }

bool DiaSaveMap::farm (int r, int c) const {
    return ov (r, c) == SAVE_OV_FARM;
}

bool DiaSaveMap::forest (int r, int c) const {
    return ov (r, c) == SAVE_OV_FOREST;
}

int DiaSaveMap::tile_ter (uint8_t terr, uint8_t clim) {
    if (terr == 1) return TER_OCEAN;
    if (terr == 2) return TER_SEA;
    if (terr == 3) return TER_COAST;
    if (terr == 8) return TER_INLAND_SEA;
    if (terr == 9) return TER_INLAND_LAKE;
    if (clim == 2) return TER_DESERT;
    if (clim == 1) return TER_PLAINS;
    if (clim == 3) return TER_GRASSLANDS;
    if (clim == 4) return TER_BLACKSOIL;
    return TER_PLAINS;
}

int DiaSaveMap::ter (int r, int c) const {
    return tile_ter (terr (r, c), clim (r, c));
}

bool DiaSaveMap::ld (const std::string &path) {
    FILE *f = std::fopen (path.c_str (), "rb");
    if (!f) return false;
    uint32_t magic = 0, ver = 0;
    uint16_t w = 0, h = 0;
    if (std::fread (&magic, 4, 1, f) != 1 || std::fread (&ver, 4, 1, f) != 1 ||
        std::fread (&w, 2, 1, f) != 1 || std::fread (&h, 2, 1, f) != 1 ||
        magic != k_magic || ver < k_ver || w == 0 || h == 0) {
        std::fclose (f);
        return false;
    }
    const size_t n = (size_t)w * (size_t)h;
    std::vector<uint8_t> raw (n * 16u);
    if (std::fread (raw.data (), 16, n, f) != n) {
        std::fclose (f);
        return false;
    }
    std::fclose (f);

    m_w = (int)w;
    m_h = (int)h;
    m_terr.assign (n, 0);
    m_clim.assign (n, 0);
    m_ov.assign (n, 0);
    m_riv.assign (n, 0);
    m_road.assign (n, 0);
    for (size_t i = 0; i < n; i++) {
        const uint8_t *p = raw.data () + i * 16u;
        uint64_t w0 = 0, w1 = 0;
        std::memcpy (&w0, p, 8);
        std::memcpy (&w1, p + 8, 8);
        m_road[i] = (uint8_t)((w0 >> 59) & 0x7u);
        m_ov[i] = (uint16_t)((w1 >> 16) & 0xffffu);
        m_terr[i] = (uint8_t)((w1 >> 32) & 0xfu);
        m_clim[i] = (uint8_t)((w1 >> 36) & 0xfu);
        m_riv[i] = (uint8_t)((w1 >> 40) & 0x1u);
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
