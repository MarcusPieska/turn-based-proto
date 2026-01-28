//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>
#include <iostream>
#include "dat15_io.h"

static SDL_Surface* create_surface_from_rgba(const unsigned char* rgba_data, int w, int h) {
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_LockSurface(surface);
    unsigned char* dst = (unsigned char*)surface->pixels;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int src_idx = (y * w + x) * 4;
            int dst_idx = y * surface->pitch + x * 4;
            dst[dst_idx + 0] = rgba_data[src_idx + 2];
            dst[dst_idx + 1] = rgba_data[src_idx + 1];
            dst[dst_idx + 2] = rgba_data[src_idx + 0];
            dst[dst_idx + 3] = rgba_data[src_idx + 3];
        }
    }
    SDL_UnlockSurface(surface);
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
    return surface;
}

//================================================================================================================================
//=> - Header struct -
//================================================================================================================================

struct dat15_top_hdr {
    unsigned int num_rows : 16;
    unsigned int num_cols : 16;
};

struct dat15_hdr {
    unsigned int has_next : 1;
    unsigned int size : 15;
};

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

static void write_header (FILE* f, int size, bool has_next) {
    dat15_hdr hdr;
    hdr.size = size;
    hdr.has_next = has_next ? 1 : 0;
    fwrite(&hdr, sizeof(hdr), 1, f);
}

//================================================================================================================================
//=> - Read class -
//================================================================================================================================

Dat15Reader::Dat15Reader (const char* path, int tile_w, int tile_h, SDL_Renderer* renderer) : m_tile_w(tile_w), m_tile_h(tile_h), m_renderer(renderer) {
    FILE* m_ptr = fopen (path, "rb");
    if (!m_ptr) { 
        std::cerr << "*** Error: Failed to open file: " << path << std::endl;
        return;
    }
    dat15_top_hdr top_hdr;
    if (fread (&top_hdr, sizeof (top_hdr), 1, m_ptr) != 1) {
        std::cerr << "*** Error: Failed to read top header: " << path << std::endl;
        return;
    }
    m_num_rows = top_hdr.num_rows;
    m_num_cols = top_hdr.num_cols;
    std::cout << "*** Dat15Reader: " << m_num_rows << " x " << m_num_cols << std::endl;
    
    int rgba_size = tile_w * tile_h * 4;
    
    while (1) {
        dat15_hdr hdr;
        if (fread (&hdr, sizeof (hdr), 1, m_ptr) != 1) {
            break;
        }
        std::vector<unsigned char> item_rgb (hdr.size);
        if (fread (item_rgb.data (), 1, hdr.size, m_ptr) != hdr.size) {
            break;
        }
        
        std::vector<unsigned char> item_rgba (rgba_size);
        const unsigned char* rgb = item_rgb.data();
        unsigned char* rgba = item_rgba.data();
        for (int i = 0; i < tile_w * tile_h; i++) {
            bool is_magenta = (rgb[i*3+0] == 255 && rgb[i*3+1] == 0 && rgb[i*3+2] == 255);
            rgba[i*4+0] = rgb[i*3+0];
            rgba[i*4+1] = rgb[i*3+1];
            rgba[i*4+2] = rgb[i*3+2];
            rgba[i*4+3] = is_magenta ? 0 : 255;
        }
        
        SDL_Texture* texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STREAMING,
            tile_w,
            tile_h
        );
        if (!texture) {
            continue;
        }
        
        void* pixels = nullptr;
        int pitch = 0;
        if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) != 0) {
            SDL_DestroyTexture(texture);
            continue;
        }
        
        unsigned char* dst = static_cast<unsigned char*>(pixels);
        for (int y = 0; y < tile_h; y++) {
            for (int x = 0; x < tile_w; x++) {
                int src_idx = (y * tile_w + x) * 4;
                int dst_idx = y * pitch + x * 4;
                dst[dst_idx + 0] = rgba[src_idx + 0]; // R
                dst[dst_idx + 1] = rgba[src_idx + 1]; // G
                dst[dst_idx + 2] = rgba[src_idx + 2]; // B
                dst[dst_idx + 3] = rgba[src_idx + 3]; // A
            }
        }
        
        SDL_UnlockTexture(texture);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        m_textures.push_back(texture);
        
        if (!hdr.has_next) {
            break;
        }
    }
    fclose(m_ptr);
}

Dat15Reader::~Dat15Reader () {
    for (SDL_Texture* tex : m_textures) {
        if (tex != nullptr) {
            SDL_DestroyTexture(tex);
        }
    }
}

SDL_Texture* Dat15Reader::get_item_rgba (int row, int col) {
    row = row % m_num_rows;
    col = col % m_num_cols;
    int idx = row * m_num_cols + col;
    return m_textures[idx];
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
