//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "whiteboard_mng_visual_util.h"

//================================================================================================================================
//=> - Basic checkout and coalesce -
//================================================================================================================================

void wb_vis_run_basic () {
    std::printf("\n######## basic ########\n");
    static const i8 s_fullest[] = {1, 1, 1, 2, -2, -1, -1, -1};
    static const i8 s_coalesce[] = {4, -4, 8, -8};
    static const i8 s_one_fill[] = {1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1};
    static const i8 s_two_pair[] = {2, 2, 2, 2, -2, -2, -2, -2};
    static const i8 s_four_half[] = {4, 4, -4, -4};
    static const i8 s_eight_full[] = {8, -8};
    static const i8 s_nest[] = {1, 2, 4, 8, -8, -4, -2, -1};
    static const i8 s_spill[] = {8, 1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -8};
    static const i8 s_two_slab_8[] = {8, 8, -8, -8};
    static const i8 s_alt_12[] = {1, 2, 1, 2, 1, 2, 1, 2, -2, -1, -2, -1, -2, -1, -2, -1};
    static const i8 s_4b_then_1b[] = {4, 1, 1, 1, 1, -1, -1, -1, -1, -4};
    static const i8 s_2b_edge[] = {2, 2, 2, 2, 1, 1, 1, 1, -1, -1, -1, -1, -2, -2, -2, -2};
    static const i8 s_reuse_empty[] = {1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, 1, 1, 1, 1};
    static const i8 s_mixed_sz[] = {1, 1, 2, 1, 4, 1, -1, -1, -1, -1, -2, -4};
    static const i8 s_full_then_2[] = {1, 1, 1, 1, 1, 1, 1, 1, 2, -2, -1, -1, -1, -1, -1, -1, -1, -1};
    WhiteboardMngTestUtil::run_seq("fullest slab 2b after three 1b", nullptr, s_fullest, WB_VIS_N(s_fullest), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("coalesce 4b then 8b", nullptr, s_coalesce, WB_VIS_N(s_coalesce), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("eight 1b fill and drain", nullptr, s_one_fill, WB_VIS_N(s_one_fill), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("four 2b pairs", nullptr, s_two_pair, WB_VIS_N(s_two_pair), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("two 4b halves", nullptr, s_four_half, WB_VIS_N(s_four_half), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("single 8b exclusive", nullptr, s_eight_full, WB_VIS_N(s_eight_full), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("nested 1-2-4-8 unwind", nullptr, s_nest, WB_VIS_N(s_nest), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("8b blocks slab 1b spill", nullptr, s_spill, WB_VIS_N(s_spill), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("two slabs each 8b", nullptr, s_two_slab_8, WB_VIS_N(s_two_slab_8), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("alternating 1b 2b", nullptr, s_alt_12, WB_VIS_N(s_alt_12), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("4b then four 1b", nullptr, s_4b_then_1b, WB_VIS_N(s_4b_then_1b), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("four 2b then four 1b", nullptr, s_2b_edge, WB_VIS_N(s_2b_edge), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("full drain partial refill", nullptr, s_reuse_empty, WB_VIS_N(s_reuse_empty), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("mixed sizes unwind", nullptr, s_mixed_sz, WB_VIS_N(s_mixed_sz), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("full slab then 2b", nullptr, s_full_then_2, WB_VIS_N(s_full_then_2), 2, 2, RET_RIGHT); 
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
