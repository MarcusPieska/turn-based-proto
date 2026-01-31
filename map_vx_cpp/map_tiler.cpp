//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cmath>
#include <algorithm>
#include <limits>
#include <cstdlib>

#include "map_tiler.h"
#include "map_model.h"

#define VAL_X(x) (x >= 0 && x < m_num_cols)
#define VAL_Y(y) (y >= 0 && y < m_num_rows)

//================================================================================================================================
//=> - MapTiler implementation -
//================================================================================================================================

MapTiler::MapTiler (int map_w, int map_h, int tile_w, int tile_h, int tile_cols, int tile_rows, MapModel *model, int top_margin) :
    m_tile_w (tile_w),
    m_tile_h (tile_h),
    m_half_w (tile_w / 2),
    m_half_h (tile_h / 2),
    m_num_cols (tile_cols),
    m_num_rows (tile_rows),
    m_x_min ((map_w - (m_num_cols * tile_w)) / 2),
    m_y_min (top_margin),
    m_model (model) {
    
    m_even_row_offsets.n = Point(0, -2);
    m_even_row_offsets.e = Point(1, 0);
    m_even_row_offsets.s = Point(0, 2);
    m_even_row_offsets.w = Point(-1, 0);

    m_even_row_offsets.ne = Point(0, -1);
    m_even_row_offsets.se = Point(0, 1);
    m_even_row_offsets.sw = Point(-1, 1);
    m_even_row_offsets.nw = Point(-1, -1);
    
    m_odd_row_offsets.n = Point(0, -2);
    m_odd_row_offsets.e = Point(1, 0);
    m_odd_row_offsets.s = Point(0, 2);
    m_odd_row_offsets.w = Point(-1, 0);

    m_odd_row_offsets.ne = Point(1, -1);
    m_odd_row_offsets.se = Point(1, 1);
    m_odd_row_offsets.sw = Point(0, 1);
    m_odd_row_offsets.nw = Point(0, -1);
    
    m_model->allocateTiles(m_num_rows, m_num_cols);
    MapModelTile** tiles = m_model->getTiles();
    
    int y = m_y_min + m_half_h;
    for (int row = 0; row < m_num_rows; row++) {
        int x_offset = (row % 2 == 1) ? m_half_w : 0;
        int x = m_x_min + x_offset + m_half_w;
        for (int col = 0; col < m_num_cols; col++) {
            Point top (x, y - m_half_h);
            Point right (x + m_half_w, y);
            Point bottom (x, y + m_half_h);
            Point left (x - m_half_w, y);
            tiles[row][col] = MapModelTile(top, right, bottom, left);
            x += tile_w;
        }
        y += m_half_h;
    }
    m_model->m_tiler = this;
    
}

MapTiler::~MapTiler () {
}

NearTiles MapTiler::getNearTiles (int row, int col) {
    NearTiles result;
    const NearTiles& offsets = (row % 2 == 0) ? m_even_row_offsets : m_odd_row_offsets;

    result.n.x = VAL_X(col + offsets.n.x) ? col + offsets.n.x : -1;
    result.n.y = VAL_Y(row + offsets.n.y) ? row + offsets.n.y : -1;
    
    result.ne.x = VAL_X(col + offsets.ne.x) ? col + offsets.ne.x : -1;
    result.ne.y = VAL_Y(row + offsets.ne.y) ? row + offsets.ne.y : -1;
 
    result.e.x = VAL_X(col + offsets.e.x) ? col + offsets.e.x : -1;
    result.e.y = VAL_Y(row + offsets.e.y) ? row + offsets.e.y : -1;
    
    result.se.x = VAL_X(col + offsets.se.x) ? col + offsets.se.x : -1;
    result.se.y = VAL_Y(row + offsets.se.y) ? row + offsets.se.y : -1;
    
    result.s.x = VAL_X(col + offsets.s.x) ? col + offsets.s.x : -1;
    result.s.y = VAL_Y(row + offsets.s.y) ? row + offsets.s.y : -1;
    
    result.sw.x = VAL_X(col + offsets.sw.x) ? col + offsets.sw.x : -1;
    result.sw.y = VAL_Y(row + offsets.sw.y) ? row + offsets.sw.y : -1;
    
    result.w.x = VAL_X(col + offsets.w.x) ? col + offsets.w.x : -1;
    result.w.y = VAL_Y(row + offsets.w.y) ? row + offsets.w.y : -1;
    
    result.nw.x = VAL_X(col + offsets.nw.x) ? col + offsets.nw.x : -1;
    result.nw.y = VAL_Y(row + offsets.nw.y) ? row + offsets.nw.y : -1;
    
    return result;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
