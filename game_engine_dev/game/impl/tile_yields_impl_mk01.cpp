//================================================================================================================================
//=> - Includes (mk01: fully allocated land axes) -
//================================================================================================================================

#include <cstdio>

#include "build_adds_array.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "general_assessor.h"
#include "improvement_yield_static_key.h"
#include "resource_static_key.h"
#include "runtime_statics.h"
#include "std_add_helper.h"
#include "tile_attr_tables.h"
#include "tile_attribute_static_key.h"
#include "tile_yield_type_enum.h"
#include "tile_yields_imp_dump.h"
#include "worker_job_enum.h"

//================================================================================================================================
//=> - ImpYldJob (mk01) -
//================================================================================================================================

struct TileYields::ImpYldJob {
    bool m_on;
    ImpYldSlot m_terr[16];
    ImpYldSlot m_clim[5];
    ImpYldSlot m_ov[16];
    ImpYldSlot m_riv;
};

//================================================================================================================================
//=> - Local helpers -
//================================================================================================================================

typedef const char* cstr;

static u8 clamp_u8 (i32 v) {
    if (v <= 0) {
        return 0u;
    }
    if (v >= 255) {
        return 255u;
    }
    return static_cast<u8>(v);
}

static void add_row (i32* food, i32* prod, i32* comm, const TileAttributeStaticDataStruct& row) {
    *food += static_cast<i32>(row.food);
    *prod += static_cast<i32>(row.production);
    *comm += static_cast<i32>(row.commerce);
}

//================================================================================================================================
//=> - TileYields private (mk01) -
//================================================================================================================================

void TileYields::clear_jobs () {
    delete[] m_jobs;
    m_jobs = nullptr;
    m_job_n = 0;
}

bool TileYields::add_amt (ImpYldSlot* s, u16 yld_typ, i16 amt) {
    if (s == nullptr) {
        return false;
    }
    if (yld_typ == static_cast<u16>(TileYieldType::FOOD)) {
        s->m_food = static_cast<i16>(s->m_food + amt);
        return true;
    }
    if (yld_typ == static_cast<u16>(TileYieldType::PRODUCTION)) {
        s->m_prod = static_cast<i16>(s->m_prod + amt);
        return true;
    }
    if (yld_typ == static_cast<u16>(TileYieldType::COMMERCE)) {
        s->m_comm = static_cast<i16>(s->m_comm + amt);
        return true;
    }
    return false;
}

TileYields::ImpYldSlot* TileYields::slot_for (ImpYldJob* job, u8 kind, u8 id) {
    if (job == nullptr) {
        return nullptr;
    }
    if (kind == TileAttrTables::k_kind_terr) {
        if (id >= k_terr_n) {
            return nullptr;
        }
        return &job->m_terr[id];
    }
    if (kind == TileAttrTables::k_kind_clim) {
        if (id >= k_clim_n) {
            return nullptr;
        }
        return &job->m_clim[id];
    }
    if (kind == TileAttrTables::k_kind_ov) {
        if (id >= k_ov_n) {
            return nullptr;
        }
        return &job->m_ov[id];
    }
    if (kind == TileAttrTables::k_kind_riv) {
        return &job->m_riv;
    }
    return nullptr;
}

bool TileYields::setup_imp (const RuntimeStatics& st) {
    clear_jobs();
    m_job_n = st.worker_job().get_item_count();
    if (m_job_n == 0u) {
        return true;
    }
    m_jobs = new ImpYldJob[m_job_n]();
    const ImprovementYieldStaticData& src = st.improvement_yield();
    const TileAttributeStaticData& attrs = st.tile_attribute();
    const u16 n = src.get_item_count();
    for (u16 i = 0; i < n; ++i) {
        const ImprovementYieldStaticDataKey key = ImprovementYieldStaticDataKey::from_raw(i);
        const ImprovementYieldStaticDataStruct& row = src.get_item(key);
        if (row.worker_job_idx >= m_job_n) {
            clear_jobs();
            return false;
        }
        if (row.cond_attr >= attrs.get_item_count()) {
            clear_jobs();
            return false;
        }
        const TileAttributeStaticDataKey akey = TileAttributeStaticDataKey::from_raw(row.cond_attr);
        cstr aname = attrs.get_name(akey);
        u8 kind = 0u;
        u8 id = 0u;
        if (!TileAttrTables::map_name(aname, &kind, &id)) {
            clear_jobs();
            return false;
        }
        ImpYldJob* job = &m_jobs[row.worker_job_idx];
        ImpYldSlot* slot = slot_for(job, kind, id);
        if (!add_amt(slot, row.yield_type, row.amount)) {
            clear_jobs();
            return false;
        }
        job->m_on = true;
    }
    return true;
}

u16 TileYields::job_on_tile (const GameArraySimple& map, u16 x, u16 y) {
    const u8 typ = map.get_add_typ(x, y);
    const u16 idx = map.get_add_idx(x, y);
    if (typ == BUILD_ADD_STD && idx != U16_KEY_NULL && StdAddHelper::has_farm(map.tile(x, y))) {
        return static_cast<u16>(WorkerJob::Farm);
    }
    return U16_KEY_NULL;
}

void TileYields::add_land (i32* food, i32* prod, i32* comm, const GameArraySimple& map, u16 x, u16 y) {
    add_row(food, prod, comm, TileAttrTables::terr(map.get_terrain(x, y)));
    add_row(food, prod, comm, TileAttrTables::clim(map.get_climate(x, y)));
    add_row(food, prod, comm, TileAttrTables::ov(map.get_overlay(x, y)));
    if (map.get_river(x, y) != 0u) {
        add_row(food, prod, comm, TileAttrTables::riv());
    }
}

void TileYields::add_job (i32* food, i32* prod, i32* comm, const GameArraySimple& map, u16 x, u16 y, u16 job_idx) {
    if (m_jobs == nullptr || m_job_n == 0u || job_idx >= m_job_n) {
        return;
    }
    const ImpYldJob& job = m_jobs[job_idx];
    if (!job.m_on) {
        return;
    }
    const u8 terr = map.get_terrain(x, y);
    const u8 clim = map.get_climate(x, y);
    const u8 ov = map.get_overlay(x, y);
    if (terr < k_terr_n) {
        *food += static_cast<i32>(job.m_terr[terr].m_food);
        *prod += static_cast<i32>(job.m_terr[terr].m_prod);
        *comm += static_cast<i32>(job.m_terr[terr].m_comm);
    }
    if (clim < k_clim_n) {
        *food += static_cast<i32>(job.m_clim[clim].m_food);
        *prod += static_cast<i32>(job.m_clim[clim].m_prod);
        *comm += static_cast<i32>(job.m_clim[clim].m_comm);
    }
    if (ov < k_ov_n) {
        *food += static_cast<i32>(job.m_ov[ov].m_food);
        *prod += static_cast<i32>(job.m_ov[ov].m_prod);
        *comm += static_cast<i32>(job.m_ov[ov].m_comm);
    }
    if (map.get_river(x, y) != 0u) {
        *food += static_cast<i32>(job.m_riv.m_food);
        *prod += static_cast<i32>(job.m_riv.m_prod);
        *comm += static_cast<i32>(job.m_riv.m_comm);
    }
}

void TileYields::add_imp (i32* food, i32* prod, i32* comm, const GameArraySimple& map, u16 x, u16 y) {
    const u16 job_idx = job_on_tile(map, x, y);
    if (job_idx == U16_KEY_NULL) {
        return;
    }
    add_job(food, prod, comm, map, x, y, job_idx);
}

void TileYields::add_res (i32* food, i32* prod, i32* comm, const GameArraySimple& map, u16 x, u16 y) {
    if (m_st == nullptr || m_ctx == nullptr || m_ctx->m_tech == nullptr) {
        return;
    }
    const u16 ri = map.get_res(x, y);
    if (ri == U16_KEY_NULL) {
        return;
    }
    const ResourceStaticData& rs = m_st->resource();
    if (ri >= rs.get_item_count()) {
        return;
    }
    const ResourceStaticDataStruct& row = rs.get_item(ResourceStaticDataKey::from_raw(ri));
    AssessorCtx actx = {};
    actx.m_tech = m_ctx->m_tech;
    if (!GeneralAssessor::chk(row.reqs, actx)) {
        return;
    }
    *food += static_cast<i32>(row.food);
    *prod += static_cast<i32>(row.shields);
    *comm += static_cast<i32>(row.commerce);
}

//================================================================================================================================
//=> - TileYields public (mk01) -
//================================================================================================================================

bool TileYields::setup (const RuntimeStatics& st) {
    m_st = &st;
    if (!TileAttrTables::setup(st)) {
        clear_jobs();
        m_st = nullptr;
        return false;
    }
    if (!setup_imp(st)) {
        TileAttrTables::clear();
        m_st = nullptr;
        return false;
    }
    return true;
}

void TileYields::bind_map (const GameArraySimple* map) {
    m_map = map;
}

void TileYields::bind_ctx (const TileYieldCtx* ctx) {
    m_ctx = ctx;
}

TileYield TileYields::get (u16 x, u16 y) {
    TileYield yld = {};
    if (m_map == nullptr || !TileAttrTables::ready()) {
        return yld;
    }
    if (x >= m_map->width() || y >= m_map->height()) {
        return yld;
    }
    i32 food = 0;
    i32 prod = 0;
    i32 comm = 0;
    add_land(&food, &prod, &comm, *m_map, x, y);
    add_res(&food, &prod, &comm, *m_map, x, y);
    add_imp(&food, &prod, &comm, *m_map, x, y);
    yld.m_food = clamp_u8(food);
    yld.m_production = clamp_u8(prod);
    yld.m_commerce = clamp_u8(comm);
    return yld;
}

u8 TileYields::food_no_imp (u16 x, u16 y) {
    if (m_map == nullptr || !TileAttrTables::ready()) {
        return 0u;
    }
    if (x >= m_map->width() || y >= m_map->height()) {
        return 0u;
    }
    i32 food = 0;
    i32 prod = 0;
    i32 comm = 0;
    add_land(&food, &prod, &comm, *m_map, x, y);
    add_res(&food, &prod, &comm, *m_map, x, y);
    return clamp_u8(food);
}

u8 TileYields::food_with_job (u16 x, u16 y, u16 job_idx) {
    if (m_map == nullptr || !TileAttrTables::ready()) {
        return 0u;
    }
    if (x >= m_map->width() || y >= m_map->height()) {
        return 0u;
    }
    i32 food = 0;
    i32 prod = 0;
    i32 comm = 0;
    add_land(&food, &prod, &comm, *m_map, x, y);
    add_res(&food, &prod, &comm, *m_map, x, y);
    add_job(&food, &prod, &comm, *m_map, x, y, job_idx);
    return clamp_u8(food);
}

bool TileYields::job_raises_food (u16 x, u16 y, u16 job_idx) {
    if (m_map == nullptr || !TileAttrTables::ready()) {
        return false;
    }
    if (x >= m_map->width() || y >= m_map->height()) {
        return false;
    }
    i32 food = 0;
    i32 prod = 0;
    i32 comm = 0;
    add_land(&food, &prod, &comm, *m_map, x, y);
    add_res(&food, &prod, &comm, *m_map, x, y);
    const i32 base = food;
    add_job(&food, &prod, &comm, *m_map, x, y, job_idx);
    return clamp_u8(food) > clamp_u8(base);
}

bool TileYields::in_bounds (u16 x, u16 y) {
    if (m_map == nullptr) {
        return false;
    }
    return x < m_map->width() && y < m_map->height();
}

//================================================================================================================================
//=> - TileYieldsImpDump (mk01) -
//================================================================================================================================

bool TileYieldsImpDump::slot_nz (const TileYields::ImpYldSlot& s) {
    return s.m_food != 0 || s.m_prod != 0 || s.m_comm != 0;
}

void TileYieldsImpDump::pr_slot (FILE* out, cstr axis, u8 id, cstr name, const TileYields::ImpYldSlot& s) {
    std::fprintf(out, "  %s[%u]", axis, static_cast<unsigned>(id));
    if (name != nullptr) {
        std::fprintf(out, " %s", name);
    }
    std::fprintf(out, " food%+d prod%+d comm%+d\n",
        static_cast<int>(s.m_food), static_cast<int>(s.m_prod), static_cast<int>(s.m_comm));
}

bool TileYieldsImpDump::clim_nm (u8 id, cstr* out) {
    static const struct { u8 m_id; cstr m_nm; } k_rows[] = {
        {CLIMATE_NONE, "CLIMATE_NONE"},
        {CLIMATE_DESERT, "CLIMATE_DESERT"},
        {CLIMATE_PLAINS, "CLIMATE_PLAINS"},
        {CLIMATE_GRASSLAND, "CLIMATE_GRASSLAND"},
        {CLIMATE_BLACK_SOIL, "CLIMATE_BLACK_SOIL"},
    };
    for (u32 i = 0; i < sizeof(k_rows) / sizeof(k_rows[0]); ++i) {
        if (k_rows[i].m_id == id) {
            *out = k_rows[i].m_nm;
            return true;
        }
    }
    *out = nullptr;
    return false;
}

bool TileYieldsImpDump::terr_nm (u8 id, cstr* out) {
    static const struct { u8 m_id; cstr m_nm; } k_rows[] = {
        {TERR_NONE[0], "TERR_NONE"},
        {TERR_OCEAN[0], "TERR_OCEAN"},
        {TERR_SEA[0], "TERR_SEA"},
        {TERR_COASTAL[0], "TERR_COASTAL"},
        {TERR_PLAINS[0], "TERR_PLAINS"},
        {TERR_HILLS[0], "TERR_HILLS"},
        {TERR_MOUNTAINS[0], "TERR_MOUNTAINS"},
        {TERR_VOLCANO[0], "TERR_VOLCANO"},
        {TERR_INLAND_SEA[0], "TERR_INLAND_SEA"},
        {TERR_INLAND_LAKE[0], "TERR_INLAND_LAKE"},
        {TERR_TILE_SENTINEL[0], "TERR_TILE_SENTINEL"},
    };
    for (u32 i = 0; i < sizeof(k_rows) / sizeof(k_rows[0]); ++i) {
        if (k_rows[i].m_id == id) {
            *out = k_rows[i].m_nm;
            return true;
        }
    }
    *out = nullptr;
    return false;
}

bool TileYieldsImpDump::ov_nm (u8 id, cstr* out) {
    static const struct { u8 m_id; cstr m_nm; } k_rows[] = {
        {OV_NONE[0], "OV_NONE"},
        {OV_FOREST[0], "OV_FORESTS"},
        {OV_SWAMP[0], "OV_SWAMPS"},
        {OV_JUNGLE[0], "OV_JUNGLES"},
        {OV_GLACIER[0], "OV_GLACIER"},
    };
    for (u32 i = 0; i < sizeof(k_rows) / sizeof(k_rows[0]); ++i) {
        if (k_rows[i].m_id == id) {
            *out = k_rows[i].m_nm;
            return true;
        }
    }
    *out = nullptr;
    return false;
}

u16 TileYieldsImpDump::pr_axis (FILE* out, cstr axis, u16 cap, const TileYields::ImpYldSlot* rows, bool (*name_fn)(u8, cstr*)) {
    u16 nonzero = 0;
    for (u16 i = 0; i < cap; ++i) {
        if (slot_nz(rows[i])) {
            nonzero++;
        }
    }
    std::fprintf(out, " axis %-4s allocated capacity=%u nonzero=%u bytes=%zu\n",
        axis, static_cast<unsigned>(cap), static_cast<unsigned>(nonzero),
        static_cast<size_t>(cap) * sizeof(TileYields::ImpYldSlot));
    for (u16 i = 0; i < cap; ++i) {
        cstr nm = nullptr;
        if (name_fn != nullptr) {
            name_fn(static_cast<u8>(i), &nm);
        }
        pr_slot(out, axis, static_cast<u8>(i), nm, rows[i]);
    }
    return nonzero;
}

u16 TileYieldsImpDump::dump_job (u16 job_idx, FILE* out) {
    if (out == nullptr) {
        out = stdout;
    }
    std::fprintf(out, "-----------------------------------------------------------\n");
    std::fprintf(out, "IMP YIELD TABLE DUMP mk01 job_idx=%u job_n=%u jobs_ptr=%p\n",
        static_cast<unsigned>(job_idx),
        static_cast<unsigned>(TileYields::m_job_n),
        static_cast<const void*>(TileYields::m_jobs));
    std::fprintf(out, " sizeof(ImpYldJob)=%zu sizeof(ImpYldSlot)=%zu\n",
        sizeof(TileYields::ImpYldJob), sizeof(TileYields::ImpYldSlot));
    if (TileYields::m_jobs == nullptr || job_idx >= TileYields::m_job_n) {
        std::fprintf(out, " (no job table for this index)\n");
        std::fprintf(out, "-----------------------------------------------------------\n");
        return 0;
    }
    const TileYields::ImpYldJob& job = TileYields::m_jobs[job_idx];
    if (!job.m_on) {
        std::fprintf(out, " m_on=0 (job branch not used; skip axes)\n");
        std::fprintf(out, "-----------------------------------------------------------\n");
        return 0;
    }
    std::fprintf(out, " m_on=1\n");
    u16 nz = 0;
    nz = static_cast<u16>(nz + pr_axis(out, "terr", TileYields::k_terr_n, job.m_terr, terr_nm));
    nz = static_cast<u16>(nz + pr_axis(out, "clim", TileYields::k_clim_n, job.m_clim, clim_nm));
    nz = static_cast<u16>(nz + pr_axis(out, "ov", TileYields::k_ov_n, job.m_ov, ov_nm));
    std::fprintf(out, " axis riv  allocated capacity=1 nonzero=%u bytes=%zu\n",
        slot_nz(job.m_riv) ? 1u : 0u, sizeof(TileYields::ImpYldSlot));
    pr_slot(out, "riv", 0u, "OV_RIVERS", job.m_riv);
    if (slot_nz(job.m_riv)) {
        nz++;
    }
    std::fprintf(out, "-----------------------------------------------------------\n");
    return nz;
}
