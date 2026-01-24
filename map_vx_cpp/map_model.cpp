//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <fstream>
#include <sstream>
#include <map>
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
            m_tiles[row_idx][col_idx].z = z_values[row_idx][col_idx];
        }
    }    
    std::map<std::pair<int, int>, std::pair<float, int>> corner_z_sums;
    for (size_t row_idx = 0; row_idx < m_tiles.size(); row_idx++) {
        for (size_t col_idx = 0; col_idx < m_tiles[row_idx].size(); col_idx++) {
            const MapTile& t = m_tiles[row_idx][col_idx];
            for (int corner = 0; corner < 4; corner++) {
                std::pair<int, int> corner_pos = std::make_pair(t.pts[corner].x, t.pts[corner].y);
                corner_z_sums[corner_pos].first += t.z;
                corner_z_sums[corner_pos].second += 1;
            }
        }
    }
    
    std::map<std::pair<int, int>, float> corner_elevations;
    for (auto& pair : corner_z_sums) {
        if (pair.second.second > 0) {
            corner_elevations[pair.first] = pair.second.first / pair.second.second;
        }
    }

    for (size_t row_idx = 0; row_idx < m_tiles.size(); row_idx++) {
        for (size_t col_idx = 0; col_idx < m_tiles[row_idx].size(); col_idx++) {
            MapTile& t = m_tiles[row_idx][col_idx];
            for (int corner = 0; corner < 4; corner++) {
                std::pair<int, int> corner_pos = std::make_pair(t.pts[corner].x, t.pts[corner].y);
                auto it = corner_elevations.find(corner_pos);
                if (it != corner_elevations.end()) {
                    t.pts[corner].y -= it->second;
                }
            }
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
            file << tile.pts[0].x << "," << tile.pts[0].y << ";";
            file << tile.pts[1].x << "," << tile.pts[1].y << ";";
            file << tile.pts[2].x << "," << tile.pts[2].y << ";";
            file << tile.pts[3].x << "," << tile.pts[3].y << "\n";
        }
    }
    file.close();
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
