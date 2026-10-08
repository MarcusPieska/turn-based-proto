//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "intf_mk1_dump_util.h"
#include "intf_mk1_snapshot_view.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_TERR = "/home/w/Projects/simple-map-gen/p1-seed-43/terrain.ppm";
static const char* G_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-43/climate.ppm";
static const char* G_RIV = "/home/w/Projects/simple-map-gen/p1-seed-43/rivers.ppm";

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!mk1_file_ok(G_TERR) || !mk1_file_ok(G_CLIM) || !mk1_file_ok(G_RIV)) {
        std::printf("FAIL: ppm inputs missing\n  terr=%s\n  clim=%s\n  riv=%s\n", G_TERR, G_CLIM, G_RIV);
        return 1;
    }
    IntfMk1_SnapshotView snap;
    if (!snap.load_map_ppms(G_TERR, G_CLIM, G_RIV, nullptr) || !snap.ready()) {
        std::printf("FAIL: load_map_ppms\n");
        return 1;
    }
    std::printf("mk1 ppm dump loaded %u x %u\n", static_cast<u32>(snap.width()), static_cast<u32>(snap.height()));
    if (!mk1_dump_layers(snap, "ppm")) {
        std::printf("FAIL: dump layers\n");
        return 1;
    }
    std::printf("ok: intf_mk1_ppm_dump_tester\n");
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
