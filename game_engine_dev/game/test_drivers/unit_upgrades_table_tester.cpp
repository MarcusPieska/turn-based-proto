//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstring>

#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "unit_static_data.h"
#include "unit_static_key.h"
#include "unit_upgrades.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static RuntimeStaticLoader g_rt_loader;
static RuntimeStatics* g_rt_statics = nullptr;
static constexpr cstr OUT_PATH = "/home/w/Projects/simple-map-gen/unit-upgrades/unit_upgrades_table.txt";

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool load_statics () {
    if (g_rt_statics != nullptr) {
        return true;
    }
    if (!g_rt_loader.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        return false;
    }
    g_rt_statics = &g_rt_loader.statics();
    return g_rt_statics != nullptr;
}

enum Filt : u8 {
    FILT_ALL = 0,
    FILT_TYPE = 1,
    FILT_TYPE_ROLE = 2
};

static bool ok_tgt (u16 from, u16 to, const UnitStaticData& units, Filt filt) {
    if (!UnitUpgrades::can_upgrade(from, to, units)) {
        return false;
    }
    if (filt == FILT_ALL) {
        return true;
    }
    const UnitStaticDataStruct& fu = units.get_item(UnitStaticDataKey::from_raw(from));
    const UnitStaticDataStruct& tu = units.get_item(UnitStaticDataKey::from_raw(to));
    if (tu.type != fu.type) {
        return false;
    }
    if (filt == FILT_TYPE_ROLE && tu.role != fu.role) {
        return false;
    }
    return true;
}

static void wr_table (FILE* fp, cstr title, const UnitStaticData& units, Filt filt) {
    const u16 n = units.get_item_count();
    std::fprintf(fp, "%s\n", title);
    for (u16 from = 0; from < n; ++from) {
        u16 n_tg = 0u;
        for (u16 to = 0; to < n; ++to) {
            if (ok_tgt(from, to, units, filt)) {
                n_tg = n_tg + 1u;
            }
        }
        const cstr from_nm = units.get_name(UnitStaticDataKey::from_raw(from));
        std::fprintf(fp, "%s (targets=%u):", from_nm, static_cast<unsigned>(n_tg));
        bool any = false;
        for (u16 to = 0; to < n; ++to) {
            if (!ok_tgt(from, to, units, filt)) {
                continue;
            }
            const cstr to_nm = units.get_name(UnitStaticDataKey::from_raw(to));
            if (!any) {
                std::fprintf(fp, " %s", to_nm);
                any = true;
            } else {
                std::fprintf(fp, ", %s", to_nm);
            }
        }
        std::fprintf(fp, "\n");
    }
    std::fprintf(fp, "\n");
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!load_statics()) {
        std::printf("ERROR: failed to load runtime statics\n");
        return 1;
    }

    const UnitStaticData& units = g_rt_statics->unit();
    const u16 n = units.get_item_count();
    u64 calls = 0u;
    u64 hits = 0u;

    const auto t0 = std::chrono::steady_clock::now();
    for (u16 from = 0; from < n; ++from) {
        for (u16 to = 0; to < n; ++to) {
            if (UnitUpgrades::can_upgrade(from, to, units)) {
                hits = hits + 1u;
            }
            calls = calls + 1u;
        }
    }
    const auto t1 = std::chrono::steady_clock::now();
    const f64 us = std::chrono::duration<f64, std::micro>(t1 - t0).count();
    std::printf("can_upgrade timed: n=%u pairs=%llu hits=%llu us=%.3f\n",
        static_cast<unsigned>(n),
        static_cast<unsigned long long>(calls),
        static_cast<unsigned long long>(hits),
        us);

    FILE* fp = std::fopen(OUT_PATH, "w");
    if (fp == nullptr) {
        std::printf("ERROR: failed to open %s\n", OUT_PATH);
        return 1;
    }
    wr_table(fp, "=== all can_upgrade ===", units, FILT_ALL);
    wr_table(fp, "=== same type only ===", units, FILT_TYPE);
    wr_table(fp, "=== same type and role ===", units, FILT_TYPE_ROLE);
    std::fclose(fp);
    std::printf("wrote %s\n", OUT_PATH);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
