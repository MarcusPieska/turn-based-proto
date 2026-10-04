//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_OWN_XFER_SAVE_TIMELINE_MNG_H
#define MAP_OWN_XFER_SAVE_TIMELINE_MNG_H

#include "game_primitives.h"

class EvalPaths;

//================================================================================================================================
//=> - MapOwnXferSaveTimelineMng -
//================================================================================================================================
//
//  Compares consecutive map saves; writes xfer_tXXXX.ppm for each save after the first (skip first),
//  plus xfer_all.ppm. Also writes the all-transfer summary to share_dir as tile_own_xfer.ppm.
//
//================================================================================================================================

class MapOwnXferSaveTimelineMng {
public:
    MapOwnXferSaveTimelineMng ();
    ~MapOwnXferSaveTimelineMng ();

    void clr ();
    bool setup (u16 save_n);
    bool fill (const EvalPaths& paths, cstr out_dir, cstr share_dir = nullptr);

    u16 save_n () const;
    u16 wrote_n () const;

private:
    MapOwnXferSaveTimelineMng (const MapOwnXferSaveTimelineMng&) = delete;
    MapOwnXferSaveTimelineMng& operator= (const MapOwnXferSaveTimelineMng&) = delete;

    u16 m_save_n;
    u16 m_wrote_n;
};

#endif // MAP_OWN_XFER_SAVE_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
