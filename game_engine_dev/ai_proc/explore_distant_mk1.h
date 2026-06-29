//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EXPLORE_DISTANT_MK1_H
#define EXPLORE_DISTANT_MK1_H

#include "game_array_simple.h"
#include "game_primitives.h"
#include "map_bit_overlay.h"

//================================================================================================================================
//=> - ExploreAi -
//================================================================================================================================

class ExploreAi {
public:
    ExploreAi (
        const GameArraySimple& map,
        MapBitOverlay& explored,
        u16 sx,
        u16 sy,
        u16 sight,
        u8 player);
    virtual ~ExploreAi ();
    virtual void move (u16 moves) = 0;
    u16 x () const;
    u16 y () const;
    u16 sx () const;
    u16 sy () const;

protected:
    const GameArraySimple& m_map;
    MapBitOverlay& m_exp;
    u16 m_x;
    u16 m_y;
    u16 m_sx;
    u16 m_sy;
    u16 m_sight;
    u8 m_player;
};

//================================================================================================================================
//=> - ExploreDistantMk1 -
//================================================================================================================================

class ExploreDistantMk1 : public ExploreAi {
public:
    ExploreDistantMk1 (
        const GameArraySimple& map,
        MapBitOverlay& explored,
        u16 sx,
        u16 sy,
        u32 seed,
        u16 sight,
        u8 player);
    ~ExploreDistantMk1 () override;
    void move (u16 moves) override;
    u8 phase () const;
    u8 lim () const;

private:
    void* m_st;
};

#endif // EXPLORE_DISTANT_MK1_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
