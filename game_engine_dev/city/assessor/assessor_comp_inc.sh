#!/bin/bash

INC="-I../../ -I../../misc -I../../data_io -I../../static_state -I../../gen_bit_banks -I."
DIO="../../data_io"

compile_assessor_deps () {
    g++ $INC -c ../../misc/opt_str_mng.cpp -o opt_str_mng.o
    g++ $INC -c ../../misc/static_string_pool.cpp -o static_string_pool.o
    g++ $INC -c ../../misc/bit_array.cpp -o bit_array.o
    g++ $INC -c $DIO/path_mng.cpp -o path_mng.o
    g++ $INC -c $DIO/data_parser_base.cpp -o data_parser_base.o
    g++ $INC -c $DIO/item_effect_handler.cpp -o item_effect_handler.o
    g++ $INC -c $DIO/item_effect_helpers.cpp -o item_effect_helpers.o
    g++ $INC -c ../../static_state/static_bit_bank.cpp -o static_bit_bank.o
    g++ $INC -c ../../static_state/unit_type_action_map.cpp -o unit_type_action_map.o
    g++ $INC -c $DIO/unit_type_action_map_parsing.cpp -o unit_type_action_map_parsing.o
    g++ $INC -c ../../static_state/civ_bld_discount_map.cpp -o civ_bld_discount_map.o
    g++ $INC -c $DIO/civ_bld_discount_map_parsing.cpp -o civ_bld_discount_map_parsing.o
    g++ $INC -c $DIO/building_parser.cpp -o building_parser.o
    g++ $INC -c $DIO/city_flag_parser.cpp -o city_flag_parser.o
    g++ $INC -c $DIO/civ_parser.cpp -o civ_parser.o
    g++ $INC -c $DIO/civ_trait_parser.cpp -o civ_trait_parser.o
    g++ $INC -c $DIO/resource_parser.cpp -o resource_parser.o
    g++ $INC -c $DIO/res_dist_parser.cpp -o res_dist_parser.o
    g++ $INC -c $DIO/mvt_cost_parser.cpp -o mvt_cost_parser.o
    g++ $INC -c $DIO/small_wonder_parser.cpp -o small_wonder_parser.o
    g++ $INC -c $DIO/tech_parser.cpp -o tech_parser.o
    g++ $INC -c $DIO/unit_parser.cpp -o unit_parser.o
    g++ $INC -c $DIO/unit_action_parser.cpp -o unit_action_parser.o
    g++ $INC -c $DIO/unit_type_parser.cpp -o unit_type_parser.o
    g++ $INC -c $DIO/wonder_parser.cpp -o wonder_parser.o
    g++ $INC -c ../../static_state/building_static_data.cpp -o building_static_data.o
    g++ $INC -c ../../static_state/city_flag_static_data.cpp -o city_flag_static_data.o
    g++ $INC -c ../../static_state/civ_static_data.cpp -o civ_static_data.o
    g++ $INC -c ../../static_state/civ_trait_static_data.cpp -o civ_trait_static_data.o
    g++ $INC -c ../../static_state/resource_static_data.cpp -o resource_static_data.o
    g++ $INC -c ../../static_state/res_dist_static_data.cpp -o res_dist_static_data.o
    g++ $INC -c ../../static_state/mvt_cost_static_data.cpp -o mvt_cost_static_data.o
    g++ $INC -c ../../static_state/small_wonder_static_data.cpp -o small_wonder_static_data.o
    g++ $INC -c ../../static_state/tech_static_data.cpp -o tech_static_data.o
    g++ $INC -c ../../static_state/unit_static_data.cpp -o unit_static_data.o
    g++ $INC -c ../../static_state/unit_action_static_data.cpp -o unit_action_static_data.o
    g++ $INC -c ../../static_state/unit_type_static_data.cpp -o unit_type_static_data.o
    g++ $INC -c ../../static_state/wonder_static_data.cpp -o wonder_static_data.o
    g++ $INC -c $DIO/static_parsing_manager.cpp -o static_parsing_manager.o
    g++ $INC -c ../../gen_bit_banks/general_bit_bank.cpp -o general_bit_bank.o
    g++ $INC -c general_assessor.cpp -o general_assessor.o
    g++ $INC -c assessor_brute_shared.cpp -o assessor_brute_shared.o
    g++ $INC -c assessor_brute_write.cpp -o assessor_brute_write.o
}

ASSESSOR_COMMON_OBJS="opt_str_mng.o static_string_pool.o bit_array.o path_mng.o data_parser_base.o item_effect_handler.o item_effect_helpers.o static_bit_bank.o unit_type_action_map.o unit_type_action_map_parsing.o civ_bld_discount_map.o civ_bld_discount_map_parsing.o building_parser.o city_flag_parser.o civ_parser.o civ_trait_parser.o resource_parser.o res_dist_parser.o mvt_cost_parser.o small_wonder_parser.o tech_parser.o unit_parser.o unit_action_parser.o unit_type_parser.o wonder_parser.o building_static_data.o city_flag_static_data.o civ_static_data.o civ_trait_static_data.o resource_static_data.o res_dist_static_data.o mvt_cost_static_data.o small_wonder_static_data.o tech_static_data.o unit_static_data.o unit_action_static_data.o unit_type_static_data.o wonder_static_data.o static_parsing_manager.o general_bit_bank.o general_assessor.o assessor_brute_shared.o assessor_brute_write.o"

clean_assessor_objs () {
    rm -f $ASSESSOR_COMMON_OBJS
}
