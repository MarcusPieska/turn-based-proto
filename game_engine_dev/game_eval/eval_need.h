//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EVAL_NEED_H
#define EVAL_NEED_H

#include "game_primitives.h"
#include "log_dbg/log_need_mask.h"

//================================================================================================================================
//=> - EvalNeed -
//================================================================================================================================
//
//  Declares what a driver requires: LogNeedMask channels and/or saves.
//  save(turn): named turns; EvalDriver fully unpacks each into EvalBin.
//  save_seq(): all matching quartets on disk, ordered by turn; chk verifies only (no bulk unpack).
//
//================================================================================================================================

class EvalNeed {
public:
    static const u16 SAVE_MAX = 16;

    EvalNeed ();

    EvalNeed& trace ();
    LogNeedMask& logs ();
    const LogNeedMask& logs () const;
    EvalNeed& save (u32 turn);
    EvalNeed& save_seq ();

    bool want_trace () const;
    bool want_logs () const;
    bool want_saves () const;
    bool want_save_seq () const;
    u16 save_n () const;
    u32 save_turn (u16 i) const;

private:
    u8 m_trace;
    u8 m_save_seq;
    LogNeedMask m_logs;
    u32 m_save_turn[SAVE_MAX];
    u16 m_save_n;
};

#endif // EVAL_NEED_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
