//================================================================================================================================
//=> - UnitRosterMng mk01 -
//================================================================================================================================

#include "bit_array.h"
#include "general_assessor.h"
#include "runtime_statics.h"
#include "unit_static_key.h"

bool UnitRosterMng::setup (const RuntimeStatics& st) {
    clear();
    const u16 tn = st.unit_type().get_item_count();
    if (tn == 0 || st.unit().get_item_count() == 0) {
        return false;
    }
    m_roster = new u16[tn];
    if (m_roster == nullptr) {
        return false;
    }
    m_type_n = tn;
    m_st = &st;
    for (u16 i = 0; i < tn; ++i) {
        m_roster[i] = U16_KEY_NULL;
    }
    return true;
}

void UnitRosterMng::clear () {
    delete[] m_roster;
    m_roster = nullptr;
    m_type_n = 0;
    m_st = nullptr;
}

bool UnitRosterMng::rebuild (const AssessorCtx& ctx) {
    if (m_st == nullptr || m_roster == nullptr || m_type_n == 0) {
        return false;
    }
    const u16 unit_n = m_st->unit().get_item_count();
    if (unit_n == 0) {
        return false;
    }
    BitArrayCL avail(unit_n);
    const UnitStaticDataStruct* items = &m_st->unit().get_item(UnitStaticDataKey::from_raw(0));
    GeneralAssessor::assess_unit(&avail, unit_n, items, ctx);
    for (u16 t = 0; t < m_type_n; ++t) {
        u16 best = U16_KEY_NULL;
        u32 best_score = 0;
        for (u16 i = 0; i < unit_n; ++i) {
            if (avail.get_bit(i) == 0) {
                continue;
            }
            const UnitStaticDataStruct& u = m_st->unit().get_item(UnitStaticDataKey::from_raw(i));
            if (u.type != t) {
                continue;
            }
            const u32 score = static_cast<u32>(u.attack) + static_cast<u32>(u.defense)
                + static_cast<u32>(u.mvt_pts) + static_cast<u32>(u.sight);
            if (best == U16_KEY_NULL || score > best_score || (score == best_score && i > best)) {
                best = i;
                best_score = score;
            }
        }
        m_roster[t] = best;
    }
    return true;
}

const u16* UnitRosterMng::roster () {
    return m_roster;
}

u16 UnitRosterMng::get_best_unit_of_type (u16 type_idx, const AssessorCtx& ctx) {
    if (m_st == nullptr || type_idx >= m_type_n) {
        return U16_KEY_NULL;
    }
    const u16 unit_n = m_st->unit().get_item_count();
    if (unit_n == 0) {
        return U16_KEY_NULL;
    }
    BitArrayCL avail(unit_n);
    const UnitStaticDataStruct* items = &m_st->unit().get_item(UnitStaticDataKey::from_raw(0));
    GeneralAssessor::assess_unit(&avail, unit_n, items, ctx);
    u16 best = U16_KEY_NULL;
    u32 best_score = 0;
    for (u16 i = 0; i < unit_n; ++i) {
        if (avail.get_bit(i) == 0) {
            continue;
        }
        const UnitStaticDataStruct& u = m_st->unit().get_item(UnitStaticDataKey::from_raw(i));
        if (u.type != type_idx) {
            continue;
        }
        const u32 score = static_cast<u32>(u.attack) + static_cast<u32>(u.defense)
            + static_cast<u32>(u.mvt_pts) + static_cast<u32>(u.sight);
        if (best == U16_KEY_NULL || score > best_score || (score == best_score && i > best)) {
            best = i;
            best_score = score;
        }
    }
    return best;
}

u16 UnitRosterMng::type_n () {
    return m_type_n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
