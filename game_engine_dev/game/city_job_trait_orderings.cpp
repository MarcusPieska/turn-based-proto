//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_job_trait_orderings.h"

#include <cstdlib>
#include <cstring>

#include "bit_array.h"
#include "city_job_static_data.h"
#include "city_job_trait_attribution.h"
#include "civ_trait_affinity.h"
#include "trait_affinity_map.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

u16* CityJobTraitOrderings::m_orders = nullptr;
u16 CityJobTraitOrderings::m_n = 0;

//================================================================================================================================
//=> - Score helpers -
//================================================================================================================================

struct JobOrdRow {
    u16 m_idx;
    i32 m_score;
};

static i32 job_score (CivTrait owner, u16 j) {
    i32 s = static_cast<i32>(CityJobTraitAttribution::base(j));
    for (u16 ti = 0; ti < CivTraitAffinity::k_n; ++ti) {
        const CivTrait tag = static_cast<CivTrait>(ti);
        if (!CityJobTraitAttribution::has(j, tag)) {
            continue;
        }
        s = s + static_cast<i32>(CivTraitAffinity::affinity(owner, tag));
    }
    return s;
}

static int job_ord_cmp (const void* a, const void* b) {
    const JobOrdRow* ra = static_cast<const JobOrdRow*>(a);
    const JobOrdRow* rb = static_cast<const JobOrdRow*>(b);
    if (ra->m_score > rb->m_score) {
        return -1;
    }
    if (ra->m_score < rb->m_score) {
        return 1;
    }
    if (ra->m_idx < rb->m_idx) {
        return -1;
    }
    if (ra->m_idx > rb->m_idx) {
        return 1;
    }
    return 0;
}

//================================================================================================================================
//=> - CityJobTraitOrderings -
//================================================================================================================================

bool CityJobTraitOrderings::begin (const CityJobStaticData& jobs, const TraitAffinityMap& aff) {
    clear();
    if (!CityJobTraitAttribution::ready()) {
        if (!CityJobTraitAttribution::begin(jobs, aff)) {
            return false;
        }
    }
    const u16 n = CityJobTraitAttribution::job_n();
    if (n == 0 || n != jobs.get_item_count()) {
        return false;
    }
    const u32 bytes = static_cast<u32>(CivTraitAffinity::k_n) * static_cast<u32>(n) * sizeof(u16);
    u16* orders = static_cast<u16*>(std::malloc(bytes));
    JobOrdRow* rows = static_cast<JobOrdRow*>(std::malloc(static_cast<size_t>(n) * sizeof(JobOrdRow)));
    if (orders == nullptr || rows == nullptr) {
        std::free(orders);
        std::free(rows);
        return false;
    }

    m_orders = orders;
    m_n = n;

    for (u16 ti = 0; ti < CivTraitAffinity::k_n; ++ti) {
        const CivTrait owner = static_cast<CivTrait>(ti);
        for (u16 j = 0; j < n; ++j) {
            rows[j].m_idx = j;
            rows[j].m_score = job_score(owner, j);
        }
        std::qsort(rows, static_cast<size_t>(n), sizeof(JobOrdRow), job_ord_cmp);
        u16* row = orders + static_cast<u32>(ti) * static_cast<u32>(n);
        for (u16 i = 0; i < n; ++i) {
            row[i] = rows[i].m_idx;
        }
    }

    std::free(rows);
    return true;
}

void CityJobTraitOrderings::clear () {
    if (m_orders != nullptr) {
        std::free(m_orders);
        m_orders = nullptr;
    }
    m_n = 0;
}

bool CityJobTraitOrderings::ready () {
    return m_orders != nullptr && m_n > 0;
}

u16 CityJobTraitOrderings::job_n () {
    return m_n;
}

u16 CityJobTraitOrderings::at (u16 trait_idx, u16 slot) {
    if (m_orders == nullptr || trait_idx >= CivTraitAffinity::k_n || slot >= m_n) {
        return U16_KEY_NULL;
    }
    return m_orders[static_cast<u32>(trait_idx) * static_cast<u32>(m_n) + static_cast<u32>(slot)];
}

u16 CityJobTraitOrderings::pick (const BitArrayCL& available, u16 trait_idx, u16 start_slot) {
    if (m_orders == nullptr || trait_idx >= CivTraitAffinity::k_n) {
        return U16_KEY_NULL;
    }
    const u16* row = m_orders + static_cast<u32>(trait_idx) * static_cast<u32>(m_n);
    const u32 avail_n = available.get_count();
    for (u16 s = start_slot; s < m_n; ++s) {
        const u16 j = row[s];
        if (static_cast<u32>(j) >= avail_n) {
            continue;
        }
        if (available.get_bit(j) != 0) {
            return j;
        }
    }
    return U16_KEY_NULL;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
