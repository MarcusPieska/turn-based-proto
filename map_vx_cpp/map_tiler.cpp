//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cmath>
#include <algorithm>
#include <limits>

#include "map_tiler.h"
#include "map_model.h"

//================================================================================================================================
//=> - MapTiler implementation -
//================================================================================================================================

MapTiler::MapTiler (int map_w, int map_h, int tile_w, int tile_h, MapModel *model, int added_top_margin) :
    m_tile_w (tile_w),
    m_tile_h (tile_h),
    m_half_w (tile_w / 2),
    m_half_h (tile_h / 2),
    m_num_cols (map_w / tile_w),
    m_num_rows ((map_h - tile_h) / m_half_h),
    m_x_min ((map_w - (m_num_cols * tile_w)) / 2),
    m_y_min ((map_h - (m_num_rows * m_half_h)) / 2 - m_half_h / 2),
    m_model (model) {
    
    std::vector<std::vector<MapTile>> tiles;
    std::vector<Line> lines;
    std::vector<Point> left_pts, right_pts, bottom_pts, top_pts;
    
    int y = m_y_min + m_half_h;
    int row = 0;
    while (row < m_num_rows) {
        int x_offset = (row % 2 == 1) ? m_half_w : 0;
        int x = m_x_min + x_offset + m_half_w;
        
        int col = 0;
        std::vector<MapTile> tile_row;
        while (x + m_half_w <= map_w - m_x_min) {
            int cx = x;
            int cy = y;
            
            Point top (cx, cy - m_half_h + added_top_margin);
            Point right (cx + m_half_w, cy + added_top_margin);
            Point bottom (cx, cy + m_half_h + added_top_margin);
            Point left (cx - m_half_w, cy + added_top_margin);
            
            if ((col == 0) && ((row % 2) == 0)) {
                left_pts.push_back(left);
            }
            if ((col == m_num_cols - 1) && ((row % 2) == 0)) {
                right_pts.push_back(right);
            }
            
            if (row == 0) {
                top_pts.push_back(top);
            } else if (row == m_num_rows - 1) {
                bottom_pts.push_back(bottom);
            }
            
            tile_row.push_back(MapTile(top, right, bottom, left));
            x += tile_w;
            col++;
        }
        
        tiles.push_back(tile_row);
        y += m_half_h;
        row++;
    }
    
    m_model->setTiles(tiles);
    
    std::vector<Point> combined_left_bottom = left_pts;
    combined_left_bottom.insert(combined_left_bottom.end(), bottom_pts.begin(), bottom_pts.end());
    std::vector<Point> combined_top_right = top_pts;
    combined_top_right.insert(combined_top_right.end(), right_pts.begin(), right_pts.end());
    
    size_t min_size = std::min(combined_left_bottom.size(), combined_top_right.size());
    for (size_t i = 0; i < min_size; i++) {
        lines.push_back(Line(combined_left_bottom[i], combined_top_right[i]));
    }
    
    std::vector<Point> bottom_reversed = bottom_pts;
    std::reverse(bottom_reversed.begin(), bottom_reversed.end());
    std::vector<Point> top_reversed = top_pts;
    std::reverse(top_reversed.begin(), top_reversed.end());
    
    std::vector<Point> combined_right_bottom = right_pts;
    combined_right_bottom.insert(combined_right_bottom.end(), bottom_reversed.begin(), bottom_reversed.end());
    std::vector<Point> combined_top_left = top_reversed;
    combined_top_left.insert(combined_top_left.end(), left_pts.begin(), left_pts.end());
    
    min_size = std::min(combined_right_bottom.size(), combined_top_left.size());
    for (size_t i = 0; i < min_size; i++) {
        lines.push_back(Line(combined_right_bottom[i], combined_top_left[i]));
    }
    
    m_model->setLines(lines);
    m_model->m_tiler = this;
}

MapTiler::~MapTiler () {
}

std::tuple<int, int> MapTiler::coordsToIndices (int x, int y, const std::vector<std::vector<MapTile>> &tiles) {
    auto result = coordsToTile(x, y, &tiles);
    int row = std::get<1>(result);
    int col = std::get<2>(result);
    return std::make_tuple(row, col);
}

std::tuple<MapTile*, int, int> MapTiler::coordsToTile (int x, int y, const std::vector<std::vector<MapTile>> *tiles_ptr) {
    std::vector<std::vector<MapTile>> model_tiles = m_model->getTilesRef();
    bool use_model = (tiles_ptr == nullptr);
    const std::vector<std::vector<MapTile>> &tiles = use_model ? model_tiles : *tiles_ptr;
    
    if (tiles.empty()) {
        return std::make_tuple(nullptr, -1, -1);
    }
    
    MapTile *best_tile = nullptr;
    int best_row = -1;
    int best_col = -1;
    double best_dist = std::numeric_limits<double>::infinity();
    
    for (size_t row = 0; row < tiles.size(); row++) {
        int row_y = m_y_min + m_half_h + row * m_half_h;
        for (size_t col = 0; col < tiles[row].size(); col++) {
            const MapTile &tile = tiles[row][col];
            int cx = tile.pts[0].x;
            int cy = row_y;
            int dx = std::abs(x - cx);
            int dy = std::abs(y - cy);
            if (dx * m_half_h + dy * m_half_w <= m_half_w * m_half_h) {
                double dist = dx * dx + dy * dy;
                if (dist < best_dist) {
                    best_dist = dist;
                    best_row = row;
                    best_col = col;
                    if (use_model) {
                        best_tile = &model_tiles[row][col];
                    } else {
                        best_tile = const_cast<MapTile*>(&tile);
                    }
                }
            }
        }
    }
    
    return std::make_tuple(best_tile, best_row, best_col);
}

std::vector<MapTile*> MapTiler::tileIdxToNearTiles (int row, int col) {
    std::vector<std::vector<MapTile>> &tiles = m_model->getTilesRef();
    std::vector<MapTile*> selected;
    
    if (row < 0 || col < 0 || row >= static_cast<int>(tiles.size()) || col >= static_cast<int>(tiles[row].size())) {
        selected.resize(4, nullptr);
        return selected;
    }
    
    int row_offset = (row % 2 == 0) ? -1 : 0;
    
    int target_col = col + row_offset;
    int target_row = row - 1;
    if (target_row >= 0) {
        if (target_col >= 0 && target_col < static_cast<int>(tiles[target_row].size())) {
            selected.push_back(&tiles[target_row][target_col]);
        } else {
            selected.push_back(nullptr);
        }
        target_col++;
        if (target_col >= 0 && target_col < static_cast<int>(tiles[target_row].size())) {
            selected.push_back(&tiles[target_row][target_col]);
        } else {
            selected.push_back(nullptr);
        }
    } else {
        selected.push_back(nullptr);
        selected.push_back(nullptr);
    }
    
    target_col = col + row_offset;
    target_row = row + 1;
    if (target_row < static_cast<int>(tiles.size())) {
        if (target_col >= 0 && target_col < static_cast<int>(tiles[target_row].size())) {
            selected.push_back(&tiles[target_row][target_col]);
        } else {
            selected.push_back(nullptr);
        }
        target_col++;
        if (target_col >= 0 && target_col < static_cast<int>(tiles[target_row].size())) {
            selected.push_back(&tiles[target_row][target_col]);
        } else {
            selected.push_back(nullptr);
        }
    } else {
        selected.push_back(nullptr);
        selected.push_back(nullptr);
    }
    
    return selected;
}

std::vector<MapTile*> MapTiler::coordsToNearTiles (int x, int y) {
    std::vector<std::vector<MapTile>> &tiles = m_model->getTilesRef();
    auto indices = coordsToIndices(x, y, tiles);
    int row = std::get<0>(indices);
    int col = std::get<1>(indices);
    
    if (row == -1 || col == -1) {
        std::vector<MapTile*> selected;
        selected.resize(4, nullptr);
        return selected;
    }
    
    return tileIdxToNearTiles(row, col);
}

std::vector<MapTile*> MapTiler::tileIdxToDiagonalTiles (int row, int col) {
    std::vector<std::vector<MapTile>> &tiles = m_model->getTilesRef();
    std::vector<MapTile*> selected;
    
    if (row < 0 || col < 0 || row >= static_cast<int>(tiles.size()) || col >= static_cast<int>(tiles[row].size())) {
        selected.resize(4, nullptr);
        return selected;
    }
    
    int target_col = col - 1;
    if (target_col >= 0 && target_col < static_cast<int>(tiles[row].size())) {
        selected.push_back(&tiles[row][target_col]);
    } else {
        selected.push_back(nullptr);
    }
    
    target_col = col + 1;
    if (target_col >= 0 && target_col < static_cast<int>(tiles[row].size())) {
        selected.push_back(&tiles[row][target_col]);
    } else {
        selected.push_back(nullptr);
    }
    
    if (row >= 2) {
        int target_row = row - 2;
        if (col >= 0 && col < static_cast<int>(tiles[target_row].size())) {
            selected.push_back(&tiles[target_row][col]);
        } else {
            selected.push_back(nullptr);
        }
    } else {
        selected.push_back(nullptr);
    }
    
    if (row < static_cast<int>(tiles.size()) - 2) {
        int target_row = row + 2;
        if (col >= 0 && col < static_cast<int>(tiles[target_row].size())) {
            selected.push_back(&tiles[target_row][col]);
        } else {
            selected.push_back(nullptr);
        }
    } else {
        selected.push_back(nullptr);
    }
    
    return selected;
}

std::vector<MapTile*> MapTiler::coordsToDiagonalTiles (int x, int y) {
    std::vector<std::vector<MapTile>> &tiles = m_model->getTilesRef();
    auto indices = coordsToIndices(x, y, tiles);
    int row = std::get<0>(indices);
    int col = std::get<1>(indices);
    
    if (row == -1 || col == -1) {
        std::vector<MapTile*> selected;
        selected.resize(4, nullptr);
        return selected;
    }
    
    return tileIdxToDiagonalTiles(row, col);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
