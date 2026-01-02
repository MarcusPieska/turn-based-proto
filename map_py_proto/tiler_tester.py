#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

from tile_regular import RegularTileMaker
from tile_diamond import DiamondTileMaker

import math
from shapely.geometry import Point, Polygon

#================================================================================================================================#
#=> - Test functions -
#================================================================================================================================#

def dummy_draw_line (pt1, pt2):
    pass

def do_print (message):
    print ("*** " + message)

def do_hdr_print (message):
    print ("\n" + "=" * 120 + "\n" + message + "\n" + "=" * 120 + "\n")

#================================================================================================================================#

def test_1_coords_no_crash (tiler, map_w, map_h, step=10):
    do_print ("Test 1: Testing coordinates across entire canvas (" + str(map_w) + "x" + str(map_h) + ")")
    do_print ("Step size: " + str(step))
    tested, crashed = 0, 0
    for x in range(0, map_w, step):
        for y in range(0, map_h, step):
            tested += 1
            try:
                tile = tiler.coords_to_tile(x, y)
            except Exception as e:
                crashed += 1
                do_print ("CRASH with tile-click at (" + str(x) + ", " + str(y) + "): " + str(e))
            tested += 1
            try:
                near_tiles = tiler.coords_to_near_tiles(x, y)
            except Exception as e:
                crashed += 1
                do_print ("CRASH with near-tiles at (" + str(x) + ", " + str(y) + "): " + str(e))
            tested += 1
            try:
                diagonal_tiles = tiler.coords_to_diagonal_tiles(x, y)
            except Exception as e:
                crashed += 1
                do_print ("CRASH with diagonal-tiles at (" + str(x) + ", " + str(y) + "): " + str(e))

    do_print ("Tested " + str(tested) + " coordinates")
    if crashed == 0:
        do_print ("PASS: No crashes")
    else:
        do_print ("FAIL: " + str(crashed) + " crashes")
    
    return crashed == 0

def test_regular_tiler_1 ():
    do_hdr_print ("Testing RegularTileMaker - Test 1")
    m_width, m_height = 6050, 6050
    return test_1_coords_no_crash (RegularTileMaker (m_width, m_height, 100, 100, dummy_draw_line), m_width, m_height, 10)

def test_diamond_tiler_1 ():
    do_hdr_print ("Testing DiamondTileMaker - Test 1")
    m_width, m_height = 1050, 1050
    return test_1_coords_no_crash (DiamondTileMaker (m_width, m_height, 100, 100, dummy_draw_line), m_width, m_height, 10)

def test_diamond_tiler_1_aspect ():
    do_hdr_print ("Testing DiamondTileMaker (100x60) - Test 1")
    m_width, m_height = 1050, 1050
    return test_1_coords_no_crash (DiamondTileMaker (m_width, m_height, 100, 60, dummy_draw_line), m_width, m_height, 10)

#================================================================================================================================#

def test_2_select_accuracy (tiler, map_w, map_h, step=10):
    do_print ("Test 2: Testing tile selection accuracy")
    do_print ("Step size: " + str(step))
    tested, failed = 0, 0
    all_tiles = [tile for row in tiler.tiles for tile in row if tile is not None]
    for x in range (0, map_w, step):
        for y in range (0, map_h, step):
            tested += 1
            c_tile = tiler.coords_to_tile (x, y)
            if c_tile is None:
                continue
            polygon = Polygon(c_tile.corners)
            point = Point(x, y)
            if not polygon.contains(point) and not polygon.touches(point):
                failed += 1
                do_print ("FAIL: Tile %s does not contain point (%d, %d)" %(str(c_tile.corners), x, y))
    
    do_print ("Tested " + str(tested) + " coordinates")
    if failed == 0:
        do_print ("PASS: All selections accurate")
    else:
        do_print ("FAIL: " + str(failed) + " failures")
    return failed == 0

def test_regular_tiler_2 ():
    do_hdr_print ("Testing RegularTileMaker - Test 2")
    m_width, m_height = 6050, 6050
    return test_2_select_accuracy (RegularTileMaker (m_width, m_height, 100, 100, dummy_draw_line), m_width, m_height, 50)

def test_diamond_tiler_2 ():
    do_hdr_print ("Testing DiamondTileMaker - Test 2")
    m_width, m_height = 1050, 1050
    return test_2_select_accuracy (DiamondTileMaker (m_width, m_height, 100, 100, dummy_draw_line), m_width, m_height, 10)

def test_diamond_tiler_2_aspect ():
    do_hdr_print ("Testing DiamondTileMaker (100x60) - Test 2")
    m_width, m_height = 1050, 1050
    return test_2_select_accuracy (DiamondTileMaker (m_width, m_height, 100, 60, dummy_draw_line), m_width, m_height, 10)

#================================================================================================================================#

def add_center_to_tiles (tiles):
    for tile in tiles:
        sum_x = sum(corner[0] for corner in tile.corners)
        sum_y = sum(corner[1] for corner in tile.corners)
        tile.center = (sum_x / len(tile.corners), sum_y / len(tile.corners))

def add_distance_to_tiles (tiles, point):
    for tile in tiles:
        dx = point[0] - tile.center[0]
        dy = point[1] - tile.center[1]
        dist_sq = dx * dx + dy * dy
        tile.dist_sq = dist_sq
        tile.dist = math.sqrt(dist_sq)

def test_3_near_selection (tiler, map_w, map_h, step=50):
    do_print ("Test 2: Testing tile selection accuracy")
    do_print ("Step size: " + str(step))
    tested, failed = 0, 0
    all_tiles = [tile for row in tiler.tiles for tile in row if tile is not None]
    add_center_to_tiles (all_tiles)
    for x in range (0, map_w, step):
        for y in range (0, map_h, step):
            tested += 1
            c_tile = tiler.coords_to_tile (x, y)
            if c_tile is None:
                continue
            n_tiles = tiler.coords_to_near_tiles (x, y)
            d_tiles = tiler.coords_to_diagonal_tiles (x, y)
            assert len(n_tiles) == 4, "Near tiles mismatch at (%d, %d): %d != 4" %(x, y, len(n_tiles))
            assert len(d_tiles) == 4, "Diagonal tiles mismatch at (%d, %d): %d != 4" %(x, y, len(d_tiles))
            add_distance_to_tiles (all_tiles, c_tile.center)
            n_tiles = [t for t in n_tiles if t is not None]
            d_tiles = [t for t in d_tiles if t is not None]
            all_tiles.sort (key=lambda t: t.dist)
            i = 1

            for _ in n_tiles:
                if all_tiles[i] not in n_tiles:
                    failed += 1
                    do_print ("FAIL: Next closest not in near tiles: %s" %(str(all_tiles[i].center)))
                    continue
                i += 1
            for _ in d_tiles:
                if all_tiles[i] not in d_tiles:
                    failed += 1
                    do_print ("FAIL: Next closest not in diagonal tiles: %s" %(str(all_tiles[i].center)))
                    continue
                i += 1

    do_print ("Tested " + str(tested) + " coordinates")
    if failed == 0:
        do_print ("PASS: All selections accurate")
    else:
        do_print ("FAIL: " + str(failed) + " failures")
    return failed == 0

def test_regular_tiler_3 ():
    do_hdr_print ("Testing RegularTileMaker - Test 3")
    m_width, m_height = 1050, 1050
    return test_3_near_selection (RegularTileMaker (m_width, m_height, 100, 100, dummy_draw_line), m_width, m_height, 10)

def test_diamond_tiler_3 ():
    do_hdr_print ("Testing DiamondTileMaker - Test 3")
    m_width, m_height = 1050, 1050
    return test_3_near_selection (DiamondTileMaker (m_width, m_height, 100, 100, dummy_draw_line), m_width, m_height, 10)

def test_diamond_tiler_3_aspect ():
    do_hdr_print ("Testing DiamondTileMaker (100x60) - Test 3")
    m_width, m_height = 1050, 1050
    return test_3_near_selection (DiamondTileMaker (m_width, m_height, 100, 60, dummy_draw_line), m_width, m_height, 10)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    results = []
    #results.append (test_regular_tiler_1())
    #results.append (test_diamond_tiler_1())
    #results.append (test_diamond_tiler_1_aspect())
    #results.append (test_regular_tiler_2())
    #results.append (test_diamond_tiler_2())
    #results.append (test_diamond_tiler_2_aspect())
    results.append (test_regular_tiler_3())
    results.append (test_diamond_tiler_3())
    results.append (test_diamond_tiler_3_aspect())
    
    do_hdr_print ("Summary")
    all_passed = all(results)
    if all_passed:
        do_print (" -> ALL TESTS PASSED <-")
    else:
        do_print (" -> SOME TESTS FAILED <-")
    do_hdr_print ("End of tests")

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
