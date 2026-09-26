#!/bin/bash
# Shared GameIo / city link set for game_eval save drivers. Source from a *_comp in this folder.
# Expects: ROOT (repo game_engine_dev), INC, CXXFLAGS, LIB_SO already set; cwd = test_drivers.

compile_eval_save_deps () {
    g++ $CXXFLAGS $INC -c "$ROOT/data_io/runtime_static_loader.cpp" -o runtime_static_loader.o
    g++ $CXXFLAGS $INC -c "$ROOT/misc/bit_array.cpp" -o bit_array.o
    g++ $CXXFLAGS $INC -c "$ROOT/misc/map_bit_overlay.cpp" -o map_bit_overlay.o
    g++ $CXXFLAGS $INC -c "$ROOT/misc/map_bit_array_overlay.cpp" -o map_bit_array_overlay.o
    g++ $CXXFLAGS $INC -c "$ROOT/map_loader/map_terrain_validate.cpp" -o map_terrain_validate.o
    g++ $CXXFLAGS $INC -c "$ROOT/game/runtime_trace_dbg.cpp" -o runtime_trace_dbg.o
    g++ $CXXFLAGS $INC -c "$ROOT/gen_bit_banks/general_bit_bank.cpp" -o general_bit_bank.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/assessor/general_assessor.cpp" -o general_assessor.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/effect_ctx.cpp" -o effect_ctx.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/effector/effect_enabler.cpp" -o effect_enabler.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/effector/booster_effect_register.cpp" -o booster_effect_register.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/effector/local_unit_exp_booster_register.cpp" -o local_unit_exp_booster_register.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/effector/civ_ship_training_booster_register.cpp" -o civ_ship_training_booster_register.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/effector/city_commerce_booster_register.cpp" -o city_commerce_booster_register.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/effector/city_production_booster_register.cpp" -o city_production_booster_register.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/effector/local_production_booster_register.cpp" -o local_production_booster_register.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/effector/local_pop_growth_booster_register.cpp" -o local_pop_growth_booster_register.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/effector/local_commerce_booster_register.cpp" -o local_commerce_booster_register.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/effector/city_pop_growth_booster_register.cpp" -o city_pop_growth_booster_register.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/effector/civ_pop_growth_booster_register.cpp" -o civ_pop_growth_booster_register.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/effector/city_culture_booster_register.cpp" -o city_culture_booster_register.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/effector/local_sanitation_booster_register.cpp" -o local_sanitation_booster_register.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/effector/city_sanitation_booster_register.cpp" -o city_sanitation_booster_register.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/effector/civ_sanitation_booster_register.cpp" -o civ_sanitation_booster_register.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/effector/booster_register_link_stubs.cpp" -o booster_register_link_stubs.o
    g++ $CXXFLAGS $INC -c "$ROOT/game/resource_ledger.cpp" -o resource_ledger.o
    g++ $CXXFLAGS $INC -c "$ROOT/dyn_state/unit_add_vector.cpp" -o unit_add_vector.o
    g++ $CXXFLAGS $INC -c "$ROOT/dyn_state/unit_movement_mng.cpp" -o unit_movement_mng.o
    g++ $CXXFLAGS $INC -c "$ROOT/game/player_ledger.cpp" -o player_ledger.o
    g++ $CXXFLAGS $INC -c "$ROOT/game/tile_yields.cpp" -o tile_yields.o
    g++ $CXXFLAGS $INC -c "$ROOT/game/tile_working.cpp" -o tile_working.o
    g++ $CXXFLAGS $INC -c "$ROOT/game/tile_attr_tables.cpp" -o tile_attr_tables.o
    g++ $CXXFLAGS $INC -c "$ROOT/game/game_array_simple.cpp" -o game_array_simple.o
    g++ $CXXFLAGS $INC -c "$ROOT/game/civ_relations.cpp" -o civ_relations.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/circular_tile_areas.cpp" -o circular_tile_areas.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/city_border.cpp" -o city_border.o
    g++ $CXXFLAGS $INC -c "$ROOT/city/city.cpp" -o city.o
    g++ $CXXFLAGS $INC -c "$ROOT/game/worker_helper.cpp" -o worker_helper.o
    g++ $CXXFLAGS $INC -c "$ROOT/game/city_array.cpp" -o city_array.o
    g++ $CXXFLAGS $INC -c "$ROOT/game/game_io.cpp" -o game_io.o
}

EVAL_SAVE_DEP_OBJS="\
    runtime_static_loader.o \
    bit_array.o \
    map_bit_overlay.o \
    map_bit_array_overlay.o \
    map_terrain_validate.o \
    runtime_trace_dbg.o \
    general_bit_bank.o \
    general_assessor.o \
    effect_ctx.o \
    effect_enabler.o \
    booster_effect_register.o \
    local_unit_exp_booster_register.o \
    civ_ship_training_booster_register.o \
    city_commerce_booster_register.o \
    city_production_booster_register.o \
    local_production_booster_register.o \
    local_pop_growth_booster_register.o \
    local_commerce_booster_register.o \
    city_pop_growth_booster_register.o \
    civ_pop_growth_booster_register.o \
    city_culture_booster_register.o \
    local_sanitation_booster_register.o \
    city_sanitation_booster_register.o \
    civ_sanitation_booster_register.o \
    booster_register_link_stubs.o \
    resource_ledger.o \
    unit_add_vector.o \
    unit_movement_mng.o \
    player_ledger.o \
    tile_yields.o \
    tile_working.o \
    tile_attr_tables.o \
    game_array_simple.o \
    civ_relations.o \
    circular_tile_areas.o \
    city_border.o \
    city.o \
    worker_helper.o \
    city_array.o \
    game_io.o"

rm_eval_save_deps () {
    rm -f $EVAL_SAVE_DEP_OBJS
}
