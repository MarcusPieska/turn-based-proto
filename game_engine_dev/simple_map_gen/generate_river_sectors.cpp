//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_river_sectors.h"

#include "generator_constants.h"

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

static bool tile_blocked (u8 cls) {
    return is_water(cls) || is_mountain(cls);
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
    std::vector<std::pair<u16, u16>> coords;
    std::vector<u16> conn;
    SectorBuild () : cx(0), cy(0) {}
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
    bool* clm = new bool[n];
    u16* overlay = new u16[n];
    if (clm == nullptr || overlay == nullptr) {
        delete[] clm;
        delete[] overlay;
        return nullptr;
    }
    std::memset(clm, 0, n * sizeof(bool));
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
        if (tile_blocked(terrain[ti])) {
            continue;
        }
        clm[ti] = true;
        overlay[ti] = static_cast<u16>(si);
        secs[si].coords.push_back({px, py});
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(px) + dx4[d];
            const i32 ny = static_cast<i32>(py) + dy4[d];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w) || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                continue;
            }
            const u32 ni = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
            if (clm[ni] || tile_blocked(terrain[ni])) {
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
                if (!clm[ti]) {
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
                if (clm[ti]) {
                    qsets[static_cast<u32>(si)].erase({x, y});
                    qs[static_cast<u32>(si)].erase(qs[static_cast<u32>(si)].begin() + static_cast<std::ptrdiff_t>(qidx));
                    continue;
                }
                if (e.dsq < min_dsq || e.dsq > max_dsq) {
                    qidx++;
                    continue;
                }
                if (tile_blocked(terrain[ti])) {
                    qsets[static_cast<u32>(si)].erase({x, y});
                    qs[static_cast<u32>(si)].erase(qs[static_cast<u32>(si)].begin() + static_cast<std::ptrdiff_t>(qidx));
                    continue;
                }
                qsets[static_cast<u32>(si)].erase({x, y});
                qs[static_cast<u32>(si)].erase(qs[static_cast<u32>(si)].begin() + static_cast<std::ptrdiff_t>(qidx));
                clm[ti] = true;
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
                    if (tile_blocked(terrain[ni])) {
                        continue;
                    }
                    if (clm[ni]) {
                        const u16 oid = overlay[ni];
                        if (oid != static_cast<u16>(RIVER_SECTOR_NONE) && oid != static_cast<u16>(si)) {
                            add_conn(&secs[static_cast<u32>(si)], &secs[oid], static_cast<u32>(si), static_cast<u32>(oid));
                        }
                    } else {
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
                }
                break;
            }
            bool has_unclaimed = false;
            for (const QueueItem& e : qs[static_cast<u32>(si)]) {
                const u32 ti = static_cast<u32>(e.y) * static_cast<u32>(w) + static_cast<u32>(e.x);
                if (!clm[ti]) {
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
        delete[] overlay;
        delete[] clm;
        return nullptr;
    }
    out->w = w;
    out->h = h;
    out->sector_n = static_cast<u16>(sector_n);
    out->overlay = overlay;
    out->nodes = new RiverSectorNode[sector_n];
    if (out->nodes == nullptr) {
        delete[] overlay;
        delete[] clm;
        delete out;
        return nullptr;
    }
    for (u32 si = 0; si < sector_n; ++si) {
        out->nodes[si].cx = secs[si].cx;
        out->nodes[si].cy = secs[si].cy;
        out->nodes[si].conn_n = static_cast<u16>(secs[si].conn.size());
        out->nodes[si].conn = nullptr;
        if (out->nodes[si].conn_n > 0) {
            out->nodes[si].conn = new u16[out->nodes[si].conn_n];
            for (u16 c = 0; c < out->nodes[si].conn_n; ++c) {
                out->nodes[si].conn[c] = secs[si].conn[c];
            }
        }
    }
    delete[] clm;
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
