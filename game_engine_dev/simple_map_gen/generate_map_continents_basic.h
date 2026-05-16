//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_MAP_CONTINENTS_BASIC_H
#define GENERATE_MAP_CONTINENTS_BASIC_H

#include <cstddef>
#include <utility>
#include <vector>

#include "game_primitives.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - MapContinentsBasicParams -
//================================================================================================================================

struct MapContinentsBasicParams {
    u32 m_seed = 0;
    u16 m_canvas_w = 1000;
    u16 m_canvas_h = 1000;
    u16 m_continent_count = 20;
    u8 m_min_cont_size_perc = 5;
    u8 m_max_cont_size_perc = 25;
    u16 m_large_continent_count = 2;
    u8 m_large_continent_size_perc = 30;
    u8 m_patch_combo_min_size_perc = 12;
};

//================================================================================================================================
//=> - MapContinentsBasic -
//================================================================================================================================

class Generate_MapContinentsBasic {
public:
    explicit Generate_MapContinentsBasic (const MapContinentsBasicParams& params);
    ~Generate_MapContinentsBasic ();

    bool is_valid () const;
    bool save_output (cstr path) const;

    u16 canvas_width () const;
    u16 canvas_height () const;
    const MapArrayTerrain& canvas () const;

    size_t layer_count () const;
    u16 layer_size (size_t i) const;
    void layer_anchor (size_t i, u16& out_ax, u16& out_ay) const;
    const MapArrayTerrain& layer_patch (size_t i) const;

    const MapContinentsBasicParams& params () const;

private:
    Generate_MapContinentsBasic (const Generate_MapContinentsBasic& other) = delete;
    Generate_MapContinentsBasic (Generate_MapContinentsBasic&& other) = delete;

    bool m_valid_generation;
    MapContinentsBasicParams m_params;
    MapArrayTerrain* m_canvas;
    std::vector<MapArrayTerrain*> m_patches;
    std::vector<u16> m_sizes;
    std::vector<std::pair<u16, u16>> m_anchors;
};

#endif // GENERATE_MAP_CONTINENTS_BASIC_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
