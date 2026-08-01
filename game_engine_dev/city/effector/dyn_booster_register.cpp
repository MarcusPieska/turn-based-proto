//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "dyn_booster_register.h"

#include "effect_ctx.h"
#include "effect_enabler.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

u16 DynBoosterRegister::key_of (ItemEffectBoosterType tp, ItemEffectsScope sc) {
    return static_cast<u16>(static_cast<u16>(tp) * SCOPE_N + static_cast<u16>(sc));
}

bool DynBoosterRegister::src_on (ItemEffectsScope sc, const EffectEnabler& en, const EffectCtx& ctx) {
    switch (sc) {
    case ItemEffectsScope::LOCAL:
        return effect_enabler_active_local(en, ctx);
    case ItemEffectsScope::CITY:
        return effect_enabler_active_city(en, ctx);
    case ItemEffectsScope::CIV:
        return effect_enabler_active_civ(en, ctx);
    case ItemEffectsScope::GLOBAL:
        return effect_enabler_active_global(en, ctx);
    default:
        return false;
    }
}

//================================================================================================================================
//=> - DynBoosterRegister -
//================================================================================================================================

DynBoosterRegister::DynBoosterRegister () = default;

DynBoosterRegister::~DynBoosterRegister () {
    clear();
}

void DynBoosterRegister::clear () {
    delete[] m_entry;
    delete[] m_off;
    m_entry = nullptr;
    m_off = nullptr;
    m_entry_n = 0;
}

void DynBoosterRegister::take_ownership () {
    if (m_entry != nullptr && m_entry_n > 0) {
        BoosterRegisterEntry* tmp = m_entry;
        m_entry = new BoosterRegisterEntry[m_entry_n];
        for (u16 i = 0; i < m_entry_n; ++i) {
            m_entry[i] = tmp[i];
        }
        delete[] tmp;
    }
    if (m_off != nullptr) {
        u16* tmp = m_off;
        m_off = new u16[KEY_N + 1u];
        for (u16 i = 0; i <= KEY_N; ++i) {
            m_off[i] = tmp[i];
        }
        delete[] tmp;
    }
}

BoosterRegisterResult DynBoosterRegister::determine (
    ItemEffectBoosterType tp, ItemEffectsScope sc, const EffectCtx& ctx) const {
    BoosterRegisterResult out = {};
    if (m_off == nullptr || m_entry_n == 0) {
        return out;
    }
    const u16 tp_u = static_cast<u16>(tp);
    const u16 sc_u = static_cast<u16>(sc);
    if (tp_u >= TYPE_N || sc_u >= SCOPE_N) {
        return out;
    }
    const u16 key = key_of(tp, sc);
    const u16 begin = m_off[key];
    const u16 end = m_off[key + 1u];
    for (u16 i = begin; i < end; ++i) {
        const BoosterRegisterEntry& e = m_entry[i];
        if (!src_on(sc, e.m_enabler, ctx)) {
            continue;
        }
        out.m_unit = static_cast<i16>(static_cast<i32>(out.m_unit) + static_cast<i32>(e.m_unit));
        out.m_perc = static_cast<i16>(static_cast<i32>(out.m_perc) + static_cast<i32>(e.m_perc));
    }
    return out;
}

const BoosterRegisterEntry* DynBoosterRegister::entries (
    ItemEffectBoosterType tp, ItemEffectsScope sc) const {
    if (m_off == nullptr || m_entry == nullptr) {
        return nullptr;
    }
    const u16 tp_u = static_cast<u16>(tp);
    const u16 sc_u = static_cast<u16>(sc);
    if (tp_u >= TYPE_N || sc_u >= SCOPE_N) {
        return nullptr;
    }
    return m_entry + m_off[key_of(tp, sc)];
}

u16 DynBoosterRegister::entry_count (ItemEffectBoosterType tp, ItemEffectsScope sc) const {
    if (m_off == nullptr) {
        return 0;
    }
    const u16 tp_u = static_cast<u16>(tp);
    const u16 sc_u = static_cast<u16>(sc);
    if (tp_u >= TYPE_N || sc_u >= SCOPE_N) {
        return 0;
    }
    const u16 key = key_of(tp, sc);
    return static_cast<u16>(m_off[key + 1u] - m_off[key]);
}

u16 DynBoosterRegister::entry_count () const {
    return m_entry_n;
}

u16 DynBoosterRegister::key_count () const {
    return KEY_N;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
