//================================================================================================================================
//=> - Includes -
//================================================================================================================================
 
#include "worker_city_jobs.h" 

#include <cstring>

#include "assert_log.h"
#include "circular_tile_areas.h"
#include "game_array_simple.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Local -
//================================================================================================================================

static bool intent_job (u8 iv) {
    return iv == AI_TILE_OV_INTENT_FORT || iv == AI_TILE_OV_INTENT_MTN_PASS;
}

static u32 count_sites (const GameArraySimple& map) {
    u32 n = 0;
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_planned_city(x, y) != 0u) {
                ++n;
            }
        }
    }
    return n;
}

//================================================================================================================================
//=> - WorkerCityJobs -
//================================================================================================================================

WorkerCityJobs::WorkerCityJobs () :
    m_sites(nullptr),
    m_sn(0),
    m_ok(false) {
}

WorkerCityJobs::~WorkerCityJobs () {
    clr();
}

void WorkerCityJobs::clr () {
    delete[] m_sites;
    m_sites = nullptr;
    m_sn = 0;
    m_ok = false;
}

bool WorkerCityJobs::ok () const {
    return m_ok;
}

u32 WorkerCityJobs::site_n () const {
    return m_sn;
}

bool WorkerCityJobs::find_site (u16 cx, u16 cy, u32* oix) const {
    if (!m_ok || m_sites == nullptr || oix == nullptr) {
        return false;
    }
    for (u32 i = 0; i < m_sn; ++i) {
        if (m_sites[i].m_x == cx && m_sites[i].m_y == cy) {
            *oix = i;
            return true;
        }
    }
    return false;
}

u16 WorkerCityJobs::job_n (u32 site) const {
    if (!m_ok || m_sites == nullptr || site >= m_sn) {
        return 0;
    }
    return m_sites[site].m_n;
}

bool WorkerCityJobs::job_at (u32 site, u16 slot, u16* ox, u16* oy) const {
    if (!m_ok || m_sites == nullptr || site >= m_sn || ox == nullptr || oy == nullptr) {
        return false;
    }
    const Site& s = m_sites[site];
    if (slot >= s.m_n) {
        return false;
    }
    *ox = s.m_jx[slot];
    *oy = s.m_jy[slot];
    return true;
}

bool WorkerCityJobs::build (const GameArraySimple& map) {
    clr();
    const u16 w = map.width();
    const u16 h = map.height();
    if (w == 0u || h == 0u) {
        return false;
    }
    const u32 sn = count_sites(map);
    if (sn == 0u) {
        m_ok = true;
        return true;
    }
    m_sites = new Site[sn];
    if (m_sites == nullptr) {
        return false;
    }
    m_sn = sn;
    {
        u32 k = 0;
        for (u16 y = 0; y < h; ++y) {
            for (u16 x = 0; x < w; ++x) {
                if (map.get_planned_city(x, y) == 0u) {
                    continue;
                }
                if (k >= m_sn) {
                    break;
                }
                m_sites[k].m_x = x;
                m_sites[k].m_y = y;
                m_sites[k].m_n = 0;
                ++k;
            }
        }
    }
    Whiteboard_1B taken("WorkerCityJobs", "taken", 0u);
    if (!taken.ok()) {
        clr();
        return false;
    }
    std::memset(taken.raw(), 0, static_cast<size_t>(map.tile_n()));
    for (u16 r = 0; r <= WCJ_R_MAX; ++r) {
        const CircArea cur = CircularTileAreas::get(r);
        if (cur.m_lim == 0u || cur.m_brd == nullptr) {
            break;
        }
        u16 i0 = 0;
        if (r > 0u) {
            const CircArea prev = CircularTileAreas::get(static_cast<u16>(r - 1u));
            i0 = prev.m_lim;
        }
        for (u32 s = 0; s < m_sn; ++s) {
            Site& site = m_sites[s];
            if (site.m_n >= WCJ_JOBS_N) {
                continue;
            }
            for (u16 i = i0; i < cur.m_lim; ++i) {
                if (site.m_n >= WCJ_JOBS_N) {
                    break;
                }
                const i32 x = static_cast<i32>(site.m_x) + static_cast<i32>(cur.m_brd[i][0]);
                const i32 y = static_cast<i32>(site.m_y) + static_cast<i32>(cur.m_brd[i][1]);
                if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
                    continue;
                }
                const u16 ux = static_cast<u16>(x);
                const u16 uy = static_cast<u16>(y);
                if (!intent_job(map.get_ai_ov_intent(ux, uy))) {
                    continue;
                }
                if (taken.rd(ux, uy) != 0u) {
                    continue;
                }
                taken.wr(ux, uy, 1u);
                site.m_jx[site.m_n] = ux;
                site.m_jy[site.m_n] = uy;
                site.m_n = static_cast<u16>(site.m_n + 1u);
            }
        }
    }
    m_ok = true;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
