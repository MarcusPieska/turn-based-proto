//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bit_array.h"
#include "building_static_key.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "general_assessor.h"
#include "map_overlay_enum.h"
#include "map_overlay_static_key.h"
#include "resource_static_data.h"
#include "resource_static_key.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "std_add_helper.h"
#include "tech_static_key.h"
#include "tile_work_assessor.h"
#include "tile_yields.h"
#include "worker_guidance.h"
#include "worker_imp_select.h"
#include "worker_job_enum.h"
#include "worker_job_imp_index.h"
#include "worker_job_imp_static_key.h"
#include "worker_job_static_data.h"
#include "worker_job_static_key.h"
#include "worker_job_target_enum.h"
#include "worker_job_type_enum.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;

static const char* G_GRN = "\033[32m";
static const char* G_RED = "\033[31m";
static const char* G_RST = "\033[0m";

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];

int print_level = 1;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void pad_name (char* buf, u32 buf_sz, cstr name) {
    u32 n = (name == nullptr) ? 0u : static_cast<u32>(std::strlen(name));
    if (n >= buf_sz) {
        n = buf_sz - 1u;
    }
    std::memcpy(buf, (name != nullptr) ? name : "", n);
    buf[n] = '\0';
    for (u32 i = n; i + 1u < buf_sz; ++i) {
        buf[i] = ' ';
    }
    if (buf_sz > 0) {
        buf[buf_sz - 1u] = '\0';
    }
}

static void fill_bits (BitArrayCL& ba) {
    for (u32 i = 0; i < ba.get_count(); ++i) {
        ba.set_bit(i);
    }
}

static bool build_paths () {
    char dir[256];
    if (std::snprintf(dir, sizeof(dir), "%s/p1-seed-%u", G_MAP_ROOT, G_SEED) <= 0) {
        return false;
    }
    if (std::snprintf(g_terr, sizeof(g_terr), "%s/terrain.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_clim, sizeof(g_clim), "%s/climate.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_riv, sizeof(g_riv), "%s/rivers.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_ov, sizeof(g_ov), "%s/overlay.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_res, sizeof(g_res), "%s/resources.ppm", dir) <= 0) {
        return false;
    }
    return true;
}

static void append_tech (char* buf, size_t cap, const RuntimeStatics& st, u16 idx) {
    const size_t pos = std::strlen(buf);
    if (pos >= cap) {
        return;
    }
    cstr nm = st.tech().get_name(TechStaticDataKey::from_raw(idx));
    if (nm == nullptr) {
        nm = "?";
    }
    std::snprintf(buf + pos, cap - pos, " tech(%s)", nm);
}

static void append_resource (char* buf, size_t cap, const RuntimeStatics& st, u16 idx) {
    const size_t pos = std::strlen(buf);
    if (pos >= cap) {
        return;
    }
    cstr nm = st.resource().get_name(ResourceStaticDataKey::from_raw(idx));
    if (nm == nullptr) {
        nm = "?";
    }
    std::snprintf(buf + pos, cap - pos, " resource(%s)", nm);
}

static void append_building (char* buf, size_t cap, const RuntimeStatics& st, u16 idx) {
    const size_t pos = std::strlen(buf);
    if (pos >= cap) {
        return;
    }
    cstr nm = st.building().get_name(BuildingStaticDataKey::from_raw(idx));
    if (nm == nullptr) {
        nm = "?";
    }
    std::snprintf(buf + pos, cap - pos, " building(%s)", nm);
}

static bool chk_reqs (const ItemReqsStruct& reqs, const BitArrayCL& tech, const BitArrayCL& resource) {
    AssessorCtx ctx = {};
    ctx.m_tech = &tech;
    ctx.m_resource = &resource;
    return GeneralAssessor::chk(reqs, ctx);
}

static void infer_reqs_ablation (char* out, size_t cap, const RuntimeStatics& st, const ItemReqsStruct& reqs) {
    out[0] = '\0';
    BitArrayCL tech(st.tech().get_item_count());
    BitArrayCL resource(st.resource().get_item_count());
    fill_bits(resource);
    for (u32 t = 0; t < tech.get_count(); ++t) {
        tech.clear_all();
        tech.set_bit(t);
        if (chk_reqs(reqs, tech, resource)) {
            append_tech(out, cap, st, static_cast<u16>(t));
        }
    }
    tech.clear_all();
    for (u32 r = 0; r < resource.get_count(); ++r) {
        resource.clear_all();
        resource.set_bit(r);
        if (chk_reqs(reqs, tech, resource)) {
            append_resource(out, cap, st, static_cast<u16>(r));
        }
    }
    tech.clear_all();
    fill_bits(resource);
    for (u32 b = 0; b < st.building().get_item_count(); ++b) {
        BitArrayCL bld(st.building().get_item_count());
        bld.set_bit(b);
        AssessorCtx ctx = {};
        ctx.m_tech = &tech;
        ctx.m_resource = &resource;
        ctx.m_building = &bld;
        if (GeneralAssessor::chk(reqs, ctx)) {
            append_building(out, cap, st, static_cast<u16>(b));
        }
    }
    if (out[0] == '\0') {
        std::snprintf(out, cap, " (none)");
    }
}

static cstr job_nm (const RuntimeStatics& st, u16 idx) {
    if (idx >= st.worker_job().get_item_count()) {
        return "?";
    }
    cstr nm = st.worker_job().get_name(WorkerJobStaticDataKey::from_raw(idx));
    return (nm != nullptr) ? nm : "?";
}

static u16 clear_job_target_ov (u16 job_idx) {
    switch (static_cast<WorkerJob>(job_idx)) {
        case WorkerJob::Clear_Forest:
            return static_cast<u16>(MapOverlay::Forest);
        case WorkerJob::Clear_Jungle:
            return static_cast<u16>(MapOverlay::Jungle);
        case WorkerJob::Clear_Swamp:
            return static_cast<u16>(MapOverlay::Swamp);
        default:
            return U16_KEY_NULL;
    }
}

static u16 job_expect_ov (const RuntimeStatics& st, u16 job_idx) {
    const WorkerJobStaticDataStruct& row = st.worker_job().get_item(WorkerJobStaticDataKey::from_raw(job_idx));
    if (row.type == static_cast<u16>(WorkerJobType::Clearing)) {
        return U16_KEY_NULL;
    }
    return row.target_idx;
}

static bool find_empty_plains (const GameArraySimple& map, u16* ox, u16* oy, u16 skip_x, u16 skip_y);

static bool find_empty_forest_site (const GameArraySimple& map, u16* ox, u16* oy, u16 skip_x, u16 skip_y) {
    for (u16 y = 0; y < map.height(); ++y) {
        for (u16 x = 0; x < map.width(); ++x) {
            if (x == skip_x && y == skip_y) {
                continue;
            }
            const u8 terr = map.get_terrain(x, y);
            if (terr != TERR_PLAINS[0] && terr != TERR_HILLS[0]) {
                continue;
            }
            if (map.get_overlay(x, y) != U16_KEY_NULL) {
                continue;
            }
            const u8 clim = map.get_climate(x, y);
            if (clim != CLIMATE_PLAINS && clim != CLIMATE_GRASSLAND && clim != CLIMATE_BLACK_SOIL) {
                continue;
            }
            *ox = x;
            *oy = y;
            return true;
        }
    }
    return false;
}

static bool find_resource_job_tile (
    const GameArraySimple& map,
    const RuntimeStatics& st,
    u16 job_idx,
    u16* ox,
    u16* oy,
    u16 skip_x,
    u16 skip_y)
{
    const u16 rn = st.resource().get_item_count();
    for (u16 y = 0; y < map.height(); ++y) {
        for (u16 x = 0; x < map.width(); ++x) {
            if (x == skip_x && y == skip_y) {
                continue;
            }
            if (map.get_overlay(x, y) != U16_KEY_NULL) {
                continue;
            }
            const u16 ri = map.get_res(x, y);
            if (ri == U16_KEY_NULL || ri >= rn) {
                continue;
            }
            if (st.resource().get_item(ResourceStaticDataKey::from_raw(ri)).worker_job_idx != job_idx) {
                continue;
            }
            const u8 terr = map.get_terrain(x, y);
            if (terr == TERR_MOUNTAINS[0] || terr == TERR_VOLCANO[0]) {
                if (map.get_road_typ(x, y) == ROAD_NONE) {
                    continue;
                }
            }
            *ox = x;
            *oy = y;
            return true;
        }
    }
    return false;
}

static bool find_land_tile (const GameArraySimple& map, u16* ox, u16* oy, u16 skip_x, u16 skip_y) {
    for (u16 y = 0; y < map.height(); ++y) {
        for (u16 x = 0; x < map.width(); ++x) {
            if (x == skip_x && y == skip_y) {
                continue;
            }
            if (overlay_is_water_terr(map.get_terrain(x, y))) {
                continue;
            }
            if (map.get_overlay(x, y) != U16_KEY_NULL) {
                continue;
            }
            *ox = x;
            *oy = y;
            return true;
        }
    }
    return false;
}

static bool locate_overlay_job_tile (
    const GameArraySimple& map,
    const RuntimeStatics& st,
    u16 job_idx,
    u16* ox,
    u16* oy,
    u16 skip_x,
    u16 skip_y)
{
    const WorkerJobStaticDataStruct& row = st.worker_job().get_item(WorkerJobStaticDataKey::from_raw(job_idx));
    switch (static_cast<WorkerJobType>(row.type)) {
        case WorkerJobType::Clearing:
        case WorkerJobType::Farm:
            return find_empty_plains(map, ox, oy, skip_x, skip_y);
        case WorkerJobType::Forest:
            return find_empty_forest_site(map, ox, oy, skip_x, skip_y);
        case WorkerJobType::Strategic:
            return find_land_tile(map, ox, oy, skip_x, skip_y);
        case WorkerJobType::Resource:
            return find_resource_job_tile(map, st, job_idx, ox, oy, skip_x, skip_y);
        default:
            return false;
    }
}

static void prep_overlay_job_tile (GameArraySimple& map, u16 job_idx, u16 x, u16 y) {
    const u16 clear_ov = clear_job_target_ov(job_idx);
    if (clear_ov != U16_KEY_NULL) {
        map.set_overlay(x, y, clear_ov);
        map.set_add_idx(x, y, 0u);
        return;
    }
    map.set_overlay(x, y, U16_KEY_NULL);
    map.set_add_idx(x, y, 0u);
}

static bool job_enabled (u16 job_idx, u16 x, u16 y) {
    if (!TileWorkAssessor::tile_ok(job_idx, x, y)) {
        return false;
    }
    TileWorkCand cands[4];
    const u16 n = TileWorkAssessor::assess_job(x, y, job_idx, cands, 4);
    return n > 0 && cands[0].m_imp == U16_KEY_NULL;
}

static bool run_overlay_job_cert (GameArraySimple& map, const RuntimeStatics& st, u16* applied_n, u16* expect_n) {
    std::printf("\noverlay job enable + apply:\n");
    u16 jobs[16];
    u16 job_n = 0;
    u16 expect = 0;
    u16 skip_x = U16_KEY_NULL;
    u16 skip_y = U16_KEY_NULL;
    const u16 wj_n = st.worker_job().get_item_count();
    for (u16 job_idx = 0; job_idx < wj_n; ++job_idx) {
        const WorkerJobStaticDataStruct& row = st.worker_job().get_item(WorkerJobStaticDataKey::from_raw(job_idx));
        if (row.target_kind != static_cast<u16>(WorkerJobTarget::Overlay)) {
            continue;
        }
        expect = static_cast<u16>(expect + 1u);
        u16 x = 0;
        u16 y = 0;
        if (!locate_overlay_job_tile(map, st, job_idx, &x, &y, skip_x, skip_y)) {
            std::printf("  %-28s  no tile\n", job_nm(st, job_idx));
            continue;
        }
        prep_overlay_job_tile(map, job_idx, x, y);
        if (!job_enabled(job_idx, x, y)) {
            std::printf("  %-28s  not enabled\n", job_nm(st, job_idx));
            skip_x = x;
            skip_y = y;
            continue;
        }
        jobs[job_n] = job_idx;
        job_n = static_cast<u16>(job_n + 1u);
        std::printf("  enable %u. %s\n", static_cast<unsigned>(job_n), job_nm(st, jobs[job_n - 1u]));
        skip_x = x;
        skip_y = y;
    }
    u16 applied = 0;
    skip_x = U16_KEY_NULL;
    skip_y = U16_KEY_NULL;
    for (u16 i = 0; i < job_n; ++i) {
        const u16 job_idx = jobs[i];
        u16 x = 0;
        u16 y = 0;
        if (!locate_overlay_job_tile(map, st, job_idx, &x, &y, skip_x, skip_y)) {
            std::printf("  apply %u. %s  no tile\n", static_cast<unsigned>(i + 1u), job_nm(st, job_idx));
            continue;
        }
        prep_overlay_job_tile(map, job_idx, x, y);
        const u16 want_ov = job_expect_ov(st, job_idx);
        const bool ok = job_enabled(job_idx, x, y)
            && WorkerGuidance::apply_job(x, y, job_idx)
            && map.get_overlay(x, y) == want_ov;
        std::printf("  apply %u. %-24s -> resulting overlay == %u %s\n",
            static_cast<unsigned>(i + 1u),
            job_nm(st, job_idx),
            static_cast<unsigned>(map.get_overlay(x, y)),
            ok ? "OK" : "FAIL");
        if (ok) {
            applied = static_cast<u16>(applied + 1u);
        }
        skip_x = x;
        skip_y = y;
    }
    const bool all = (applied == expect && expect > 0);
    std::printf("  %senabled %u/%u  applied %u/%u%s\n",
        all ? G_GRN : G_RED,
        static_cast<unsigned>(job_n),
        static_cast<unsigned>(expect),
        static_cast<unsigned>(applied),
        static_cast<unsigned>(expect),
        G_RST);
    *applied_n = applied;
    *expect_n = expect;
    return all;
}

static cstr imp_nm (const RuntimeStatics& st, u16 idx) {
    if (idx >= st.worker_job_imp().get_item_count()) {
        return "?";
    }
    cstr nm = st.worker_job_imp().get_name(WorkerJobImpStaticDataKey::from_raw(idx));
    return (nm != nullptr) ? nm : "?";
}

static cstr ov_nm (const RuntimeStatics& st, u16 idx) {
    if (idx >= st.map_overlay().get_item_count()) {
        return "?";
    }
    cstr nm = st.map_overlay().get_name(MapOverlayStaticDataKey::from_raw(idx));
    return (nm != nullptr) ? nm : "?";
}

static void print_overlay_inventory (const RuntimeStatics& st) {
    std::printf("\noverlays:\n");
    const WorkerJobImpIndex& ix = st.worker_job_imp_index();
    const u16 ov_n = st.map_overlay().get_item_count();
    for (u16 i = 0; i < ov_n; ++i) {
        char nm[32];
        pad_name(nm, 32, ov_nm(st, i));
        std::printf("  %s: %u imps\n", nm, static_cast<unsigned>(ix.imp_n(i)));
    }
}

static void print_imp_ablation (const RuntimeStatics& st) {
    std::printf("\nimprovement reqs (ablation):\n");
    const u16 n = st.worker_job_imp().get_item_count();
    char line[320];
    char reqs[160];
    for (u16 i = 0; i < n; ++i) {
        char nm[32];
        pad_name(nm, 32, imp_nm(st, i));
        const ItemReqsStruct& rq = st.worker_job_imp().get_item(WorkerJobImpStaticDataKey::from_raw(i)).reqs;
        infer_reqs_ablation(reqs, sizeof(reqs), st, rq);
        std::snprintf(line, sizeof(line), "  %s:%s", nm, reqs);
        std::printf("%s\n", line);
    }
}

static cstr job_tile_tag (u16 typ) {
    switch (static_cast<WorkerJobType>(typ)) {
        case WorkerJobType::Clearing:
            return "remove-overlay";
        case WorkerJobType::Farm:
            return "add-overlay tile(plains,empty)";
        case WorkerJobType::Forest:
            return "add-overlay tile(plains|hills)";
        case WorkerJobType::Strategic:
            return "add-overlay tile(land)";
        case WorkerJobType::Resource:
            return "add-overlay tile(resource)";
        default:
            return "tile(?)";
    }
}

static void print_overlay_job_ablation (const RuntimeStatics& st) {
    std::printf("\noverlay job reqs (ablation):\n");
    const u16 n = st.worker_job().get_item_count();
    char reqs[160];
    for (u16 i = 0; i < n; ++i) {
        const WorkerJobStaticDataStruct& row = st.worker_job().get_item(WorkerJobStaticDataKey::from_raw(i));
        if (row.target_kind != static_cast<u16>(WorkerJobTarget::Overlay)) {
            continue;
        }
        char nm[32];
        cstr jnm = st.worker_job().get_name(WorkerJobStaticDataKey::from_raw(i));
        pad_name(nm, 32, (jnm != nullptr) ? jnm : "?");
        infer_reqs_ablation(reqs, sizeof(reqs), st, row.reqs);
        std::printf("  %s:%s  %s\n", nm, reqs, job_tile_tag(row.type));
    }
}

static bool find_empty_plains (const GameArraySimple& map, u16* ox, u16* oy, u16 skip_x, u16 skip_y) {
    for (u16 y = 0; y < map.height(); ++y) {
        for (u16 x = 0; x < map.width(); ++x) {
            if (x == skip_x && y == skip_y) {
                continue;
            }
            if (map.get_terrain(x, y) != TERR_PLAINS[0]) {
                continue;
            }
            if (map.get_overlay(x, y) != U16_KEY_NULL) {
                continue;
            }
            if (map.get_res(x, y) != U16_KEY_NULL) {
                continue;
            }
            *ox = x;
            *oy = y;
            return true;
        }
    }
    return false;
}

static bool find_forest_plains (const GameArraySimple& map, u16* ox, u16* oy) {
    for (u16 y = 0; y < map.height(); ++y) {
        for (u16 x = 0; x < map.width(); ++x) {
            if (map.get_terrain(x, y) != TERR_PLAINS[0]) {
                continue;
            }
            if (map.get_overlay(x, y) != static_cast<u16>(MapOverlay::Forest)) {
                continue;
            }
            if (map.get_res(x, y) != U16_KEY_NULL) {
                continue;
            }
            *ox = x;
            *oy = y;
            return true;
        }
    }
    return false;
}

static u16 run_imp_chain (
    GameArraySimple& map,
    const RuntimeStatics& st,
    u16 x,
    u16 y,
    u16 job_idx,
    u16 expect_n,
    cstr label)
{
    std::printf("\n%s selection (expect %u):\n", label, static_cast<unsigned>(expect_n));
    u16 got = 0;
    for (u16 step = 0; step < 32u; ++step) {
        const u16 imp = WorkerImpSelect::pick(x, y, job_idx);
        if (imp == U16_KEY_NULL) {
            break;
        }
        std::printf("  %u. %s\n", static_cast<unsigned>(got + 1u), imp_nm(st, imp));
        if (!WorkerGuidance::apply_work(x, y, job_idx, imp)) {
            std::printf("  apply failed on %s\n", imp_nm(st, imp));
            break;
        }
        got = static_cast<u16>(got + 1u);
    }
    const bool ok = (got == expect_n);
    std::printf("  %sselected %u/%u%s\n", ok ? G_GRN : G_RED, static_cast<unsigned>(got),
        static_cast<unsigned>(expect_n), G_RST);
    (void)map;
    return got;
}

static void print_banner (bool ok, cstr msg) {
    std::printf("%s%s%s\n", ok ? G_GRN : G_RED, msg, G_RST);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    if (!build_paths()) {
        std::printf("path build failed\n");
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("load runtime statics failed\n");
        return 1;
    }
    RuntimeStatics& st = loader.statics();
    if (!TileYields::setup(st)) {
        std::printf("TileYields::setup failed\n");
        return 1;
    }
    if (!TileWorkAssessor::setup(st)) {
        std::printf("TileWorkAssessor::setup failed\n");
        return 1;
    }
    GameArraySimple map;
    if (!Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov)) {
        std::printf("load map failed\n");
        return 1;
    }
    if (!Factory_GameArraySimple::load_res_dist_data(&map, g_res)) {
        std::printf("load resources failed\n");
        return 1;
    }
    BitArrayCL tech(st.tech().get_item_count());
    BitArrayCL resource(st.resource().get_item_count());
    fill_bits(tech);
    fill_bits(resource);
    TileYieldCtx yctx = {};
    yctx.m_tech = &tech;
    TileWorkCtx wctx = {};
    wctx.m_tech = &tech;
    wctx.m_resource = &resource;
    TileYields::bind_map(&map);
    TileYields::bind_ctx(&yctx);
    TileWorkAssessor::bind_map(&map);
    TileWorkAssessor::bind_ctx(&wctx);
    WorkerGuidance::bind_statics(&st);
    WorkerGuidance::bind_map(&map);

    std::printf("=======================================================\n");
    std::printf(" WORKER CATALOG CERT  seed=%u  print_level=%d\n", G_SEED, print_level);
    std::printf("=======================================================\n");

    print_overlay_inventory(st);
    print_imp_ablation(st);
    print_overlay_job_ablation(st);

    const u16 farm_ov = static_cast<u16>(MapOverlay::Farm);
    const u16 forest_ov = static_cast<u16>(MapOverlay::Forest);
    const u16 mine_ov = static_cast<u16>(MapOverlay::Mine);
    const u16 plant_ov = static_cast<u16>(MapOverlay::Plantation);
    const u16 fort_ov = static_cast<u16>(MapOverlay::Fort);
    const u16 farm_expect = st.worker_job_imp_index().imp_n(farm_ov);
    const u16 forest_expect = st.worker_job_imp_index().imp_n(forest_ov);
    const u16 mine_expect = st.worker_job_imp_index().imp_n(mine_ov);
    const u16 plant_expect = st.worker_job_imp_index().imp_n(plant_ov);
    const u16 fort_expect = st.worker_job_imp_index().imp_n(fort_ov);
    const u16 farm_job = static_cast<u16>(WorkerJob::Cultivate_Farm);
    const u16 forest_job = static_cast<u16>(WorkerJob::Plant_Forest);
    const u16 mine_job = static_cast<u16>(WorkerJob::Build_Mine);
    const u16 plant_job = static_cast<u16>(WorkerJob::Build_Plantation);
    const u16 fort_job = static_cast<u16>(WorkerJob::Build_Fort);

    u16 fx = 0;
    u16 fy = 0;
    u16 gx = 0;
    u16 gy = 0;
    u16 mx = 0;
    u16 my = 0;
    u16 px = 0;
    u16 py = 0;
    u16 tx = 0;
    u16 ty = 0;
    const bool got_farm = find_empty_plains(map, &fx, &fy, U16_KEY_NULL, U16_KEY_NULL);
    const bool got_forest = find_forest_plains(map, &gx, &gy);
    const bool got_mine = find_empty_plains(map, &mx, &my, fx, fy);
    const bool got_plant = find_empty_plains(map, &px, &py, mx, my);
    const bool got_fort = find_empty_plains(map, &tx, &ty, px, py);

    u16 farm_got = 0;
    u16 forest_got = 0;
    u16 mine_got = 0;
    u16 plant_got = 0;
    u16 fort_got = 0;
    if (got_farm) {
        map.set_overlay(fx, fy, farm_ov);
        map.set_add_idx(fx, fy, 0u);
        farm_got = run_imp_chain(map, st, fx, fy, farm_job, farm_expect, "Farm");
    } else {
        std::printf("\nFarm selection: no test tile\n");
    }
    if (got_forest) {
        forest_got = run_imp_chain(map, st, gx, gy, forest_job, forest_expect, "Forest");
    } else {
        std::printf("\nForest selection: no test tile\n");
    }
    if (got_mine) {
        map.set_overlay(mx, my, mine_ov);
        map.set_add_idx(mx, my, 0u);
        mine_got = run_imp_chain(map, st, mx, my, mine_job, mine_expect, "Mine");
    } else {
        std::printf("\nMine selection: no test tile\n");
    }
    if (got_plant) {
        map.set_overlay(px, py, plant_ov);
        map.set_add_idx(px, py, 0u);
        plant_got = run_imp_chain(map, st, px, py, plant_job, plant_expect, "Plantation");
    } else {
        std::printf("\nPlantation selection: no test tile\n");
    }
    if (got_fort) {
        map.set_overlay(tx, ty, fort_ov);
        map.set_add_idx(tx, ty, 0u);
        fort_got = run_imp_chain(map, st, tx, ty, fort_job, fort_expect, "Fort");
    } else {
        std::printf("\nFort selection: no test tile\n");
    }

    const bool farm_ok = got_farm && farm_got == farm_expect;
    const bool forest_ok = got_forest && forest_got == forest_expect;
    const bool mine_ok = got_mine && mine_got == mine_expect;
    const bool plant_ok = got_plant && plant_got == plant_expect;
    const bool fort_ok = got_fort && fort_got == fort_expect;
    const bool imp_ok = farm_ok && forest_ok && mine_ok && plant_ok && fort_ok;
    u16 job_applied = 0;
    u16 job_expect = 0;
    const bool job_ok = run_overlay_job_cert(map, st, &job_applied, &job_expect);
    const bool all_ok = imp_ok && job_ok;

    std::printf("\n=======================================================\n");
    std::printf(" asserts:\n");
    print_banner(farm_ok, "  Farm: all improvements selected and applied");
    print_banner(forest_ok, "  Forest: all improvements selected and applied");
    print_banner(mine_ok, "  Mine: all improvements selected and applied");
    print_banner(plant_ok, "  Plantation: all improvements selected and applied");
    print_banner(fort_ok, "  Fort: all improvements selected and applied");
    print_banner(imp_ok, "  PASS (imp certification phase C)");
    print_banner(job_ok, "  PASS (overlay job certification phase D)");
    print_banner(all_ok, "  PASS (full worker catalog cert)");
    std::printf("=======================================================\n");
    return all_ok ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
