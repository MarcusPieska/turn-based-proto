//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "whiteboard_mng_visual_util.h"

static void run_gap (cstr label, const i8* seq, u32 n, WbRetPol ret) {
    WhiteboardMngTestUtil::run_seq(label, nullptr, seq, n, 2, 2, ret);
}

//================================================================================================================================
//=> - Gap formation via return policy -
//================================================================================================================================

void wb_vis_run_gap () {
    std::printf("\n######## gap ########\n");
    static const i8 s_gap_1[] = {1, 1, 1, 1, 1, 1, 1, 1, -1, 2};
    static const i8 s_gap_2[] = {1, 1, 1, 1, 1, 1, 1, 1, -1, -1, 4};
    static const i8 s_gap_3[] = {1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, 2, 2};
    static const i8 s_gap_4[] = {1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, 4, 4};
    static const i8 s_mid_2b[] = {2, 2, 2, 2, 2, 2, 2, 2, -2, 1, 1, 1, 1};
    static const i8 s_center_4[] = {1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, 8};
    static const i8 s_swiss[] = {1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, 1, 1, 1, 1, 1};
    static const i8 s_punch[] = {4, 1, 1, 1, 1, -1, -1, 2, 2};
    static const i8 s_ladder_gap[] = {1, 1, 2, 2, 4, -4, -2, -1, -1, 1, 2, 4};
    static const i8 s_dual_gap[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, -1, 2, 2};
    run_gap("gap: one hole then 2b LEFT", s_gap_1, WB_VIS_N(s_gap_1), RET_LEFT);
    run_gap("gap: one hole then 2b RIGHT", s_gap_1, WB_VIS_N(s_gap_1), RET_RIGHT);
    run_gap("gap: one hole then 2b MID", s_gap_1, WB_VIS_N(s_gap_1), RET_MID);
    run_gap("gap: two holes then 4b LEFT", s_gap_2, WB_VIS_N(s_gap_2), RET_LEFT);
    run_gap("gap: two holes then 4b RIGHT", s_gap_2, WB_VIS_N(s_gap_2), RET_RIGHT);
    run_gap("gap: two holes then 4b MID", s_gap_2, WB_VIS_N(s_gap_2), RET_MID);
    run_gap("gap: three holes two 2b LEFT", s_gap_3, WB_VIS_N(s_gap_3), RET_LEFT);
    run_gap("gap: three holes two 2b MID", s_gap_3, WB_VIS_N(s_gap_3), RET_MID);
    run_gap("gap: three holes two 2b RIGHT", s_gap_3, WB_VIS_N(s_gap_3), RET_RIGHT);
    run_gap("gap: four holes two 4b LEFT", s_gap_4, WB_VIS_N(s_gap_4), RET_LEFT);
    run_gap("gap: four holes two 4b MID", s_gap_4, WB_VIS_N(s_gap_4), RET_MID);
    run_gap("gap: four holes two 4b RIGHT", s_gap_4, WB_VIS_N(s_gap_4), RET_RIGHT);
    run_gap("gap: 2b hole four 1b LEFT", s_mid_2b, WB_VIS_N(s_mid_2b), RET_LEFT);
    run_gap("gap: 2b hole four 1b MID", s_mid_2b, WB_VIS_N(s_mid_2b), RET_MID);
    run_gap("gap: 2b hole four 1b RIGHT", s_mid_2b, WB_VIS_N(s_mid_2b), RET_RIGHT);
    run_gap("gap: half drain then 8b LEFT", s_center_4, WB_VIS_N(s_center_4), RET_LEFT);
    run_gap("gap: half drain then 8b MID", s_center_4, WB_VIS_N(s_center_4), RET_MID);
    run_gap("gap: half drain then 8b RIGHT", s_center_4, WB_VIS_N(s_center_4), RET_RIGHT);
    run_gap("gap: swiss refill LEFT", s_swiss, WB_VIS_N(s_swiss), RET_LEFT);
    run_gap("gap: swiss refill MID", s_swiss, WB_VIS_N(s_swiss), RET_MID);
    run_gap("gap: swiss refill RIGHT", s_swiss, WB_VIS_N(s_swiss), RET_RIGHT);
    run_gap("gap: 4b punch 1b 2b LEFT", s_punch, WB_VIS_N(s_punch), RET_LEFT);
    run_gap("gap: 4b punch 1b 2b RIGHT", s_punch, WB_VIS_N(s_punch), RET_RIGHT);
    run_gap("gap: ladder reopen LEFT", s_ladder_gap, WB_VIS_N(s_ladder_gap), RET_LEFT);
    run_gap("gap: ladder reopen RIGHT", s_ladder_gap, WB_VIS_N(s_ladder_gap), RET_RIGHT);
    run_gap("gap: dual slab hole LEFT", s_dual_gap, WB_VIS_N(s_dual_gap), RET_LEFT);
    run_gap("gap: dual slab hole MID", s_dual_gap, WB_VIS_N(s_dual_gap), RET_MID);
    run_gap("gap: dual slab hole RIGHT", s_dual_gap, WB_VIS_N(s_dual_gap), RET_RIGHT);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
