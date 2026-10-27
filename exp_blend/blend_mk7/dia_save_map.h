//================================================================================================================================
// DiaSaveMap: viz-only TILS reader. Unpacks GameTileSimple fields used by farm/road/river overlays.
//================================================================================================================================

#ifndef DIA_SAVE_MAP_H
#define DIA_SAVE_MAP_H

#include <cstdint>
#include <vector>
#include <string>

#include "dia_cfg.h"

//================================================================================================================================
//=> - Overlay ids (MapOverlay; Farm only needed here) -
//================================================================================================================================

static const uint16_t SAVE_OV_NONE = 0;
static const uint16_t SAVE_OV_FOREST = 3;
static const uint16_t SAVE_OV_FARM = 6;

//================================================================================================================================
//=> - Class: DiaSaveMap -
//================================================================================================================================

class DiaSaveMap {
public:
    DiaSaveMap ();
    ~DiaSaveMap ();

    bool ld (const std::string &path);
    int w () const;
    int h () const;
    uint8_t terr (int r, int c) const; // raw TERR_* class
    uint8_t clim (int r, int c) const; // raw CLIMATE_* class
    uint16_t ov (int r, int c) const; // MapOverlay id
    uint8_t riv (int r, int c) const; // river flag
    uint8_t road (int r, int c) const; // ROAD_* type
    int ter (int r, int c) const; // blend TerrainId
    bool farm (int r, int c) const;
    bool forest (int r, int c) const;

private:
    static const uint32_t k_magic = 0x534c4954u; // 'TILS'
    static const uint32_t k_ver = 2;

    int m_w; // tile cols
    int m_h; // tile rows
    std::vector<uint8_t> m_terr; // row-major TERR_*
    std::vector<uint8_t> m_clim; // row-major CLIMATE_*
    std::vector<uint16_t> m_ov; // row-major MapOverlay
    std::vector<uint8_t> m_riv; // row-major river
    std::vector<uint8_t> m_road; // row-major ROAD_*

    size_t ix (int r, int c) const;
    static int tile_ter (uint8_t terr, uint8_t clim);
};

#endif // DIA_SAVE_MAP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
