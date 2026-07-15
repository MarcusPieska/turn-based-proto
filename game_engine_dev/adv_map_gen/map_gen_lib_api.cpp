//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "map_gen_api.h"

#include "p1_make_map.h"
#include "p1_wb_util.h"
#include "res_statics.h"

#include <cstring>

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static MakeMapRslt make_map_rslt_fail () {
    MakeMapRslt r = {};
    r.m_ok = false;
    return r;
}

static bool copy_u8 (u8** dst, const u8* src, u32 n) {
    if (dst == nullptr || src == nullptr || n == 0u) {
        return false;
    }
    *dst = new u8[n];
    if (*dst == nullptr) {
        return false;
    }
    std::memcpy(*dst, src, static_cast<size_t>(n));
    return true;
}

static bool copy_u16 (u16** dst, const u16* src, u32 n) {
    if (dst == nullptr || src == nullptr || n == 0u) {
        return false;
    }
    *dst = new u16[n];
    if (*dst == nullptr) {
        return false;
    }
    std::memcpy(*dst, src, static_cast<size_t>(n) * sizeof(u16));
    return true;
}

static bool fill_canonical (MakeMapRslt* out, const P1_MakeMapRslt& src) {
    if (out == nullptr || src.m_terrain == nullptr || src.m_climate == nullptr || src.m_rivers == nullptr
        || src.m_overlay == nullptr || src.m_resources == nullptr || src.m_w == 0u || src.m_h == 0u) {
        return false;
    }
    const u32 npx = static_cast<u32>(src.m_w) * static_cast<u32>(src.m_h);
    if (!copy_u8(&out->m_terrain, src.m_terrain, npx)
        || !copy_u8(&out->m_climate, src.m_climate, npx)
        || !copy_u8(&out->m_rivers, src.m_rivers, npx)
        || !copy_u8(&out->m_overlay, src.m_overlay, npx)
        || !copy_u16(&out->m_resources, src.m_resources, npx)) {
        map_gen_free_rslt(out);
        return false;
    }
    out->m_ok = true;
    out->m_w = src.m_w;
    out->m_h = src.m_h;
    return true;
}

//================================================================================================================================
//=> - map_gen API -
//================================================================================================================================

MakeMapRslt map_gen_generate (const MapGenReq* req) {
    if (req == nullptr || req->m_type != MAP_CONTINENTAL || !p1_run_prm_ok({req->m_seed, req->m_w, req->m_h})) {
        return make_map_rslt_fail();
    }
    if (!p1_map_gen_init(req->m_statics)) {
        return make_map_rslt_fail();
    }
    P1_RunPrm prm = {};
    prm.m_seed = req->m_seed;
    prm.m_w = req->m_w;
    prm.m_h = req->m_h;
    P1_MakeMap mk(prm, req->m_cfg);
    const bool gen_ok = mk.generate(k_p1_step_resources);
    p1_wb_term();
    p1_map_gen_set_statics(nullptr);
    res_statics_session_reset();
    if (!gen_ok || !mk.is_valid()) {
        return make_map_rslt_fail();
    }
    MakeMapRslt out = make_map_rslt_fail();
    if (!fill_canonical(&out, mk.result())) {
        return make_map_rslt_fail();
    }
    return out;
}

void map_gen_free_rslt (MakeMapRslt* rslt) {
    if (rslt == nullptr) {
        return;
    }
    delete[] rslt->m_terrain;
    delete[] rslt->m_climate;
    delete[] rslt->m_rivers;
    delete[] rslt->m_overlay;
    delete[] rslt->m_resources;
    rslt->m_terrain = nullptr;
    rslt->m_climate = nullptr;
    rslt->m_rivers = nullptr;
    rslt->m_overlay = nullptr;
    rslt->m_resources = nullptr;
    rslt->m_w = 0;
    rslt->m_h = 0;
    rslt->m_ok = false;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
