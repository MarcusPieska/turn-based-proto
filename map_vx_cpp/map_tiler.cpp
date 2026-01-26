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
    
    std::vector<std::vector<MapTile>> tiles;
    int y = m_y_min + m_half_h;
    for (int row = 0; row < m_num_rows; row++) {
        int x_offset = (row % 2 == 1) ? m_half_w : 0;
        int x = m_x_min + x_offset + m_half_w;
        std::vector<MapTile> tile_row;
        for (int col = 0; col < m_num_cols; col++) {
            Point top (x, y - m_half_h);
            Point right (x + m_half_w, y);
            Point bottom (x, y + m_half_h);
            Point left (x - m_half_w, y);
            tile_row.push_back(MapTile(top, right, bottom, left));
            x += tile_w;
        }
        tiles.push_back(tile_row);
        y += m_half_h;
    }
    m_model->setTiles(tiles);
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
            int cx = tile.top.x;
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
