//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "r1_gen_res_sectors.h"
#include "game_map_defs.h"
#include "p1_wb_util.h"
#include "wb_que_xy.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};

static bool is_wat (u8 cls) {
    return overlay_is_water_terr(cls); 
}

static bool is_mtn (u8 cls) {
    return cls == TERR_MOUNTAINS[0];
}

static bool can_claim (u8 cls) {
    if (cls == TERR_NONE[0] || is_wat(cls) || is_mtn(cls)) {
        return false;
    }
    return true;
}

static u32 tidx (u16 w, u32 x, u32 y) {
    return y * static_cast<u32>(w) + x;
}

static u32 rng_next (u32* s) {
    *s = (*s) * 1664525u + 1013904223u;
    return *s;
}

static i32 rng_jit (u32* s, i32 span) {
    if (span <= 0) {
        return 0;
    }
    const u32 r = rng_next(s);
    return static_cast<i32>(r % static_cast<u32>(span + span + 1)) - span;
}

static u16 clamp_u16 (i32 v, u16 lo, u16 hi) {
    if (v < static_cast<i32>(lo)) {
        return lo;
    }
    if (v > static_cast<i32>(hi)) {
        return hi;
    }
    return static_cast<u16>(v);
}

static bool find_seed_tile (
    const u8* terr,
    u16 w,
    u16 h,
    const u16* ov,
    u16 cx,
    u16 cy,
    u16* ox,
    u16* oy)
{
    const u32 ti0 = tidx(w, cx, cy);
    if (can_claim(terr[ti0]) && ov[ti0] == static_cast<u16>(R1_RES_SECTOR_NONE)) {
        *ox = cx;
        *oy = cy;
        return true;
    }
    const i32 wi = static_cast<i32>(w);
    const i32 hi = static_cast<i32>(h);
    const i32 rad = static_cast<i32>(R1_RES_SECTOR_SEED_WIN_R);
    i32 best_d = 0x7fffffff;
    i32 best_x = -1;
    i32 best_y = -1;
    for (i32 dy = -rad; dy <= rad; ++dy) {
        for (i32 dx = -rad; dx <= rad; ++dx) {
            const i32 x = static_cast<i32>(cx) + dx;
            const i32 y = static_cast<i32>(cy) + dy;
            if (x < 0 || y < 0 || x >= wi || y >= hi) {
                continue;
            }
            const u32 ti = tidx(w, static_cast<u32>(x), static_cast<u32>(y));
            if (!can_claim(terr[ti]) || ov[ti] != static_cast<u16>(R1_RES_SECTOR_NONE)) {
                continue;
            }
            const i32 d = dx * dx + dy * dy;
            if (d >= best_d) {
                continue;
            }
            best_d = d;
            best_x = x;
            best_y = y;
        }
    }
    if (best_x < 0) {
        return false;
    }
    *ox = static_cast<u16>(best_x);
    *oy = static_cast<u16>(best_y);
    return true;
}

static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    *r = 0;
    *g = 0;
    *b = 0;
    if (cls == TERR_OCEAN[0]) {
        *r = TERR_OCEAN[1]; *g = TERR_OCEAN[2]; *b = TERR_OCEAN[3];
    } else if (cls == TERR_SEA[0]) {
        *r = TERR_SEA[1]; *g = TERR_SEA[2]; *b = TERR_SEA[3];
    } else if (cls == TERR_COASTAL[0]) {
        *r = TERR_COASTAL[1]; *g = TERR_COASTAL[2]; *b = TERR_COASTAL[3];
    } else if (cls == TERR_PLAINS[0]) {
        *r = TERR_PLAINS[1]; *g = TERR_PLAINS[2]; *b = TERR_PLAINS[3];
    } else if (cls == TERR_HILLS[0]) {
        *r = TERR_HILLS[1]; *g = TERR_HILLS[2]; *b = TERR_HILLS[3];
    } else if (cls == TERR_MOUNTAINS[0]) {
        *r = TERR_MOUNTAINS[1]; *g = TERR_MOUNTAINS[2]; *b = TERR_MOUNTAINS[3];
    }
}

static void sec_rgb (u16 sid, u8* r, u8* g, u8* b) {
    const u32 s = static_cast<u32>(sid) + 1u;
    *r = static_cast<u8>(40u + ((s * 97u) % 180u));
    *g = static_cast<u8>(40u + ((s * 57u) % 180u));
    *b = static_cast<u8>(40u + ((s * 31u) % 180u));
}

//================================================================================================================================
//=> - R1_Gen_ResSectors -
//================================================================================================================================

R1_Gen_ResSectors::R1_Gen_ResSectors () :
    m_ok(false),
    m_rslt(),
    m_wb_ov(nullptr),
    m_seeds(nullptr) {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_sec_n = 0;
    m_rslt.m_ov = nullptr;
}

R1_Gen_ResSectors::~R1_Gen_ResSectors () {
    clr();
}

void R1_Gen_ResSectors::clr () {
    m_rslt.m_ov = nullptr;
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_sec_n = 0;
    m_ok = false;
    delete m_seeds;
    m_seeds = nullptr;
    delete m_wb_ov;
    m_wb_ov = nullptr;
}

bool R1_Gen_ResSectors::is_valid () const {
    return m_ok;
}

const R1_Gen_ResSectorsRslt& R1_Gen_ResSectors::result () const {
    return m_rslt;
}

bool R1_Gen_ResSectors::generate (const u8* terr, u16 w, u16 h, const LandMassIndexRslt& mass, u32 seed, u16 pct) 
{
    clr();
    if (terr == nullptr || w == 0 || h == 0 || mass.m_ov == nullptr || mass.m_w != w || mass.m_h != h
        || pct == 0 || pct > 100u) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    m_wb_ov = new Whiteboard_2B("R1_Gen_ResSectors", "ov", 0u);
    P1_WB_CHK(*m_wb_ov);
    u16* ov = m_wb_ov->get_iter_ptr();
    for (u32 i = 0; i < n; ++i) {
        ov[i] = static_cast<u16>(R1_RES_SECTOR_NONE);
    }
    WB_QueXY cur;
    WB_QueXY nxt;
    m_seeds = new WB_QueXY();
    if (!cur.ok() || !nxt.ok() || m_seeds == nullptr || !m_seeds->ok()) {
        clr();
        return false;
    }
    u32 lx = (wi * static_cast<u32>(pct)) / 100u;
    u32 ly = (hi * static_cast<u32>(pct)) / 100u;
    if (lx == 0u) {
        lx = 1u;
    }
    if (ly == 0u) {
        ly = 1u;
    }
    const i32 jx_span = static_cast<i32>((lx * 40u) / 100u);
    const i32 jy_span = static_cast<i32>((ly * 40u) / 100u);
    u32 rng = seed != 0u ? seed : 1u;
    u16 sec_n = 0;
    for (u32 gy = 0; gy < hi; gy += ly) {
        for (u32 gx = 0; gx < wi; gx += lx) {
            const i32 jx = rng_jit(&rng, jx_span);
            const i32 jy = rng_jit(&rng, jy_span);
            const u16 cx = clamp_u16(static_cast<i32>(gx) + jx, 0u, static_cast<u16>(w - 1u));
            const u16 cy = clamp_u16(static_cast<i32>(gy) + jy, 0u, static_cast<u16>(h - 1u));
            u16 sx = 0;
            u16 sy = 0;
            if (!find_seed_tile(terr, w, h, ov, cx, cy, &sx, &sy)) {
                continue;
            }
            if (sec_n == U16_KEY_NULL) {
                clr();
                return false;
            }
            const u32 ti = tidx(w, sx, sy);
            ov[ti] = sec_n;
            if (!cur.push(sx, sy) || !m_seeds->push(sx, sy)) {
                clr();
                return false;
            }
            sec_n = static_cast<u16>(sec_n + 1u);
        }
    }
    for (;;) {
        u32 claimed = 0;
        nxt.clear();
        const u32 fn = cur.count();
        for (u32 qi = 0; qi < fn; ++qi) {
            const u16 px = cur.x_at(qi);
            const u16 py = cur.y_at(qi);
            const u32 ti = tidx(w, px, py);
            const u16 sid = ov[ti];
            if (sid == static_cast<u16>(R1_RES_SECTOR_NONE)) {
                continue;
            }
            for (i32 d = 0; d < 4; ++d) {
                const i32 nx = static_cast<i32>(px) + k_dx4[d];
                const i32 ny = static_cast<i32>(py) + k_dy4[d];
                if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                    continue;
                }
                const u32 ni = tidx(w, static_cast<u32>(nx), static_cast<u32>(ny));
                if (ov[ni] != static_cast<u16>(R1_RES_SECTOR_NONE)) {
                    continue;
                }
                if (!can_claim(terr[ni])) {
                    continue;
                }
                ov[ni] = sid;
                if (!nxt.push(static_cast<u16>(nx), static_cast<u16>(ny))) {
                    clr();
                    return false;
                }
                claimed = claimed + 1u;
            }
        }
        if (claimed == 0u) {
            break;
        }
        cur.swap(nxt);
    }
    cur.clear();
    nxt.clear();
    for (u32 y = 0; y < hi; ++y) {
        for (u32 x = 0; x < wi; ++x) {
            const u32 ti = tidx(w, x, y);
            if (ov[ti] == static_cast<u16>(R1_RES_SECTOR_NONE)) {
                continue;
            }
            bool edge = false;
            for (i32 d = 0; d < 4; ++d) {
                const i32 nx = static_cast<i32>(x) + k_dx4[d];
                const i32 ny = static_cast<i32>(y) + k_dy4[d];
                if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                    continue;
                }
                const u32 ni = tidx(w, static_cast<u32>(nx), static_cast<u32>(ny));
                if (ov[ni] != static_cast<u16>(R1_RES_SECTOR_NONE)) {
                    continue;
                }
                if (!is_mtn(terr[ni])) {
                    continue;
                }
                edge = true;
                break;
            }
            if (!edge) {
                continue;
            }
            if (!cur.push(static_cast<u16>(x), static_cast<u16>(y))) {
                clr();
                return false;
            }
        }
    }
    for (;;) {
        u32 claimed = 0;
        nxt.clear();
        const u32 fn = cur.count();
        for (u32 qi = 0; qi < fn; ++qi) {
            const u16 px = cur.x_at(qi);
            const u16 py = cur.y_at(qi);
            const u32 ti = tidx(w, px, py);
            const u16 sid = ov[ti];
            if (sid == static_cast<u16>(R1_RES_SECTOR_NONE)) {
                continue;
            }
            for (i32 d = 0; d < 4; ++d) {
                const i32 nx = static_cast<i32>(px) + k_dx4[d];
                const i32 ny = static_cast<i32>(py) + k_dy4[d];
                if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                    continue;
                }
                const u32 ni = tidx(w, static_cast<u32>(nx), static_cast<u32>(ny));
                if (ov[ni] != static_cast<u16>(R1_RES_SECTOR_NONE)) {
                    continue;
                }
                if (is_wat(terr[ni])) {
                    continue;
                }
                ov[ni] = sid;
                if (!nxt.push(static_cast<u16>(nx), static_cast<u16>(ny))) {
                    clr();
                    return false;
                }
                claimed = claimed + 1u;
            }
        }
        if (claimed == 0u) {
            break;
        }
        cur.swap(nxt);
    }
    Whiteboard_1B wb_hit("R1_Gen_ResSectors", "mass_hit", 0u);
    P1_WB_CHK(wb_hit);
    u8* hit = wb_hit.raw();
    const u32 hit_n = static_cast<u32>(mass.m_mass_n) + 1u;
    for (u32 i = 0; i < hit_n; ++i) {
        hit[i] = 0;
    }
    for (u32 i = 0; i < n; ++i) {
        if (ov[i] == static_cast<u16>(R1_RES_SECTOR_NONE)) {
            continue;
        }
        const u16 mid = mass.m_ov[i];
        if (mid == static_cast<u16>(LAND_MASS_IDX_NONE) || mid > mass.m_mass_n) {
            continue;
        }
        hit[mid] = 1;
    }
    cur.clear();
    nxt.clear();
    for (u32 i = 0; i < n; ++i) {
        if (ov[i] != static_cast<u16>(R1_RES_SECTOR_NONE)) {
            continue;
        }
        const u16 mid = mass.m_ov[i];
        if (mid == static_cast<u16>(LAND_MASS_IDX_NONE) || mid > mass.m_mass_n || hit[mid] != 0) {
            continue;
        }
        if (is_wat(terr[i])) {
            continue;
        }
        if (sec_n == U16_KEY_NULL) {
            clr();
            return false;
        }
        const u16 sid = sec_n;
        sec_n = static_cast<u16>(sec_n + 1u);
        hit[mid] = 1;
        const u16 sx = static_cast<u16>(i % wi);
        const u16 sy = static_cast<u16>(i / wi);
        ov[i] = sid;
        cur.clear();
        if (!cur.push(sx, sy)) {
            clr();
            return false;
        }
        for (;;) {
            u32 claimed = 0;
            nxt.clear();
            const u32 fn = cur.count();
            for (u32 qi = 0; qi < fn; ++qi) {
                const u16 px = cur.x_at(qi);
                const u16 py = cur.y_at(qi);
                for (i32 d = 0; d < 4; ++d) {
                    const i32 nx = static_cast<i32>(px) + k_dx4[d];
                    const i32 ny = static_cast<i32>(py) + k_dy4[d];
                    if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                        continue;
                    }
                    const u32 ni = tidx(w, static_cast<u32>(nx), static_cast<u32>(ny));
                    if (ov[ni] != static_cast<u16>(R1_RES_SECTOR_NONE)) {
                        continue;
                    }
                    if (is_wat(terr[ni])) {
                        continue;
                    }
                    ov[ni] = sid;
                    if (!nxt.push(static_cast<u16>(nx), static_cast<u16>(ny))) {
                        clr();
                        return false;
                    }
                    claimed = claimed + 1u;
                }
            }
            if (claimed == 0u) {
                break;
            }
            cur.swap(nxt);
        }
    }
    m_rslt.m_w = w;
    m_rslt.m_h = h;
    m_rslt.m_sec_n = sec_n;
    m_rslt.m_ov = ov;
    m_ok = true;
    return true;
}

bool R1_Gen_ResSectors::save_ppm (cstr path, const u8* terr) const {
    if (!m_ok || path == nullptr || terr == nullptr || m_rslt.m_ov == nullptr || m_rslt.m_w == 0 || m_rslt.m_h == 0) {
        return false;
    }
    const u16 w = m_rslt.m_w;
    const u16 h = m_rslt.m_h;
    const u16* sec_ov = m_rslt.m_ov;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        const u16 sid = sec_ov[i];
        if (sid != static_cast<u16>(R1_RES_SECTOR_NONE)) {
            sec_rgb(sid, &r, &g, &b);
        } else {
            terr_rgb(terr[i], &r, &g, &b);
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    if (m_seeds != nullptr) {
        const i32 rad = static_cast<i32>(R1_RES_SECTOR_SEED_RAD);
        const i32 r2 = rad * rad;
        const u32 sn = m_seeds->count();
        for (u32 si = 0; si < sn; ++si) {
            const i32 cx = static_cast<i32>(m_seeds->x_at(si));
            const i32 cy = static_cast<i32>(m_seeds->y_at(si));
            for (i32 dy = -rad; dy <= rad; ++dy) {
                for (i32 dx = -rad; dx <= rad; ++dx) {
                    if (dx * dx + dy * dy > r2) {
                        continue;
                    }
                    const i32 x = cx + dx;
                    const i32 y = cy + dy;
                    if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
                        continue;
                    }
                    u8* p = &rgb[(static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u];
                    p[0] = 255;
                    p[1] = 0;
                    p[2] = 0;
                }
            }
        }
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        delete[] rgb;
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(n) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
