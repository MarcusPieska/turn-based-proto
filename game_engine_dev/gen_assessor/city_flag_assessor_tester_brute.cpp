//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "assessor_brute_shared.h"
#include "assessor_brute_write.h"
#include "static_parsing_manager.h"
#include "city_flag_static_data.h"

//================================================================================================================================
//=> - Item access -
//================================================================================================================================

static u16 get_cnt (const StaticParsingManager& mgr) {
    return mgr.get_city_flag_count();
}

static const ItemReqsStruct* get_reqs (const StaticParsingManager& mgr) {
    const CityFlagStaticDataStruct* items = mgr.get_city_flag_data();
    if (items == nullptr) {
        return nullptr;
    }
    static ItemReqsStruct s_buf[4096];
    u16 n = get_cnt(mgr);
    if (n > 4096) {
        n = 4096;
    }
    for (u16 i = 0; i < n; ++i) {
        s_buf[i] = items[i].reqs;
    }
    return s_buf;
}

static const char* get_nm (const StaticParsingManager& mgr, u16 idx) {
    const CityFlagStaticDataStruct* items = mgr.get_city_flag_data();
    if (items == nullptr || idx >= get_cnt(mgr)) {
        return "";
    }
    return items[idx].name.c_str();
}

//================================================================================================================================
//=> - Line emit -
//================================================================================================================================

struct EmitUd {
    const StaticParsingManager* m_mgr;
};

static void emit_ln (FILE* out, u16 idx, const InferredReqs& ir, void* ud) {
    EmitUd* eu = static_cast<EmitUd*>(ud);
    const StaticParsingManager& mgr = *eu->m_mgr;
    char nm[64];
    AssessorBruteWrite::pad_name(nm, 64, get_nm(mgr, idx));
    std::fprintf(out, "%s", nm);
    
    AssessorBruteWrite::emit_reqs(out, ir, mgr);
    std::fprintf(out, "\n");
}

//================================================================================================================================
//=> - Run -
//================================================================================================================================

int run_city_flag_assessor_brute () {
    StaticParsingManager mgr("../");
    u16 n = get_cnt(mgr);
    if (n == 0) {
        return 0;
    }
    static const char* s_nms[4096];
    for (u16 i = 0; i < n && i < 4096; ++i) {
        s_nms[i] = get_nm(mgr, i);
    }
    const ItemReqsStruct* reqs = get_reqs(mgr);
    EmitUd eu = { &mgr };
    BruteRunCfg cfg = {};
    cfg.m_item_count = n;
    cfg.m_reqs = reqs;
    cfg.m_names = s_nms;
    cfg.m_results_to_match_path = "RESULTS_TO_MATCH_CITY_FLAG";
    cfg.m_results_readable_path = "RESULTS_READABLE_CITY_FLAG";
    cfg.m_init_building_all = true;
    cfg.m_building_isolate = false;
    cfg.m_emit_line = emit_ln;
    cfg.m_emit_ud = &eu;
    return AssessorBrute::run(mgr, cfg);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
