//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef ASSESSOR_BRUTE_NAMES_H
#define ASSESSOR_BRUTE_NAMES_H

#include "static_parsing_manager.h"

//================================================================================================================================
//=> - Prereq name lookup -
//================================================================================================================================

static inline cstr assessor_prereq_tech_nm (const StaticParsingManager& mgr, u16 idx) {
    if (idx >= mgr.get_tech_count()) {
        return "";
    }
    return mgr.get_tech_name_parser().idx_to_name(idx);
}

static inline cstr assessor_prereq_resource_nm (const StaticParsingManager& mgr, u16 idx) {
    if (idx >= mgr.get_resource_count()) {
        return "";
    }
    return mgr.get_resource_name_parser().idx_to_name(idx);
}

static inline cstr assessor_prereq_building_nm (const StaticParsingManager& mgr, u16 idx) {
    if (idx >= mgr.get_building_count()) {
        return "";
    }
    return mgr.get_building_name_parser().idx_to_name(idx);
}

static inline cstr assessor_prereq_city_flag_nm (const StaticParsingManager& mgr, u16 idx) {
    if (idx >= mgr.get_city_flag_count()) {
        return "";
    }
    return mgr.get_city_flag_name_parser().idx_to_name(idx);
}

static inline cstr assessor_prereq_civ_nm (const StaticParsingManager& mgr, u16 idx) {
    if (idx >= mgr.get_civ_count()) {
        return "";
    }
    return mgr.get_civ_name_parser().idx_to_name(idx);
}

//================================================================================================================================
//=> - Item name lookup -
//================================================================================================================================

static inline cstr assessor_item_building_nm (const StaticParsingManager& mgr, u16 idx) {
    return assessor_prereq_building_nm(mgr, idx);
}

static inline cstr assessor_item_city_flag_nm (const StaticParsingManager& mgr, u16 idx) {
    return assessor_prereq_city_flag_nm(mgr, idx);
}

static inline cstr assessor_item_resource_nm (const StaticParsingManager& mgr, u16 idx) {
    return assessor_prereq_resource_nm(mgr, idx);
}

static inline cstr assessor_item_small_wonder_nm (const StaticParsingManager& mgr, u16 idx) {
    if (idx >= mgr.get_small_wonder_count()) {
        return "";
    }
    return mgr.get_small_wonder_name_parser().idx_to_name(idx);
}

static inline cstr assessor_item_tech_nm (const StaticParsingManager& mgr, u16 idx) {
    return assessor_prereq_tech_nm(mgr, idx);
}

static inline cstr assessor_item_unit_nm (const StaticParsingManager& mgr, u16 idx) {
    if (idx >= mgr.get_unit_count()) {
        return "";
    }
    return mgr.get_unit_name_parser().idx_to_name(idx);
}

static inline cstr assessor_item_wonder_nm (const StaticParsingManager& mgr, u16 idx) {
    if (idx >= mgr.get_wonder_count()) {
        return "";
    }
    return mgr.get_wonder_name_parser().idx_to_name(idx);
}

#endif // ASSESSOR_BRUTE_NAMES_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
