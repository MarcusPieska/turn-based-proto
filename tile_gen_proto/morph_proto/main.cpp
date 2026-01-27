//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdlib>
#include <ctime>
#include <cstring>
#include <chrono>
#include <SDL2/SDL.h>
#include "col_morph.h"
#include "dat15_io.h"

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

static void load_tile_into_texture (SDL_Texture* tex, int w, int h, const char* dat15_path, int tile_idx) {
    void* pixels;
    int pitch;
    SDL_LockTexture(tex, nullptr, &pixels, &pitch);
    u8* img = (u8*)pixels;
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * pitch + x * 4;
            img[idx + 0] = 255;
            img[idx + 1] = 0;
            img[idx + 2] = 255;
            img[idx + 3] = 0;
        }
    }
    
    Dat15Reader reader(dat15_path);
    int size_out = 0;
    const void* item_data = reader.get_item(0, tile_idx, &size_out);
    if (item_data && size_out > 0) {
        int tile_w = 101;
        int tile_h = 51;
        int src_channels = 3;
        int expected_size = tile_w * tile_h * src_channels;
        if (size_out >= expected_size) {
            int tile_start_y = h - tile_h;
            const u8* src = (const u8*)item_data;
            for (int y = 0; y < tile_h; y++) {
                for (int x = 0; x < tile_w; x++) {
                    int src_idx = (y * tile_w + x) * src_channels;
                    int dst_idx = (tile_start_y + y) * pitch + x * 4;
                    img[dst_idx + 0] = src[src_idx + 0];
                    img[dst_idx + 1] = src[src_idx + 1];
                    img[dst_idx + 2] = src[src_idx + 2];
                    img[dst_idx + 3] = 255;
                }
            }
        }
    }
    
    SDL_UnlockTexture(tex);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    std::srand(std::time(nullptr));
    
    const int tile_w = 101;
    const int tile_h = 51;
    const int morph_h = tile_h * 3;
    const int channels = 4;
    const float scale = 3.0f;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }
    
    int window_w = int(tile_w * scale);
    int window_h = (int)(morph_h * scale);
    
    SDL_Window* window = SDL_CreateWindow(
        "Tile Morpher",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        window_w,
        window_h,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    SDL_Texture* src_tex = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        tile_w,
        morph_h
    );
    
    SDL_Texture* dst_tex = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        tile_w,
        morph_h
    );
    
    if (!src_tex || !dst_tex) {
        SDL_Log("Texture creation failed: %s", SDL_GetError());
        if (src_tex) SDL_DestroyTexture(src_tex);
        if (dst_tex) SDL_DestroyTexture(dst_tex);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    SDL_SetTextureBlendMode(dst_tex, SDL_BLENDMODE_BLEND);
    
    const char* dat15_path = "/home/w/Projects/img-content/texture-grassland3/colors-grassland3_palette_texture_blurred.dat15";
    load_tile_into_texture(src_tex, tile_w, morph_h, dat15_path, 0);
    
    int half_w = tile_w / 2;
    int half_h = tile_h / 2;
    int tile_center_y = morph_h - tile_h + half_h;
    
    tile_pts pts = {
        {tile_w / 2, tile_center_y - half_h},
        {tile_w / 2 + half_w, tile_center_y},
        {tile_w / 2, tile_center_y + half_h},
        {tile_w / 2 - half_w, tile_center_y}
    };
    
    deltas d = {0, 0, 0, 0};
    
    size src_size = {tile_w, morph_h};
    morph_tile(src_tex, dst_tex, src_size, channels, pts, d);
    
    bool running = true;
    SDL_Event event;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    auto start_time = std::chrono::high_resolution_clock::now();
                    
                    for (int i = 0; i < 512; i++) {
                        d.top_dy = (std::rand() % 81) - 40;
                        d.right_dy = (std::rand() % 41) - 20;
                        d.left_dy = (std::rand() % 41) - 20;
                        d.bottom_dy = 0;
                        
                        morph_tile(src_tex, dst_tex, src_size, channels, pts, d);
                    }
                    
                    auto end_time = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                    SDL_Log("Generated 512 morphs in %ld ms (%.2f ms per morph)", duration.count(), duration.count() / 512.0f);
                }
            }
        }
        
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
        SDL_RenderClear(renderer);
        
        int scaled_w = (int)(tile_w * scale);
        int scaled_h = (int)(morph_h * scale);
        int dst_x = (window_w - scaled_w) / 2;
        int dst_y = (window_h - scaled_h) / 2;
        
        SDL_Rect dst_rect = {dst_x, dst_y, scaled_w, scaled_h};
        SDL_RenderCopy(renderer, dst_tex, nullptr, &dst_rect);
        
        SDL_RenderPresent(renderer);
    }
    
    SDL_DestroyTexture(src_tex);
    SDL_DestroyTexture(dst_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
