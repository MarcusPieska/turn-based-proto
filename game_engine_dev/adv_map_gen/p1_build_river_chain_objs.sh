#!/bin/bash

INC="-I. -I.. -I../map_loader -I../misc"

source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/p1_build_tester_cli_obj.sh"

g++ -std=c++11 $INC -c generator_constants.cpp -o generator_constants.o
g++ -std=c++11 $INC -c perlin_noise.cpp -o perlin_noise.o
g++ -std=c++11 $INC -c ../misc/whiteboard_mng.cpp -o whiteboard_mng.o
g++ -std=c++11 $INC -c wb_que_xy.cpp -o wb_que_xy.o
g++ -std=c++11 $INC -c p1_gen_outline.cpp -o p1_gen_outline.o
g++ -std=c++11 $INC -I../simple_map_gen -c ../simple_map_gen/generate_terrain_rotation.cpp -o generate_terrain_rotation.o
g++ -std=c++11 $INC -I../simple_map_gen -c p1_gen_cont_outlines.cpp -o p1_gen_cont_outlines.o
g++ -std=c++11 $INC -c p1_gen_land_depth.cpp -o p1_gen_land_depth.o
g++ -std=c++11 $INC -c p1_adj_outline_fill.cpp -o p1_adj_outline_fill.o
g++ -std=c++11 $INC -c p1_gen_shaped_outline.cpp -o p1_gen_shaped_outline.o
g++ -std=c++11 $INC -c p1_gen_river_pts.cpp -o p1_gen_river_pts.o
g++ -std=c++11 $INC -c p1_gen_ocean_index.cpp -o p1_gen_ocean_index.o
g++ -std=c++11 $INC -c p1_gen_river_sectors.cpp -o p1_gen_river_sectors.o
g++ -std=c++11 $INC -c p1_gen_river_network.cpp -o p1_gen_river_network.o
g++ -std=c++11 $INC -c p1_gen_coastal_mtn_limits.cpp -o p1_gen_coastal_mtn_limits.o
g++ -std=c++11 $INC -c p1_gen_river_lines.cpp -o p1_gen_river_lines.o
g++ -std=c++11 $INC -c p1_adj_coastal_mtn_rivers.cpp -o p1_adj_coastal_mtn_rivers.o
g++ -std=c++11 $INC -c p1_adj_river_lakes.cpp -o p1_adj_river_lakes.o
g++ -std=c++11 $INC -c p1_adj_river_inlets.cpp -o p1_adj_river_inlets.o
g++ -std=c++11 $INC -c p1_gen_distance_to_river.cpp -o p1_gen_distance_to_river.o
g++ -std=c++11 $INC -c ../map_loader/map_terrain_validate.cpp -o map_terrain_validate.o
g++ -std=c++11 $INC -c ../map_loader/map_terrain_data.cpp -o map_terrain_data.o

RIV_CHAIN_CORE_OBJS="generator_constants.o perlin_noise.o whiteboard_mng.o wb_que_xy.o p1_gen_outline.o generate_terrain_rotation.o p1_gen_cont_outlines.o p1_gen_land_depth.o p1_adj_outline_fill.o p1_gen_shaped_outline.o p1_gen_river_pts.o p1_gen_ocean_index.o p1_gen_river_sectors.o p1_tester_cli.o"

RIV_CHAIN_BASE_OBJS="$RIV_CHAIN_CORE_OBJS p1_gen_coastal_mtn_limits.o"

RIV_CHAIN_LINE_OBJS="$RIV_CHAIN_CORE_OBJS p1_gen_river_network.o p1_gen_river_lines.o"

RIV_CHAIN_LAKE_OBJS="$RIV_CHAIN_LINE_OBJS p1_adj_river_lakes.o"

RIV_CHAIN_INLET_OBJS="$RIV_CHAIN_LAKE_OBJS p1_adj_coastal_mtn_rivers.o p1_adj_river_inlets.o"

RIV_CHAIN_DIST_OBJS="$RIV_CHAIN_INLET_OBJS p1_gen_distance_to_river.o"

RIV_CHAIN_MAP_OBJS="map_terrain_validate.o map_terrain_data.o"
