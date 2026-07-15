//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef FACTORY_GAME_ARRAY_SIMPLE_H
#define FACTORY_GAME_ARRAY_SIMPLE_H

#include "game_array_simple.h"
#include "map_gen_api.h"

//================================================================================================================================
//=> - Factory_GameArraySimple -
//================================================================================================================================
//
//  Stateless loader for map PPMs into a GameArraySimple. Used by GameSetup during
//  new-game setup. adv_map_gen layers first; res_dist overlay second on an existing grid.
//  Not used for save/load; GameSetup reads and writes GameState files directly.
//
//================================================================================================================================

class Factory_GameArraySimple {
public:
    static bool load_map_gen_data (
        GameArraySimple* out, // Destination grid; cleared then filled
        cstr terr_path, // Terrain PPM from adv_map_gen
        cstr clim_path, // Climate PPM from adv_map_gen
        cstr riv_path, // River PPM from adv_map_gen
        cstr ov_path = nullptr); // Overlay PPM from adv_map_gen

    static bool load_res_dist_data (
        GameArraySimple* out, // Grid from load_map_gen_data; must match res PPM size
        cstr res_path); // Resource overlay PPM from res_dist

    static bool load_from_rslt (
        GameArraySimple* out, // Destination grid; cleared then filled
        const MakeMapRslt& rslt); // In-memory map from map_gen_generate

    static bool init_test_grid (GameArraySimple* out, u16 w, u16 h); // Blank grid for unit tests

private:
    Factory_GameArraySimple () = delete;
};

#endif // FACTORY_GAME_ARRAY_SIMPLE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
