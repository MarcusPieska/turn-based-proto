#!/bin/bash

INC="-I. -I.. -I../map_loader -I../static_state -I../misc"

g++ -std=c++11 $INC -c ../map_loader/map_terrain_validate.cpp -o map_terrain_validate.o
g++ -std=c++11 $INC -c ../map_loader/map_terrain_data.cpp -o map_terrain_data.o
g++ -std=c++11 $INC -c ../map_loader/map_loader.cpp -o map_loader.o
g++ -std=c++11 $INC -c ../misc/whiteboard_mng.cpp -o whiteboard_mng.o
g++ -std=c++11 $INC -c wb_que_xy.cpp -o wb_que_xy.o
g++ -std=c++11 $INC -O2 -c generator_constants.cpp -o generator_constants.o
g++ -std=c++11 $INC -O3 -ffast-math -c perlin_noise.cpp -o perlin_noise.o
g++ -std=c++11 $INC -c p1_gen_outline.cpp -o p1_gen_outline.o
g++ -std=c++11 $INC -I../simple_map_gen -c ../simple_map_gen/generate_terrain_rotation.cpp -o generate_terrain_rotation.o
g++ -std=c++11 $INC -I../simple_map_gen -c p1_gen_cont_outlines.cpp -o p1_gen_cont_outlines.o
g++ -std=c++11 $INC -c p1_gen_land_depth.cpp -o p1_gen_land_depth.o
g++ -std=c++11 $INC -c p1_gen_noise_perlin.cpp -o p1_gen_noise_perlin.o
g++ -std=c++11 $INC -c p1_adj_outline_fill.cpp -o p1_adj_outline_fill.o
g++ -std=c++11 $INC -c p1_gen_shaped_outline.cpp -o p1_gen_shaped_outline.o
g++ -std=c++11 $INC -c p1_gen_river_prob.cpp -o p1_gen_river_prob.o
g++ -std=c++11 $INC -c p1_gen_river_dynamic_pts.cpp -o p1_gen_river_dynamic_pts.o
g++ -std=c++11 $INC -c p1_gen_river_pts.cpp -o p1_gen_river_pts.o
g++ -std=c++11 $INC -c p1_gen_river_sectors.cpp -o p1_gen_river_sectors.o
g++ -std=c++11 $INC -c p1_gen_river_sect_adj.cpp -o p1_gen_river_sect_adj.o
g++ -std=c++11 $INC -c p1_gen_ocean_index.cpp -o p1_gen_ocean_index.o
g++ -std=c++11 $INC -c p1_tester_cli.cpp -o p1_tester_cli.o
g++ -std=c++11 $INC -c p1_tester_mk_stub.cpp -o p1_tester_mk_stub.o
g++ -std=c++11 $INC -c p1_tester_harness.cpp -o p1_tester_harness.o
g++ -std=c++11 $INC -c p1_tester_early_chain.cpp -o p1_tester_early_chain.o
g++ -std=c++11 $INC -c p1_tester_early_views.cpp -o p1_tester_early_views.o
g++ -std=c++11 $INC -c p1_gen_cont_outlines_view.cpp -o p1_gen_cont_outlines_view.o
g++ -std=c++11 $INC -c p1_adj_outline_fill_view.cpp -o p1_adj_outline_fill_view.o
g++ -std=c++11 $INC -c p1_gen_noise_perlin_view.cpp -o p1_gen_noise_perlin_view.o
g++ -std=c++11 $INC -c p1_gen_land_depth_view.cpp -o p1_gen_land_depth_view.o
g++ -std=c++11 $INC -c p1_gen_shaped_outline_view.cpp -o p1_gen_shaped_outline_view.o
g++ -std=c++11 $INC -c p1_gen_river_prob_view.cpp -o p1_gen_river_prob_view.o
g++ -std=c++11 $INC -c p1_gen_river_pts_view.cpp -o p1_gen_river_pts_view.o
g++ -std=c++11 $INC -c p1_gen_river_sectors_view.cpp -o p1_gen_river_sectors_view.o

EARLY_MAP_OBJS="map_terrain_validate.o map_terrain_data.o map_loader.o generator_constants.o perlin_noise.o whiteboard_mng.o wb_que_xy.o p1_gen_outline.o generate_terrain_rotation.o p1_gen_cont_outlines.o p1_gen_land_depth.o p1_gen_noise_perlin.o p1_adj_outline_fill.o p1_gen_shaped_outline.o p1_gen_river_prob.o p1_gen_river_dynamic_pts.o p1_gen_river_pts.o p1_gen_river_sectors.o p1_gen_river_sect_adj.o p1_gen_ocean_index.o p1_tester_cli.o p1_tester_mk_stub.o p1_tester_harness.o p1_tester_early_chain.o p1_tester_early_views.o p1_gen_cont_outlines_view.o p1_adj_outline_fill_view.o p1_gen_noise_perlin_view.o p1_gen_land_depth_view.o p1_gen_shaped_outline_view.o p1_gen_river_prob_view.o p1_gen_river_pts_view.o p1_gen_river_sectors_view.o"
