//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>

#include "res_statics.h"
#include "item_reqs.h"
#include "res_placement_defs.h"
#include "game_map_defs.h"
#include "resource_static_key.h"
#include "res_dist_static_key.h"
#include "tech_static_key.h"
#include "building_static_key.h"
#include "city_flag_static_key.h"
#include "civ_static_key.h"

typedef const char* cstr;

static const char* g_lib_path = "../data_io/runtime_static_loader_lib.so";
static const char* g_data_path = "../";
static int g_plvl = 0;

//================================================================================================================================
//=> - Print helpers -
//================================================================================================================================

static cstr terr_tok (u8 v) {
    if (v == RES_TERR_ALL) return "ALL_TERRAINS";
    if (v == TERR_OCEAN[0]) return "TERR_OCEAN";
    if (v == TERR_SEA[0]) return "TERR_SEA";
    if (v == TERR_COASTAL[0]) return "TERR_COASTAL";
    if (v == TERR_PLAINS[0]) return "TERR_PLAINS";
    if (v == TERR_HILLS[0]) return "TERR_HILLS";
    if (v == TERR_MOUNTAINS[0]) return "TERR_MOUNTAINS";
    return "?";
}

static cstr clim_tok (u8 v) {
    if (v == RES_CLIM_ALL) return "ALL_CLIMATES";
    if (v == CLIMATE_TUNDRA) return "CLIMATE_TUNDRA";
    if (v == CLIMATE_GRASSLAND) return "CLIMATE_GRASSLAND";
    if (v == CLIMATE_PLAINS) return "CLIMATE_PLAINS";
    if (v == CLIMATE_DESERT) return "CLIMATE_DESERT";
    return "?";
}

static cstr ov_tok (u8 v) {
    if (v == RES_OV_ALL) return "ALL_OVERLAYS";
    if (v == RES_OV_NONE) return "NO_OVERLAYS";
    if (v == RES_OV_SWAMPS) return "OVERLAY_SWAMPS";
    if (v == RES_OV_FORESTS) return "OVERLAY_FORESTS";
    if (v == RES_OV_JUNGLES) return "OVERLAY_JUNGLES";
    if (v == RES_OV_RIVERS) return "OVERLAY_RIVERS";
    return "?";
}

static void pr_u16 (cstr label, u16 value) {
    std::printf("  %s: %u\n", label, value);
}

static void pr_reqs (const RuntimeStatics& s, cstr label, const ItemReqsStruct& reqs) {
    std::printf("  %s:\n", label);
    for (u32 j = 0; j < MAX_PREREQ_COUNT; ++j) {
        if (reqs.types[j] == ITEM_REQ_TYPE_NONE) {
            continue;
        }
        const u16 idx = reqs.indices[j];
        if (idx == U16_KEY_NULL) {
            continue;
        }
        const u8 type = reqs.types[j];
        cstr nm = "<unknown>";
        if (type == ITEM_REQ_TYPE_BUILDING && idx < s.building().get_item_count()) {
            nm = s.building().get_name(BuildingStaticDataKey::from_raw(idx));
        } else if (type == ITEM_REQ_TYPE_FLAG && idx < s.city_flag().get_item_count()) {
            nm = s.city_flag().get_name(CityFlagStaticDataKey::from_raw(idx));
        } else if (type == ITEM_REQ_TYPE_CIV && idx < s.civ().get_item_count()) {
            nm = s.civ().get_name(CivStaticDataKey::from_raw(idx));
        } else if (type == ITEM_REQ_TYPE_RESOURCE && idx < s.resource().get_item_count()) {
            nm = s.resource().get_name(ResourceStaticDataKey::from_raw(idx));
        } else if (type == ITEM_REQ_TYPE_TECH && idx < s.tech().get_item_count()) {
            nm = s.tech().get_name(TechStaticDataKey::from_raw(idx));
        }
        std::printf("    [%u] type=%u %s (%u)", j, type, nm, idx);
        if (reqs.added_args[j] != 0) {
            std::printf(" arg=%u", reqs.added_args[j]);
        }
        std::printf("\n");
    }
}

static void pr_plc (const ResPlacement& plc) {
    pr_u16("res_wt", plc.m_res_wt);
    pr_u16("quad_n", plc.m_quad_n);
    for (u32 qi = 0; qi < plc.m_quad_n; ++qi) {
        const ResQuad& q = plc.m_quads[qi];
        std::printf("  quad[%u]: terr=%s clim=%s ov=%s wt=%u\n", qi,
            terr_tok(q.m_terr), clim_tok(q.m_clim), ov_tok(q.m_ov), q.m_wt);
    }
}

static void pr_res_item (const RuntimeStatics& s, u16 i) {
    const ResourceStaticDataKey rk = ResourceStaticDataKey::from_raw(i);
    const ResourceStaticDataStruct& res = s.resource().get_item(rk);
    cstr nm = s.resource().get_name(rk);
    std::printf("name: %s\n", nm);
    pr_u16("food", res.food);
    pr_u16("shields", res.shields);
    pr_u16("commerce", res.commerce);
    pr_reqs(s, "reqs", res.reqs);
    if (res.res_dist_idx < s.res_dist().get_item_count()) {
        cstr rd_nm = s.res_dist().get_name(ResDistStaticDataKey::from_raw(res.res_dist_idx));
        std::printf("  res_dist: %s (%u)\n", rd_nm, res.res_dist_idx);
        const ResDistStaticDataStruct& rd = s.res_dist().get_item(
            ResDistStaticDataKey::from_raw(res.res_dist_idx));
        std::printf("  has_plc: %u\n", rd.has_plc);
        if (rd.has_plc != 0) {
            pr_plc(rd.plc);
        }
    } else {
        std::printf("  res_dist: <oob> (%u)\n", res.res_dist_idx);
    }
}

static void print_all (const RuntimeStatics& s) {
    const u16 n = s.resource().get_item_count();
    if (g_plvl >= 1) {
        std::printf("resource count: %u\n", n);
        std::printf("res_dist count: %u\n", s.res_dist().get_item_count());
    }
    if (g_plvl < 2) {
        return;
    }
    for (u16 i = 0; i < n; ++i) {
        pr_res_item(s, i);
        std::printf("-----------------------------------------------------------\n");
    }
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        g_plvl = std::atoi(argv[1]);
    }
    if (argc > 2) {
        g_lib_path = argv[2];
    }
    if (argc > 3) {
        g_data_path = argv[3];
    }
    ResStatics rs;
    if (!rs.load(g_lib_path, g_data_path)) {
        std::printf("failed to load %s\n", g_lib_path);
        return 1;
    }
    print_all(rs.s());
    rs.unload();
    std::printf("=======================================================\n");
    std::printf(" RESOURCE STATICS PRINT DONE (plvl=%d)\n", g_plvl);
    std::printf("=======================================================\n");
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
