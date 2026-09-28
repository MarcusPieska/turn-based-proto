//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LUCKY_SEAT_SAVE_LEDGER_MNG_H
#define LUCKY_SEAT_SAVE_LEDGER_MNG_H

#include "game_primitives.h"

class EvalPaths;
class RuntimeStatics;

//================================================================================================================================
//=> - LuckySeatSaveLedgerMng -
//================================================================================================================================
//
//  Discovers lucky seats from the first players.bin, then for each save appends a block to
//  one text file per lucky seat: header (units/commerce/cities) + unit-name counts.
//
//================================================================================================================================

class LuckySeatSaveLedgerMng {
public:
    LuckySeatSaveLedgerMng ();
    ~LuckySeatSaveLedgerMng ();

    void clr ();
    bool setup (u16 player_n, u16 save_n);
    bool fill (const EvalPaths& paths, cstr out_dir, const RuntimeStatics& st);

    u16 save_n () const;
    u16 lucky_n () const;

private:
    LuckySeatSaveLedgerMng (const LuckySeatSaveLedgerMng&) = delete;
    LuckySeatSaveLedgerMng& operator= (const LuckySeatSaveLedgerMng&) = delete;

    u16* m_lucky;
    u16 m_lucky_n;
    u16 m_player_n;
    u16 m_save_n;
};

#endif // LUCKY_SEAT_SAVE_LEDGER_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
