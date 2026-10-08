//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <SDL2/SDL.h>
#include <iostream>

#include "intf_mk1_snapshot_view.h"
#include "map_tiler.h"
#include "map_model.h"
#include "map_view.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_TERR = "/home/w/Projects/simple-map-gen/p1-seed-43/terrain.ppm";
static const char* G_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-43/climate.ppm";
static const char* G_RIV = "/home/w/Projects/simple-map-gen/p1-seed-43/rivers.ppm";

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    IntfMk1_SnapshotView snap;
    if (!snap.load_map_ppms(G_TERR, G_CLIM, G_RIV, nullptr) || !snap.ready()) {
        std::cerr << "Failed to load map PPMs via IntfMk1_SnapshotView" << std::endl;
        return 1;
    }

    float height_factor = 0.0f;
    int t_width = 100;
    int t_height = 50;
    int w_width = 1600;
    int w_height = 800;
    int tile_cols = static_cast<int>(snap.width());
    int tile_rows = static_cast<int>(snap.height());

    int added_top_margin = 50;
    int added_bottom_margin = 100;
    int m_width = tile_cols * t_width + t_width * 2;
    int m_height = (tile_rows + 1) * t_height / 2 + added_top_margin + added_bottom_margin;

    std::cout << "tile_cols: " << tile_cols << std::endl;
    std::cout << "tile_rows: " << tile_rows << std::endl;
    std::cout << "m_width: " << m_width << std::endl;
    std::cout << "m_height: " << m_height << std::endl;

    MapModel model;
    MapTiler tiler (m_width, m_height, t_width, t_height, tile_cols, tile_rows, &model, added_top_margin);

    MapView view (w_width, w_height, m_width, m_height, t_width, t_height, &model);
    if (!view.initialize ()) {
        return 1;
    }
    
    view.preRenderSetup (&snap, height_factor);
    while (view.isRunning ()) {
        view.handleEvents ();
        view.update ();
        view.renderOpt ();
        SDL_Delay (16);
    }
    
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
