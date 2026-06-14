//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_river_sectors.h"

#include "generate_global_ocean.h"
#include "generator_constants.h"
#include "generator_whiteboard.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>
#include <vector>

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static const int k_ring_w = 3;

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool is_mountain (u8 cls) {
    return cls == TERR_MOUNTAINS[0];
}

static bool tile_blocked (const u8* terrain, const u8* glob, u32 ti) {
    if (is_mountain(terrain[ti])) {
        return true;
    }
    if (glob[ti] != 0) {
        return true;
    }
    return false;
}

static u8 sect_terr (const u8* terrain, const u8* glob, u32 ti) {
    const u8 c = terrain[ti];
    if (is_water(c) && glob[ti] == 0) {
        return TERR_PLAINS[0];
    }
    return c;
}

struct QueueItem {
    i32 dsq;
    u16 x;
    u16 y;
    bool operator<(const QueueItem& o) const {
        return dsq < o.dsq;
    }
};

struct SectorBuild {
    u16 cx;
    u16 cy;
    u8 terr;
    std::vector<std::pair<u16, u16>> coords;
    std::vector<u16> conn;
    SectorBuild () : cx(0), cy(0), terr(0) {}
};

static void add_conn (SectorBuild* a, SectorBuild* b, u32 ai, u32 bi) {
    if (std::find(a->conn.begin(), a->conn.end(), static_cast<u16>(bi)) == a->conn.end()) {
        a->conn.push_back(static_cast<u16>(bi));
    }
    if (std::find(b->conn.begin(), b->conn.end(), static_cast<u16>(ai)) == b->conn.end()) {
        b->conn.push_back(static_cast<u16>(ai));
    }
}

//================================================================================================================================
//=> - Generate_RiverSectors -
//================================================================================================================================

RiverSectorsResult* Generate_RiverSectors::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    const RiverPtsResult* pts,
    u32 seed) 
{
    (void)seed;
    if (terrain == nullptr || w == 0 || h == 0 || pts == nullptr || pts->n == 0) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* glob = Generate_GlobalOcean::build_mask(terrain, w, h);
    if (glob == nullptr) {
        return nullptr;
    }
    std::vector<SectorBuild> secs;
    secs.reserve(pts->n);
    for (u32 p = 0; p < pts->n; ++p) {
        SectorBuild s;
        s.cx = pts->pts[p].x;
        s.cy = pts->pts[p].y;
        secs.push_back(s);
    }
    const u32 sector_n = static_cast<u32>(secs.size());
    std::vector<std::vector<QueueItem>> qs(sector_n);
    std::vector<std::set<std::pair<u16, u16>>> qsets(sector_n);
    u16* clm = GeneratorWhiteboard::alloc(static_cast<i32>(n));
    u16* overlay = new u16[n];
    if (clm == nullptr || overlay == nullptr) {
        GeneratorWhiteboard::release(clm);
        delete[] overlay;
        delete[] glob;
        return nullptr;
    }
    for (u32 i = 0; i < n; ++i) {
        clm[i] = 0;
    }
    for (u32 i = 0; i < n; ++i) {
        overlay[i] = static_cast<u16>(RIVER_SECTOR_NONE);
    }
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    for (u32 si = 0; si < sector_n; ++si) {
        const u16 px = secs[si].cx;
        const u16 py = secs[si].cy;
        if (px >= w || py >= h) {
            continue;
        }
        const u32 ti = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
        if (tile_blocked(terrain, glob, ti)) {
            continue;
        }
        secs[si].terr = sect_terr(terrain, glob, ti);
        clm[ti] = 1;
        overlay[ti] = static_cast<u16>(si);
        secs[si].coords.push_back({px, py});
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(px) + dx4[d];
            const i32 ny = static_cast<i32>(py) + dy4[d];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w) || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                continue;
            }
            const u32 ni = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
            if (tile_blocked(terrain, glob, ni)) {
                continue;
            }
            if (clm[ni] != 0) {
                const u16 oid = overlay[ni];
                if (oid != static_cast<u16>(RIVER_SECTOR_NONE) && oid != static_cast<u16>(si)) {
                    add_conn(&secs[si], &secs[oid], si, oid);
                }
                continue;
            }
            if (sect_terr(terrain, glob, ni) != secs[si].terr) {
                continue;
            }
            const i32 dsq = (nx - static_cast<i32>(px)) * (nx - static_cast<i32>(px))
                + (ny - static_cast<i32>(py)) * (ny - static_cast<i32>(py));
            const std::pair<u16, u16> c = {static_cast<u16>(nx), static_cast<u16>(ny)};
            if (qsets[si].find(c) == qsets[si].end()) {
                QueueItem e = {};
                e.dsq = dsq;
                e.x = static_cast<u16>(nx);
                e.y = static_cast<u16>(ny);
                qs[si].push_back(e);
                std::sort(qs[si].begin(), qs[si].end());
                qsets[si].insert(c);
            }
        }
    }
    std::vector<i32> act;
    act.reserve(sector_n);
    for (u32 si = 0; si < sector_n; ++si) {
        act.push_back(static_cast<i32>(si));
    }
    while (!act.empty()) {
        i32 min_dsq = -1;
        for (i32 si : act) {
            if (qs[static_cast<u32>(si)].empty()) {
                continue;
            }
            for (const QueueItem& e : qs[static_cast<u32>(si)]) {
                const u32 ti = static_cast<u32>(e.y) * static_cast<u32>(w) + static_cast<u32>(e.x);
                if (clm[ti] == 0) {
                    min_dsq = (min_dsq == -1 || e.dsq < min_dsq) ? e.dsq : min_dsq;
                    break;
                }
            }
        }
        if (min_dsq == -1) {
            break;
        }
        const i32 min_d = static_cast<i32>(std::sqrt(static_cast<double>(min_dsq)));
        const i32 max_d = min_d + k_ring_w;
        const i32 max_dsq = max_d * max_d;
        std::vector<i32> new_act;
        i32 claimed_cnt = 0;
        for (i32 si : act) {
            if (qs[static_cast<u32>(si)].empty()) {
                continue;
            }
            const u16 px = secs[static_cast<u32>(si)].cx;
            const u16 py = secs[static_cast<u32>(si)].cy;
            std::size_t qidx = 0;
            while (qidx < qs[static_cast<u32>(si)].size()) {
                const QueueItem e = qs[static_cast<u32>(si)][qidx];
                const u16 x = e.x;
                const u16 y = e.y;
                const u32 ti = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
                if (clm[ti] != 0) {
                    qsets[static_cast<u32>(si)].erase({x, y});
                    qs[static_cast<u32>(si)].erase(qs[static_cast<u32>(si)].begin() + static_cast<std::ptrdiff_t>(qidx));
                    continue;
                }
                if (e.dsq < min_dsq || e.dsq > max_dsq) {
                    qidx++;
                    continue;
                }
                if (tile_blocked(terrain, glob, ti) || sect_terr(terrain, glob, ti) != secs[static_cast<u32>(si)].terr) {
                    qsets[static_cast<u32>(si)].erase({x, y});
                    qs[static_cast<u32>(si)].erase(qs[static_cast<u32>(si)].begin() + static_cast<std::ptrdiff_t>(qidx));
                    continue;
                }
                qsets[static_cast<u32>(si)].erase({x, y});
                qs[static_cast<u32>(si)].erase(qs[static_cast<u32>(si)].begin() + static_cast<std::ptrdiff_t>(qidx));
                clm[ti] = 1;
                overlay[ti] = static_cast<u16>(si);
                secs[static_cast<u32>(si)].coords.push_back({x, y});
                claimed_cnt++;
                for (i32 d = 0; d < 4; ++d) {
                    const i32 nx = static_cast<i32>(x) + dx4[d];
                    const i32 ny = static_cast<i32>(y) + dy4[d];
                    if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w) || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                        continue;
                    }
                    const u32 ni = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                    if (tile_blocked(terrain, glob, ni)) {
                        continue;
                    }
                    if (clm[ni] != 0) {
                        const u16 oid = overlay[ni];
                        if (oid != static_cast<u16>(RIVER_SECTOR_NONE) && oid != static_cast<u16>(si)) {
                            add_conn(&secs[static_cast<u32>(si)], &secs[oid], static_cast<u32>(si), static_cast<u32>(oid));
                        }
                        continue;
                    }
                    if (sect_terr(terrain, glob, ni) != secs[static_cast<u32>(si)].terr) {
                        continue;
                    }
                    const std::pair<u16, u16> c = {static_cast<u16>(nx), static_cast<u16>(ny)};
                    if (qsets[static_cast<u32>(si)].find(c) == qsets[static_cast<u32>(si)].end()) {
                        const i32 ndsq = (nx - static_cast<i32>(px)) * (nx - static_cast<i32>(px))
                            + (ny - static_cast<i32>(py)) * (ny - static_cast<i32>(py));
                        QueueItem ne = {};
                        ne.dsq = ndsq;
                        ne.x = static_cast<u16>(nx);
                        ne.y = static_cast<u16>(ny);
                        qs[static_cast<u32>(si)].push_back(ne);
                        std::sort(qs[static_cast<u32>(si)].begin(), qs[static_cast<u32>(si)].end());
                        qsets[static_cast<u32>(si)].insert(c);
                    }
                }
                break;
            }
            bool has_unclaimed = false;
            for (const QueueItem& e : qs[static_cast<u32>(si)]) {
                const u32 ti = static_cast<u32>(e.y) * static_cast<u32>(w) + static_cast<u32>(e.x);
                if (clm[ti] == 0) {
                    has_unclaimed = true;
                    break;
                }
            }
            if (has_unclaimed) {
                new_act.push_back(si);
            }
        }
        if (claimed_cnt == 0) {
            break;
        }
        act = new_act;
    }
    RiverSectorsResult* out = new RiverSectorsResult();
    if (out == nullptr) {
        GeneratorWhiteboard::release(clm);
        delete[] overlay;
        delete[] glob;
        return nullptr;
    }
    out->w = w;
    out->h = h;
    out->sector_n = static_cast<u16>(sector_n);
    out->overlay = overlay;
    out->nodes = new RiverSectorNode[sector_n];
    if (out->nodes == nullptr) {
        GeneratorWhiteboard::release(clm);
        delete[] overlay;
        delete[] glob;
        delete out;
        return nullptr;
    }
    for (u32 si = 0; si < sector_n; ++si) {
        out->nodes[si].cx = secs[si].cx;
        out->nodes[si].cy = secs[si].cy;
        out->nodes[si].terr = secs[si].terr;
        out->nodes[si].conn_n = static_cast<u16>(secs[si].conn.size());
        out->nodes[si].conn = nullptr;
        if (out->nodes[si].conn_n > 0) {
            out->nodes[si].conn = new u16[out->nodes[si].conn_n];
            for (u16 c = 0; c < out->nodes[si].conn_n; ++c) {
                out->nodes[si].conn[c] = secs[si].conn[c];
            }
        }
    }
    GeneratorWhiteboard::release(clm);
    delete[] glob;
    return out;
}

void Generate_RiverSectors::free_result (RiverSectorsResult* res) {
    if (res == nullptr) {
        return;
    }
    if (res->nodes != nullptr) {
        for (u32 i = 0; i < static_cast<u32>(res->sector_n); ++i) {
            delete[] res->nodes[i].conn;
        }
        delete[] res->nodes;
    }
    delete[] res->overlay;
    delete res;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
