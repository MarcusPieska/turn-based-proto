//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "dyn_job_yield_register.h"

#include <cstring>

#include "dyn_job_slot_register.h"
#include "effect_ctx.h"

//================================================================================================================================
//=> - DynJobYieldRegister -
//================================================================================================================================

DynJobYieldRegister::DynJobYieldRegister () = default;

DynJobYieldRegister::~DynJobYieldRegister () {
    clear();
}

void DynJobYieldRegister::clear () {
    delete[] m_entry;
    delete[] m_off;
    delete[] m_row;
    delete[] m_remain;
    delete[] m_taken;
    m_entry = nullptr;
    m_off = nullptr;
    m_row = nullptr;
    m_remain = nullptr;
    m_taken = nullptr;
    clear_pack();
    m_entry_n = 0;
    m_job_n = 0;
}

void DynJobYieldRegister::take_ownership () {
    if (m_entry != nullptr && m_entry_n > 0) {
        DynJobYieldEntry* tmp = m_entry;
        m_entry = new DynJobYieldEntry[m_entry_n];
        for (u16 i = 0; i < m_entry_n; ++i) {
            m_entry[i] = tmp[i];
        }
        delete[] tmp;
    }
    if (m_off != nullptr) {
        u16* tmp = m_off;
        m_off = new u16[YIELD_N + 1u];
        for (u16 i = 0; i <= YIELD_N; ++i) {
            m_off[i] = tmp[i];
        }
        delete[] tmp;
    }
    if (m_row != nullptr && m_job_n > 0) {
        DynJobYieldRow* tmp = m_row;
        m_row = new DynJobYieldRow[m_job_n];
        for (u16 i = 0; i < m_job_n; ++i) {
            m_row[i] = tmp[i];
        }
        delete[] tmp;
    }
    if (m_remain != nullptr && m_job_n > 0) {
        u16* tmp = m_remain;
        m_remain = new u16[m_job_n];
        for (u16 i = 0; i < m_job_n; ++i) {
            m_remain[i] = tmp[i];
        }
        delete[] tmp;
    }
    if (m_taken != nullptr && m_job_n > 0) {
        u16* tmp = m_taken;
        m_taken = new u16[m_job_n];
        for (u16 i = 0; i < m_job_n; ++i) {
            m_taken[i] = tmp[i];
        }
        delete[] tmp;
    }
}

void DynJobYieldRegister::clear_pack () {
    m_pack.m_food = 0;
    m_pack.m_production = 0;
    m_pack.m_commerce = 0;
    m_pack.m_culture = 0;
    m_pack.m_science = 0;
    m_pack.m_religion = 0;
    m_pack.m_n = 0;
}

DynJobYieldPack& DynJobYieldRegister::reset_remain () {
    clear_pack();
    if (m_remain != nullptr && m_job_n > 0) {
        std::memset(m_remain, 0xFF, static_cast<size_t>(m_job_n) * sizeof(u16));
    }
    if (m_taken != nullptr && m_job_n > 0) {
        std::memset(m_taken, 0, static_cast<size_t>(m_job_n) * sizeof(u16));
    }
    return m_pack;
}

u16* DynJobYieldRegister::remain () {
    return m_remain;
}

const u16* DynJobYieldRegister::remain () const {
    return m_remain;
}

u16* DynJobYieldRegister::taken () {
    return m_taken;
}

const u16* DynJobYieldRegister::taken () const {
    return m_taken;
}

DynJobYieldPack& DynJobYieldRegister::pack () {
    return m_pack;
}

const DynJobYieldPack& DynJobYieldRegister::pack () const {
    return m_pack;
}

u16 DynJobYieldRegister::fill (
    DynJobYield prefer,
    u16 pop_limit,
    const DynJobSlotRegister& slots,
    const EffectCtx& ctx) {
    if (m_entry == nullptr || m_row == nullptr || m_remain == nullptr || m_taken == nullptr) {
        return 0;
    }
    const u16 yi = static_cast<u16>(prefer);
    if (yi >= YIELD_N || pop_limit == 0 || m_pack.m_n >= pop_limit) {
        return 0;
    }
    u16 left = static_cast<u16>(pop_limit - m_pack.m_n);
    const u16 begin_n = m_pack.m_n;
    const u16 begin = m_off[yi];
    const u16 end = m_off[yi + 1u];
    for (u16 i = begin; i < end && left > 0; ++i) {
        const DynJobYieldEntry& e = m_entry[i];
        if (e.m_job_id >= m_job_n) {
            continue;
        }
        if (m_remain[e.m_job_id] == U16_KEY_NULL) {
            m_remain[e.m_job_id] = slots.capacity(e.m_job_id, m_row[e.m_job_id].m_slots, ctx);
        }
        u16 take = m_remain[e.m_job_id];
        if (take > left) {
            take = left;
        }
        if (take == 0) {
            continue;
        }
        m_remain[e.m_job_id] = static_cast<u16>(m_remain[e.m_job_id] - take);
        m_taken[e.m_job_id] = static_cast<u16>(m_taken[e.m_job_id] + take);
        left = static_cast<u16>(left - take);
        m_pack.m_n = static_cast<u16>(m_pack.m_n + take);
        const DynJobYieldRow& r = m_row[e.m_job_id];
        const i32 n = static_cast<i32>(take);
        m_pack.m_food += n * static_cast<i32>(r.m_food);
        m_pack.m_production += n * static_cast<i32>(r.m_production);
        m_pack.m_commerce += n * static_cast<i32>(r.m_commerce);
        m_pack.m_culture += n * static_cast<i32>(r.m_culture);
        m_pack.m_science += n * static_cast<i32>(r.m_science);
        m_pack.m_religion += n * static_cast<i32>(r.m_religion);
    }
    return static_cast<u16>(m_pack.m_n - begin_n);
}

const DynJobYieldEntry* DynJobYieldRegister::entries () const {
    return m_entry;
}

u16 DynJobYieldRegister::entry_count () const {
    return m_entry_n;
}

u16 DynJobYieldRegister::job_count () const {
    return m_job_n;
}

u16 DynJobYieldRegister::group_begin (DynJobYield y) const {
    const u16 yi = static_cast<u16>(y);
    if (m_off == nullptr || yi >= YIELD_N) {
        return 0;
    }
    return m_off[yi];
}

u16 DynJobYieldRegister::group_end (DynJobYield y) const {
    const u16 yi = static_cast<u16>(y);
    if (m_off == nullptr || yi >= YIELD_N) {
        return 0;
    }
    return m_off[yi + 1u];
}

const DynJobYieldRow* DynJobYieldRegister::rows () const {
    return m_row;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
