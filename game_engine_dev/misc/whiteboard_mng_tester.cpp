//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>

#include "whiteboard_mng.h"

static int g_tc = 0;
static int g_tp = 0;
static int g_tf = 0;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void nt (bool ok, cstr msg) {
    ++g_tc;
    if (ok) {
        ++g_tp;
    } else {
        ++g_tf;
        std::printf("*** TEST FAILED: %s\n", msg ? msg : "");
    }
}

static u8 slot_fill (u8* slots, u16 blk) {
    u8 v = slots[blk];
    u8 n = 0;
    for (u8 b = 0; b < 8; ++b) {
        if ((v & (u8)(1u << b)) != 0) {
            ++n;
        }
    }
    return n;
}

static void run_basic () {
    WhiteboardMng::init(4, 4);
    nt(WhiteboardMng::width() == 4, "width");
    nt(WhiteboardMng::height() == 4, "height");
    nt(WhiteboardMng::tile_n() == 16, "tile_n");
    {
        Whiteboard_1B a("T", "basic_1b", 1);
        nt(a.ok(), "1b ok");
        a.wr(2, 3, 42);
        nt(a.rd(2, 3) == 42, "1b rd/wr");
    }
    {
        Whiteboard_2B b("T", "basic_2b", 2);
        b.wr_i(0, 0xABCD);
        nt(b.rd_i(0) == 0xABCD, "2b rd_i/wr_i");
    }
    WhiteboardMng::terminate();
}

static void run_fullest_chunk () {
    WhiteboardMng::init(2, 2);
    Whiteboard_1B a("T", "fill_a", 1);
    Whiteboard_1B b("T", "fill_b", 1);
    Whiteboard_1B c("T", "fill_c", 1);
    nt(a.blk() == b.blk(), "1b same slab a/b");
    nt(b.blk() == c.blk(), "1b same slab b/c");
    nt(a.blk() != 255, "1b valid blk");
    u8* slots = WhiteboardMngTestUtil::slot_allocator();
    nt(slot_fill(slots, a.blk()) == 3, "three 1b in one slab");
    Whiteboard_2B d("T", "fill_d", 1);
    nt(d.blk() == a.blk(), "2b reuses fullest slab");
    nt(WhiteboardMngTestUtil::allocated_max() == 1, "one slab allocated");
    WhiteboardMngTestUtil::dump_inv();
    WhiteboardMng::terminate();
}

static void run_coalesce () {
    WhiteboardMng::init(2, 2);
    {
        Whiteboard_4B x("T", "coalesce", 5);
        nt(x.blk() == 0, "4b first blk");
    }
    u8* slots = WhiteboardMngTestUtil::slot_allocator();
    nt(slots[0] == 0, "slab cleared after 4b return");
    {
        Whiteboard_8B y("T", "coalesce", 6);
        nt(y.sub() == 0, "8b sub");
        nt(slots[y.blk()] == 0xFFu, "8b fills slab");
    }
    WhiteboardMng::terminate();
}

static void run_oob () {
    WhiteboardMng::init(2, 2);
    Whiteboard_1B a("T", "oob", 9);
    bool threw = false;
    std::printf("expect OOB abort next (WB_TRACE_DB)...\n");
    if (a.ok()) {
        a.rd(9, 0);
    }
    (void)threw;
    WhiteboardMng::terminate();
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

static void run_soft_fail () {
    WhiteboardMng::init(2, 2, 2u);
    {
        Whiteboard_8B a("T", "sf_a", 1);
        Whiteboard_8B b("T", "sf_b", 1);
        nt(a.ok(), "soft fail a");
        nt(b.ok(), "soft fail b");
        Whiteboard_8B c("T", "sf_c", 1);
        nt(!c.ok(), "soft fail c exhausted");
    }
    WhiteboardMng::terminate();
}

int main () {
    run_basic();
    run_fullest_chunk();
    run_coalesce();
    run_soft_fail();
    std::printf("skip run_oob in batch (aborts under WB_TRACE_DB)\n");
    std::printf("tests: %d run, %d pass, %d fail\n", g_tc, g_tp, g_tf);
    return g_tf > 0 ? 1 : 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
