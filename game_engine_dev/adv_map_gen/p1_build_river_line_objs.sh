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
g++ -std=c++11 $INC -c p1_gen_noise_perlin.cpp -o p1_gen_noise_perlin.o
g++ -std=c++11 $INC -c p1_adj_outline_fill.cpp -o p1_adj_outline_fill.o
g++ -std=c++11 $INC -c p1_gen_shaped_outline.cpp -o p1_gen_shaped_outline.o
g++ -std=c++11 $INC -c p1_gen_river_pts.cpp -o p1_gen_river_pts.o
g++ -std=c++11 $INC -c p1_gen_ocean_index.cpp -o p1_gen_ocean_index.o
g++ -std=c++11 $INC -c p1_gen_river_sectors.cpp -o p1_gen_river_sectors.o
g++ -std=c++11 $INC -c p1_gen_river_network.cpp -o p1_gen_river_network.o
g++ -std=c++11 $INC -c p1_gen_river_lines.cpp -o p1_gen_river_lines.o

RIV_CHAIN_LINE_OBJS="generator_constants.o perlin_noise.o whiteboard_mng.o wb_que_xy.o p1_gen_outline.o generate_terrain_rotation.o p1_gen_cont_outlines.o p1_gen_land_depth.o p1_gen_noise_perlin.o p1_adj_outline_fill.o p1_gen_shaped_outline.o p1_gen_river_pts.o p1_gen_ocean_index.o p1_gen_river_sectors.o p1_gen_river_network.o p1_gen_river_lines.o p1_tester_cli.o"
