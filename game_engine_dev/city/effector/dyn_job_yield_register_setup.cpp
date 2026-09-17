//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "dyn_job_yield_register_setup.h"

#include "city_job_static_key.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static i16 yield_of (const CityJobStaticDataStruct& j, DynJobYield y) {
    switch (y) {
    case DynJobYield::FOOD: return j.food;
    case DynJobYield::PRODUCTION: return static_cast<i16>(j.production);
    case DynJobYield::COMMERCE: return static_cast<i16>(j.commerce);
    case DynJobYield::CULTURE: return static_cast<i16>(j.culture);
    case DynJobYield::SCIENCE: return static_cast<i16>(j.science);
    case DynJobYield::RELIGION: return static_cast<i16>(j.religion);
    default: return 0;
    }
}

static void sort_group (DynJobYieldEntry* a, u16 n) {
    for (u16 i = 1; i < n; ++i) {
        DynJobYieldEntry key = a[i];
        u16 j = i;
        while (j > 0) {
            const DynJobYieldEntry& prev = a[j - 1u];
            const bool worse = (prev.m_score < key.m_score) ||
                (prev.m_score == key.m_score && prev.m_job_id > key.m_job_id);
            if (!worse) {
                break;
            }
            a[j] = prev;
            --j;
        }
        a[j] = key;
    }
}

//================================================================================================================================
//=> - DynJobYieldRegisterSetup -
//================================================================================================================================

bool DynJobYieldRegisterSetup::build (const RuntimeStatics& st, DynJobYieldRegister& out) {
    out.clear();
    const u16 job_n = st.city_job().get_item_count();
    if (job_n == 0) {
        return false;
    }
    u16 counts[DynJobYieldRegister::YIELD_N] = {};
    u16 total = 0;
    for (u16 jid = 0; jid < job_n; ++jid) {
        const CityJobStaticDataStruct& job = st.city_job().get_item(CityJobStaticDataKey::from_raw(jid));
        for (u16 yi = 0; yi < DynJobYieldRegister::YIELD_N; ++yi) {
            if (yield_of(job, static_cast<DynJobYield>(yi)) > 0) {
                ++counts[yi];
                ++total;
            }
        }
    }
    out.m_job_n = job_n;
    out.m_entry_n = total;
    out.m_off = new u16[DynJobYieldRegister::YIELD_N + 1u];
    out.m_off[0] = 0;
    for (u16 yi = 0; yi < DynJobYieldRegister::YIELD_N; ++yi) {
        out.m_off[yi + 1u] = static_cast<u16>(out.m_off[yi] + counts[yi]);
    }
    out.m_entry = (total > 0) ? new DynJobYieldEntry[total] : nullptr;
    out.m_row = new DynJobYieldRow[job_n];
    out.m_remain = new u16[job_n];
    out.m_taken = new u16[job_n];
    for (u16 jid = 0; jid < job_n; ++jid) {
        const CityJobStaticDataStruct& job = st.city_job().get_item(CityJobStaticDataKey::from_raw(jid));
        DynJobYieldRow& r = out.m_row[jid];
        r.m_food = job.food;
        r.m_production = static_cast<i16>(job.production);
        r.m_commerce = static_cast<i16>(job.commerce);
        r.m_culture = static_cast<i16>(job.culture);
        r.m_science = static_cast<i16>(job.science);
        r.m_religion = static_cast<i16>(job.religion);
        r.m_slots = job.slots;
    }
    out.reset_remain();
    u16 curs[DynJobYieldRegister::YIELD_N];
    for (u16 yi = 0; yi < DynJobYieldRegister::YIELD_N; ++yi) {
        curs[yi] = out.m_off[yi];
    }
    for (u16 jid = 0; jid < job_n; ++jid) {
        const CityJobStaticDataStruct& job = st.city_job().get_item(CityJobStaticDataKey::from_raw(jid));
        for (u16 yi = 0; yi < DynJobYieldRegister::YIELD_N; ++yi) {
            const i16 score = yield_of(job, static_cast<DynJobYield>(yi));
            if (score <= 0) {
                continue;
            }
            DynJobYieldEntry& e = out.m_entry[curs[yi]++];
            e.m_job_id = jid;
            e.m_score = score;
        }
    }
    for (u16 yi = 0; yi < DynJobYieldRegister::YIELD_N; ++yi) {
        const u16 begin = out.m_off[yi];
        const u16 n = static_cast<u16>(out.m_off[yi + 1u] - begin);
        if (n > 1 && out.m_entry != nullptr) {
            sort_group(out.m_entry + begin, n);
        }
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
