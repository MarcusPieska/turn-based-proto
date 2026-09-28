//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_OWN_SAVE_TIMELINE_MNG_H
#define MAP_OWN_SAVE_TIMELINE_MNG_H

#include "game_primitives.h"

class EvalPaths;

//================================================================================================================================
//=> - MapOwnSaveTimelineMng -
//================================================================================================================================
//
//  Loads one map blob per save and writes ownership PPMs into out_dir (own_tXXXX.ppm).
//
//================================================================================================================================

class MapOwnSaveTimelineMng {
public:
    MapOwnSaveTimelineMng ();
    ~MapOwnSaveTimelineMng ();

    void clr ();
    bool setup (u16 save_n);
    bool fill (const EvalPaths& paths, cstr out_dir);

    u16 save_n () const;
    u16 wrote_n () const;

private:
    MapOwnSaveTimelineMng (const MapOwnSaveTimelineMng&) = delete;
    MapOwnSaveTimelineMng& operator= (const MapOwnSaveTimelineMng&) = delete;

    u16 m_save_n;
    u16 m_wrote_n;
};

#endif // MAP_OWN_SAVE_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
