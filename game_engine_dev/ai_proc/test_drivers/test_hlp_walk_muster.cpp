//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>

#include "test_hlp_walk_muster.h"
#include "conduct_campaign.h"
#include "game_state.h"
#include "unit_chain_validation.h"
#include "test_hlp_unit_validate.h"

//================================================================================================================================
//=> - TestHlpWalkMuster -
//================================================================================================================================

bool TestHlpWalkMuster::run (
    GameState& s,
    ConductCampaign& camp,
    const u16* ov,
    u16 pa,
    u16 pb,
    u16 stx,
    u16 sty,
    cstr in_base,
    cstr out_base,
    bool print_turn,
    bool chain_check,
    bool chain_print_all,
    u32* turn_io) {
    if (ov == nullptr || in_base == nullptr || out_base == nullptr || turn_io == nullptr) {
        return false;
    }
    if (chain_check && !TestHlpUnitChainValidation::run(s, chain_print_all, nullptr)) {
        std::printf("*** FAILED unit chain validation at muster turn %u\n", *turn_io);
        return false;
    }
    if (print_turn) {
        std::printf("turn %u (post-muster):\n", *turn_io);
        TestHlpUnitValidate::print_at_xy(s, stx, sty);
    }
    if (!TestHlpUnitValidate::write_turn(s, ov, pa, pb, stx, sty, *turn_io, in_base, out_base)) {
        std::printf("*** FAILED save muster turn %u\n", *turn_io);
        return false;
    }
    (*turn_io)++;
    u32 local_n = 0;
    double walk_us_sum = 0.0;
    double last_step_us = 0.0;
    for (;;) {
        const auto w0 = std::chrono::steady_clock::now();
        const bool moved = camp.walk_muster();
        const auto w1 = std::chrono::steady_clock::now();
        last_step_us = std::chrono::duration<double, std::micro>(w1 - w0).count();
        if (!moved) {
            break;
        }
        local_n++;
        walk_us_sum += last_step_us;
        if (chain_check && !TestHlpUnitChainValidation::run(s, chain_print_all, nullptr)) {
            std::printf("*** FAILED unit chain validation at muster turn %u\n", *turn_io);
            return false;
        }
        if (print_turn) {
            std::printf("turn %u walk_us=%.2f\n", *turn_io, last_step_us);
            TestHlpUnitValidate::print_at_xy(s, stx, sty);
        }
        if (!TestHlpUnitValidate::write_turn(s, ov, pa, pb, stx, sty, *turn_io, in_base, out_base)) {
            std::printf("*** FAILED save muster turn %u\n", *turn_io);
            return false;
        }
        (*turn_io)++;
    }
    std::printf("muster done %u turns (step_us=%.2f walk_us=%.2f frm=%u)\n", local_n, last_step_us, walk_us_sum, *turn_io);
    std::printf("final muster point:\n");
    TestHlpUnitValidate::print_at_xy(s, stx, sty);
    std::printf("staging=(%u,%u) muster_turns=%u sum_walk_us=%.2f\n", (u32)stx, (u32)sty, local_n, walk_us_sum);
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
