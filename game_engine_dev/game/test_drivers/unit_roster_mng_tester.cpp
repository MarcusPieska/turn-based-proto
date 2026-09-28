//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "bit_array.h"
#include "civ_static_data.h"
#include "civ_static_key.h"
#include "general_assessor.h"
#include "linear_tech.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "tech_static_key.h"
#include "unit_roster_mng.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_RT_LIB = "/home/w/Projects/rts-proto/game_engine_dev/data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "/home/w/Projects/rts-proto/game_engine_dev/";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/unit-roster";
#ifndef UNIT_ROSTER_MNG_MK
#define UNIT_ROSTER_MNG_MK "mk03"
#endif
#ifndef UNIT_ROSTER_SEED
#define UNIT_ROSTER_SEED 0
#endif
static char G_OUT[512];
static char G_OUT_BEST[512];
static const u16 G_COL_W = 10;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void trunc_cell (cstr in, char* out) {
    if (out == nullptr) {
        return;
    }
    if (in == nullptr || in[0] == 0) {
        for (u16 i = 0; i < G_COL_W; ++i) {
            out[i] = '-';
        }
        out[G_COL_W] = 0;
        return;
    }
    const u32 n = static_cast<u32>(std::strlen(in));
    if (n <= static_cast<u32>(G_COL_W)) {
        for (u32 i = 0; i < n; ++i) {
            out[i] = in[i];
        }
        for (u32 i = n; i < static_cast<u32>(G_COL_W); ++i) {
            out[i] = ' ';
        }
        out[G_COL_W] = 0;
        return;
    }
    for (u16 i = 0; i < 8u; ++i) {
        out[i] = in[i];
    }
    out[8] = '.';
    out[9] = ' ';
    out[G_COL_W] = 0;
}

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
    std::snprintf(G_OUT, sizeof(G_OUT), "%s/unit_roster_%s_linear.txt", G_OUT_DIR, UNIT_ROSTER_MNG_MK);
    std::snprintf(G_OUT_BEST, sizeof(G_OUT_BEST), "%s/unit_roster_%s_best_of_type.txt",
        G_OUT_DIR, UNIT_ROSTER_MNG_MK);
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("unit_roster_mng: cannot load runtime statics\n");
        return 1;
    }
    const RuntimeStatics& st = loader.statics();
    if (!UnitRosterMng::setup(st)) {
        std::printf("unit_roster_mng: setup failed\n");
        return 1;
    }
    const u16 type_n = UnitRosterMng::type_n();
    const u16 tech_n = st.tech().get_item_count();
    if (type_n == 0 || tech_n == 0) {
        std::printf("unit_roster_mng: empty catalogs\n");
        UnitRosterMng::clear();
        return 1;
    }

    BitArrayCL owned(tech_n);
    BitArrayCL resource(st.resource().get_item_count());
    BitArrayCL building(st.building().get_item_count());
    BitArrayCL toggle_city(st.toggle_city().get_item_count());
    BitArrayCL civ(st.civ().get_item_count());
    BitArrayCL civ_trait(st.civ_trait().get_item_count());
    for (u32 i = 0; i < resource.get_count(); ++i) {
        resource.set_bit(i);
    }
    for (u32 i = 0; i < building.get_count(); ++i) {
        building.set_bit(i);
    }
    if (civ.get_count() == 0) {
        std::printf("unit_roster_mng: no civs\n");
        UnitRosterMng::clear();
        return 1;
    }
    unsigned seed = static_cast<unsigned>(UNIT_ROSTER_SEED);
    if (seed == 0u) {
        seed = static_cast<unsigned>(std::time(nullptr));
    }
    std::srand(seed);
    const u16 civ_idx = static_cast<u16>(static_cast<u32>(std::rand()) % civ.get_count());
    civ.set_bit(civ_idx);
    const CivTraitStruct& tr = st.civ().get_item(CivStaticDataKey::from_raw(civ_idx)).traits;
    for (u32 t = 0; t < MAX_CIV_TRAIT_COUNT; ++t) {
        const u16 tix = tr.indices[t];
        if (tix != U16_KEY_NULL && tix < civ_trait.get_count()) {
            civ_trait.set_bit(tix);
        }
    }
    cstr civ_nm = st.civ().get_name(CivStaticDataKey::from_raw(civ_idx));
    std::printf("unit_roster_mng: mk=%s seed=%u civ=%u %s\n",
        UNIT_ROSTER_MNG_MK, seed, static_cast<u32>(civ_idx), civ_nm != nullptr ? civ_nm : "?");

    AssessorCtx ctx = {};
    ctx.m_tech = &owned;
    ctx.m_civ = &civ;
    ctx.m_city_idx = 0;
    ctx.m_resource = &resource;
    ctx.m_building = &building;
    ctx.m_toggle_city = &toggle_city;
    ctx.m_civ_trait = &civ_trait;

    u16* hist = new u16[static_cast<u32>(tech_n) * static_cast<u32>(type_n)];
    cstr* col_tech = new cstr[tech_n];
    if (hist == nullptr || col_tech == nullptr) {
        delete[] hist;
        delete[] col_tech;
        UnitRosterMng::clear();
        return 1;
    }
    for (u16 i = 0; i < tech_n; ++i) {
        col_tech[i] = nullptr;
    }

    u32 step = 0;
    u64 rebuild_ns = 0;
    u32 rebuild_n = 0;
    u64 best_ns = 0;
    u32 best_n = 0;
    const TechStaticDataStruct* tech_items = &st.tech().get_item(TechStaticDataKey::from_raw(0));

    while (count_set(owned) < static_cast<u32>(tech_n)) {
        BitArrayCL available(tech_n);
        GeneralAssessor::assess_tech(&available, tech_n, tech_items, ctx);
        clr_owned(available, owned);
        if (count_set(available) == 0) {
            std::printf("stop: zero tech options after %u picks (%u / %u owned)\n",
                step, count_set(owned), static_cast<u32>(tech_n));
            break;
        }
        u16 pick = U16_KEY_NULL;
        if (!LinearTech::pick(available, &pick)) {
            std::printf("stop: LinearTech pick failed after %u picks\n", step);
            break;
        }
        owned.set_bit(pick);
        const auto t0 = std::chrono::steady_clock::now();
        if (!UnitRosterMng::rebuild(ctx)) {
            std::printf("stop: roster rebuild failed\n");
            break;
        }
        const auto t1 = std::chrono::steady_clock::now();
        rebuild_ns = rebuild_ns + static_cast<u64>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        rebuild_n = rebuild_n + 1u;
        const auto t2 = std::chrono::steady_clock::now();
        for (u16 t = 0; t < type_n; ++t) {
            hist[static_cast<u32>(step) * static_cast<u32>(type_n) + t] =
                UnitRosterMng::get_best_unit_of_type(t, ctx);
        }
        const auto t3 = std::chrono::steady_clock::now();
        best_ns = best_ns + static_cast<u64>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count());
        best_n = best_n + static_cast<u32>(type_n);
        col_tech[step] = st.tech().get_name(TechStaticDataKey::from_raw(pick));
        step = step + 1u;
    }

    std::FILE* fp = std::fopen(G_OUT, "w");
    if (fp == nullptr) {
        std::printf("unit_roster_mng: cannot write %s\n", G_OUT);
        delete[] hist;
        delete[] col_tech;
        UnitRosterMng::clear();
        return 1;
    }
    char cell[G_COL_W + 1u];
    trunc_cell("type", cell);
    std::fprintf(fp, "%s", cell);
    for (u32 c = 0; c < step; ++c) {
        trunc_cell(col_tech[c], cell);
        std::fprintf(fp, " %s", cell);
    }
    std::fprintf(fp, "\n");
    for (u16 t = 0; t < type_n; ++t) {
        trunc_cell(st.unit_type().get_name(UnitTypeStaticDataKey::from_raw(t)), cell);
        std::fprintf(fp, "%s", cell);
        for (u32 c = 0; c < step; ++c) {
            const u16 uidx = hist[c * static_cast<u32>(type_n) + t];
            cstr unm = nullptr;
            if (uidx != U16_KEY_NULL) {
                unm = st.unit().get_name(UnitStaticDataKey::from_raw(uidx));
            }
            trunc_cell(unm, cell);
            std::fprintf(fp, " %s", cell);
        }
        std::fprintf(fp, "\n");
    }
    std::fclose(fp);
    std::printf("wrote %s cols=%u types=%u\n", G_OUT, step, static_cast<u32>(type_n));

    std::FILE* fb = std::fopen(G_OUT_BEST, "w");
    if (fb == nullptr) {
        std::printf("unit_roster_mng: cannot write %s\n", G_OUT_BEST);
        delete[] hist;
        delete[] col_tech;
        UnitRosterMng::clear();
        return 1;
    }
    for (u16 t = 0; t < type_n; ++t) {
        cstr tnm = st.unit_type().get_name(UnitTypeStaticDataKey::from_raw(t));
        std::fprintf(fb, "%s\n", tnm != nullptr ? tnm : "?");
        u16 prev = U16_KEY_NULL;
        for (u32 c = 0; c < step; ++c) {
            const u16 uidx = hist[c * static_cast<u32>(type_n) + t];
            if (uidx == U16_KEY_NULL || uidx == prev) {
                continue;
            }
            cstr unm = st.unit().get_name(UnitStaticDataKey::from_raw(uidx));
            cstr tch = col_tech[c];
            std::fprintf(fb, "  %s  %s  %u\n",
                unm != nullptr ? unm : "?",
                tch != nullptr ? tch : "?",
                c);
            prev = uidx;
        }
    }
    std::fclose(fb);
    std::printf("wrote %s\n", G_OUT_BEST);

    const double sum_us = static_cast<double>(rebuild_ns) / 1000.0;
    const double avg_us = (rebuild_n == 0) ? 0.0 : sum_us / static_cast<double>(rebuild_n);
    std::printf("UnitRosterMng::rebuild: sum=%.2f us  avg=%.2f us  calls=%u\n",
        sum_us, avg_us, rebuild_n);
    const double best_sum_us = static_cast<double>(best_ns) / 1000.0;
    const double best_avg_us = (best_n == 0) ? 0.0 : best_sum_us / static_cast<double>(best_n);
    std::printf("UnitRosterMng::get_best_unit_of_type: sum=%.2f us  avg=%.2f us  calls=%u\n",
        best_sum_us, best_avg_us, best_n);

    delete[] hist;
    delete[] col_tech;
    UnitRosterMng::clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
