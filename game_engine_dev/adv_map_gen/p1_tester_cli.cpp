//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "p1_tester_cli.h"
#include "p1_gen_climate.h"

static const char* k_seed_file = "seed.txt";

//================================================================================================================================
//=> - Private CLI helpers -
//================================================================================================================================

static bool g_out_subdir = true;
static bool g_batch_export = false;

static u32 rd_seed_file () {
    FILE* f = std::fopen(k_seed_file, "r");
    if (f == nullptr) {
        return 42u;
    }
    u32 seed = 0;
    if (std::fscanf(f, "%u", &seed) != 1) {
        std::fclose(f);
        return 42u;
    }
    std::fclose(f);
    return seed;
}

static P1_Adj_LandAltitudePrm lap_def () {
    P1_Adj_LandAltitudePrm sp = p1_adj_land_altitude_prm_def();
    sp.m_w_noise = 0.7f;
    sp.m_w_near = 0.85f;
    sp.m_w_riv = 0.5f;
    sp.m_lim_hills = 0.4f;
    sp.m_lim_mtn = 0.90f;
    return sp;
}

static bool tok_is_full (cstr s) {
    return s != nullptr && std::strcmp(s, "full") == 0;
}

static bool tok_is_keep (cstr s) {
    return s != nullptr && std::strcmp(s, "keep") == 0;
}

static bool tok_rain_wt (cstr s, u8* wt) {
    if (s == nullptr || wt == nullptr || std::strncmp(s, "--rain-wt=", 10) != 0) {
        return false;
    }
    const i32 v = static_cast<i32>(std::strtol(s + 10, nullptr, 10));
    *wt = (v < 0) ? 0u : ((v > CLIMATE_WT_MAX) ? static_cast<u8>(CLIMATE_WT_MAX) : static_cast<u8>(v));
    return true;
}

static bool tok_is_batch (cstr s) {
    return s != nullptr && std::strcmp(s, "batch") == 0;
}

static bool tok_u32 (cstr s, u32* v) {
    if (s == nullptr || v == nullptr || s[0] == '\0') {
        return false;
    }
    char* end = nullptr;
    const u32 x = static_cast<u32>(std::strtoul(s, &end, 10));
    if (end == s) {
        return false;
    }
    *v = x;
    return true;
}

static bool tok_f32 (cstr s, f32* v) {
    if (s == nullptr || v == nullptr || s[0] == '\0') {
        return false;
    }
    char* end = nullptr;
    const f64 x = std::strtod(s, &end);
    if (end == s) {
        return false;
    }
    *v = static_cast<f32>(x);
    return true;
}

bool p1_tester_out_subdir () {
    return g_out_subdir;
}

void p1_tester_set_out_subdir (bool v) {
    g_out_subdir = v;
}

bool p1_tester_batch_export () {
    return g_batch_export;
}

void p1_tester_set_batch_export (bool v) {
    g_batch_export = v;
}

//================================================================================================================================
//=> - P1_TesterCli -
//================================================================================================================================

P1_TesterCli::P1_TesterCli () :
    m_prm(),
    m_full(false),
    m_keep(false),
    m_batch(false),
    m_out_subdir(true),
    m_lap(),
    m_rain_wt(0),
    m_rain_wt_set(false) {
    m_prm = p1_run_prm_def();
    m_lap = lap_def();
    m_rain_wt = p1_gen_climate_prm_def().m_wts.m_w_rain;
}

bool P1_TesterCli::parse (i32 argc, char* argv[]) {
    m_prm = p1_run_prm_def();
    m_full = false;
    m_keep = false;
    m_batch = false;
    m_out_subdir = true;
    m_lap = lap_def();
    m_rain_wt = p1_gen_climate_prm_def().m_wts.m_w_rain;
    m_rain_wt_set = false;
    cstr pos[16];
    i32 pos_n = 0;
    for (i32 a = 1; a < argc; ++a) {
        if (argv[a] == nullptr) {
            continue;
        }
        if (tok_is_full(argv[a])) {
            m_full = true;
            continue;
        }
        if (tok_is_keep(argv[a])) {
            m_keep = true;
            continue;
        }
        if (tok_is_batch(argv[a])) {
            m_batch = true;
            continue;
        }
        u8 rw = 0;
        if (tok_rain_wt(argv[a], &rw)) {
            m_rain_wt = rw;
            m_rain_wt_set = true;
            continue;
        }
        if (pos_n < 16) {
            pos[pos_n++] = argv[a];
        }
    }
    if (pos_n >= 1) {
        u32 seed = 0;
        if (!tok_u32(pos[0], &seed)) {
            return false;
        }
        m_prm.m_seed = seed;
        m_out_subdir = false;
    } else {
        m_prm.m_seed = rd_seed_file();
        m_out_subdir = true;
    }
    if (pos_n >= 2) {
        u32 w = 0;
        if (!tok_u32(pos[1], &w)) {
            return false;
        }
        m_prm.m_w = static_cast<u16>(w);
    }
    if (pos_n >= 3) {
        u32 h = 0;
        if (!tok_u32(pos[2], &h)) {
            return false;
        }
        m_prm.m_h = static_cast<u16>(h);
    }
    if (pos_n >= 4) {
        if (!tok_f32(pos[3], &m_lap.m_w_noise)) {
            return false;
        }
    }
    if (pos_n >= 5) {
        if (!tok_f32(pos[4], &m_lap.m_w_near)) {
            return false;
        }
    }
    if (pos_n >= 6) {
        if (!tok_f32(pos[5], &m_lap.m_w_riv)) {
            return false;
        }
    }
    if (pos_n >= 7) {
        if (!tok_f32(pos[6], &m_lap.m_lim_hills)) {
            return false;
        }
    }
    if (pos_n >= 8) {
        if (!tok_f32(pos[7], &m_lap.m_lim_mtn)) {
            return false;
        }
    }
    p1_tester_set_out_subdir(m_out_subdir);
    p1_tester_set_batch_export(m_batch);
    return true;
}

const P1_RunPrm& P1_TesterCli::prm () const {
    return m_prm;
}

bool P1_TesterCli::full () const {
    return m_full;
}

bool P1_TesterCli::keep () const {
    return m_keep;
}

bool P1_TesterCli::batch () const {
    return m_batch;
}

bool P1_TesterCli::out_subdir () const {
    return m_out_subdir;
}

const P1_Adj_LandAltitudePrm& P1_TesterCli::lap () const {
    return m_lap;
}

P1_Adj_LandAltitudePrm& P1_TesterCli::lap_mut () {
    return m_lap;
}

u8 P1_TesterCli::rain_wt () const {
    return m_rain_wt;
}

bool P1_TesterCli::rain_wt_set () const {
    return m_rain_wt_set;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
