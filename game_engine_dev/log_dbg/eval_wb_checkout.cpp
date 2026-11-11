//================================================================================================================================
//= WARNING =
//================================================================================================================================
//=
//=  - First-create stub from gen_log_dbg.py (TEMPLATE_eval.cpp).
//=  - Regen will NOT overwrite this file once it exists; implement EVAL here.
//=  - Real body always compiled into log_dbg.so (LOG_DBG_SO_BUILD).
//=
//================================================================================================================================
//= Includes =
//================================================================================================================================

#define LOG_DBG_SO_BUILD
#include "eval_wb_checkout.h"

#include "log_wb_inv.h"
#include "log_wb_inv_item.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//= EVAL_WB_CHECKOUT =
//================================================================================================================================

struct WbInvEmit {
    unsigned turn;
};

static void wb_inv_emit (u8 sz_b, u8 blk, u8 sub, u32 wb_turn, cstr cls, cstr fn, void* ctx) {
    WbInvEmit* e = static_cast<WbInvEmit*>(ctx);
    if (e == nullptr) {
        return;
    }
    LOG_WB_INV_ITEM::LOG(
        e->turn,
        SC_U32(sz_b),
        SC_U32(blk),
        SC_U32(sub),
        wb_turn,
        cls != nullptr ? cls : "",
        fn != nullptr ? fn : "");
}

void EVAL_WB_CHECKOUT::EVAL (unsigned turn) {
    LOG_WB_INV::LOG(turn, "begin");
    WbInvEmit e;
    e.turn = turn;
    WhiteboardMngTestUtil::each_chkout(wb_inv_emit, &e);
    LOG_WB_INV::LOG(turn, "end");
}

//================================================================================================================================
//= End of file =
//================================================================================================================================
