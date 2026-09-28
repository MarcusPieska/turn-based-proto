//================================================================================================================================
//=> - UnitRosterMng mk02 -
//================================================================================================================================

#include "bit_array.h"
#include "general_assessor.h"
#include "runtime_statics.h"
#include "unit_static_key.h"

static u32 s_score (const UnitStaticDataStruct& u) {
    return static_cast<u32>(u.attack) + static_cast<u32>(u.defense)
        + static_cast<u32>(u.mvt_pts) + static_cast<u32>(u.sight);
}

static bool s_better (const UnitStaticDataStruct* items, u16 a, u16 b) {
    const u32 sa = s_score(items[a]);
    const u32 sb = s_score(items[b]);
    if (sa != sb) {
        return sa > sb;
    }
    return a > b;
}

static void s_sort_slice (u16* sl, u16 n, const UnitStaticDataStruct* items) {
    for (u16 i = 0; i < n; ++i) {
        u16 best = i;
        for (u16 j = i + 1u; j < n; ++j) {
            if (s_better(items, sl[j], sl[best])) {
                best = j;
            }
        }
        if (best != i) {
            const u16 tmp = sl[i];
            sl[i] = sl[best];
            sl[best] = tmp;
        }
    }
}

bool UnitRosterMng::setup (const RuntimeStatics& st) {
    clear();
    const u16 tn = st.unit_type().get_item_count();
    const u16 un = st.unit().get_item_count();
    if (tn == 0 || un == 0) {
        return false;
    }
    m_roster = new u16[tn];
    m_type_ix = new TypeIx[tn];
    m_pack = new u16[un];
    if (m_roster == nullptr || m_type_ix == nullptr || m_pack == nullptr) {
        clear();
        return false;
    }
    m_type_n = tn;
    m_unit_n = un;
    m_st = &st;
    for (u16 t = 0; t < tn; ++t) {
        m_roster[t] = U16_KEY_NULL;
        m_type_ix[t].m_cnt = 0;
        m_type_ix[t].m_start = U16_KEY_NULL;
    }
    for (u16 i = 0; i < un; ++i) {
        const u16 t = st.unit().get_item(UnitStaticDataKey::from_raw(i)).type;
        if (t < tn) {
            m_type_ix[t].m_cnt = m_type_ix[t].m_cnt + 1u;
        }
    }
    u16 off = 0;
    for (u16 t = 0; t < tn; ++t) {
        m_type_ix[t].m_start = off;
        off = off + m_type_ix[t].m_cnt;
    }
    u16 curs[tn];
    for (u16 t = 0; t < tn; ++t) {
        curs[t] = 0;
    }
    for (u16 i = 0; i < un; ++i) {
        const u16 t = st.unit().get_item(UnitStaticDataKey::from_raw(i)).type;
        if (t >= tn) {
            continue;
        }
        m_pack[m_type_ix[t].m_start + curs[t]] = i;
        curs[t] = curs[t] + 1u;
    }
    const UnitStaticDataStruct* items = &st.unit().get_item(UnitStaticDataKey::from_raw(0));
    for (u16 t = 0; t < tn; ++t) {
        const u16 n = m_type_ix[t].m_cnt;
        if (n <= 1u) {
            continue;
        }
        s_sort_slice(m_pack + m_type_ix[t].m_start, n, items);
    }
    return true;
}

void UnitRosterMng::clear () {
    delete[] m_roster;
    delete[] m_type_ix;
    delete[] m_pack;
    m_roster = nullptr;
    m_type_ix = nullptr;
    m_pack = nullptr;
    m_type_n = 0;
    m_unit_n = 0;
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
            const u32 score = s_score(u);
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
    if (m_st == nullptr || m_type_ix == nullptr || m_pack == nullptr || type_idx >= m_type_n) {
        return U16_KEY_NULL;
    }
    const TypeIx& ix = m_type_ix[type_idx];
    if (ix.m_cnt == 0 || m_unit_n == 0) {
        return U16_KEY_NULL;
    }
    const UnitStaticDataStruct* items = &m_st->unit().get_item(UnitStaticDataKey::from_raw(0));
    for (u16 s = 0; s < ix.m_cnt; ++s) {
        const u16 i = m_pack[ix.m_start + s];
        if (GeneralAssessor::chk(items[i].reqs, ctx)) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

u16 UnitRosterMng::type_n () {
    return m_type_n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
