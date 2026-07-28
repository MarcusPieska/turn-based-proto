//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <chrono>

#include "conduct_campaign.h"
#include "game_primitives.h"
#include "game_state.h"
#include "generate_access_mask.h"
#include "test_hlp_city_attack.h"
#include "test_hlp_city_spawning.h"
#include "test_hlp_city_spawning_validate.h"
#include "test_hlp_civ_spawning.h"
#include "test_hlp_map_load.h"
#include "test_hlp_tile_ownership.h"
#include "test_hlp_tile_ownership_validate.h"
#include "unit_chain_validation.h"
#include "test_hlp_unit_spawning.h"
#include "test_hlp_unit_validate.h"
#include "test_hlp_walk_army.h"
#include "test_hlp_walk_muster.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const cstr G_IN = "/home/w/Projects/simple-map-gen/p1-seed-43/";
static const cstr G_OUT = "/home/w/Projects/simple-map-gen/conduct-campaign";
static const cstr G_LIB = "../../data_io/runtime_static_loader_lib.so";
static const cstr G_DATA = "../../";

static const u16 k_lat = 10u;
static const u16 k_jit_pct = 30u;
static const u32 k_seed = 43u;

//================================================================================================================================
//=> - CLI -
//================================================================================================================================

static bool arg_has (int argc, char** argv, cstr flag) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] != nullptr && std::strcmp(argv[i], flag) == 0) {
            return true;
        }
    }
    return false;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    const bool c_check = arg_has(argc, argv, "--chain-check");
    const bool c_print_all = arg_has(argc, argv, "--chain-print-all");
    if (!TestHlpUnitValidate::ensure_out_dirs(G_OUT)) {
        std::printf("*** FAILED mkdir %s\n", G_OUT);
        return 1;
    }
    GameState state;
    if (!TestHlpMapLoad::load(state, G_IN, G_LIB, G_DATA)) {
        std::printf("*** FAILED map load\n");
        return 1;
    }
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    WhiteboardMng::init(w, h);
    Whiteboard_2B ov_wb("conduct_campaign_tester", "ov", 0u);
    if (!ov_wb.ok()) {
        std::printf("*** FAILED ov whiteboard\n");
        WhiteboardMng::terminate();
        return 1;
    }
    u16 seed_x[TestHlpTileOwnership::k_seed_cap];
    u16 seed_y[TestHlpTileOwnership::k_seed_cap];
    u16 sec_n = 0;
    if (!TestHlpTileOwnership::mock_sectors(state, ov_wb.get_iter_ptr(), &sec_n, seed_x, seed_y)) {
        std::printf("*** FAILED mock_sectors\n");
        WhiteboardMng::terminate();
        return 1;
    }
    u16 pa = 0;
    u16 pb = 0;
    u32 bord_n = 0;
    if (!TestHlpTileOwnership::find_wide_border(ov_wb.get_iter_ptr(), w, h, sec_n, &pa, &pb, &bord_n)) {
        std::printf("*** FAILED find_wide_border\n");
        WhiteboardMng::terminate();
        return 1;
    }
    TestHlpTileOwnershipValidate::print_summary(sec_n, pa, pb, bord_n);
    if (!TestHlpCivSpawning::init_seats(state, sec_n, pa, pb)) {
        std::printf("*** FAILED civ init_seats\n");
        WhiteboardMng::terminate();
        return 1;
    }
    TestHlpTileOwnership::apply_owners(state, ov_wb.get_iter_ptr());
    const u16 city_n = TestHlpCitySpawning::spawn_lattice(state, ov_wb.get_iter_ptr(), pa, pb, k_lat, k_jit_pct, k_seed);
    if (city_n == 0u) {
        std::printf("*** FAILED spawn_lattice\n");
        WhiteboardMng::terminate();
        return 1;
    }
    TestHlpCitySpawningValidate::print_summary(state, pa, pb);
    if (!TestHlpUnitSpawning::spawn_at_cities(state, pa, "Spearman", 2u)
        || !TestHlpUnitSpawning::spawn_at_cities(state, pa, "Swordsman", 3u)
        || !TestHlpUnitSpawning::spawn_at_cities(state, pa, "Catapult", 2u)) {
        std::printf("*** FAILED spawn attacker units\n");
        WhiteboardMng::terminate();
        return 1;
    }
    if (!TestHlpUnitSpawning::spawn_at_cities(state, pb, "Spearman", 2u)) {
        std::printf("*** FAILED spawn defender units\n");
        WhiteboardMng::terminate();
        return 1;
    }
    TestHlpUnitValidate::print_counts(state);
    u16 stx = 0;
    u16 sty = 0;
    const auto t0 = std::chrono::steady_clock::now();
    const bool pick_ok = ConductCampaign::pick_staging_city(state, pa, pb, &stx, &sty);
    const auto t1 = std::chrono::steady_clock::now();
    const double pick_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
    if (!pick_ok) {
        std::printf("*** FAILED pick_staging_city (%.2f us)\n", pick_us);
        WhiteboardMng::terminate();
        return 1;
    }
    std::printf("pick_staging_city pick_us=%.2f\n", pick_us);
    Whiteboard_1B msk("conduct_campaign_tester", "msk", 0u);
    if (!GenerateAccessMask::generate(state, pa, msk)) {
        std::printf("*** FAILED GenerateAccessMask\n");
        WhiteboardMng::terminate();
        return 1;
    }
    if (!TestHlpUnitValidate::write_sectors(state, ov_wb.get_iter_ptr(), pa, pb, stx, sty, G_IN, G_OUT)) {
        std::printf("*** FAILED write sectors\n");
        WhiteboardMng::terminate();
        return 1;
    }
    if (!TestHlpUnitValidate::write_access(state, msk, pa, stx, sty, G_IN, G_OUT)) {
        std::printf("*** FAILED write access\n");
        WhiteboardMng::terminate();
        return 1;
    }
    ConductCampaign camp(state, pa);
    if (!camp.ok() || !camp.make_muster_gradient(stx, sty)) {
        std::printf("*** FAILED make_muster_gradient\n");
        WhiteboardMng::terminate();
        return 1;
    }
    const u16 grp_n = camp.do_total_muster();
    std::printf("muster_groups=%u\n", (u32)grp_n);
    if (!camp.determine_exposure(pb)) {
        std::printf("*** FAILED determine_exposure\n");
        WhiteboardMng::terminate();
        return 1;
    }
    u16 exp_n = 0;
    for (u16 i = 0; i < camp.muster_n(); ++i) {
        if (camp.is_exposed(i)) {
            exp_n++;
        }
    }
    std::printf("exposed_cities=%u (lim=%u)\n", (u32)exp_n, (u32)ConductCampaign::k_exp_lim);
    u32 turn = 0;
    if (!TestHlpWalkMuster::run(state, camp, ov_wb.get_iter_ptr(), pa, pb, stx, sty, G_IN, G_OUT, false,
            c_check, c_print_all, &turn)) {
        WhiteboardMng::terminate();
        return 1;
    }
    if (!camp.form_army()) {
        std::printf("*** FAILED form_army\n");
        WhiteboardMng::terminate();
        return 1;
    }
    std::printf("army formed at staging; units:\n");
    TestHlpUnitValidate::print_at_xy(state, stx, sty);
    if (!TestHlpUnitValidate::write_turn(state, ov_wb.get_iter_ptr(), pa, pb, stx, sty, turn, G_IN, G_OUT)) {
        std::printf("*** FAILED save form_army turn %u\n", turn);
        WhiteboardMng::terminate();
        return 1;
    }
    turn++;
    u16 tx = 0;
    u16 ty = 0;
    u32 wave = 0;
    u32 captured = 0;
    const auto a0 = std::chrono::steady_clock::now();
    const bool tgt_ok = camp.set_target_city(pb, &tx, &ty);
    const auto a1 = std::chrono::steady_clock::now();
    const double tgt_us = std::chrono::duration<double, std::micro>(a1 - a0).count();
    if (!tgt_ok) {
        std::printf("*** FAILED set_target_city (%.2f us)\n", tgt_us);
        WhiteboardMng::terminate();
        return 1;
    }
    std::printf("target=(%u,%u) set_target_us=%.2f\n", (u32)tx, (u32)ty, tgt_us);
    for (;;) {
        if (!TestHlpWalkArmy::run(state, camp, ov_wb.get_iter_ptr(), pa, pb, tx, ty, G_IN, G_OUT, false,
                c_check, c_print_all, &turn)) {
            WhiteboardMng::terminate();
            return 1;
        }
        if (!TestHlpCityAttack::run(state, camp, tx, ty, true)) {
            WhiteboardMng::terminate();
            return 1;
        }
        captured++;
        std::printf("captured=%u city=(%u,%u) owner_now seat=%u\n",
            captured, (u32)tx, (u32)ty, (u32)pa);
        if (c_check && !TestHlpUnitChainValidation::run(state, c_print_all, nullptr)) {
            std::printf("*** FAILED unit chain validation after assault\n");
            WhiteboardMng::terminate();
            return 1;
        }
        std::printf("city tile after assault:\n");
        TestHlpUnitValidate::print_at_xy(state, tx, ty);
        if (!TestHlpUnitValidate::write_turn(state, ov_wb.get_iter_ptr(), pa, pb, tx, ty, turn, G_IN, G_OUT)) {
            std::printf("*** FAILED save assault turn %u\n", turn);
            WhiteboardMng::terminate();
            return 1;
        }
        turn++;
        const bool had_split = camp.split_hd(0) != U16_KEY_NULL;
        u32 rejoin_n = 0;
        for (;;) {
            const u16 sh = camp.split_hd(0);
            if (sh == U16_KEY_NULL) {
                break;
            }
            const UnitAddStruct* occ = state.m_units.get_unit_add(UnitAddKey::from_raw(camp.atk_hd(0)));
            const UnitAddStruct* spl = state.m_units.get_unit_add(UnitAddKey::from_raw(sh));
            if (occ == nullptr || spl == nullptr || occ->m_x == U16_KEY_NULL || spl->m_x == U16_KEY_NULL) {
                std::printf("*** FAILED rejoin unit lookup\n");
                WhiteboardMng::terminate();
                return 1;
            }
            if (occ->m_x == spl->m_x && occ->m_y == spl->m_y) {
                break;
            }
            if (!camp.rejoin_move(0u)) {
                std::printf("*** FAILED rejoin_move\n");
                WhiteboardMng::terminate();
                return 1;
            }
            rejoin_n++;
            if (!TestHlpUnitValidate::write_turn(state, ov_wb.get_iter_ptr(), pa, pb, tx, ty, turn, G_IN, G_OUT)) {
                std::printf("*** FAILED save rejoin_move turn %u\n", turn);
                WhiteboardMng::terminate();
                return 1;
            }
            turn++;
        }
        if (!camp.rejoin_link(0u)) {
            std::printf("*** FAILED rejoin_link\n");
            WhiteboardMng::terminate();
            return 1;
        }
        if (had_split) {
            if (!TestHlpUnitValidate::write_turn(state, ov_wb.get_iter_ptr(), pa, pb, tx, ty, turn, G_IN, G_OUT)) {
                std::printf("*** FAILED save rejoin_link turn %u\n", turn);
                WhiteboardMng::terminate();
                return 1;
            }
            turn++;
        }
        std::printf("rejoin done; moves=%u army head=%u split=%u\n", rejoin_n, (u32)camp.atk_hd(0), (u32)camp.split_hd(0));
        if (c_check && !TestHlpUnitChainValidation::run(state, c_print_all, nullptr)) {
            std::printf("*** FAILED unit chain validation after rejoin\n");
            WhiteboardMng::terminate();
            return 1;
        }
        if (!camp.army_can_fight(0u)) {
            std::printf("war stop: army has no offensive units left\n");
            break;
        }
        wave++;
        const auto t0 = std::chrono::steady_clock::now();
        const bool next_ok = camp.set_target_city(pb, &tx, &ty);
        const auto t1 = std::chrono::steady_clock::now();
        const double next_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        if (!next_ok) {
            std::printf("war stop: no more enemy cities (set_target_us=%.2f)\n", next_us);
            break;
        }
        std::printf("next target=(%u,%u) set_target_us=%.2f wave=%u\n", (u32)tx, (u32)ty, next_us, wave);
    }
    std::printf("out_base=%s/turns frames=%u captured=%u\n", G_OUT, turn, captured);
    std::printf("*** PASSED conduct_campaign muster+war loop\n");
    WhiteboardMng::terminate();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
