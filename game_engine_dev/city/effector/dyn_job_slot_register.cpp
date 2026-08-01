//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "dyn_job_slot_register.h"

#include "bit_array.h"
#include "effect_ctx.h"
#include "general_bit_bank.h"

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

static i16 add_sat (i16 a, i16 b) {
    const i32 next = static_cast<i32>(a) + static_cast<i32>(b);
    if (next < -32768) {
        return -32768;
    }
    if (next > 32767) {
        return 32767;
    }
    return static_cast<i16>(next);
}

//================================================================================================================================
//=> - DynJobSlotRegister -
//================================================================================================================================

DynJobSlotRegister::DynJobSlotRegister () = default;

DynJobSlotRegister::~DynJobSlotRegister () {
    clear();
}

void DynJobSlotRegister::clear () {
    delete[] m_entry;
    delete[] m_off;
    m_entry = nullptr;
    m_off = nullptr;
    m_entry_n = 0;
    m_job_n = 0;
}

void DynJobSlotRegister::take_ownership () {
    if (m_entry != nullptr && m_entry_n > 0) {
        DynJobSlotEntry* tmp = m_entry;
        m_entry = new DynJobSlotEntry[m_entry_n];
        for (u16 i = 0; i < m_entry_n; ++i) {
            m_entry[i] = tmp[i];
        }
        delete[] tmp;
    }
    if (m_off != nullptr && m_job_n > 0) {
        u16* tmp = m_off;
        m_off = new u16[m_job_n + 1u];
        for (u16 i = 0; i <= m_job_n; ++i) {
            m_off[i] = tmp[i];
        }
        delete[] tmp;
    }
}

i16 DynJobSlotRegister::bonus (u16 job_id, u16 base_slots, const EffectCtx& ctx) const {
    if (job_id >= m_job_n || m_entry_n == 0) {
        return 0;
    }
    const u16 begin = m_off[job_id];
    const u16 end = m_off[job_id + 1u];
    i16 sum = 0;
    for (u16 i = begin; i < end; ++i) {
        const DynJobSlotEntry& e = m_entry[i];
        if (!src_on(e.m_enabler, ctx)) {
            continue;
        }
        if (e.m_mode == ItemEffectAmountMode::PERCENTAGE) {
            const i32 part = (static_cast<i32>(base_slots) * static_cast<i32>(e.m_amount)) / 100;
            sum = add_sat(sum, static_cast<i16>(part));
            continue;
        }
        sum = add_sat(sum, e.m_amount);
    }
    return sum;
}

u16 DynJobSlotRegister::capacity (u16 job_id, u16 base_slots, const EffectCtx& ctx) const {
    const i32 total = static_cast<i32>(base_slots) + static_cast<i32>(bonus(job_id, base_slots, ctx));
    if (total <= 0) {
        return 0;
    }
    if (total > 65535) {
        return 65535;
    }
    return static_cast<u16>(total);
}

const DynJobSlotEntry* DynJobSlotRegister::entries () const {
    return m_entry;
}

u16 DynJobSlotRegister::entry_count () const {
    return m_entry_n;
}

u16 DynJobSlotRegister::job_count () const {
    return m_job_n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
