//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "assessor_brute_shared.h"
#include "assessor_brute_write.h"
#include "general_assessor.h"
#include "static_parsing_manager.h"

#include "building_static_data.h"
#include "city_flag_static_data.h"
#include "resource_static_data.h"
#include "small_wonder_static_data.h"
#include "tech_static_data.h"
#include "unit_static_data.h"
#include "wonder_static_data.h"

static const BuildingStaticDataStruct* s_building_items;

static BitArrayCL* s_assess_building (u16 n, const AssessorCtx& ctx) {
    return GeneralAssessor::assess_building(n, s_building_items, ctx);
}

static u16 get_building_cnt (const StaticParsingManager& mgr) {
    return mgr.get_building_count();
}

static const char* get_building_nm (const StaticParsingManager& mgr, u16 idx) {
    const BuildingStaticDataStruct* items = mgr.get_building_data();
    if (items == nullptr || idx >= get_building_cnt(mgr)) {
        return "";
    }
    return items[idx].name.c_str();
}

struct EmitUd_BUILDING {
    const StaticParsingManager* m_mgr;
};

static void emit_ln_building (FILE* out, u16 idx, const InferredReqs& ir, void* ud) {
    EmitUd_BUILDING* eu = static_cast<EmitUd_BUILDING*>(ud);
    const StaticParsingManager& mgr = *eu->m_mgr;
    char nm[64];
    AssessorBruteWrite::pad_name(nm, 64, get_building_nm(mgr, idx));
    std::fprintf(out, "%s", nm);
const BuildingStaticDataStruct* items = mgr.get_building_data();
    if (items != nullptr) {
        std::fprintf(out, " : %5u", items[idx].cost);
    }
    AssessorBruteWrite::emit_reqs(out, ir, mgr);
    std::fprintf(out, "\n");
}

int run_building_assessor_brute () {
    StaticParsingManager mgr("../");
    u16 n = get_building_cnt(mgr);
    if (n == 0) {
        return 0;
    }
    s_building_items = mgr.get_building_data();
    if (s_building_items == nullptr) {
        return 0;
    }
    static const char* s_nms[4096];
    for (u16 i = 0; i < n && i < 4096; ++i) {
        s_nms[i] = get_building_nm(mgr, i);
    }
    EmitUd_BUILDING eu = { &mgr };
    BruteRunCfg cfg = {};
    cfg.m_item_count = n;
    cfg.m_assess = s_assess_building;
    cfg.m_names = s_nms;
    cfg.m_results_to_match_path = "RESULTS_TO_MATCH_BUILDING";
    cfg.m_results_readable_path = "RESULTS_READABLE_BUILDING";
    cfg.m_init_building_all = false;
    cfg.m_building_isolate = true;
    cfg.m_emit_line = emit_ln_building;
    cfg.m_emit_ud = &eu;
    return AssessorBrute::run(mgr, cfg);
}

static const CityFlagStaticDataStruct* s_city_flag_items;

static BitArrayCL* s_assess_city_flag (u16 n, const AssessorCtx& ctx) {
    return GeneralAssessor::assess_city_flag(n, s_city_flag_items, ctx);
}

static u16 get_city_flag_cnt (const StaticParsingManager& mgr) {
    return mgr.get_city_flag_count();
}

static const char* get_city_flag_nm (const StaticParsingManager& mgr, u16 idx) {
    const CityFlagStaticDataStruct* items = mgr.get_city_flag_data();
    if (items == nullptr || idx >= get_city_flag_cnt(mgr)) {
        return "";
    }
    return items[idx].name.c_str();
}

struct EmitUd_CITY_FLAG {
    const StaticParsingManager* m_mgr;
};

static void emit_ln_city_flag (FILE* out, u16 idx, const InferredReqs& ir, void* ud) {
    EmitUd_CITY_FLAG* eu = static_cast<EmitUd_CITY_FLAG*>(ud);
    const StaticParsingManager& mgr = *eu->m_mgr;
    char nm[64];
    AssessorBruteWrite::pad_name(nm, 64, get_city_flag_nm(mgr, idx));
    std::fprintf(out, "%s", nm);
    AssessorBruteWrite::emit_reqs(out, ir, mgr);
    std::fprintf(out, "\n");
}

int run_city_flag_assessor_brute () {
    StaticParsingManager mgr("../");
    u16 n = get_city_flag_cnt(mgr);
    if (n == 0) {
        return 0;
    }
    s_city_flag_items = mgr.get_city_flag_data();
    if (s_city_flag_items == nullptr) {
        return 0;
    }
    static const char* s_nms[4096];
    for (u16 i = 0; i < n && i < 4096; ++i) {
        s_nms[i] = get_city_flag_nm(mgr, i);
    }
    EmitUd_CITY_FLAG eu = { &mgr };
    BruteRunCfg cfg = {};
    cfg.m_item_count = n;
    cfg.m_assess = s_assess_city_flag;
    cfg.m_names = s_nms;
    cfg.m_results_to_match_path = "RESULTS_TO_MATCH_CITY_FLAG";
    cfg.m_results_readable_path = "RESULTS_READABLE_CITY_FLAG";
    cfg.m_init_building_all = true;
    cfg.m_building_isolate = false;
    cfg.m_emit_line = emit_ln_city_flag;
    cfg.m_emit_ud = &eu;
    return AssessorBrute::run(mgr, cfg);
}

static const ResourceStaticDataStruct* s_resource_items;

static BitArrayCL* s_assess_resource (u16 n, const AssessorCtx& ctx) {
    return GeneralAssessor::assess_resource(n, s_resource_items, ctx);
}

static u16 get_resource_cnt (const StaticParsingManager& mgr) {
    return mgr.get_resource_count();
}

static const char* get_resource_nm (const StaticParsingManager& mgr, u16 idx) {
    const ResourceStaticDataStruct* items = mgr.get_resource_data();
    if (items == nullptr || idx >= get_resource_cnt(mgr)) {
        return "";
    }
    return items[idx].name.c_str();
}

struct EmitUd_RESOURCE {
    const StaticParsingManager* m_mgr;
};

static void emit_ln_resource (FILE* out, u16 idx, const InferredReqs& ir, void* ud) {
    EmitUd_RESOURCE* eu = static_cast<EmitUd_RESOURCE*>(ud);
    const StaticParsingManager& mgr = *eu->m_mgr;
    char nm[64];
    AssessorBruteWrite::pad_name(nm, 64, get_resource_nm(mgr, idx));
    std::fprintf(out, "%s", nm);
const ResourceStaticDataStruct* items = mgr.get_resource_data();
    if (items != nullptr) {
        std::fprintf(out, " : %3u : %3u : %3u", items[idx].food, items[idx].shields, items[idx].commerce);
    }
    AssessorBruteWrite::emit_reqs(out, ir, mgr);
    std::fprintf(out, "\n");
}

int run_resource_assessor_brute () {
    StaticParsingManager mgr("../");
    u16 n = get_resource_cnt(mgr);
    if (n == 0) {
        return 0;
    }
    s_resource_items = mgr.get_resource_data();
    if (s_resource_items == nullptr) {
        return 0;
    }
    static const char* s_nms[4096];
    for (u16 i = 0; i < n && i < 4096; ++i) {
        s_nms[i] = get_resource_nm(mgr, i);
    }
    EmitUd_RESOURCE eu = { &mgr };
    BruteRunCfg cfg = {};
    cfg.m_item_count = n;
    cfg.m_assess = s_assess_resource;
    cfg.m_names = s_nms;
    cfg.m_results_to_match_path = "RESULTS_TO_MATCH_RESOURCE";
    cfg.m_results_readable_path = "RESULTS_READABLE_RESOURCE";
    cfg.m_init_building_all = true;
    cfg.m_building_isolate = false;
    cfg.m_emit_line = emit_ln_resource;
    cfg.m_emit_ud = &eu;
    return AssessorBrute::run(mgr, cfg);
}

static const SmallWonderStaticDataStruct* s_small_wonder_items;

static BitArrayCL* s_assess_small_wonder (u16 n, const AssessorCtx& ctx) {
    return GeneralAssessor::assess_small_wonder(n, s_small_wonder_items, ctx);
}

static u16 get_small_wonder_cnt (const StaticParsingManager& mgr) {
    return mgr.get_small_wonder_count();
}

static const char* get_small_wonder_nm (const StaticParsingManager& mgr, u16 idx) {
    const SmallWonderStaticDataStruct* items = mgr.get_small_wonder_data();
    if (items == nullptr || idx >= get_small_wonder_cnt(mgr)) {
        return "";
    }
    return items[idx].name.c_str();
}

struct EmitUd_SMALL_WONDER {
    const StaticParsingManager* m_mgr;
};

static void emit_ln_small_wonder (FILE* out, u16 idx, const InferredReqs& ir, void* ud) {
    EmitUd_SMALL_WONDER* eu = static_cast<EmitUd_SMALL_WONDER*>(ud);
    const StaticParsingManager& mgr = *eu->m_mgr;
    char nm[64];
    AssessorBruteWrite::pad_name(nm, 64, get_small_wonder_nm(mgr, idx));
    std::fprintf(out, "%s", nm);
const SmallWonderStaticDataStruct* items = mgr.get_small_wonder_data();
    if (items != nullptr) {
        std::fprintf(out, " : %5u", items[idx].cost);
    }
    AssessorBruteWrite::emit_reqs(out, ir, mgr);
    std::fprintf(out, "\n");
}

int run_small_wonder_assessor_brute () {
    StaticParsingManager mgr("../");
    u16 n = get_small_wonder_cnt(mgr);
    if (n == 0) {
        return 0;
    }
    s_small_wonder_items = mgr.get_small_wonder_data();
    if (s_small_wonder_items == nullptr) {
        return 0;
    }
    static const char* s_nms[4096];
    for (u16 i = 0; i < n && i < 4096; ++i) {
        s_nms[i] = get_small_wonder_nm(mgr, i);
    }
    EmitUd_SMALL_WONDER eu = { &mgr };
    BruteRunCfg cfg = {};
    cfg.m_item_count = n;
    cfg.m_assess = s_assess_small_wonder;
    cfg.m_names = s_nms;
    cfg.m_results_to_match_path = "RESULTS_TO_MATCH_SMALL_WONDER";
    cfg.m_results_readable_path = "RESULTS_READABLE_SMALL_WONDER";
    cfg.m_init_building_all = true;
    cfg.m_building_isolate = false;
    cfg.m_emit_line = emit_ln_small_wonder;
    cfg.m_emit_ud = &eu;
    return AssessorBrute::run(mgr, cfg);
}

static const TechStaticDataStruct* s_tech_items;

static BitArrayCL* s_assess_tech (u16 n, const AssessorCtx& ctx) {
    return GeneralAssessor::assess_tech(n, s_tech_items, ctx);
}

static u16 get_tech_cnt (const StaticParsingManager& mgr) {
    return mgr.get_tech_count();
}

static const char* get_tech_nm (const StaticParsingManager& mgr, u16 idx) {
    const TechStaticDataStruct* items = mgr.get_tech_data();
    if (items == nullptr || idx >= get_tech_cnt(mgr)) {
        return "";
    }
    return items[idx].name.c_str();
}

struct EmitUd_TECH {
    const StaticParsingManager* m_mgr;
};

static void emit_ln_tech (FILE* out, u16 idx, const InferredReqs& ir, void* ud) {
    EmitUd_TECH* eu = static_cast<EmitUd_TECH*>(ud);
    const StaticParsingManager& mgr = *eu->m_mgr;
    char nm[64];
    AssessorBruteWrite::pad_name(nm, 64, get_tech_nm(mgr, idx));
    std::fprintf(out, "%s", nm);
const TechStaticDataStruct* items = mgr.get_tech_data();
    if (items != nullptr) {
        std::fprintf(out, " : %5u", items[idx].cost);
    }
    AssessorBruteWrite::emit_reqs(out, ir, mgr);
    std::fprintf(out, "\n");
}

int run_tech_assessor_brute () {
    StaticParsingManager mgr("../");
    u16 n = get_tech_cnt(mgr);
    if (n == 0) {
        return 0;
    }
    s_tech_items = mgr.get_tech_data();
    if (s_tech_items == nullptr) {
        return 0;
    }
    static const char* s_nms[4096];
    for (u16 i = 0; i < n && i < 4096; ++i) {
        s_nms[i] = get_tech_nm(mgr, i);
    }
    EmitUd_TECH eu = { &mgr };
    BruteRunCfg cfg = {};
    cfg.m_item_count = n;
    cfg.m_assess = s_assess_tech;
    cfg.m_names = s_nms;
    cfg.m_results_to_match_path = "RESULTS_TO_MATCH_TECH";
    cfg.m_results_readable_path = "RESULTS_READABLE_TECH";
    cfg.m_init_building_all = true;
    cfg.m_building_isolate = false;
    cfg.m_emit_line = emit_ln_tech;
    cfg.m_emit_ud = &eu;
    return AssessorBrute::run(mgr, cfg);
}

static const UnitStaticDataStruct* s_unit_items;

static BitArrayCL* s_assess_unit (u16 n, const AssessorCtx& ctx) {
    return GeneralAssessor::assess_unit(n, s_unit_items, ctx);
}

static u16 get_unit_cnt (const StaticParsingManager& mgr) {
    return mgr.get_unit_count();
}

static const char* get_unit_nm (const StaticParsingManager& mgr, u16 idx) {
    const UnitStaticDataStruct* items = mgr.get_unit_data();
    if (items == nullptr || idx >= get_unit_cnt(mgr)) {
        return "";
    }
    return items[idx].name.c_str();
}

struct EmitUd_UNIT {
    const StaticParsingManager* m_mgr;
};

static void emit_ln_unit (FILE* out, u16 idx, const InferredReqs& ir, void* ud) {
    EmitUd_UNIT* eu = static_cast<EmitUd_UNIT*>(ud);
    const StaticParsingManager& mgr = *eu->m_mgr;
    char nm[64];
    AssessorBruteWrite::pad_name(nm, 64, get_unit_nm(mgr, idx));
    std::fprintf(out, "%s", nm);
    AssessorBruteWrite::emit_reqs(out, ir, mgr);
    std::fprintf(out, "\n");
}

int run_unit_assessor_brute () {
    StaticParsingManager mgr("../");
    u16 n = get_unit_cnt(mgr);
    if (n == 0) {
        return 0;
    }
    s_unit_items = mgr.get_unit_data();
    if (s_unit_items == nullptr) {
        return 0;
    }
    static const char* s_nms[4096];
    for (u16 i = 0; i < n && i < 4096; ++i) {
        s_nms[i] = get_unit_nm(mgr, i);
    }
    EmitUd_UNIT eu = { &mgr };
    BruteRunCfg cfg = {};
    cfg.m_item_count = n;
    cfg.m_assess = s_assess_unit;
    cfg.m_names = s_nms;
    cfg.m_results_to_match_path = "RESULTS_TO_MATCH_UNIT";
    cfg.m_results_readable_path = "RESULTS_READABLE_UNIT";
    cfg.m_init_building_all = true;
    cfg.m_building_isolate = false;
    cfg.m_emit_line = emit_ln_unit;
    cfg.m_emit_ud = &eu;
    return AssessorBrute::run(mgr, cfg);
}

static const WonderStaticDataStruct* s_wonder_items;

static BitArrayCL* s_assess_wonder (u16 n, const AssessorCtx& ctx) {
    return GeneralAssessor::assess_wonder(n, s_wonder_items, ctx);
}

static u16 get_wonder_cnt (const StaticParsingManager& mgr) {
    return mgr.get_wonder_count();
}

static const char* get_wonder_nm (const StaticParsingManager& mgr, u16 idx) {
    const WonderStaticDataStruct* items = mgr.get_wonder_data();
    if (items == nullptr || idx >= get_wonder_cnt(mgr)) {
        return "";
    }
    return items[idx].name.c_str();
}

struct EmitUd_WONDER {
    const StaticParsingManager* m_mgr;
};

static void emit_ln_wonder (FILE* out, u16 idx, const InferredReqs& ir, void* ud) {
    EmitUd_WONDER* eu = static_cast<EmitUd_WONDER*>(ud);
    const StaticParsingManager& mgr = *eu->m_mgr;
    char nm[64];
    AssessorBruteWrite::pad_name(nm, 64, get_wonder_nm(mgr, idx));
    std::fprintf(out, "%s", nm);
const WonderStaticDataStruct* items = mgr.get_wonder_data();
    if (items != nullptr) {
        std::fprintf(out, " : %5u", items[idx].cost);
    }
    AssessorBruteWrite::emit_reqs(out, ir, mgr);
    std::fprintf(out, "\n");
}

int run_wonder_assessor_brute () {
    StaticParsingManager mgr("../");
    u16 n = get_wonder_cnt(mgr);
    if (n == 0) {
        return 0;
    }
    s_wonder_items = mgr.get_wonder_data();
    if (s_wonder_items == nullptr) {
        return 0;
    }
    static const char* s_nms[4096];
    for (u16 i = 0; i < n && i < 4096; ++i) {
        s_nms[i] = get_wonder_nm(mgr, i);
    }
    EmitUd_WONDER eu = { &mgr };
    BruteRunCfg cfg = {};
    cfg.m_item_count = n;
    cfg.m_assess = s_assess_wonder;
    cfg.m_names = s_nms;
    cfg.m_results_to_match_path = "RESULTS_TO_MATCH_WONDER";
    cfg.m_results_readable_path = "RESULTS_READABLE_WONDER";
    cfg.m_init_building_all = true;
    cfg.m_building_isolate = false;
    cfg.m_emit_line = emit_ln_wonder;
    cfg.m_emit_ud = &eu;
    return AssessorBrute::run(mgr, cfg);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
