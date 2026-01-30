//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <set>
#include <algorithm>
#include "map_model.h"

//================================================================================================================================
//=> - MapModel implementation -
//================================================================================================================================

MapModel::MapModel () : m_tiler(nullptr) {
}

MapModel::~MapModel () {
}

void MapModel::setTiles (const std::vector<std::vector<MapTile>> &tiles) {
    m_tiles = tiles;
}

void MapModel::setLines (const std::vector<Line> &lines) {
    m_lines = lines;
}

void MapModel::setTileElevations (int **z_values) {
    for (size_t row_idx = 0; row_idx < m_tiles.size(); row_idx++) {
        for (size_t col_idx = 0; col_idx < m_tiles[row_idx].size(); col_idx++) {
            m_tiles[row_idx][col_idx].z = static_cast<int16_t>(z_values[row_idx][col_idx]);
        }
    }    
    std::map<std::pair<int16_t, int16_t>, std::pair<float, int>> corner_z_sums;
    for (size_t row_idx = 0; row_idx < m_tiles.size(); row_idx++) {
        for (size_t col_idx = 0; col_idx < m_tiles[row_idx].size(); col_idx++) {
            const MapTile& t = m_tiles[row_idx][col_idx];
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

    for (size_t row_idx = 0; row_idx < m_tiles.size(); row_idx++) {
        for (size_t col_idx = 0; col_idx < m_tiles[row_idx].size(); col_idx++) {
            MapTile& t = m_tiles[row_idx][col_idx];
            
            auto it_top = corner_elevations.find(std::make_pair(t.top.x, t.top.y));
            if (it_top != corner_elevations.end()) {
                t.deltas.top = static_cast<int16_t>(-it_top->second);
            }
            auto it_right = corner_elevations.find(std::make_pair(t.right.x, t.right.y));
            if (it_right != corner_elevations.end()) {
                t.deltas.right = static_cast<int16_t>(-it_right->second);
            }
            auto it_bottom = corner_elevations.find(std::make_pair(t.bottom.x, t.bottom.y));
            if (it_bottom != corner_elevations.end()) {
                t.deltas.bottom = static_cast<int16_t>(-it_bottom->second);
            }
            auto it_left = corner_elevations.find(std::make_pair(t.left.x, t.left.y));
            if (it_left != corner_elevations.end()) {
                t.deltas.left = static_cast<int16_t>(-it_left->second);
            }
            
            int16_t y0 = t.top.y + t.deltas.top;
            int16_t y1 = t.right.y + t.deltas.right;
            int16_t y2 = t.bottom.y + t.deltas.bottom;
            int16_t y3 = t.left.y + t.deltas.left;
            t.lowest = std::min({y0, y1, y2, y3});
            t.highest = std::max({y0, y1, y2, y3});
        }
    }
}

int MapModel::getWidth () const {
    return m_tiles.size();
}

int MapModel::getHeight () const {
    return m_tiles[0].size();
}

std::vector<std::vector<MapTile>> MapModel::getTiles () const {
    return m_tiles;
}

std::vector<std::vector<MapTile>> &MapModel::getTilesRef () {
    return m_tiles;
}

std::vector<Line> MapModel::getLines () const {
    return m_lines;
}

void MapModel::saveTilesToFile (const std::string &filename) const {
    std::ofstream file (filename);
    for (const auto &row : m_tiles) {
        for (const auto &tile : row) {
            file << tile.top.x << "," << tile.top.y << ";";
            file << tile.right.x << "," << tile.right.y << ";";
            file << tile.bottom.x << "," << tile.bottom.y << ";";
            file << tile.left.x << "," << tile.left.y << "\n";
        }
    }
    file.close();
}

void MapModel::validateCornerElevations () const {
    std::map<std::pair<int32_t, int32_t>, std::vector<int32_t>> corner_elevations;
    for (size_t row_idx = 0; row_idx < m_tiles.size(); row_idx++) {
        for (size_t col_idx = 0; col_idx < m_tiles[row_idx].size(); col_idx++) {
            const MapTile& tile = m_tiles[row_idx][col_idx];
            std::pair<int32_t, int32_t> top_key = std::make_pair(tile.top.x, tile.top.y);
            std::pair<int32_t, int32_t> right_key = std::make_pair(tile.right.x, tile.right.y);
            std::pair<int32_t, int32_t> bottom_key = std::make_pair(tile.bottom.x, tile.bottom.y);
            std::pair<int32_t, int32_t> left_key = std::make_pair(tile.left.x, tile.left.y);
            corner_elevations[top_key].push_back(tile.top.y + tile.deltas.top);
            corner_elevations[right_key].push_back(tile.right.y + tile.deltas.right);
            corner_elevations[bottom_key].push_back(tile.bottom.y + tile.deltas.bottom);
            corner_elevations[left_key].push_back(tile.left.y + tile.deltas.left);
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
