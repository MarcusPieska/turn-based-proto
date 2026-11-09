//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LUCKY_SECT_ORDER_MNG_H
#define LUCKY_SECT_ORDER_MNG_H

#include "game_primitives.h"

class EvalLog;
class EvalPaths;

//================================================================================================================================
//=> - LuckySectOrderMng -
//================================================================================================================================
//
//  Per lucky seat: sector walk from WarSect* logs (begin/select/skip), paint from
//  derived_statics.bin, bleached map PPM + sequence annotation under out_dir.
//
//================================================================================================================================

class LuckySectOrderMng {
public:
    static const u16 k_ord_cap = 1024u;

    LuckySectOrderMng ();
    ~LuckySectOrderMng ();

    void clr ();
    bool setup (u16 player_n, u16 save_n);
    bool fill (const EvalPaths& paths, const EvalLog& log, cstr out_dir);

    u16 wrote_n () const;
    u16 lucky_n () const;
    u16 lucky_at (u16 i) const;

private:
    LuckySectOrderMng (const LuckySectOrderMng&) = delete;
    LuckySectOrderMng& operator= (const LuckySectOrderMng&) = delete;

    bool push_ord (u16 li, u16 sector);
    bool push_skip (u16 li, u16 sector);

    u16* m_lucky;
    u16 m_lucky_n;
    u16 m_player_n;
    u16 m_save_n;
    u16 m_wrote_n;
    u16* m_ord;
    u16* m_ord_n;
    u16* m_skip;
    u16* m_skip_n;
    u16* m_begin;
};

#endif // LUCKY_SECT_ORDER_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
