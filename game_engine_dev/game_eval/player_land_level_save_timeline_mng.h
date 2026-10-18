//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_LAND_LEVEL_SAVE_TIMELINE_MNG_H
#define PLAYER_LAND_LEVEL_SAVE_TIMELINE_MNG_H

#include "game_primitives.h"
#include "player_land_level_save_timeline.h"

class EvalPaths;
class RuntimeStatics;

//================================================================================================================================
//=> - PlayerLandLevelSaveTimelineMng -
//================================================================================================================================
//
//  One PlayerLandLevelSaveTimeline per seat. fill() loads units+players per save; avg uses m_level
//  for domain LAND with role != NONE. Max uses UNIT_TRAINING_MAX. Final-save level hist by seat.
//  Captures m_lucky; sort by land n.
//
//================================================================================================================================

class PlayerLandLevelSaveTimelineMng {
public:
    PlayerLandLevelSaveTimelineMng ();
    ~PlayerLandLevelSaveTimelineMng ();

    void clr ();
    bool setup (u16 player_n, u16 save_n);
    bool fill (const EvalPaths& paths, const RuntimeStatics& st);
    void sort ();

    u16 player_n () const;
    u16 save_n () const;
    u16 lvl_n () const;
    u8 lucky (u16 seat) const;
    u32 final_lvl_count (u16 seat, u16 lv) const;

    PlayerLandLevelSaveTimeline& at (u16 i);
    const PlayerLandLevelSaveTimeline& at (u16 i) const;
    u16 seat (u16 i) const;

private:
    PlayerLandLevelSaveTimelineMng (const PlayerLandLevelSaveTimelineMng&) = delete;
    PlayerLandLevelSaveTimelineMng& operator= (const PlayerLandLevelSaveTimelineMng&) = delete;

    PlayerLandLevelSaveTimeline* m_tl;
    u16* m_seat;
    u8* m_lucky;
    u32* m_final_lvl;
    u16 m_player_n;
    u16 m_save_n;
    u16 m_lvl_n;
};

#endif // PLAYER_LAND_LEVEL_SAVE_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
