//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <algorithm>
#include <cstring>
#include <set>
#include <utility>
#include <vector>

#include "generate_river_network.h"

#include "generate_global_ocean.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static bool is_hills (u8 terr) {
    return terr == TERR_HILLS[0];
}

static bool is_plains (u8 terr) {
    return terr == TERR_PLAINS[0];
}

static bool conn_ok (const RiverSectorNode* from, const RiverSectorNode* to) {
    if (is_hills(from->terr) && is_plains(to->terr)) {
        return false;
    }
    return true;
}

static u16* alloc_overlay (u16 w, u16 h) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* ov = new u16[n];
    if (ov == nullptr) {
        return nullptr;
    }
    for (u32 i = 0; i < n; ++i) {
        ov[i] = static_cast<u16>(RIVER_BASIN_NONE);
    }
    return ov;
}

static void build_basins (RiverNetworkResult* out, const RiverSectorsResult* sectors) 
{
    if (out == nullptr || sectors == nullptr || out->overlay == nullptr || out->river_sys == nullptr) {
        return;
    }
    const u32 n = static_cast<u32>(out->w) * static_cast<u32>(out->h);
    std::set<u16> roots;
    for (u32 si = 0; si < static_cast<u32>(out->sector_n); ++si) {
        const u16 sys = out->river_sys[si];
        if (sys != static_cast<u16>(RIVER_SYS_NONE)) {
            roots.insert(sys);
        }
    }
    if (roots.empty()) {
        out->basin_n = 0;
        out->basins = nullptr;
        return;
    }
    std::vector<u16> root_lst(roots.begin(), roots.end());
    std::vector<u16> sys_idx(static_cast<size_t>(out->sector_n), static_cast<u16>(RIVER_BASIN_NONE));
    std::vector<RiverBasinEntry> recs;
    recs.reserve(root_lst.size());
    u16 next_idx = 1;
    for (u16 root : root_lst) {
        RiverBasinEntry e = {};
        e.idx = next_idx;
        e.mouth_x = sectors->nodes[root].cx;
        e.mouth_y = sectors->nodes[root].cy;
        e.tile_n = 0;
        recs.push_back(e);
        for (u32 si = 0; si < static_cast<u32>(out->sector_n); ++si) {
            if (out->river_sys[si] == root) {
                sys_idx[si] = next_idx;
            }
        }
        next_idx++;
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 sid = sectors->overlay[i];
        if (sid == static_cast<u16>(RIVER_SECTOR_NONE) || sid >= out->sector_n) {
            continue;
        }
        const u16 bidx = sys_idx[sid];
        if (bidx == static_cast<u16>(RIVER_BASIN_NONE)) {
            continue;
        }
        out->overlay[i] = bidx;
        for (RiverBasinEntry& e : recs) {
            if (e.idx == bidx) {
                e.tile_n++;
                break;
            }
        }
    }
    std::sort(recs.begin(), recs.end(), [](const RiverBasinEntry& a, const RiverBasinEntry& b) {
        return a.tile_n > b.tile_n;
    });
    out->basin_n = static_cast<u16>(recs.size());
    out->basins = new RiverBasinEntry[out->basin_n];
    if (out->basins == nullptr) {
        out->basin_n = 0;
        return;
    }
    for (u16 i = 0; i < out->basin_n; ++i) {
        out->basins[i] = recs[i];
    }
}

//================================================================================================================================
//=> - Generate_RiverNetwork -
//================================================================================================================================

RiverNetworkResult* Generate_RiverNetwork::generate (const u8* terrain, u16 w, u16 h, const RiverSectorsResult* sectors)  {
    if (terrain == nullptr || w == 0 || h == 0 || sectors == nullptr
        || sectors->overlay == nullptr || sectors->nodes == nullptr || sectors->sector_n == 0) {
        return nullptr;
    }
    const u32 sector_n = static_cast<u32>(sectors->sector_n);
    u8* glob = Generate_GlobalOcean::build_mask(terrain, w, h);
    if (glob == nullptr) {
        return nullptr;
    }
    bool* has_land = new bool[sector_n];
    bool* coastal = new bool[sector_n];
    if (has_land == nullptr || coastal == nullptr) {
        delete[] glob;
        delete[] has_land;
        delete[] coastal;
        return nullptr;
    }
    std::memset(has_land, 0, sector_n * sizeof(bool));
    std::memset(coastal, 0, sector_n * sizeof(bool));
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    for (u32 py = 0; py < static_cast<u32>(h); ++py) {
        for (u32 px = 0; px < static_cast<u32>(w); ++px) {
            const u32 ti = py * static_cast<u32>(w) + px;
            const u16 sid = sectors->overlay[ti];
            if (sid == static_cast<u16>(RIVER_SECTOR_NONE)) {
                continue;
            }
            has_land[sid] = true;
            for (i32 d = 0; d < 4; ++d) {
                const i32 nx = static_cast<i32>(px) + dx4[d];
                const i32 ny = static_cast<i32>(py) + dy4[d];
                if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w) || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                    continue;
                }
                const u32 ni = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                if (glob[ni] != 0) {
                    coastal[sid] = true;
                    break;
                }
            }
        }
    }
    std::vector<u16> coast;
    for (u32 si = 0; si < sector_n; ++si) {
        if (has_land[si] && coastal[si]) {
            coast.push_back(static_cast<u16>(si));
        }
    }
    RiverNetworkResult* out = new RiverNetworkResult();
    if (out == nullptr) {
        delete[] glob;
        delete[] has_land;
        delete[] coastal;
        return nullptr;
    }
    out->w = w;
    out->h = h;
    out->sector_n = sectors->sector_n;
    out->conn_n = 0;
    out->conns = nullptr;
    out->basin_n = 0;
    out->basins = nullptr;
    out->overlay = alloc_overlay(w, h);
    out->river_sys = new u16[sector_n];
    out->coastal = new bool[sector_n];
    if (out->overlay == nullptr || out->river_sys == nullptr || out->coastal == nullptr) {
        delete[] out->overlay;
        delete[] out->river_sys;
        delete[] out->coastal;
        delete out;
        delete[] glob;
        delete[] has_land;
        delete[] coastal;
        return nullptr;
    }
    for (u32 si = 0; si < sector_n; ++si) {
        out->river_sys[si] = static_cast<u16>(RIVER_SYS_NONE);
        out->coastal[si] = coastal[si] && has_land[si];
    }
    delete[] has_land;
    delete[] coastal;
    if (coast.empty()) {
        delete[] glob;

        return out;
    }
    for (u16 cid : coast) {
        out->river_sys[cid] = cid;
    }
    std::set<u16> clmd;
    std::vector<u16> q = coast;
    std::set<std::pair<u16, u16>> rconns;
    for (u16 id : coast) {
        clmd.insert(id);
    }
    while (!q.empty()) {
        std::vector<u16> nq;
        for (u16 sidx : q) {
            bool any = false;
            const RiverSectorNode* sec = &sectors->nodes[sidx];
            for (u16 c = 0; c < sec->conn_n; ++c) {
                const u16 aid = sec->conn[c];
                if (clmd.find(aid) == clmd.end() && conn_ok(sec, &sectors->nodes[aid])) {
                    clmd.insert(aid);
                    out->river_sys[aid] = out->river_sys[sidx];
                    rconns.insert({sidx, aid});
                    nq.push_back(aid);
                    any = true;
                }
            }
            if (any) {
                nq.push_back(sidx);
            }
        }
        q = nq;
    }
    out->conn_n = static_cast<u16>(rconns.size());
    if (out->conn_n > 0) {
        out->conns = new RiverConn[out->conn_n];
        u16 ci = 0;
        for (const auto& conn : rconns) {
            out->conns[ci].a = conn.first;
            out->conns[ci].b = conn.second;
            ci++;
        }
    }
    build_basins(out, sectors);
    delete[] glob;
    return out;
}

void Generate_RiverNetwork::free_result (RiverNetworkResult* res) {
    if (res == nullptr) {
        return;
    }
    delete[] res->conns;
    delete[] res->river_sys;
    delete[] res->coastal;
    delete[] res->overlay;
    delete[] res->basins;
    delete res;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
