//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "tile_imp_helper.h"

#include "assert_log.h"
#include "game_array_simple.h"
#include "map_overlay_enum.h"
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
    if (ov == static_cast<u16>(MapOverlay::City)) {
        return 0xFFFFu;
    }
    if (ov >= st.worker_job_imp_index().ov_n()) {
        return 0u;
    }
    const u16 n = st.worker_job_imp_index().imp_n(ov);
    if (n == 0u) {
        return 0u;
    }
    if (n >= 16u) {
        return 0xFFFFu;
    }
    return static_cast<u16>((1u << n) - 1u);
}

bool TileImpHelper::add_idx_ok (const RuntimeStatics& st, u16 ov, u16 add_idx) {
    if (ov == U16_KEY_NULL) {
        return add_idx == 0u;
    }
    if (ov == static_cast<u16>(MapOverlay::City)) {
        return true;
    }
    const u16 mask = payload_mask_for_ov(st, ov);
    return (add_idx & static_cast<u16>(~mask)) == 0u;
}

bool TileImpHelper::add_idx_ok (u16 ov, u16 add_idx) {
    GAME_EXPECT(m_st != nullptr, "TileImpHelper::add_idx_ok null statics");
    return add_idx_ok(*m_st, ov, add_idx);
}

bool TileImpHelper::has_imp (const GameTileSimple* t, const RuntimeStatics& st, u16 imp_idx) {
    GAME_EXPECT(t != nullptr, "TileImpHelper::has_imp null tile");
    if (imp_idx >= st.worker_job_imp().get_item_count()) {
        return false;
    }
    const u16 ov = static_cast<u16>(t->m_ov);
    const u16 imp_ov = st.worker_job_imp().get_item(WorkerJobImpStaticDataKey::from_raw(imp_idx)).map_overlay_idx;
    if (ov != imp_ov) {
        return false;
    }
    const u16 bit = payload_bit(st, imp_idx);
    if (bit == 0u) {
        return false;
    }
    return (static_cast<u16>(t->m_add_idx) & bit) != 0u;
}

bool TileImpHelper::set_imp (GameTileSimple* t, const RuntimeStatics& st, u16 imp_idx) {
    GAME_EXPECT(t != nullptr, "TileImpHelper::set_imp null tile");
    if (imp_idx >= st.worker_job_imp().get_item_count()) {
        return false;
    }
    const u16 ov = static_cast<u16>(t->m_ov);
    const u16 imp_ov = st.worker_job_imp().get_item(WorkerJobImpStaticDataKey::from_raw(imp_idx)).map_overlay_idx;
    if (ov != imp_ov) {
        return false;
    }
    const u16 bit = payload_bit(st, imp_idx);
    if (bit == 0u) {
        return false;
    }
    t->m_add_idx = static_cast<u16>(t->m_add_idx) | bit;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
