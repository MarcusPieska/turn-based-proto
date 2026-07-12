//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "whiteboard_mng_visual_util.h"

//================================================================================================================================
//=> - Packing and multi-slab -
//================================================================================================================================

void wb_vis_run_pack () {
    std::printf("\n######## pack ########\n");
    static const i8 s_pack[] = {1, 1, 1, 1, 1, 1, 1, 1, 2, 4, -4, -2, -1, -1, -1, -1, -1, -1, -1, -1};
    static const i8 s_dual_slab[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    static const i8 s_8_4_2_1[] = {8, 4, 2, 1, -1, -2, -4, -8};
    static const i8 s_fill_a[] = {1, 1, 1, 1, 2, 2, -2, -1, -1, -1, -1};
    static const i8 s_fill_b[] = {2, 2, 2, 2, 1, 1, 1, 1, -1, -1, -1, -1, -2, -2, -2, -2};
    static const i8 s_three_slab[] = {8, 8, 8, -8, -8, -8};
    static const i8 s_ladder_up[] = {1, 1, 2, 2, 4, 4, 8, -8, -4, -4, -2, -2, -1, -1};
    static const i8 s_ladder_dn[] = {8, 4, 2, 1, 1, 1, 1, -1, -1, -1, -1, -2, -4, -8};
    static const i8 s_ping[] = {1, 2, 4, 2, 1, 2, 4, 2, -2, -4, -2, -1, -2, -4, -2, -1};
    static const i8 s_sandwich[] = {4, 1, 1, 1, 1, 4, -4, -1, -1, -1, -1, -4};
    static const i8 s_heavy_2b[] = {2, 2, 2, 2, 2, 2, 2, 2, -2, -2, -2, -2, -2, -2, -2, -2};
    static const i8 s_1b_burst[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    static const i8 s_4b_block[] = {4, 4, 4, 4, -4, -4, -4, -4};
    static const i8 s_cross[] = {1, 2, 4, 8, 1, 2, 4, 8, -1, -2, -4, -8, -1, -2, -4, -8};
    static const i8 s_tetris[] = {2, 2, 1, 1, 1, 1, 4, -4, -1, -1, -1, -1, -2, -2};
    WhiteboardMngTestUtil::run_seq("fill slab split to second", nullptr, s_pack, WB_VIS_N(s_pack), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("sixteen 1b two slabs", nullptr, s_dual_slab, WB_VIS_N(s_dual_slab), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("8-4-2-1 same slab", nullptr, s_8_4_2_1, WB_VIS_N(s_8_4_2_1), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("four 1b two 2b partial", nullptr, s_fill_a, WB_VIS_N(s_fill_a), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("four 2b four 1b swap", nullptr, s_fill_b, WB_VIS_N(s_fill_b), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("three full 8b slabs", nullptr, s_three_slab, WB_VIS_N(s_three_slab), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("ladder up sizes", nullptr, s_ladder_up, WB_VIS_N(s_ladder_up), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("ladder down sizes", nullptr, s_ladder_dn, WB_VIS_N(s_ladder_dn), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("ping pong 1-2-4-2", nullptr, s_ping, WB_VIS_N(s_ping), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("4b sandwich 1b", nullptr, s_sandwich, WB_VIS_N(s_sandwich), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("eight 2b fill", nullptr, s_heavy_2b, WB_VIS_N(s_heavy_2b), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("twenty-four 1b three slabs", nullptr, s_1b_burst, WB_VIS_N(s_1b_burst), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("four 4b one slab", nullptr, s_4b_block, WB_VIS_N(s_4b_block), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("cross two slabs", nullptr, s_cross, WB_VIS_N(s_cross), 2, 2, RET_RIGHT);
    WhiteboardMngTestUtil::run_seq("tetris 2b 1b 4b", nullptr, s_tetris, WB_VIS_N(s_tetris), 2, 2, RET_RIGHT);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
