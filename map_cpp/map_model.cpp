//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <fstream>
#include <sstream>
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
