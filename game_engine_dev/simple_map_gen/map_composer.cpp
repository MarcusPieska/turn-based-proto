//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "map_composer.h"

#include "continent_maker_pn.h"
#include "map_combiner.h"

#include <cstring>

//================================================================================================================================
//=> - MapComposer -
//================================================================================================================================

bool MapComposer::shift_land_by_coastal_limit (ContinentMakerPnParams& p, f64 coastal_lim_01) {
    const f64 new_c = coastal_lim_01;
    if (!(new_c > 0.0) || !(new_c < 1.0)) {
        return false;
    }
    const f64 old_c = p.m_terr_lim_coastal;
    const f64 old_lp = p.m_terr_lim_plains;
    const f64 old_lh = p.m_terr_lim_hills;
    if (!(old_c > 0.0) || !(old_c < 1.0)) {
        return false;
    }
    const f64 land_denom = 1.0 - old_c;
    if (!(land_denom > 0.0)) {
        return false;
    }
    const f64 ratio = old_c / new_c;
    p.m_terr_lim_ocean /= ratio;
    p.m_terr_lim_sea /= ratio;
    p.m_terr_lim_coastal = new_c;
    const f64 land_w = 1.0 - new_c;
    p.m_terr_lim_plains = new_c + (old_lp - old_c) / land_denom * land_w;
    p.m_terr_lim_hills = new_c + (old_lh - old_c) / land_denom * land_w;
    return true;
}

bool MapComposer::compose_central_with_archipelago (
    const ComposeCentralWithArchipelagoArgs& a,
    std::vector<u8>& out_terrain_rgb,
    std::vector<u8>* out_base_terrain_rgb,
    std::vector<u8>* out_overlay_terrain_rgb) {
    out_terrain_rgb.clear();
    if (out_base_terrain_rgb != nullptr) {
        out_base_terrain_rgb->clear();
    }
    if (out_overlay_terrain_rgb != nullptr) {
        out_overlay_terrain_rgb->clear();
    }
    ContinentMakerPn mk_base(a.m_base_params);
    ContinentMakerPn mk_over(a.m_overlay_params);
    if (!mk_base.is_valid() || !mk_over.is_valid()) {
        return false;
    }
    const u16 wb = mk_base.width();
    const u16 hb = mk_base.height();
    const u16 wo = mk_over.width();
    const u16 ho = mk_over.height();
    if (wb < wo || hb < ho) {
        return false;
    }
    const u32 n3 = static_cast<u32>(wb) * static_cast<u32>(hb) * 3u;
    out_terrain_rgb.assign(static_cast<size_t>(n3), 0);
    std::memcpy(out_terrain_rgb.data(), mk_base.terrain_rgb(), static_cast<size_t>(n3));
    if (out_base_terrain_rgb != nullptr) {
        *out_base_terrain_rgb = out_terrain_rgb;
    }
    if (out_overlay_terrain_rgb != nullptr) {
        const u32 n3o = static_cast<u32>(wo) * static_cast<u32>(ho) * 3u;
        out_overlay_terrain_rgb->resize(static_cast<size_t>(n3o));
        std::memcpy(out_overlay_terrain_rgb->data(), mk_over.terrain_rgb(), static_cast<size_t>(n3o));
    }
    if (!MapCombiner::combine_terrain_into_base(out_terrain_rgb.data(), wb, hb, mk_over.terrain_rgb(), wo, ho, 0, 0)) {
        out_terrain_rgb.clear();
        return false;
    }
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
