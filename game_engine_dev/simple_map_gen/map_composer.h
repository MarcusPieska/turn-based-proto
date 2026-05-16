//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_COMPOSER_H
#define MAP_COMPOSER_H

#include "continent_maker_pn.h"
#include "game_primitives.h"

#include <vector>

//================================================================================================================================
//=> - ComposeCentralWithArchipelagoArgs -
//================================================================================================================================

struct ComposeCentralWithArchipelagoArgs {
    ContinentMakerPnParams m_base_params;
    ContinentMakerPnParams m_overlay_params;
};

//================================================================================================================================
//=> - MapComposer -
//================================================================================================================================

class MapComposer {
public:
    static bool compose_central_with_archipelago (
        const ComposeCentralWithArchipelagoArgs& a,
        std::vector<u8>& out_terrain_rgb,
        std::vector<u8>* out_base_terrain_rgb = nullptr,
        std::vector<u8>* out_overlay_terrain_rgb = nullptr);

    static bool shift_land_by_coastal_limit (ContinentMakerPnParams& p, f64 coastal_lim_01);

private:
    MapComposer () = delete;
};

#endif // MAP_COMPOSER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
