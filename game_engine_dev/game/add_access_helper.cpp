//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "add_access_helper.h"

#include <cstring>

#include "assert_log.h"
#include "game_array_simple.h"
#include "general_bit_bank.h"
#include "map_overlay_static_data.h"
#include "map_overlay_static_key.h"
#include "runtime_statics.h"
#include "worker_job_imp_index.h"
#include "worker_job_imp_static_key.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

const RuntimeStatics* AddAccessHelper::m_st = nullptr;
GeneralBitBank** AddAccessHelper::m_banks = nullptr;
u16 AddAccessHelper::m_ov_n = 0;
u16 AddAccessHelper::m_city_ov = U16_KEY_NULL;

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static u16 find_city_ov (const RuntimeStatics& st) {
    const u16 n = st.map_overlay().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        cstr nm = st.map_overlay().get_name(MapOverlayStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, "City") == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static u16 imp_slot_on_ov (const RuntimeStatics& st, u16 ov, u16 imp_idx) {
    if (ov >= st.worker_job_imp_index().ov_n()) {
        return U16_KEY_NULL;
    }
    const u16 n = st.worker_job_imp_index().imp_n(ov);
    const u16* imps = st.worker_job_imp_index().imps(ov);
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

//================================================================================================================================
//=> - AddAccessHelper -
//================================================================================================================================

bool AddAccessHelper::setup (const RuntimeStatics& st) {
    clear();
    m_st = &st;
    m_ov_n = st.map_overlay().get_item_count();
    m_city_ov = find_city_ov(st);
    GAME_EXPECT(m_city_ov != U16_KEY_NULL, "AddAccessHelper::setup City overlay missing");
    if (m_ov_n == 0u) {
        return m_city_ov != U16_KEY_NULL;
    }
    m_banks = new GeneralBitBank*[m_ov_n];
    for (u16 i = 0; i < m_ov_n; ++i) {
        m_banks[i] = nullptr;
    }
    const WorkerJobImpIndex& ix = st.worker_job_imp_index();
    const u16 ix_n = ix.ov_n();
    for (u16 ov = 0; ov < m_ov_n; ++ov) {
        if (ov == m_city_ov) {
            continue;
        }
        if (ov >= ix_n) {
            continue;
        }
        const u16 n = ix.imp_n(ov);
        if (n > 16u) {
            m_banks[ov] = new GeneralBitBank(n);
        }
    }
    return true;
}

void AddAccessHelper::clear () {
    if (m_banks != nullptr) {
        for (u16 i = 0; i < m_ov_n; ++i) {
            delete m_banks[i];
            m_banks[i] = nullptr;
        }
        delete[] m_banks;
        m_banks = nullptr;
    }
    m_st = nullptr;
    m_ov_n = 0;
    m_city_ov = U16_KEY_NULL;
}

u16 AddAccessHelper::city_ov () {
    return m_city_ov;
}

bool AddAccessHelper::is_city (u16 ov) {
    return m_city_ov != U16_KEY_NULL && ov == m_city_ov;
}

bool AddAccessHelper::uses_bank (u16 ov) {
    return bank(ov) != nullptr;
}

GeneralBitBank* AddAccessHelper::bank (u16 ov) {
    if (m_banks == nullptr || ov >= m_ov_n) {
        return nullptr;
    }
    return m_banks[ov];
}

u16 AddAccessHelper::empty_add (u16 ov) {
    if (uses_bank(ov)) {
        return U16_KEY_NULL;
    }
    return 0u;
}

u16 AddAccessHelper::payload_mask (u16 ov) {
    if (is_city(ov) || uses_bank(ov)) {
        return 0xFFFFu;
    }
    if (m_st == nullptr) {
        return 0u;
    }
    if (ov >= m_st->worker_job_imp_index().ov_n()) {
        return 0u;
    }
    const u16 n = m_st->worker_job_imp_index().imp_n(ov);
    if (n == 0u) {
        return 0u;
    }
    if (n >= 16u) {
        return 0xFFFFu;
    }
    return static_cast<u16>((1u << n) - 1u);
}

bool AddAccessHelper::add_ok (u16 ov, u16 add_idx) {
    if (ov == U16_KEY_NULL) {
        return add_idx == 0u;
    }
    if (is_city(ov) || uses_bank(ov)) {
        return true;
    }
    const u16 mask = payload_mask(ov);
    return (add_idx & static_cast<u16>(~mask)) == 0u;
}

bool AddAccessHelper::has_imp (const GameTileSimple* t, u16 imp_idx) {
    GAME_EXPECT(t != nullptr, "AddAccessHelper::has_imp null tile");
    GAME_EXPECT(m_st != nullptr, "AddAccessHelper::has_imp null statics");
    if (imp_idx >= m_st->worker_job_imp().get_item_count()) {
        return false;
    }
    const u16 ov = static_cast<u16>(t->m_ov);
    const u16 imp_ov = m_st->worker_job_imp().get_item(WorkerJobImpStaticDataKey::from_raw(imp_idx)).map_overlay_idx;
    if (ov != imp_ov || is_city(ov)) {
        return false;
    }
    const u16 slot = imp_slot_on_ov(*m_st, ov, imp_idx);
    if (slot == U16_KEY_NULL) {
        return false;
    }
    GeneralBitBank* b = bank(ov);
    if (b != nullptr) {
        const u16 batch = static_cast<u16>(t->m_add_idx);
        if (batch == U16_KEY_NULL) {
            return false;
        }
        return b->is_flagged(batch, slot);
    }
    if (slot >= 16u) {
        return false;
    }
    const u16 bit = static_cast<u16>(1u << slot);
    return (static_cast<u16>(t->m_add_idx) & bit) != 0u;
}

bool AddAccessHelper::set_imp (GameTileSimple* t, u16 imp_idx) {
    GAME_EXPECT(t != nullptr, "AddAccessHelper::set_imp null tile");
    GAME_EXPECT(m_st != nullptr, "AddAccessHelper::set_imp null statics");
    if (imp_idx >= m_st->worker_job_imp().get_item_count()) {
        return false;
    }
    const u16 ov = static_cast<u16>(t->m_ov);
    const u16 imp_ov = m_st->worker_job_imp().get_item(WorkerJobImpStaticDataKey::from_raw(imp_idx)).map_overlay_idx;
    if (ov != imp_ov || is_city(ov)) {
        return false;
    }
    const u16 slot = imp_slot_on_ov(*m_st, ov, imp_idx);
    if (slot == U16_KEY_NULL) {
        return false;
    }
    GeneralBitBank* b = bank(ov);
    if (b != nullptr) {
        u16 batch = static_cast<u16>(t->m_add_idx);
        if (batch == U16_KEY_NULL) {
            batch = b->claim_batch();
            t->m_add_idx = batch;
        }
        b->set_flag(batch, slot);
        return true;
    }
    if (slot >= 16u) {
        return false;
    }
    const u16 bit = static_cast<u16>(1u << slot);
    t->m_add_idx = static_cast<u16>(t->m_add_idx) | bit;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
