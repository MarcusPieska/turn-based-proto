//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "job_count_array.h"

#include "city_job_static_data.h"
#include "city_job_static_key.h"
#include "dyn_job_slot_register.h"
#include "general_bit_bank.h"
#include "item_effects.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

GeneralBitBank* JobCountArray::s_bank = nullptr;
JobCountSlot* JobCountArray::s_slots = nullptr;
u16 JobCountArray::s_job_n = 0;
u16 JobCountArray::s_bits = 0;
bool JobCountArray::s_ready = false;

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static u8 s_wid (u8 nv) {
    if (nv <= 1) {
        return 0;
    }
    u8 w = 0;
    for (u8 x = SC_U8(nv - 1); x != 0; x = SC_U8(x >> 1)) {
        ++w;
    }
    return w;
}

static u8 s_theor_max (const DynJobSlotRegister& reg, u16 job_id, u16 base) {
    i32 count_sum = 0;
    i32 pct_sum = 0;
    const DynJobSlotEntry* ents = reg.entries();
    const u16 en = reg.entry_count();
    for (u16 i = 0; i < en; ++i) {
        if (ents[i].m_job_id != job_id) {
            continue;
        }
        if (ents[i].m_mode == ItemEffectAmountMode::PERCENTAGE) {
            pct_sum += SC_I32(ents[i].m_amount);
        } else {
            count_sum += SC_I32(ents[i].m_amount);
        }
    }
    i32 total = SC_I32(base) + count_sum + (SC_I32(base) * pct_sum) / 100;
    if (total < 0) {
        total = 0;
    }
    if (total > 255) {
        total = 255;
    }
    return SC_U8(total);
}

//================================================================================================================================
//=> - JobCountArray -
//================================================================================================================================

bool JobCountArray::setup (const RuntimeStatics& st) {
    clear();
    const CityJobStaticData& jobs = st.city_job();
    const DynJobSlotRegister& slots_reg = st.dyn_job_slot();
    const u16 jn = jobs.get_item_count();
    if (jn == 0) {
        return false;
    }
    s_slots = new JobCountSlot[jn];
    if (s_slots == nullptr) {
        return false;
    }
    u32 bits = 0;
    for (u16 j = 0; j < jn; ++j) {
        const CityJobStaticDataStruct& row = jobs.get_item(CityJobStaticDataKey::from_raw(j));
        const u8 mx = s_theor_max(slots_reg, j, row.slots);
        const u8 vals = SC_U8(mx + 1u);
        s_slots[j].m_off = SC_U16(bits);
        s_slots[j].m_wid = s_wid(vals);
        s_slots[j].m_max = mx;
        bits += SC_U32(s_slots[j].m_wid);
    }
    if (bits == 0) {
        bits = 1;
    }
    if (bits > 65535u) {
        clear();
        return false;
    }
    s_job_n = jn;
    s_bits = SC_U16(bits);
    s_bank = new GeneralBitBank(s_bits);
    if (s_bank == nullptr) {
        clear();
        return false;
    }
    s_ready = true;
    return true;
}

void JobCountArray::clear () {
    delete s_bank;
    delete[] s_slots;
    s_bank = nullptr;
    s_slots = nullptr;
    s_job_n = 0;
    s_bits = 0;
    s_ready = false;
}

bool JobCountArray::ready () {
    return s_ready;
}

u16 JobCountArray::claim () {
    if (!s_ready || s_bank == nullptr) {
        return U16_KEY_NULL;
    }
    return s_bank->claim_batch();
}

u16 JobCountArray::job_n () {
    return s_job_n;
}

u16 JobCountArray::bits () {
    return s_bits;
}

u8 JobCountArray::max_n (u16 job_id) {
    if (!s_ready || job_id >= s_job_n) {
        return 0;
    }
    return s_slots[job_id].m_max;
}

u8 JobCountArray::get (u16 city_idx, u16 job_id) {
    if (!s_ready || s_bank == nullptr || job_id >= s_job_n) {
        return 0;
    }
    const JobCountSlot& s = s_slots[job_id];
    return SC_U8(s_bank->get_slice(city_idx, s.m_off, s.m_wid));
}

void JobCountArray::set (u16 city_idx, u16 job_id, u8 n) {
    if (!s_ready || s_bank == nullptr || job_id >= s_job_n) {
        return;
    }
    const JobCountSlot& s = s_slots[job_id];
    if (n > s.m_max) {
        n = s.m_max;
    }
    s_bank->set_slice(city_idx, s.m_off, s.m_wid, SC_U32(n));
}

GeneralBitBank* JobCountArray::bank () {
    return s_bank;
}

const JobCountSlot* JobCountArray::slots () {
    return s_slots;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
