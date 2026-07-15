//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_tester_harness.h"

#include "p1_tester_chain_core.h"
#include "p1_tester_cli.h"
#include "p1_tester_early_chain.h"
#include "p1_tester_early_views.h"
#include "p1_tester_util.h"
#include "p1_wb_util.h"
#include "p1_rprint.h"

#include <dirent.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>

//================================================================================================================================
//=> - Private harness helpers -
//================================================================================================================================

static bool ends_with_ppm (cstr name) {
    if (name == nullptr) {
        return false;
    }
    const size_t n = std::strlen(name);
    return n >= 4u && std::strcmp(name + n - 4u, ".ppm") == 0;
}

static bool unlink_in_dir (cstr dir) {
    if (dir == nullptr || dir[0] == '\0') {
        return false;
    }
    DIR* d = opendir(dir);
    if (d == nullptr) {
        return p1_ensure_dir(dir);
    }
    bool ok = true;
    while (true) {
        const dirent* e = readdir(d);
        if (e == nullptr) {
            break;
        }
        if (e->d_name[0] == '.') {
            continue;
        }
        if (!ends_with_ppm(e->d_name)) {
            continue;
        }
        char path[512];
        std::snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
        if (unlink(path) != 0) {
            ok = false;
        }
    }
    closedir(d);
    return ok;
}

static bool unlink_flat_seed_ppms (u32 seed) {
    DIR* d = opendir(P1_OUT_ROOT);
    if (d == nullptr) {
        return p1_ensure_dir(P1_OUT_ROOT);
    }
    char prefix[64];
    std::snprintf(prefix, sizeof(prefix), "p1_seed_%u_", static_cast<unsigned>(seed));
    const size_t prefix_n = std::strlen(prefix);
    bool ok = true;
    while (true) {
        const dirent* e = readdir(d);
        if (e == nullptr) {
            break;
        }
        if (std::strncmp(e->d_name, prefix, prefix_n) != 0) {
            continue;
        }
        if (!ends_with_ppm(e->d_name)) {
            continue;
        }
        char path[512];
        std::snprintf(path, sizeof(path), "%s/%s", P1_OUT_ROOT, e->d_name);
        if (unlink(path) != 0) {
            ok = false;
        }
    }
    closedir(d);
    return ok;
}

bool p1_tester_clear_out (u32 seed) {
    if (!p1_ensure_dir(P1_OUT_ROOT)) {
        return false;
    }
    if (p1_tester_out_subdir()) {
        char dir[256];
        std::snprintf(dir, sizeof(dir), "%s/p1-seed-%u", P1_OUT_ROOT, static_cast<unsigned>(seed));
        return unlink_in_dir(dir);
    }
    return unlink_flat_seed_ppms(seed);
}

//================================================================================================================================
//=> - P1_TesterHarness -
//================================================================================================================================

P1_TesterHarness::P1_TesterHarness () :
    m_prm(),
    m_mk(),
    m_early(),
    m_begun(false),
    m_has_mk(false),
    m_has_early(false),
    m_input_ok(false),
    m_input_sec(0.0) {
}

P1_TesterHarness::~P1_TesterHarness () {
    free_all();
}

void P1_TesterHarness::free_all () {
    if (m_has_mk) {
        P1_MakeMap::free_rslt(&m_mk);
        m_has_mk = false;
    }
    if (m_has_early) {
        p1_free_early_chain(&m_early);
        m_has_early = false;
    }
}

bool P1_TesterHarness::begin (i32 argc, char* argv[]) {
    free_all();
    m_begun = false;
    m_input_ok = false;
    m_input_sec = 0.0;
    if (!p1_tester_checkout(argc, argv)) {
        return false;
    }
    if (!m_cli.parse(argc, argv)) {
        std::printf("P1 tester CLI parse failed\n");
        return false;
    }
    m_prm = m_cli.prm();
    m_cfg = map_config_def();
    if (!p1_run_prm_ok(m_prm)) {
        std::printf("P1 tester invalid run prm\n");
        return false;
    }
    if (!p1_map_gen_init()) {
        std::printf("P1 map generator static init failed\n");
        return false;
    }
    const u32 tile_n = static_cast<u32>(m_prm.m_w) * static_cast<u32>(m_prm.m_h);
    (void)tile_n;
    p1_wb_init(m_prm.m_w, m_prm.m_h);
    if (!m_cli.keep() && !m_cli.batch()) {
        if (!p1_tester_clear_out(m_prm.m_seed)) {
            std::printf("P1 tester failed to clear output for seed %u\n", m_prm.m_seed);
            return false;
        }
    }
    m_begun = true;
    return true;
}

bool P1_TesterHarness::run_input (const P1_Adj_LandAltitudePrm* lap) {
    if (!m_begun || g_p1_tester_cfg == nullptr) {
        return false;
    }
    m_input_ok = false;
    m_input_sec = 0.0;
    const u8 kind = g_p1_tester_cfg->m_in_kind;
    const u16 in_step = g_p1_tester_cfg->m_in_step;
    if (kind == P1_TIN_NONE) {
        m_input_ok = true;
        return true;
    }
    if (kind == P1_TIN_MK) {
        if (!p1_build_chain_core(m_prm, in_step, &m_mk, &m_input_sec)) {
            return false;
        }
        m_has_mk = true;
        m_input_ok = true;
        return true;
    }
    if (kind == P1_TIN_C14 || kind == P1_TIN_CENS) {
        std::printf("P1 tester chain15 input removed; rebuild step 18+ testers against P1_MakeMap\n");
        return false;
    }
    if (kind == P1_TIN_EARLY) {
        if (!p1_build_early_chain(m_prm, m_cfg, in_step, &m_early, &m_input_sec)) {
            return false;
        }
        m_has_early = true;
        m_input_ok = true;
        if (m_cli.full() && !p1_tester_save_early_views(m_prm.m_seed, in_step, m_early)) {
            P1_RPrint::rprint_info("Failed to save full prereq views");
            return false;
        }
        return true;
    }
    return false;
}

bool P1_TesterHarness::finish () {
    free_all();
    return p1_tester_whiteboard_chk();
}

const P1_RunPrm& P1_TesterHarness::prm () const {
    return m_prm;
}

const MapConfig& P1_TesterHarness::cfg () const {
    return m_cfg;
}

const P1_TesterCli& P1_TesterHarness::cli () const {
    return m_cli;
}

bool P1_TesterHarness::full () const {
    return m_cli.full();
}

bool P1_TesterHarness::keep () const {
    return m_cli.keep();
}

const P1_Adj_LandAltitudePrm& P1_TesterHarness::lap () const {
    return m_cli.lap();
}

u8 P1_TesterHarness::rain_wt () const {
    return m_cli.rain_wt();
}

bool P1_TesterHarness::rain_wt_set () const {
    return m_cli.rain_wt_set();
}

u32 P1_TesterHarness::seed () const {
    return m_prm.m_seed;
}

u32 P1_TesterHarness::step () const {
    return g_p1_tester_cfg != nullptr ? g_p1_tester_cfg->m_step : 0u;
}

double P1_TesterHarness::input_sec () const {
    return m_input_sec;
}

bool P1_TesterHarness::path_pri (char* out, size_t cap) const {
    if (!m_begun || g_p1_tester_cfg == nullptr || g_p1_tester_cfg->m_out_pri == nullptr) {
        return false;
    }
    return p1_make_out_path(m_prm.m_seed, g_p1_tester_cfg->m_out_pri, out, cap);
}

bool P1_TesterHarness::path_sec (char* out, size_t cap) const {
    if (!has_sec()) {
        return false;
    }
    return p1_make_out_path(m_prm.m_seed, g_p1_tester_cfg->m_out_sec, out, cap);
}

bool P1_TesterHarness::has_sec () const {
    return m_begun && g_p1_tester_cfg != nullptr && g_p1_tester_cfg->m_out_sec != nullptr
        && g_p1_tester_cfg->m_out_sec[0] != '\0';
}

bool P1_TesterHarness::path_extra (cstr suffix, char* out, size_t cap) const {
    if (!m_begun || suffix == nullptr) {
        return false;
    }
    return p1_tester_make_step_out(m_prm.m_seed, step(), suffix, out, cap);
}

bool P1_TesterHarness::has_mk () const {
    return m_has_mk;
}

const P1_MakeMapRslt& P1_TesterHarness::mk () const {
    return m_mk;
}

P1_MakeMapRslt& P1_TesterHarness::mk_mut () {
    return m_mk;
}

bool P1_TesterHarness::has_early () const {
    return m_has_early;
}

const P1_EarlyChainRslt& P1_TesterHarness::early () const {
    return m_early;
}

P1_EarlyChainRslt& P1_TesterHarness::early_mut () {
    return m_early;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
