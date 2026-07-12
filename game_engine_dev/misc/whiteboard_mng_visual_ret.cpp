//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "whiteboard_mng_visual_util.h"

#include <cstdio>

static void run_tri (cstr stem, const i8* seq, u32 n, u16 w, u16 h) {
    char buf[128];
    std::snprintf(buf, sizeof(buf), "%s LEFT", stem);
    WhiteboardMngTestUtil::run_seq(buf, nullptr, seq, n, w, h, RET_LEFT);
    std::snprintf(buf, sizeof(buf), "%s RIGHT", stem);
    WhiteboardMngTestUtil::run_seq(buf, nullptr, seq, n, w, h, RET_RIGHT);
    std::snprintf(buf, sizeof(buf), "%s MID", stem);
    WhiteboardMngTestUtil::run_seq(buf, nullptr, seq, n, w, h, RET_MID);
}

//================================================================================================================================
//=> - Return policy comparison -
//================================================================================================================================

void wb_vis_run_ret () {
    std::printf("\n######## ret policy ########\n");
    static const i8 s_one_1b[] = {1, 1, 1, 1, 1, 1, 1, 1, -1};
    static const i8 s_two_1b[] = {1, 1, 1, 1, 1, 1, 1, 1, -1, -1};
    static const i8 s_three_1b[] = {1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1};
    static const i8 s_four_1b[] = {1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1};
    static const i8 s_one_2b[] = {2, 2, 2, 2, 2, 2, 2, 2, -2};
    static const i8 s_two_2b[] = {2, 2, 2, 2, 2, 2, 2, 2, -2, -2};
    static const i8 s_one_4b[] = {4, 4, 4, 4, -4};
    static const i8 s_one_8b[] = {8, 8, 8, 8, -8};
    static const i8 s_mix_a[] = {1, 1, 2, 1, 1, 2, -2, -1, -1, -2, -1, -1};
    static const i8 s_mix_b[] = {1, 2, 1, 2, 1, 2, 1, 2, -1, -2, -1, -2, -1, -2, -1, -2};
    static const i8 s_drain_half[] = {1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1};
    static const i8 s_2b_mid[] = {2, 2, 2, 2, -2, -2, -2, -2};
    static const i8 s_interleave[] = {1, 2, 1, 2, 1, 2, 1, 2, -1, -2, -1, -2, -1, -2, -1, -2};
    run_tri("single 1b ret among eight", s_one_1b, WB_VIS_N(s_one_1b), 2, 2);
    run_tri("two 1b ret among eight", s_two_1b, WB_VIS_N(s_two_1b), 2, 2);
    run_tri("three 1b ret among eight", s_three_1b, WB_VIS_N(s_three_1b), 2, 2);
    run_tri("four 1b ret among eight", s_four_1b, WB_VIS_N(s_four_1b), 2, 2);
    run_tri("single 2b ret among eight", s_one_2b, WB_VIS_N(s_one_2b), 2, 2);
    run_tri("two 2b ret among eight", s_two_2b, WB_VIS_N(s_two_2b), 2, 2);
    run_tri("single 4b ret among four", s_one_4b, WB_VIS_N(s_one_4b), 2, 2);
    run_tri("single 8b ret among four", s_one_8b, WB_VIS_N(s_one_8b), 2, 2);
    run_tri("mixed 1b 2b returns", s_mix_a, WB_VIS_N(s_mix_a), 2, 2);
    run_tri("interleaved 1b 2b alt ret", s_mix_b, WB_VIS_N(s_mix_b), 2, 2);
    run_tri("drain half eight 1b", s_drain_half, WB_VIS_N(s_drain_half), 2, 2);
    run_tri("four 2b drain order", s_2b_mid, WB_VIS_N(s_2b_mid), 2, 2);
    run_tri("interleave paired ret", s_interleave, WB_VIS_N(s_interleave), 2, 2);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
