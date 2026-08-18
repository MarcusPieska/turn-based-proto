//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TILE_YIELDS_IMP_DUMP_H
#define TILE_YIELDS_IMP_DUMP_H

#include <cstdio>

#include "game_primitives.h"
#include "tile_yields.h"

//================================================================================================================================
//=> - TileYieldsImpDump -
//================================================================================================================================
//
//  Friend of TileYields; dump_job and helpers are defined in the selected impl/tile_yields_impl mk body.
//
//================================================================================================================================

class TileYieldsImpDump {
public:
    static int run ();
    static u16 dump_job (u16 job_idx, FILE* out);

private:
    static bool slot_nz (const TileYields::ImpYldSlot& s);
    static void pr_slot (FILE* out, cstr axis, u8 id, cstr name, const TileYields::ImpYldSlot& s);
    static bool clim_nm (u8 id, cstr* out);
    static bool terr_nm (u8 id, cstr* out);
    static bool ov_nm (u8 id, cstr* out);
    static u16 pr_axis (FILE* out, cstr axis, u16 cap, const TileYields::ImpYldSlot* rows, bool (*name_fn)(u8, cstr*));
};

#endif // TILE_YIELDS_IMP_DUMP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
