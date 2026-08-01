//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "dyn_produce_register.h"

#include "bit_array.h"
#include "effect_ctx.h"
#include "general_bit_bank.h"

#include <cstring>

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static bool src_on (const EffectEnabler& en, const EffectCtx& ctx) {
    switch (en.m_kind) {
    case EffectEnablerKind::BUILDING:
        if (ctx.m_bld_bank == nullptr) {
            return false;
        }
        return ctx.m_bld_bank->is_flagged(ctx.m_city_idx, en.m_idx);
    case EffectEnablerKind::TECH:
        if (ctx.m_tech == nullptr || en.m_idx >= ctx.m_tech->get_count()) {
            return false;
        }
        return ctx.m_tech->get_bit(en.m_idx) == 1;
    case EffectEnablerKind::SMALL_WONDER:
        if (ctx.m_small_wonder_city == nullptr || en.m_idx >= ctx.m_small_wonder_n) {
            return false;
        }
        return ctx.m_small_wonder_city[en.m_idx] == ctx.m_city_idx;
    case EffectEnablerKind::WONDER:
        if (ctx.m_wonder_city == nullptr || en.m_idx >= ctx.m_wonder_n) {
            return false;
        }
        return ctx.m_wonder_city[en.m_idx] == ctx.m_city_idx;
    default:
        return false;
    }
}

static void add_sat (i16& dst, i16 delta) {
    const i32 next = static_cast<i32>(dst) + static_cast<i32>(delta);
    if (next < -32768) {
        dst = -32768;
    } else if (next > 32767) {
        dst = 32767;
    } else {
        dst = static_cast<i16>(next);
    }
}

static bool group_can_pay (const DynProduceGroup& g, const DynProduceRegister::Inventory& inv) {
    i16 need_amt[MAX_EFFECT_COUNT];
    u16 need_tgt[MAX_EFFECT_COUNT];
    u8 need_n = 0;
    for (u8 s = 0; s < g.m_slot_n; ++s) {
        const DynProduceSlot& sl = g.m_slots[s];
        if (sl.m_kind != ItemProduceKind::RESOURCE || sl.m_amount >= 0) {
            continue;
        }
        if (inv.m_v == nullptr || sl.m_target >= inv.m_n) {
            return false;
        }
        u8 found = 0;
        for (u8 k = 0; k < need_n; ++k) {
            if (need_tgt[k] == sl.m_target) {
                add_sat(need_amt[k], static_cast<i16>(-sl.m_amount));
                found = 1;
                break;
            }
        }
        if (found == 0) {
            need_tgt[need_n] = sl.m_target;
            need_amt[need_n] = static_cast<i16>(-sl.m_amount);
            ++need_n;
        }
    }
    for (u8 k = 0; k < need_n; ++k) {
        if (inv.m_v[need_tgt[k]] < need_amt[k]) {
            return false;
        }
    }
    return true;
}

static void apply_slot (DynProduceRegister::Inventory& inv, DynProduceRegister::CityYields& yld,
    const DynProduceSlot& sl, u8* touch_res, u8* touch_yld) {
    if (sl.m_kind == ItemProduceKind::RESOURCE) {
        if (inv.m_v == nullptr || sl.m_target >= inv.m_n) {
            return;
        }
        add_sat(inv.m_v[sl.m_target], sl.m_amount);
        if (touch_res != nullptr) {
            touch_res[sl.m_target] = 1;
        }
        return;
    }
    if (sl.m_kind == ItemProduceKind::YIELD) {
        if (sl.m_target == 0 || sl.m_target >= DynProduceRegister::YIELD_N) {
            return;
        }
        add_sat(yld.m_v[sl.m_target], sl.m_amount);
        if (touch_yld != nullptr) {
            touch_yld[sl.m_target] = 1;
        }
    }
}

static void clamp_marked (i16* v, const u8* flags, u16 n) {
    for (u16 i = 0; i < n; ++i) {
        if (flags[i] != 0 && v[i] < 0) {
            v[i] = 0;
        }
    }
}

//================================================================================================================================
//=> - DynProduceRegister -
//================================================================================================================================

DynProduceRegister::DynProduceRegister () = default;

DynProduceRegister::~DynProduceRegister () {
    clear();
}

void DynProduceRegister::clear () {
    delete[] m_group;
    delete[] m_act;
    delete[] m_touch_res;
    delete[] m_res_ids;
    delete[] m_res_map;
    m_group = nullptr;
    m_act = nullptr;
    m_touch_res = nullptr;
    m_res_ids = nullptr;
    m_res_map = nullptr;
    m_group_n = 0;
    m_inv_n = 0;
    m_res_n = 0;
}

void DynProduceRegister::take_ownership () {
    if (m_group != nullptr && m_group_n > 0) {
        DynProduceGroup* tmp = m_group;
        m_group = new DynProduceGroup[m_group_n];
        for (u16 i = 0; i < m_group_n; ++i) {
            m_group[i] = tmp[i];
        }
        delete[] tmp;
    }
    if (m_res_ids != nullptr && m_inv_n > 0) {
        u16* tmp = m_res_ids;
        m_res_ids = new u16[m_inv_n];
        for (u16 i = 0; i < m_inv_n; ++i) {
            m_res_ids[i] = tmp[i];
        }
        delete[] tmp;
    }
    if (m_res_map != nullptr && m_res_n > 0) {
        u16* tmp = m_res_map;
        m_res_map = new u16[m_res_n];
        for (u16 i = 0; i < m_res_n; ++i) {
            m_res_map[i] = tmp[i];
        }
        delete[] tmp;
    }
    delete[] m_act;
    delete[] m_touch_res;
    m_act = (m_group_n > 0) ? new u16[m_group_n] : nullptr;
    m_touch_res = (m_inv_n > 0) ? new u8[m_inv_n] : nullptr;
}

void DynProduceRegister::clear_yld (CityYields& yld) const {
    std::memset(yld.m_v, 0, sizeof(yld.m_v));
}

void DynProduceRegister::apply (Inventory& inv, CityYields& yld, const EffectCtx& ctx) const {
    if (m_group_n == 0) {
        return;
    }
    u16 act_n = 0;
    for (u16 i = 0; i < m_group_n; ++i) {
        if (!src_on(m_group[i].m_enabler, ctx)) {
            continue;
        }
        m_act[act_n++] = i;
    }
    if (act_n == 0) {
        return;
    }
    u8 touch_yld[YIELD_N];
    u32 applied = 0;
    for (u16 a = 0; a < act_n; ++a) {
        const DynProduceGroup& g = m_group[m_act[a]];
        if (!group_can_pay(g, inv)) {
            continue;
        }
        if (applied == 0) {
            if (m_touch_res != nullptr && m_inv_n > 0) {
                std::memset(m_touch_res, 0, m_inv_n);
            }
            std::memset(touch_yld, 0, sizeof(touch_yld));
        }
        for (u8 s = 0; s < g.m_slot_n; ++s) {
            apply_slot(inv, yld, g.m_slots[s], m_touch_res, touch_yld);
        }
        ++applied;
    }
    if (applied == 0) {
        return;
    }
    if (inv.m_v != nullptr && m_touch_res != nullptr) {
        clamp_marked(inv.m_v, m_touch_res, inv.m_n);
    }
    clamp_marked(yld.m_v, touch_yld, YIELD_N);
}

const DynProduceGroup* DynProduceRegister::groups () const {
    return m_group;
}

u16 DynProduceRegister::group_count () const {
    return m_group_n;
}

u16 DynProduceRegister::inv_n () const {
    return m_inv_n;
}

u16 DynProduceRegister::res_n () const {
    return m_res_n;
}

u16 DynProduceRegister::res_id (u16 dense) const {
    if (m_res_ids == nullptr || dense >= m_inv_n) {
        return U16_KEY_NULL;
    }
    return m_res_ids[dense];
}

u16 DynProduceRegister::dense_of (u16 catalog_id) const {
    if (m_res_map == nullptr || catalog_id >= m_res_n) {
        return U16_KEY_NULL;
    }
    return m_res_map[catalog_id];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
