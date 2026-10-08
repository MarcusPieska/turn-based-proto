//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <map>
#include <set>
#include <algorithm>
#include "map_model.h"

//================================================================================================================================
//=> - MapModel implementation -
//================================================================================================================================

MapModel::MapModel () : m_tiler(nullptr), m_tiles(nullptr), m_num_rows(0), m_num_cols(0) {
}

MapModel::~MapModel () {
    if (m_tiles) {
        if (m_num_rows > 0) {
            free(m_tiles[0]);
        }
        free(m_tiles);
        m_tiles = nullptr;
    }
}

void MapModel::allocateTiles (int num_rows, int num_cols) {
    m_num_rows = num_rows;
    m_num_cols = num_cols;
    m_tiles = (MapModelTile**)malloc(num_rows * sizeof(MapModelTile*));
    MapModelTile* data = (MapModelTile*)malloc(num_rows * num_cols * sizeof(MapModelTile));
    for (int i = 0; i < num_rows; i++) {
        m_tiles[i] = data + i * num_cols;
        for (int j = 0; j < num_cols; j++) {
            m_tiles[i][j] = MapModelTile();
        }
    }
}

void MapModel::setLines (const std::vector<Line> &lines) {
    m_lines = lines;
}

void MapModel::setTileElevations (int **z_values) {
    for (int row_idx = 0; row_idx < m_num_rows; row_idx++) {
        for (int col_idx = 0; col_idx < m_num_cols; col_idx++) {
            m_tiles[row_idx][col_idx].z = static_cast<int32_t>(z_values[row_idx][col_idx]);
        }
    }    
    std::map<std::pair<int32_t, int32_t>, std::pair<float, int>> corner_z_sums;
    for (int row_idx = 0; row_idx < m_num_rows; row_idx++) {
        for (int col_idx = 0; col_idx < m_num_cols; col_idx++) {
            const MapModelTile& t = m_tiles[row_idx][col_idx];
            corner_z_sums[std::make_pair(t.top.x, t.top.y)].first += t.z;
            corner_z_sums[std::make_pair(t.top.x, t.top.y)].second += 1;
            corner_z_sums[std::make_pair(t.right.x, t.right.y)].first += t.z;
            corner_z_sums[std::make_pair(t.right.x, t.right.y)].second += 1;
            corner_z_sums[std::make_pair(t.bottom.x, t.bottom.y)].first += t.z;
            corner_z_sums[std::make_pair(t.bottom.x, t.bottom.y)].second += 1;
            corner_z_sums[std::make_pair(t.left.x, t.left.y)].first += t.z;
            corner_z_sums[std::make_pair(t.left.x, t.left.y)].second += 1;
        }
    }
    
    std::map<std::pair<int16_t, int16_t>, float> corner_elevations;
    for (auto& pair : corner_z_sums) {
        if (pair.second.second > 0) {
            corner_elevations[pair.first] = pair.second.first / pair.second.second;
        }
    }

    for (int row_idx = 0; row_idx < m_num_rows; row_idx++) {
        for (int col_idx = 0; col_idx < m_num_cols; col_idx++) {
            MapModelTile& t = m_tiles[row_idx][col_idx];
            
        }
    }
}

int MapModel::getWidth () const {
    return m_num_rows;
}

int MapModel::getHeight () const {
    return m_num_cols;
}

MapModelTile** MapModel::getTiles () {
    return m_tiles;
}

MapModelTile* MapModel::getTile (int row, int col) {
    if (row >= 0 && row < m_num_rows && col >= 0 && col < m_num_cols) {
        return &m_tiles[row][col];
    }
    return nullptr;
}

std::vector<Line> MapModel::getLines () const {
    return m_lines;
}

void MapModel::validateCornerElevations () const {
    std::map<std::pair<int32_t, int32_t>, std::vector<int32_t>> corner_elevations;
    for (int row_idx = 0; row_idx < m_num_rows; row_idx++) {
        for (int col_idx = 0; col_idx < m_num_cols; col_idx++) {
            const MapModelTile& tile = m_tiles[row_idx][col_idx];
            std::pair<int32_t, int32_t> top_key = std::make_pair(tile.top.x, tile.top.y);
            std::pair<int32_t, int32_t> right_key = std::make_pair(tile.right.x, tile.right.y);
            std::pair<int32_t, int32_t> bottom_key = std::make_pair(tile.bottom.x, tile.bottom.y);
            std::pair<int32_t, int32_t> left_key = std::make_pair(tile.left.x, tile.left.y);
            // Note: deltas are stored in MapViewTile, not MapModelTile
            // This validation needs to be updated to work with the new structure
            corner_elevations[top_key].push_back(tile.top.y);
            corner_elevations[right_key].push_back(tile.right.y);
            corner_elevations[bottom_key].push_back(tile.bottom.y);
            corner_elevations[left_key].push_back(tile.left.y);
        }
    }
    int total_checks = 0;
    int failures = 0;
    int count_4_corners = 0;
    for (const auto& pair : corner_elevations) {
        const std::vector<int32_t>& elevations = pair.second;
        int co_located_count = elevations.size();
        if (co_located_count == 4) {
            count_4_corners++;
        }
        if (co_located_count > 0) {
            int32_t first_elevation = elevations[0];
            bool all_match = true;
            for (size_t i = 1; i < elevations.size(); i++) {
                if (elevations[i] != first_elevation) {
                    all_match = false;
                    break;
                }
            }
            total_checks++;
            if (!all_match) {
                failures++;
            }
        }
    }
    double percent_4_corners = corner_elevations.size() > 0 ? (100.0 * count_4_corners / corner_elevations.size()) : 0.0;
    std::cout << "*** Corner elevation validation results:" << std::endl;
    std::cout << "    Total checks: " << total_checks << std::endl;
    std::cout << "    Failures: " << failures << std::endl;
    std::cout << "    Percentage with 4 co-located corners: " << percent_4_corners << "%" << std::endl;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
