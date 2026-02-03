//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <SDL2/SDL_image.h>
#include <iostream>

#include "map_tiler.h"
#include "map_model.h"
#include "map_view.h"

//================================================================================================================================
//=> - Main -
//================================================================================================================================

SDL_Surface* load_image (const char* img_path) {
    SDL_Surface* img_surface = IMG_Load(img_path);
    if (!img_surface) {
        std::cerr << "Unable to load image " << img_path << "! SDL_image Error: " << IMG_GetError() << std::endl;
        IMG_Quit();
        return nullptr;
    }
    return img_surface;
}

int main () {
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        return 1;
    }
    const char* img_path_map_terrain = "/home/w/Projects/rts-proto-map/first-test/cont001.png";
    const char* img_path_map_height = "/home/w/Projects/rts-proto-map/first-test/cont001_hm.png";
    SDL_Surface* img_surface_terrain = load_image (img_path_map_terrain);
    SDL_Surface* img_surface_height = load_image (img_path_map_height);
    float height_factor = 1.00f;
    int t_width = 100;
    int t_height = 50;
    int w_width = 1600;
    int w_height = 800;
    int tile_cols = img_surface_terrain->w;
    int tile_rows = img_surface_terrain->h;

    int added_top_margin = 255 * height_factor; 
    int added_bottom_margin = 100;
    int img_width = img_surface_terrain->w;
    int img_height = img_surface_terrain->h;
    int m_width = img_width * t_width + t_width * 2; 
    int m_height = (img_height + 1) * t_height / 2 + added_top_margin + added_bottom_margin;

    std::cout << "img_width: " << img_width << std::endl;
    std::cout << "img_height: " << img_height << std::endl;
    std::cout << "m_width: " << m_width << std::endl;
    std::cout << "m_height: " << m_height << std::endl;

    MapModel model;
    MapTiler tiler (m_width, m_height, t_width, t_height, tile_cols, tile_rows, &model, added_top_margin);

    MapView view (w_width, w_height, m_width, m_height, t_width, t_height, &model);
    if (!view.initialize ()) {
        return 1;
    }
    
    view.preRenderSetup (img_surface_height, height_factor);
    //view.testTileClickDetection ();
    //model.validateCornerElevations ();
    //return 0;
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
