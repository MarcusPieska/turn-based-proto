#!/bin/bash

INC="-I. -I.. -I../map_loader"

g++ -std=c++11 $INC -c ../map_loader/map_terrain_validate.cpp -o map_terrain_validate.o
g++ -std=c++11 $INC -c ../map_loader/map_terrain_data.cpp -o map_terrain_data.o
g++ -std=c++11 $INC -c ../map_loader/map_loader.cpp -o map_loader.o
g++ -std=c++11 $INC -c whiteboard.cpp -o whiteboard.o
g++ -std=c++11 $INC -c wb_sheet.cpp -o wb_sheet.o
g++ -std=c++11 $INC -c wb_que_xy.cpp -o wb_que_xy.o
g++ -std=c++11 $INC -c generator_constants.cpp -o generator_constants.o
g++ -std=c++11 $INC -c perlin_noise.cpp -o perlin_noise.o
g++ -std=c++11 $INC -c p1_gen_outline.cpp -o p1_gen_outline.o
g++ -std=c++11 $INC -c p1_gen_land_depth.cpp -o p1_gen_land_depth.o
g++ -std=c++11 $INC -c p1_gen_noise_perlin.cpp -o p1_gen_noise_perlin.o
g++ -std=c++11 $INC -c p1_adj_outline_fill.cpp -o p1_adj_outline_fill.o
g++ -std=c++11 $INC -c p1_gen_shaped_outline.cpp -o p1_gen_shaped_outline.o
g++ -std=c++11 $INC -c p1_gen_river_pts.cpp -o p1_gen_river_pts.o
g++ -std=c++11 $INC -c p1_gen_river_sectors.cpp -o p1_gen_river_sectors.o
g++ -std=c++11 $INC -c p1_gen_river_network.cpp -o p1_gen_river_network.o
g++ -std=c++11 $INC -c p1_gen_river_lines.cpp -o p1_gen_river_lines.o
g++ -std=c++11 $INC -c p1_adj_river_lakes.cpp -o p1_adj_river_lakes.o
g++ -std=c++11 $INC -c p1_adj_river_inlets.cpp -o p1_adj_river_inlets.o
g++ -std=c++11 $INC -c p1_gen_watershed_mountains.cpp -o p1_gen_watershed_mountains.o
g++ -std=c++11 $INC -c p1_gen_watershed_mountain_line_sets.cpp -o p1_gen_watershed_mountain_line_sets.o
g++ -std=c++11 $INC -c p1_gen_distance_to_river.cpp -o p1_gen_distance_to_river.o
g++ -std=c++11 $INC -c p1_gen_nearness_to_watershed_mtn.cpp -o p1_gen_nearness_to_watershed_mtn.o
g++ -std=c++11 $INC -c p1_adj_land_altitude.cpp -o p1_adj_land_altitude.o
g++ -std=c++11 $INC -c p1_adj_ensure_coasts.cpp -o p1_adj_ensure_coasts.o
g++ -std=c++11 $INC -c p1_adj_ensure_seas.cpp -o p1_adj_ensure_seas.o
g++ -std=c++11 $INC -c p1_adj_ensure_river_valleys.cpp -o p1_adj_ensure_river_valleys.o
g++ -std=c++11 $INC -c p1_adj_ensure_mtn_foothills.cpp -o p1_adj_ensure_mtn_foothills.o
g++ -std=c++11 $INC -c p1_gen_climate.cpp -o p1_gen_climate.o
g++ -std=c++11 $INC -c p1_gen_desert_river_cull.cpp -o p1_gen_desert_river_cull.o
g++ -std=c++11 $INC -c p1_gen_wind_pattern_adv.cpp -o p1_gen_wind_pattern_adv.o
g++ -std=c++11 $INC -c p1_gen_loess_boost.cpp -o p1_gen_loess_boost.o
g++ -std=c++11 $INC -c p1_adj_grassland_loess_tiles.cpp -o p1_adj_grassland_loess_tiles.o
g++ -std=c++11 $INC -c p1_gen_rain_orographic.cpp -o p1_gen_rain_orographic.o
g++ -std=c++11 $INC -c p1_make_map.cpp -o p1_make_map.o
g++ -std=c++11 $INC -c p1_tester_chain_core.cpp -o p1_tester_chain_core.o

CORE_MAP_OBJS="map_terrain_validate.o map_terrain_data.o map_loader.o generator_constants.o perlin_noise.o whiteboard.o wb_sheet.o wb_que_xy.o p1_gen_outline.o p1_gen_land_depth.o p1_gen_noise_perlin.o p1_adj_outline_fill.o p1_gen_shaped_outline.o p1_gen_river_pts.o p1_gen_river_sectors.o p1_gen_river_network.o p1_gen_river_lines.o p1_adj_river_lakes.o p1_adj_river_inlets.o p1_gen_watershed_mountains.o p1_gen_watershed_mountain_line_sets.o p1_gen_distance_to_river.o p1_gen_nearness_to_watershed_mtn.o p1_adj_land_altitude.o p1_adj_ensure_coasts.o p1_adj_ensure_seas.o p1_adj_ensure_river_valleys.o p1_adj_ensure_mtn_foothills.o p1_gen_climate.o p1_gen_desert_river_cull.o p1_gen_wind_pattern_adv.o p1_gen_loess_boost.o p1_adj_grassland_loess_tiles.o p1_gen_rain_orographic.o p1_make_map.o p1_tester_chain_core.o"
