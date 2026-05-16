//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CONT_INDEXER_H
#define CONT_INDEXER_H

#include "game_primitives.h"
#include "water_land_overlay.h"

#include <vector>

//================================================================================================================================
//=> - ContRegionRec -
//================================================================================================================================

struct ContRegionRec {
    u16 m_sx;
    u16 m_sy;
    u32 m_area_px;
    u8 m_r;
    u8 m_g;
    u8 m_b;
};

//================================================================================================================================
//=> - ContIndexer -
//================================================================================================================================

class ContIndexer {
public:
    explicit ContIndexer (const WaterLandOverlay& overlay);

    bool is_valid () const;
    u16 width () const;
    u16 height () const;
    u32 region_count () const;
    const std::vector<ContRegionRec>& regions () const;
    const u8* count_map_rgb () const;
    bool save_count_map_ppm (cstr path) const;
    void print_regions_by_area_desc () const;

private:
    ContIndexer (const ContIndexer& other) = delete;
    ContIndexer (ContIndexer&& other) = delete;

    bool m_ok;
    u16 m_w;
    u16 m_h;
    std::vector<ContRegionRec> m_regions;
    std::vector<u8> m_rgb;
};

#endif // CONT_INDEXER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
