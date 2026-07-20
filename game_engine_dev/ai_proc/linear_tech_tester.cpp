//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>

#include "bit_array.h"
#include "general_assessor.h"
#include "linear_tech.h"
#include "static_parsing_manager.h"
#include "tech_static_data.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u32 count_set (const BitArrayCL& ba) {
    u32 n = 0;
    const u32 lim = ba.get_count();
    for (u32 i = 0; i < lim; ++i) {
        if (ba.get_bit(i) != 0) {
            n = n + 1u;
        }
    }
    return n;
}

static void clr_owned (BitArrayCL& available, const BitArrayCL& owned) {
    const u32 n = available.get_count();
    for (u32 i = 0; i < n; ++i) {
        if (owned.get_bit(i) != 0) {
            available.clear_bit(i);
        }
    }
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    StaticParsingManager mgr("../");
    const u16 tech_n = mgr.get_tech_count();
    if (tech_n == 0) {
        std::printf("no techs loaded\n");
        return 1;
    }
    const TechStaticDataStruct* items = mgr.get_tech_data();
    if (items == nullptr) {
        std::printf("tech data null\n");
        return 1;
    }

    BitArrayCL owned(tech_n);
    BitArrayCL resource(mgr.get_resource_count());
    BitArrayCL building(mgr.get_building_count());
    BitArrayCL city_flag(mgr.get_city_flag_count());
    BitArrayCL civ(mgr.get_civ_count());
    for (u32 i = 0; i < resource.get_count(); ++i) {
        resource.set_bit(i);
    }
    for (u32 i = 0; i < city_flag.get_count(); ++i) {
        city_flag.set_bit(i);
    }
    for (u32 i = 0; i < civ.get_count(); ++i) {
        civ.set_bit(i);
    }

    AssessorCtx ctx = {};
    ctx.m_tech = &owned;
    ctx.m_civ = &civ;
    ctx.m_city_idx = 0;
    ctx.m_resource_bank = nullptr;
    ctx.m_building_bank = nullptr;
    ctx.m_city_flag_bank = nullptr;
    ctx.m_resource = &resource;
    ctx.m_building = &building;
    ctx.m_city_flag = &city_flag;

    u32 step = 0;
    u64 pick_ns = 0;
    u32 pick_n = 0;
    while (count_set(owned) < static_cast<u32>(tech_n)) {
        BitArrayCL available(tech_n);
        GeneralAssessor::assess_tech(&available, tech_n, items, ctx);
        clr_owned(available, owned);
        if (count_set(available) == 0) {
            std::printf("stop: zero options after %u picks (%u / %u owned)\n",
                step, count_set(owned), static_cast<u32>(tech_n));
            break;
        }
        u16 pick = U16_KEY_NULL;
        const auto t0 = std::chrono::steady_clock::now();
        const bool ok = LinearTech::pick(available, &pick);
        const auto t1 = std::chrono::steady_clock::now();
        pick_ns = pick_ns + static_cast<u64>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        pick_n = pick_n + 1u;
        if (!ok) {
            std::printf("stop: LinearTech pick failed after %u picks\n", step);
            break;
        }
        const char* nm = mgr.get_tech_name_parser().idx_to_name(pick);
        std::printf("pick %u: idx=%u name=%s\n", step, pick, nm != nullptr ? nm : "?");
        std::printf("  options:");
        for (u32 i = 0; i < available.get_count(); ++i) {
            if (available.get_bit(i) == 0) {
                continue;
            }
            const char* onm = mgr.get_tech_name_parser().idx_to_name(static_cast<u16>(i));
            std::printf(" %u:%s", i, onm != nullptr ? onm : "?");
        }
        std::printf("\n");
        owned.set_bit(pick);
        step = step + 1u;
    }
    if (count_set(owned) == static_cast<u32>(tech_n)) {
        std::printf("done: full tech array (%u)\n", tech_n);
    }
    const double sum_us = static_cast<double>(pick_ns) / 1000.0;
    const double avg_us = (pick_n == 0) ? 0.0 : sum_us / static_cast<double>(pick_n);
    std::printf("LinearTech::pick: sum=%.2f us  avg=%.2f us  calls=%u\n", sum_us, avg_us, pick_n);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
