//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_LAND_SIZE_SAVE_TIMELINE_MNG_H
#define PLAYER_LAND_SIZE_SAVE_TIMELINE_MNG_H

#include "game_primitives.h"
#include "player_land_size_save_timeline.h"

class EvalPaths;
class RuntimeStatics;

//================================================================================================================================
//=> - PlayerLandSizeSaveTimelineMng -
//================================================================================================================================
//
//  One PlayerLandSizeSaveTimeline per seat. fill() loads units+players per save; land avg uses
//  (1 + m_unit_size), max uses (1 + CIV MAX_UNIT_SIZE). sort() by final-save land count desc.
//
//================================================================================================================================

class PlayerLandSizeSaveTimelineMng {
public:
    PlayerLandSizeSaveTimelineMng ();
    ~PlayerLandSizeSaveTimelineMng ();

    void clr ();
    bool setup (u16 player_n, u16 save_n);
    bool fill (const EvalPaths& paths, const RuntimeStatics& st);
    void sort ();

    u16 player_n () const;
    u16 save_n () const;

    PlayerLandSizeSaveTimeline& at (u16 i);
    const PlayerLandSizeSaveTimeline& at (u16 i) const;
    u16 seat (u16 i) const;

private:
    PlayerLandSizeSaveTimelineMng (const PlayerLandSizeSaveTimelineMng&) = delete;
    PlayerLandSizeSaveTimelineMng& operator= (const PlayerLandSizeSaveTimelineMng&) = delete;

    PlayerLandSizeSaveTimeline* m_tl;
    u16* m_seat;
    u16 m_player_n;
    u16 m_save_n;
};

#endif // PLAYER_LAND_SIZE_SAVE_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
