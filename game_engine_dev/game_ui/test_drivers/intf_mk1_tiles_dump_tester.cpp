//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include <dirent.h>

#include "intf_mk1_dump_util.h"
#include "intf_mk1_snapshot_view.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_SAVES = "/home/w/Projects/game-saves";

//================================================================================================================================
//=> - Save pick -
//================================================================================================================================

struct MapPick {
    char m_map[512];
    u32 m_seed;
    u32 m_turn;
    u16 m_pn;
};

static bool better (u32 turn, u32 seed, u16 pn, const MapPick& cur) {
    if (turn != cur.m_turn) {
        return turn > cur.m_turn;
    }
    if (seed != cur.m_seed) {
        return seed > cur.m_seed;
    }
    return pn > cur.m_pn;
}

static bool pick_latest_map (MapPick* out) {
    if (out == nullptr) {
        return false;
    }
    DIR* d = ::opendir(G_SAVES);
    if (d == nullptr) {
        std::printf("FAIL: cannot open save folder '%s'\n", G_SAVES);
        return false;
    }
    bool found = false;
    MapPick best = {};
    while (const dirent* e = ::readdir(d)) {
        if (e->d_name[0] == '.') {
            continue;
        }
        unsigned seed = 0;
        unsigned pn = 0;
        unsigned turn = 0;
        if (std::sscanf(e->d_name, "game-loop-seed-%u-p%u-%u.bin", &seed, &pn, &turn) != 3) {
            continue;
        }
        char expect[256];
        if (std::snprintf(expect, sizeof(expect), "game-loop-seed-%u-p%u-%04u.bin", seed, pn, turn) <= 0
            || std::strcmp(e->d_name, expect) != 0) {
            continue;
        }
        MapPick cand = {};
        cand.m_seed = seed;
        cand.m_turn = turn;
        cand.m_pn = static_cast<u16>(pn);
        if (std::snprintf(cand.m_map, sizeof(cand.m_map), "%s/%s", G_SAVES, expect) <= 0) {
            continue;
        }
        if (!mk1_file_ok(cand.m_map)) {
            continue;
        }
        if (!found || better(cand.m_turn, cand.m_seed, cand.m_pn, best)) {
            best = cand;
            found = true;
        }
    }
    ::closedir(d);
    if (!found) {
        std::printf("FAIL: no map tiles .bin in '%s'\n", G_SAVES);
        return false;
    }
    *out = best;
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    MapPick src = {};
    if (!pick_latest_map(&src)) {
        return 1;
    }
    std::printf("mk1 tiles dump using seed=%u players=%u turn=%u\n",
        src.m_seed, static_cast<u32>(src.m_pn), src.m_turn);
    std::printf("  map=%s\n", src.m_map);

    IntfMk1_SnapshotView snap;
    if (!snap.load_map_tiles(src.m_map) || !snap.ready()) {
        std::printf("FAIL: load_map_tiles\n");
        return 1;
    }
    std::printf("loaded %u x %u\n",
        static_cast<u32>(snap.width()), static_cast<u32>(snap.height()));
    if (!mk1_dump_layers(snap, "tiles")) {
        std::printf("FAIL: dump layers\n");
        return 1;
    }
    std::printf("ok: intf_mk1_tiles_dump_tester\n");
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
