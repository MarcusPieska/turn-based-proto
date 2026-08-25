//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "tile_imp_helper.h"

#include "add_access_helper.h"
#include "assert_log.h"
#include "game_array_simple.h"
#include "runtime_statics.h"
#include "worker_job_imp_index.h"
#include "worker_job_imp_static_key.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

const RuntimeStatics* TileImpHelper::m_st = nullptr;

//================================================================================================================================
//=> - TileImpHelper -
//================================================================================================================================

void TileImpHelper::bind_statics (const RuntimeStatics* st) {
    m_st = st;
    if (st == nullptr) {
        AddAccessHelper::clear();
        return;
    }
    AddAccessHelper::setup(*st);
}

u16 TileImpHelper::imp_slot (const RuntimeStatics& st, u16 imp_idx) {
    if (imp_idx >= st.worker_job_imp().get_item_count()) {
        return U16_KEY_NULL;
    }
    const u16 ov = st.worker_job_imp().get_item(WorkerJobImpStaticDataKey::from_raw(imp_idx)).map_overlay_idx;
    const WorkerJobImpIndex& ix = st.worker_job_imp_index();
    if (ov >= ix.ov_n()) {
        return U16_KEY_NULL;
    }
    const u16 n = ix.imp_n(ov);
    const u16* imps = ix.imps(ov);
    if (imps == nullptr) {
        return U16_KEY_NULL;
    }
    for (u16 i = 0; i < n; ++i) {
        if (imps[i] == imp_idx) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

u16 TileImpHelper::payload_bit (const RuntimeStatics& st, u16 imp_idx) {
    const u16 slot = imp_slot(st, imp_idx);
    if (slot >= 16u) {
        return 0u;
    }
    return static_cast<u16>(1u << slot);
}

u16 TileImpHelper::payload_mask_for_ov (const RuntimeStatics& st, u16 ov) {
    (void)st;
    return AddAccessHelper::payload_mask(ov);
}

bool TileImpHelper::add_idx_ok (const RuntimeStatics& st, u16 ov, u16 add_idx) {
    (void)st;
    return AddAccessHelper::add_ok(ov, add_idx);
}

bool TileImpHelper::add_idx_ok (u16 ov, u16 add_idx) {
    GAME_EXPECT(m_st != nullptr, "TileImpHelper::add_idx_ok null statics");
    return AddAccessHelper::add_ok(ov, add_idx);
}

bool TileImpHelper::has_imp (const GameTileSimple* t, const RuntimeStatics& st, u16 imp_idx) {
    (void)st;
    return AddAccessHelper::has_imp(t, imp_idx);
}

bool TileImpHelper::set_imp (GameTileSimple* t, const RuntimeStatics& st, u16 imp_idx) {
    (void)st;
    return AddAccessHelper::set_imp(t, imp_idx);
}

#include "add_access_helper.cpp"

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
