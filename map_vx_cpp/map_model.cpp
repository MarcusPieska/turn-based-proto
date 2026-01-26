//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <fstream>
#include <sstream>
#include <map>
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

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
