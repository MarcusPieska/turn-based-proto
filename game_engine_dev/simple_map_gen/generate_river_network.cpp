//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_river_network.h"

#include "generator_constants.h"

#include <cstring>
#include <set>
#include <utility>
#include <vector>

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

//================================================================================================================================
//=> - Generate_RiverNetwork -
//================================================================================================================================

RiverNetworkResult* Generate_RiverNetwork::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    const RiverSectorsResult* sectors) 
{
    if (terrain == nullptr || w == 0 || h == 0 || sectors == nullptr
        || sectors->overlay == nullptr || sectors->nodes == nullptr || sectors->sector_n == 0) {
        return nullptr;
    }
    const u32 sector_n = static_cast<u32>(sectors->sector_n);
    bool* has_land = new bool[sector_n];
    bool* coastal = new bool[sector_n];
    if (has_land == nullptr || coastal == nullptr) {
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
                if (is_water(terrain[ni])) {
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
        delete[] has_land;
        delete[] coastal;
        return nullptr;
    }
    out->sector_n = sectors->sector_n;
    out->conn_n = 0;
    out->conns = nullptr;
    out->river_sys = new u16[sector_n];
    out->coastal = new bool[sector_n];
    if (out->river_sys == nullptr || out->coastal == nullptr) {
        delete[] out->river_sys;
        delete[] out->coastal;
        delete out;
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
                if (clmd.find(aid) == clmd.end()) {
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
    return out;
}

void Generate_RiverNetwork::free_result (RiverNetworkResult* res) {
    if (res == nullptr) {
        return;
    }
    delete[] res->conns;
    delete[] res->river_sys;
    delete[] res->coastal;
    delete res;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
