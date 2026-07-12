//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "whiteboard_mng_visual_util.h"

static const u16 k_lim = 5u;

static int g_tc = 0;
static int g_tp = 0;
static int g_tf = 0;

static void nt (bool ok, cstr msg) {
    ++g_tc;
    if (ok) {
        ++g_tp;
        std::printf("  ok  %s\n", msg ? msg : "");
    } else {
        ++g_tf;
        std::printf("  FAIL %s\n", msg ? msg : "");
    }
}

static void run_lim (cstr label, cstr tag, const i8* seq, u32 n) {
    WhiteboardMngTestUtil::run_seq(label, tag, seq, n, 2, 2, RET_RIGHT, k_lim);
}

static void test_wrappers () {
    std::printf("\n=== wrapper ok() slab_lim=5 ===\n");
    WhiteboardMng::init(2, 2, k_lim);
    {
        Whiteboard_8B a("Lim", "w8_a", 1);
        Whiteboard_8B b("Lim", "w8_b", 1);
        Whiteboard_8B c("Lim", "w8_c", 1);
        Whiteboard_8B d("Lim", "w8_d", 1);
        Whiteboard_8B e("Lim", "w8_e", 1);
        nt(a.ok(), "five 8b a");
        nt(b.ok(), "five 8b b");
        nt(c.ok(), "five 8b c");
        nt(d.ok(), "five 8b d");
        nt(e.ok(), "five 8b e");
        nt(WhiteboardMngTestUtil::allocated_max() == 5, "five slabs used");
        Whiteboard_8B f("Lim", "w8_f", 2);
        nt(!f.ok(), "sixth 8b fails while five held");
        nt(WhiteboardMngTestUtil::allocated_max() == 5, "still five slabs");
        (void)f;
    }
    WhiteboardMng::terminate();
    WhiteboardMng::init(2, 2, k_lim);
    {
        Whiteboard_8B h0("Lim", "w8_h0", 3);
        Whiteboard_8B h1("Lim", "w8_h1", 3);
        Whiteboard_8B h2("Lim", "w8_h2", 3);
        Whiteboard_8B h3("Lim", "w8_h3", 3);
        Whiteboard_8B h4("Lim", "w8_h4", 3);
        nt(h0.ok() && h1.ok() && h2.ok() && h3.ok() && h4.ok(), "five held 8b all ok");
        Whiteboard_8B over("Lim", "w8_over", 3);
        nt(!over.ok(), "sixth while five held fails");
        (void)h0; (void)h1; (void)h2; (void)h3; (void)h4;
    }
    {
        u8 clr = 1;
        u16 n = WhiteboardMngTestUtil::allocated_max();
        u8* sl = WhiteboardMngTestUtil::slot_allocator();
        for (u16 i = 0; i < n; ++i) {
            if (sl[i] != 0) {
                clr = 0;
            }
        }
        nt(clr != 0, "all slot bits cleared after scope");
    }
    WhiteboardMng::terminate();
    WhiteboardMng::init(2, 2, k_lim);
    {
        Whiteboard_8B a("Lim", "w8_rel", 4);
        Whiteboard_8B b("Lim", "w8_rel", 4);
        Whiteboard_8B c("Lim", "w8_rel", 4);
        Whiteboard_8B d("Lim", "w8_rel", 4);
        Whiteboard_8B e("Lim", "w8_rel", 4);
        (void)a; (void)b; (void)c; (void)d; (void)e;
        Whiteboard_8B f("Lim", "w8_rel", 4);
        nt(!f.ok(), "full before release");
    }
    {
        Whiteboard_8B g("Lim", "w8_after", 4);
        nt(g.ok(), "checkout after full release");
    }
    WhiteboardMng::terminate();
    std::printf("wrapper checks: %d run %d pass %d fail\n", g_tc, g_tp, g_tf);
    WhiteboardMngTestUtil::wait_enter();
}

//================================================================================================================================
//=> - Limit sequences (slab_lim=5) -
//================================================================================================================================

void wb_vis_run_limit () {
    std::printf("\n######## limit (5 slabs) ########\n");
    test_wrappers();
    static const i8 s_five_8b[] = {8, 8, 8, 8, 8, 8};
    static const i8 s_rel_one[] = {8, 8, 8, 8, 8, -8, 8};
    static const i8 s_forty_1b[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    static const i8 s_fit_reuse[] = {8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    static const i8 s_two_full[] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};
    static const i8 s_4b_five[] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
    static const i8 s_drain_reopen[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 1, 1, 1, 1, 1};
    static const i8 s_mix_fail[] = {8, 8, 8, 8, 8, 2, 4, 1};
    static const i8 s_partial_rel[] = {8, 8, 8, 8, 8, -8, -8, 8, 8, 8, 8, 8, 8};
    run_lim("five 8b sixth fails", "lim5_five_8b", s_five_8b, WB_VIS_N(s_five_8b));
    run_lim("release one 8b then ok", "lim5_rel_one", s_rel_one, WB_VIS_N(s_rel_one));
    run_lim("forty 1b then fail", "lim5_forty_1b", s_forty_1b, WB_VIS_N(s_forty_1b));
    run_lim("8b plus 1b in same slab", "lim5_fit_reuse", s_fit_reuse, WB_VIS_N(s_fit_reuse));
    run_lim("eleven 8b two waves", "lim5_two_full", s_two_full, WB_VIS_N(s_two_full));
    run_lim("eleven 4b pairs", "lim5_4b_five", s_4b_five, WB_VIS_N(s_4b_five));
    run_lim("drain 1b reopen slots", "lim5_drain_reopen", s_drain_reopen, WB_VIS_N(s_drain_reopen));
    run_lim("full then small ok fail", "lim5_mix_fail", s_mix_fail, WB_VIS_N(s_mix_fail));
    run_lim("partial 8b release refill", "lim5_partial_rel", s_partial_rel, WB_VIS_N(s_partial_rel));
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
