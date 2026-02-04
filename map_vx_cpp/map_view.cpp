//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <tuple>
#include <cmath>
#include <climits>
#include <cstdlib>
#include <cstring>

#include "map_tiler.h"
#include "col_morph.h"
#include "dat15_io.h"
#include "dat31_io.h"
#include "map_types.h"
#include "hlp_canvas/map_mini.h"

#include "map_view.h"

#define HEIGHT_COLOR_MAX 255
#define U8_MAX 255

#define ADJ_CX(x) ((x - m_cx) * m_zoom_level / 100)
#define ADJ_CY(y) ((y - m_cy) * m_zoom_level / 100)
#define ADJ_CY_TOP(t) ((t.top.y - m_cy + t.deltas.top) * m_zoom_level / 100)
#define ADJ_CY_RIGHT(t) ((t.right.y - m_cy + t.deltas.right) * m_zoom_level / 100)
#define ADJ_CY_BOTTOM(t) ((t.bottom.y - m_cy + t.deltas.bottom) * m_zoom_level / 100)
#define ADJ_CY_LEFT(t) ((t.left.y - m_cy + t.deltas.left) * m_zoom_level / 100)

const Uint8 COLOR_OCEAN[] = {32, 26, 120};
const Uint8 COLOR_GRASSLAND[] = {121, 189, 36};
const Uint8 COLOR_PLAINS[] = {222, 199, 89};
const Uint8 COLOR_DESERT[] = {244, 203, 141};
const Uint8 COLOR_TUNDRA[] = {248, 251, 252};
const Uint8 COLOR_MOUNTAIN[] = {100, 50, 25};

static bool colorsMatch(const Uint8* color1, const Uint8* color2) {
    return (color1[0] == color2[0] && color1[1] == color2[1] && color1[2] == color2[2]);
}

static inline int32_t get_zy_top (const MapModelTile& t, const MapViewTile& v) { return t.top.y + v.deltas.top; }
static inline int32_t get_zy_right (const MapModelTile& t, const MapViewTile& v) { return t.right.y + v.deltas.right; }
static inline int32_t get_zy_bottom (const MapModelTile& t, const MapViewTile& v) { return t.bottom.y + v.deltas.bottom; }
static inline int32_t get_zy_left (const MapModelTile& t, const MapViewTile& v) { return t.left.y + v.deltas.left; }

//================================================================================================================================
//=> - MapView public methods -
//================================================================================================================================

MapView::MapView (int wnd_w, int wnd_h, int map_w, int map_h, int tile_w, int tile_h, MapModel *model) :
    m_wnd (nullptr),
    m_rend (nullptr),
    m_model (model),
    m_wnd_w (wnd_w),
    m_wnd_h (wnd_h),
    m_wnd_w_orig (wnd_w),
    m_wnd_h_orig (wnd_h),
    m_map_w (map_w),
    m_map_h (map_h),
    m_tile_w (tile_w),
    m_tile_h (tile_h),
    m_mtn_decal_w (200),
    m_mtn_decal_h (200),
    m_running (false),
    m_cx (0),
    m_cy (0),
    m_scroll_add (0),
    m_clicked_tile (nullptr),
    m_zoom_level (ZOOM_DEFAULT),
    m_tex_read_ocean (nullptr),
    m_tex_read_desert (nullptr),
    m_tex_read_plains (nullptr),
    m_tex_read_grassland (nullptr),
    m_tex_read_tundra (nullptr),
    m_tile_text_w (tile_w + 1),
    m_tile_text_h (tile_h + 1),
    m_mini (nullptr),
    m_terrain_map (nullptr),
    m_view_tiles (nullptr),
    m_num_rows (0),
    m_num_cols (0),
    m_prev_min_row (-1),
    m_prev_min_col (-1),
    m_prev_max_row (-1),
    m_prev_max_col (-1) {
}

MapView::~MapView () {
    cleanup ();
}

bool MapView::initialize () {
    if (SDL_Init (SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError () << std::endl;
        return false;
    }
    m_wnd = SDL_CreateWindow ("Map View", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_wnd_w, m_wnd_h, SDL_WINDOW_SHOWN);
    if (m_wnd == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError () << std::endl;
        SDL_Quit ();
        return false;
    }
    m_rend = SDL_CreateRenderer (m_wnd, -1, SDL_RENDERER_ACCELERATED);
    if (m_rend == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError () << std::endl;
        SDL_DestroyWindow (m_wnd);
        SDL_Quit ();
        return false;
    }
    m_running = true;
    m_tex_read_ocean = new Dat15Reader("tile_ocean.dat15", m_tile_text_w, m_tile_text_h, m_rend);
    m_tex_read_desert = new Dat15Reader("tile_desert.dat15", m_tile_text_w, m_tile_text_h, m_rend);
    m_tex_read_plains = new Dat15Reader("tile_plains.dat15", m_tile_text_w, m_tile_text_h, m_rend);
    m_tex_read_grassland = new Dat15Reader("tile_grassland.dat15", m_tile_text_w, m_tile_text_h, m_rend);
    m_tex_read_tundra = new Dat15Reader("tile_tundra.dat15", m_tile_text_w, m_tile_text_h, m_rend);
    m_decal_read_mtn = new Dat31Reader("decal_mtn2.dat31", m_mtn_decal_w, m_mtn_decal_h, m_rend);
    const char* mini_img_path = "/home/w/Projects/rts-proto-map/first-test/cont001.png";
    SDL_Surface* loaded_surface = IMG_Load(mini_img_path);
    if (loaded_surface) {
        m_terrain_map = SDL_ConvertSurfaceFormat(loaded_surface, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(loaded_surface);
    }
    Size mini_size(200, 200);
    Size canvas_size(m_wnd_w, m_wnd_h);
    Size map_size(400, 400);
    m_mini = new MapMini(m_rend, mini_img_path, mini_size, canvas_size, map_size);
    return true;
}

void MapView::update () {
    int mouse_x, mouse_y;
    Uint32 buttons = SDL_GetMouseState (&mouse_x, &mouse_y);
    int scroll_speed_x = EDGE_SCROLL_SPEED * 100 / m_zoom_level;
    int scroll_speed_y = EDGE_SCROLL_SPEED * 100 / m_zoom_level;
    if (mouse_x < EDGE_SCROLL_MARGIN) {
        m_cx -= scroll_speed_x + (EDGE_SCROLL_MARGIN - mouse_x) * 100 / m_zoom_level;
    } else if (mouse_x > m_wnd_w - EDGE_SCROLL_MARGIN) {
        m_cx += scroll_speed_x + (EDGE_SCROLL_MARGIN - (m_wnd_w - mouse_x)) * 100 / m_zoom_level;
    }
    if (mouse_y < EDGE_SCROLL_MARGIN) {
        m_cy -= scroll_speed_y + (EDGE_SCROLL_MARGIN - mouse_y) * 100 / m_zoom_level;
    } else if (mouse_y > m_wnd_h - EDGE_SCROLL_MARGIN) {
        m_cy += scroll_speed_y + (EDGE_SCROLL_MARGIN - (m_wnd_h - mouse_y)) * 100 / m_zoom_level;
    }
    __handleWndLimits ();
}

static float getHeightFromPixel (SDL_Surface* surface, int col, int row) {
    SDL_LockSurface(surface);
    Uint8* pixels = (Uint8*)surface->pixels;
    int pitch = surface->pitch;
    int bpp = surface->format->BytesPerPixel;
    Uint8* pixel = pixels + row * pitch + col * bpp;
    
    Uint32 pixel_value;
    if (bpp == 1) {
        pixel_value = *pixel;
    } else if (bpp == 2) {
        pixel_value = *(Uint16*)pixel;
    } else if (bpp == 3) {
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            pixel_value = pixel[0] << 16 | pixel[1] << 8 | pixel[2];
        } else {
            pixel_value = pixel[0] | pixel[1] << 8 | pixel[2] << 16;
        }
    } else {
        pixel_value = *(Uint32*)pixel;
    }
    Uint8 r, g, b;
    SDL_GetRGB(pixel_value, surface->format, &r, &g, &b);
    SDL_UnlockSurface(surface);
    if (b > r && b > g) {
        return 0.0f;
    }
    float gray = (r + g + b) / 3.0f;
    return gray;
}

void MapView::preRenderSetup (SDL_Surface* img_surface_h, float factor) {
    m_factor = factor;
    int num_rows = m_model->getWidth ();
    int num_cols = m_model->getHeight ();
    m_num_rows = num_rows;
    m_num_cols = num_cols;
    
    m_view_tiles = (MapViewTile**)malloc(num_rows * sizeof(MapViewTile*));
    MapViewTile* data = (MapViewTile*)malloc(num_rows * num_cols * sizeof(MapViewTile));
    for (int i = 0; i < num_rows; i++) {
        m_view_tiles[i] = data + i * num_cols;
        for (int j = 0; j < num_cols; j++) {
            m_view_tiles[i][j] = MapViewTile();
        }
    }
    
    int** z_values = new int*[num_rows];
    for (int row_idx = 0; row_idx < num_rows; row_idx++) {
        z_values[row_idx] = new int[num_cols];
        for (int col_idx = 0; col_idx < num_cols; col_idx++) {
            float height = getHeightFromPixel (img_surface_h, col_idx, row_idx);
            z_values[row_idx][col_idx] = static_cast<int>(height * factor);
        }
    }   
    m_model->setTileElevations(z_values);
    
    MapModelTile** model_tiles = m_model->getTiles();
    
    std::map<std::pair<int32_t, int32_t>, std::pair<float, int>> corner_z_sums;
    for (int row_idx = 0; row_idx < num_rows; row_idx++) {
        for (int col_idx = 0; col_idx < num_cols; col_idx++) {
            const MapModelTile& t = model_tiles[row_idx][col_idx];
            int32_t z = z_values[row_idx][col_idx];
            corner_z_sums[std::make_pair(t.top.x, t.top.y)].first += z;
            corner_z_sums[std::make_pair(t.top.x, t.top.y)].second += 1;
            corner_z_sums[std::make_pair(t.right.x, t.right.y)].first += z;
            corner_z_sums[std::make_pair(t.right.x, t.right.y)].second += 1;
            corner_z_sums[std::make_pair(t.bottom.x, t.bottom.y)].first += z;
            corner_z_sums[std::make_pair(t.bottom.x, t.bottom.y)].second += 1;
            corner_z_sums[std::make_pair(t.left.x, t.left.y)].first += z;
            corner_z_sums[std::make_pair(t.left.x, t.left.y)].second += 1;
        }
    }
    
    std::map<std::pair<int32_t, int32_t>, float> corner_elevations;
    for (auto& pair : corner_z_sums) {
        if (pair.second.second > 0) {
            corner_elevations[pair.first] = pair.second.first / pair.second.second;
        }
    }
    
    SDL_LockSurface(m_terrain_map);
    Uint8* pixels = (Uint8*)m_terrain_map->pixels;
    int pitch = m_terrain_map->pitch;
    
    for (int row_idx = 0; row_idx < num_rows; row_idx++) {
        for (int col_idx = 0; col_idx < num_cols; col_idx++) {
            MapModelTile& t = model_tiles[row_idx][col_idx];
            MapViewTile& v = m_view_tiles[row_idx][col_idx];
            
            auto it_top = corner_elevations.find(std::make_pair(t.top.x, t.top.y));
            if (it_top != corner_elevations.end()) {
                v.deltas.top = static_cast<int16_t>(-it_top->second);
            }
            auto it_right = corner_elevations.find(std::make_pair(t.right.x, t.right.y));
            if (it_right != corner_elevations.end()) {
                v.deltas.right = static_cast<int16_t>(-it_right->second);
            }
            auto it_bottom = corner_elevations.find(std::make_pair(t.bottom.x, t.bottom.y));
            if (it_bottom != corner_elevations.end()) {
                v.deltas.bottom = static_cast<int16_t>(-it_bottom->second);
            }
            auto it_left = corner_elevations.find(std::make_pair(t.left.x, t.left.y));
            if (it_left != corner_elevations.end()) {
                v.deltas.left = static_cast<int16_t>(-it_left->second);
            }
            
            int32_t y0 = t.top.y + v.deltas.top;
            int32_t y1 = t.right.y + v.deltas.right;
            int32_t y2 = t.bottom.y + v.deltas.bottom;
            int32_t y3 = t.left.y + v.deltas.left;
            v.lowest = std::min({y0, y1, y2, y3});
            v.highest = std::max({y0, y1, y2, y3});
            
            int img_y = row_idx;
            Uint8* pixel = pixels + img_y * pitch + col_idx * 4;
            Uint8 r = pixel[0];
            Uint8 g = pixel[1];
            Uint8 b = pixel[2];
            const Uint8 pixel_color[] = {r, g, b};
            
            v.flags.mountain = 0;
            if (colorsMatch(pixel_color, COLOR_MOUNTAIN)) {
                v.flags.mountain = 1;
                v.tex_read = m_tex_read_grassland;
            } else if (colorsMatch(pixel_color, COLOR_OCEAN)) {
                v.tex_read = m_tex_read_ocean;
            } else if (colorsMatch(pixel_color, COLOR_GRASSLAND)) {
                v.tex_read = m_tex_read_grassland;
            } else if (colorsMatch(pixel_color, COLOR_PLAINS)) {
                v.tex_read = m_tex_read_plains;
            } else if (colorsMatch(pixel_color, COLOR_DESERT)) {
                v.tex_read = m_tex_read_desert;
            } else if (colorsMatch(pixel_color, COLOR_TUNDRA)) {
                v.tex_read = m_tex_read_tundra;
            } else {
                v.tex_read = m_tex_read_grassland;
            }
        }
    }
    
    SDL_UnlockSurface(m_terrain_map);
    
    for (int row_idx = 0; row_idx < num_rows; row_idx++) {
        delete[] z_values[row_idx];
    }
    delete[] z_values;
}

void MapView::renderOpt () {
    SDL_SetRenderDrawColor (m_rend, U8_MAX, U8_MAX, U8_MAX, U8_MAX);
    SDL_RenderClear (m_rend);
    SDL_SetRenderDrawColor (m_rend, 0, 0, 0, U8_MAX);
    SDL_SetRenderDrawBlendMode(m_rend, SDL_BLENDMODE_BLEND);
    MapModelTile** model_tiles = m_model->getTiles();
    VisMeta meta = __getVisibilityMeta();


    __clearOldTextures(meta);
    m_prev_min_row = meta.min_row;
    m_prev_min_col = meta.min_col;
    m_prev_max_row = meta.max_row;
    m_prev_max_col = meta.max_col;

    int tex_w = m_tex_read_grassland->get_tile_w ();
    int tex_h = m_tex_read_grassland->get_tile_h ();
    int half_w = tex_w / 2;
    int half_h = tex_h / 2;
    int new_h = 0;
    tile_pts pts = {{half_w, 0}, {tex_w, half_h}, {half_w, tex_h}, {0, half_h}};
    size src_size = {tex_w, tex_h};
    pt top, right, bottom, left;
    SDL_Texture* tex = nullptr;
    for (int row_idx = meta.min_row; row_idx <= meta.max_row && row_idx < m_num_rows; row_idx++) {
        for (int col_idx = meta.min_col; col_idx <= meta.max_col && col_idx < m_num_cols; col_idx++) {
            const MapModelTile& t = model_tiles[row_idx][col_idx];
            const MapViewTile& v = m_view_tiles[row_idx][col_idx];
            deltas d = { v.deltas.top, v.deltas.right, v.deltas.bottom, v.deltas.left };
            
            if (v.tex == nullptr) {
                tex = morph_tile(m_rend, v.tex_read->get_item_rgba(row_idx, col_idx), src_size, 4, pts, d, new_h);
                m_view_tiles[row_idx][col_idx].tex = tex;
                m_view_tiles[row_idx][col_idx].prev_morph_tile_h = new_h;
            } else {
                tex = v.tex;
                new_h = v.prev_morph_tile_h;
            }
            
            SDL_Rect dst_rect = {ADJ_CX(t.left.x), ADJ_CY(v.lowest), tex_w, new_h};
            SDL_RenderCopy(m_rend, tex, nullptr, &dst_rect);
            //SDL_DestroyTexture(tex);
            top = {t.top.x, t.top.y + v.deltas.top};
            right = {t.right.x, t.right.y + v.deltas.right};
            bottom = {t.bottom.x, t.bottom.y + v.deltas.bottom};
            left = {t.left.x, t.left.y + v.deltas.left};
            SDL_RenderDrawLine (m_rend, ADJ_CX(left.x), ADJ_CY(left.y), ADJ_CX(top.x), ADJ_CY(top.y));
            SDL_RenderDrawLine (m_rend, ADJ_CX(left.x), ADJ_CY(left.y), ADJ_CX(bottom.x), ADJ_CY(bottom.y));
            SDL_RenderDrawLine (m_rend, ADJ_CX(right.x), ADJ_CY(right.y), ADJ_CX(bottom.x), ADJ_CY(bottom.y));
            SDL_RenderDrawLine (m_rend, ADJ_CX(right.x), ADJ_CY(right.y), ADJ_CX(top.x), ADJ_CY(top.y));
        }
    }
    
    int decal_size = 100;
    int decal_offset = (decal_size - m_tile_w) / 2;
    for (int row_idx = meta.min_row; row_idx <= meta.max_row && row_idx < m_num_rows; row_idx++) {
        for (int col_idx = meta.min_col; col_idx <= meta.max_col && col_idx < m_num_cols; col_idx++) {
            const MapViewTile& v = m_view_tiles[row_idx][col_idx];
            const MapModelTile& t = model_tiles[row_idx][col_idx];
            if (v.flags.mountain) {
                int left_x = ADJ_CX((t.left.x - decal_offset));
                int top_y = ADJ_CY((t.bottom.y - t.z - decal_size));
                SDL_Rect decal_rect = {left_x, top_y, decal_size, decal_size};
                SDL_Texture* decal_tex = m_decal_read_mtn->get_item_rgba(row_idx + col_idx * 10);
                SDL_RenderCopy(m_rend, decal_tex, nullptr, &decal_rect);
            }
        }
    }
    
    if (m_clicked_tile != nullptr) {
        __highlightCurrentTile(m_clicked_tile, 0, U8_MAX, 0);
        MapModelTile** tiles = m_model->getTiles();
        
        if (m_near_tiles.n.x >= 0 && m_near_tiles.n.y >= 0) {
            __highlightTile(&tiles[m_near_tiles.n.y][m_near_tiles.n.x], 0, U8_MAX, 0);
        }
        if (m_near_tiles.s.x >= 0 && m_near_tiles.s.y >= 0) {
            __highlightTile(&tiles[m_near_tiles.s.y][m_near_tiles.s.x], 0, U8_MAX, 0);
        }
        if (m_near_tiles.e.x >= 0 && m_near_tiles.e.y >= 0) {
            __highlightTile(&tiles[m_near_tiles.e.y][m_near_tiles.e.x], 0, U8_MAX, 0);
        }
        if (m_near_tiles.w.x >= 0 && m_near_tiles.w.y >= 0) {
            __highlightTile(&tiles[m_near_tiles.w.y][m_near_tiles.w.x], 0, U8_MAX, 0);
        }
        
        if (m_near_tiles.ne.x >= 0 && m_near_tiles.ne.y >= 0) {
            __highlightTile(&tiles[m_near_tiles.ne.y][m_near_tiles.ne.x], 0, static_cast<Uint8>(U8_MAX / 2), 0);
        }
        if (m_near_tiles.se.x >= 0 && m_near_tiles.se.y >= 0) {
            __highlightTile(&tiles[m_near_tiles.se.y][m_near_tiles.se.x], 0, static_cast<Uint8>(U8_MAX / 2), 0);
        }
        if (m_near_tiles.sw.x >= 0 && m_near_tiles.sw.y >= 0) {
            __highlightTile(&tiles[m_near_tiles.sw.y][m_near_tiles.sw.x], 0, static_cast<Uint8>(U8_MAX / 2), 0);
        }
        if (m_near_tiles.nw.x >= 0 && m_near_tiles.nw.y >= 0) {
            __highlightTile(&tiles[m_near_tiles.nw.y][m_near_tiles.nw.x], 0, static_cast<Uint8>(U8_MAX / 2), 0);
        }
    }
    if (m_mini) {
        int vis_w = m_wnd_w * 100 / m_zoom_level;
        int vis_h = m_wnd_h * 100 / m_zoom_level;
        m_mini->renderFast(m_cx, m_cy, vis_w, vis_h);
    }
    SDL_RenderPresent (m_rend);
}

void MapView::handleEvents () {
    SDL_Event e;
    while (SDL_PollEvent (&e)) {
        if (e.type == SDL_QUIT) {
            m_running = false;
        } else if (e.type == SDL_MOUSEMOTION) {
            __handleEventMouseMotion (e);
        } else if (e.type == SDL_KEYDOWN) {
            __handleEventsKeyDown (e);
        } else if (e.type == SDL_KEYUP) {
            __handleEventsKeyUp (e);
        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
            __handleMouseClick (e);
        }
    }
}

bool MapView::isRunning () const {
    return m_running;
}

void MapView::cleanup () {
    if (m_view_tiles) {
        if (m_num_rows > 0) {
            free(m_view_tiles[0]);
        }
        free(m_view_tiles);
        m_view_tiles = nullptr;
    }
    if (m_terrain_map != nullptr) {
        SDL_FreeSurface(m_terrain_map);
        m_terrain_map = nullptr;
    }
    if (m_mini != nullptr) {
        delete m_mini;
        m_mini = nullptr;
    }
    if (m_tex_read_ocean != nullptr) {
        delete m_tex_read_ocean;
        m_tex_read_ocean = nullptr;
    }
    if (m_tex_read_desert != nullptr) {
        delete m_tex_read_desert;
        m_tex_read_desert = nullptr;
    }
    if (m_tex_read_plains != nullptr) {
        delete m_tex_read_plains;
        m_tex_read_plains = nullptr;
    }
    if (m_tex_read_grassland != nullptr) {
        delete m_tex_read_grassland;
        m_tex_read_grassland = nullptr;
    }
    if (m_tex_read_tundra != nullptr) {
        delete m_tex_read_tundra;
        m_tex_read_tundra = nullptr;
    }
    if (m_rend != nullptr) {
        SDL_DestroyRenderer (m_rend);
        m_rend = nullptr;
    }
    if (m_wnd != nullptr) {
        SDL_DestroyWindow (m_wnd);
        m_wnd = nullptr;
    }
    SDL_Quit ();
}

//================================================================================================================================
//=> - MapView private methods -
//================================================================================================================================

void MapView::__clearOldTextures (const VisMeta &meta) {
    // Check if previous bounds are initialized (first frame check)
    if (m_prev_min_row < 0 || m_prev_max_row < 0 || 
        m_prev_min_row > m_prev_max_row ||
        m_prev_min_row >= m_num_rows || m_prev_max_row >= m_num_rows) {
        return; // First frame or invalid previous bounds, nothing to clear
    }
    for (int row = m_prev_min_row; row < meta.min_row && row < m_num_rows; row++) {
        for (int col = m_prev_min_col; col <= m_prev_max_col && col < m_num_cols; col++) {
            if (m_view_tiles[row][col].tex != nullptr) {
                SDL_DestroyTexture(m_view_tiles[row][col].tex);
                m_view_tiles[row][col].tex = nullptr;
            }
        }
    }
    for (int row = meta.max_row + 1; row <= m_prev_max_row && row < m_num_rows; row++) {
        for (int col = m_prev_min_col; col <= m_prev_max_col && col < m_num_cols; col++) {
            if (m_view_tiles[row][col].tex != nullptr) {
                SDL_DestroyTexture(m_view_tiles[row][col].tex);
                m_view_tiles[row][col].tex = nullptr;
            }
        }
    }
    for (int col = m_prev_min_col; col < meta.min_col && col < m_num_cols; col++) {
        for (int row = m_prev_min_row; row <= m_prev_max_row && row < m_num_rows; row++) {
            if (m_view_tiles[row][col].tex != nullptr) {
                SDL_DestroyTexture(m_view_tiles[row][col].tex);
                m_view_tiles[row][col].tex = nullptr;
            }
        }
    }
    for (int col = meta.max_col + 1; col <= m_prev_max_col && col < m_num_cols; col++) {
        for (int row = m_prev_min_row; row <= m_prev_max_row && row < m_num_rows; row++) {
            if (m_view_tiles[row][col].tex != nullptr) {
                SDL_DestroyTexture(m_view_tiles[row][col].tex);
                m_view_tiles[row][col].tex = nullptr;
            }
        }
    }
}

MapView::VisMeta MapView::__getVisibilityMeta () {
    VisMeta meta;
    meta.visible_h = m_wnd_h * 100 / m_zoom_level + HEIGHT_COLOR_MAX * m_factor;
    meta.visible_w = m_wnd_w * 100 / m_zoom_level;
    meta.y_min = std::max(0, m_cy - 3 * m_tile_h / 2 - int(HEIGHT_COLOR_MAX * m_factor));
    meta.y_max = std::min(m_map_h, m_cy + meta.visible_h + 3 * m_tile_h / 2);
    meta.min_col = (m_cx) / m_tile_w - 2;
    meta.max_col = (m_cx + meta.visible_w) / m_tile_w;
    meta.min_col = meta.min_col < 0 ? 0 : meta.min_col;
    meta.max_col = meta.max_col >= m_num_cols ? m_num_cols - 1 : meta.max_col;
    meta.min_row = meta.y_min / (m_tile_h/2);
    meta.max_row = meta.y_max / (m_tile_h/2);
    meta.min_row = meta.min_row < 0 ? 0 : meta.min_row;
    meta.max_row = meta.max_row >= m_num_rows ? m_num_rows - 1 : meta.max_row;
    return meta;
}

void MapView::__handleEventMouseMotion (SDL_Event &e) {
}

void MapView::__handleEventsKeyDown (SDL_Event &e) {
    switch (e.key.keysym.sym) {
        case SDLK_c:
            __centerOnTile (m_clicked_tile);
            break;
        case SDLK_f:
            __flipVertically ();
            break;
        case SDLK_PLUS:
        case SDLK_EQUALS:
            if (m_zoom_level + ZOOM_STEP <= ZOOM_MAX) {
                m_zoom_level += ZOOM_STEP;
                __handleWndLimits ();
            }
            break;
        case SDLK_MINUS:
            if (m_zoom_level - ZOOM_STEP >= ZOOM_MIN) {
                m_zoom_level -= ZOOM_STEP;
                __handleWndLimits ();
            }
            break;
        case SDLK_LEFT:
            m_scroll_add = m_scroll_add < SCROLL_MAX ? m_scroll_add + 1 : SCROLL_MAX;
            m_cx -= SCROLL_SPEED + m_scroll_add;
            __handleWndLimits ();
            break;
        case SDLK_RIGHT:
            m_scroll_add = m_scroll_add < SCROLL_MAX ? m_scroll_add + 1 : SCROLL_MAX;
            m_cx += SCROLL_SPEED + m_scroll_add;
            __handleWndLimits ();
            break;
        case SDLK_UP:
            m_scroll_add = m_scroll_add < SCROLL_MAX ? m_scroll_add + 1 : SCROLL_MAX;
            m_cy -= SCROLL_SPEED + m_scroll_add;
            __handleWndLimits ();
            break;
        case SDLK_DOWN:
            m_scroll_add = m_scroll_add < SCROLL_MAX ? m_scroll_add + 1 : SCROLL_MAX;
            m_cy += SCROLL_SPEED + m_scroll_add;
            __handleWndLimits ();
            break;
        default: break;
    }
}

void MapView::__handleEventsKeyUp (SDL_Event &e) {
    switch (e.key.keysym.sym) {
        case SDLK_LEFT:
            m_scroll_add = 0;
            break;
        case SDLK_RIGHT:
            m_scroll_add = 0;
            break;
        case SDLK_UP:
            m_scroll_add = 0;
            break;
        case SDLK_DOWN:
            m_scroll_add = 0;
            break;
        default: break;
    }
}

bool MapView::__inBounds (int x, int y, const MapModelTile &t, int row, int col) {
    const MapViewTile& v = m_view_tiles[row][col];
    int zy_top = t.top.y + v.deltas.top;
    int zy_right = t.right.y + v.deltas.right;
    int zy_bottom = t.bottom.y + v.deltas.bottom;
    int zy_left = t.left.y + v.deltas.left;
    int cross1 = (t.right.x - t.top.x) * (y - zy_top) - (zy_right - zy_top) * (x - t.top.x);
    int cross2 = (t.bottom.x - t.right.x) * (y - zy_right) - (zy_bottom - zy_right) * (x - t.right.x);
    int cross3 = (t.left.x - t.bottom.x) * (y - zy_bottom) - (zy_left - zy_bottom) * (x - t.bottom.x);
    int cross4 = (t.top.x - t.left.x) * (y - zy_left) - (zy_top - zy_left) * (x - t.left.x);
    return (cross1 > 0 && cross2 > 0 && cross3 > 0 && cross4 > 0) || (cross1 < 0 && cross2 < 0 && cross3 < 0 && cross4 < 0);
}

std::tuple<MapModelTile*, int, int> MapView::__searchTileFromPt (int map_x, int map_y, int start_row, int start_col) {
    MapModelTile** tiles = m_model->getTiles();
    std::queue<std::pair<int, int>> search_queue;
    std::set<std::pair<int, int>> visited;
    if (start_row >= 0 && start_row < m_num_rows && start_col >= 0 && start_col < m_num_cols) {
        search_queue.push(std::make_pair(start_row, start_col));
        visited.insert(std::make_pair(start_row, start_col));
    }
    int iter_budget = HEIGHT_COLOR_MAX * m_factor / m_tile_h * 4 + 1;
    iter_budget *= iter_budget;
    while (!search_queue.empty() && iter_budget-- > 0) {
        int row = search_queue.front().first;
        int col = search_queue.front().second;
        search_queue.pop();
        if (row < 0 || row >= m_num_rows || col < 0 || col >= m_num_cols) {
            continue;
        }
        const MapModelTile& tile = tiles[row][col];
        if (__inBounds(map_x, map_y, tile, row, col)) {
            return std::make_tuple(&tiles[row][col], row, col);
        }
        int offsets[][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
        for (int i = 0; i < 8; i++) {
            int new_row = row + offsets[i][0];
            int new_col = col + offsets[i][1];
            if (new_row >= 0 && new_row < m_num_rows && new_col >= 0 && new_col < m_num_cols) {
                std::pair<int, int> key = std::make_pair(new_row, new_col);
                if (visited.find(key) == visited.end()) {
                    search_queue.push(key);
                    visited.insert(key);
                }
            }
        }
    }
    return std::make_tuple(nullptr, -1, -1);
}

void MapView::__handleMouseClick (SDL_Event &e) {
    switch (e.button.button) {
        case SDL_BUTTON_LEFT: {
            int screen_x = e.button.x;
            int screen_y = e.button.y;
            Point scr_pt(screen_x, screen_y);
            if (m_mini && m_mini->isPointOnMinimap(scr_pt)) {
                Point map_pt;
                m_mini->minimapToMap(scr_pt, map_pt);
                int vis_w = m_wnd_w * 100 / m_zoom_level;
                int vis_h = m_wnd_h * 100 / m_zoom_level;
                map_pt.x = int(map_pt.x * m_tile_w * 100 / m_zoom_level);
                map_pt.y = int(map_pt.y * m_tile_h / 2 * 100 / m_zoom_level);
                m_cx = std::max(0, std::min(m_map_w - vis_w, map_pt.x - vis_w / 2));
                m_cy = std::max(0, std::min(m_map_h - vis_h, map_pt.y - vis_h / 2));
                __handleWndLimits();
                return;
            }
            int map_x = (screen_x * 100 / m_zoom_level) + m_cx;
            int map_y = (screen_y * 100 / m_zoom_level) + m_cy;
            MapModelTile** tiles = m_model->getTiles();
            int first_row_y = tiles[0][0].top.y;
            int first_col_x = tiles[0][0].left.x;
            int half_h = m_tile_h / 2;
            int start_row = std::max(0, (map_y - first_row_y) / half_h);
            int start_col = std::max(0, (map_x - first_col_x) / m_tile_w);
            
            auto result = __searchTileFromPt(map_x, map_y, start_row, start_col);
            MapModelTile *tile = std::get<0>(result);
            int row = std::get<1>(result);
            int col = std::get<2>(result);
            if (tile != nullptr) {
                __clearHighlights ();
                m_clicked_tile = tile;
                m_near_tiles = m_model->m_tiler->getNearTiles(row, col);
            }
            break;
        }
        default: break;
    }
}

void MapView::__highlightTile (MapModelTile *tile, Uint8 r, Uint8 g, Uint8 b) {
    MapModelTile** model_tiles = m_model->getTiles();
    int tile_row = -1, tile_col = -1;
    for (int i = 0; i < m_num_rows && tile_row == -1; i++) {
        for (int j = 0; j < m_num_cols; j++) {
            if (&model_tiles[i][j] == tile) {
                tile_row = i;
                tile_col = j;
                break;
            }
        }
    }
    const MapViewTile& v = m_view_tiles[tile_row][tile_col];
    int center_x = (tile->top.x + tile->right.x + tile->bottom.x + tile->left.x) / 4;
    int center_y = (get_zy_top(*tile, v) + get_zy_right(*tile, v) + get_zy_bottom(*tile, v) + get_zy_left(*tile, v)) / 4;
    SDL_SetRenderDrawColor (m_rend, r, g, b, U8_MAX);
    int dot_size = 5;
    for (int dy = -dot_size/2; dy <= dot_size/2; dy++) {
        for (int dx = -dot_size/2; dx <= dot_size/2; dx++) {
            if (dx*dx + dy*dy <= (dot_size/2)*(dot_size/2)) {
                SDL_RenderDrawPoint(m_rend, ADJ_CX(center_x + dx), ADJ_CY(center_y + dy));
            }
        }
    }
}

void MapView::__highlightCurrentTile (MapModelTile *tile, Uint8 r, Uint8 g, Uint8 b) {
    MapModelTile** model_tiles = m_model->getTiles();
    int tile_row = -1, tile_col = -1;
    for (int i = 0; i < m_num_rows && tile_row == -1; i++) {
        for (int j = 0; j < m_num_cols; j++) {
            if (&model_tiles[i][j] == tile) {
                tile_row = i;
                tile_col = j;
                break;
            }
        }
    }
    const MapViewTile& v = m_view_tiles[tile_row][tile_col];
    SDL_SetRenderDrawColor (m_rend, r, g, b, U8_MAX);
    int thickness = 3;
    for (int offset = -thickness/2; offset <= thickness/2; offset++) {
        int x1 = ADJ_CX(tile->top.x);
        int y1 = ADJ_CY(get_zy_top(*tile, v));
        int x2 = ADJ_CX(tile->right.x);
        int y2 = ADJ_CY(get_zy_right(*tile, v));
        int dx = x2 - x1;
        int dy = y2 - y1;
        float len = sqrtf(dx*dx + dy*dy);
        if (len > 0) {
            int perp_x = -dy * offset / len;
            int perp_y = dx * offset / len;
            SDL_RenderDrawLine(m_rend, x1 + perp_x, y1 + perp_y, x2 + perp_x, y2 + perp_y);
        }
        x1 = ADJ_CX(tile->right.x);
        y1 = ADJ_CY(get_zy_right(*tile, v));
        x2 = ADJ_CX(tile->bottom.x);
        y2 = ADJ_CY(get_zy_bottom(*tile, v));
        dx = x2 - x1;
        dy = y2 - y1;
        len = sqrtf(dx*dx + dy*dy);
        if (len > 0) {
            int perp_x = -dy * offset / len;
            int perp_y = dx * offset / len;
            SDL_RenderDrawLine(m_rend, x1 + perp_x, y1 + perp_y, x2 + perp_x, y2 + perp_y);
        }
        x1 = ADJ_CX(tile->bottom.x);
        y1 = ADJ_CY(get_zy_bottom(*tile, v));
        x2 = ADJ_CX(tile->left.x);
        y2 = ADJ_CY(get_zy_left(*tile, v));
        dx = x2 - x1;
        dy = y2 - y1;
        len = sqrtf(dx*dx + dy*dy);
        if (len > 0) {
            int perp_x = -dy * offset / len;
            int perp_y = dx * offset / len;
            SDL_RenderDrawLine(m_rend, x1 + perp_x, y1 + perp_y, x2 + perp_x, y2 + perp_y);
        }
        x1 = ADJ_CX(tile->left.x);
        y1 = ADJ_CY(get_zy_left(*tile, v));
        x2 = ADJ_CX(tile->top.x);
        y2 = ADJ_CY(get_zy_top(*tile, v));
        dx = x2 - x1;
        dy = y2 - y1;
        len = sqrtf(dx*dx + dy*dy);
        if (len > 0) {
            int perp_x = -dy * offset / len;
            int perp_y = dx * offset / len;
            SDL_RenderDrawLine(m_rend, x1 + perp_x, y1 + perp_y, x2 + perp_x, y2 + perp_y);
        }
    }
}

void MapView::__clearHighlights () {
    m_clicked_tile = nullptr;
    m_near_tiles = NearTiles();
}

void MapView::__centerOnTile (MapModelTile *tile) {
    int center_x = (tile->top.x + tile->right.x + tile->bottom.x + tile->left.x) / 4;
    int center_y = (tile->top.y + tile->right.y + tile->bottom.y + tile->left.y) / 4;
    m_cx = center_x - m_wnd_w / 2;
    m_cy = center_y - m_wnd_h / 2;
    
    m_cx = m_cx < 0 ? 0 : m_cx;
    m_cx = m_cx > m_map_w - m_wnd_w ? m_map_w - m_wnd_w : m_cx;
    m_cy = m_cy < 0 ? 0 : m_cy;
    m_cy = m_cy > m_map_h - m_wnd_h ? m_map_h - m_wnd_h : m_cy;
}

void MapView::__toggleFullscreen () {
    Uint32 flags = SDL_GetWindowFlags (m_wnd);
    if (flags & SDL_WINDOW_FULLSCREEN) { 
        SDL_SetWindowFullscreen (m_wnd, 0);
        m_wnd_w = m_wnd_w_orig;
        m_wnd_h = m_wnd_h_orig;
    } else {
        SDL_SetWindowFullscreen (m_wnd, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_GetWindowSize (m_wnd, &m_wnd_w, &m_wnd_h);
    }
}

void MapView::__flipVertically () {
    return; // Short circuit for now
    MapModelTile** tiles = m_model->getTiles();
    int min_y = INT_MAX;
    int max_y = INT_MIN;
     
    // Find the min and max y values
    for (int row = 0; row < m_num_rows; row++) {
        for (int col = 0; col < m_num_cols; col++) {
            MapModelTile& t = tiles[row][col];
            if (t.top.y < min_y) min_y = t.top.y;
            if (t.top.y > max_y) max_y = t.top.y;
            if (t.right.y < min_y) min_y = t.right.y;
            if (t.right.y > max_y) max_y = t.right.y;
            if (t.bottom.y < min_y) min_y = t.bottom.y;
            if (t.bottom.y > max_y) max_y = t.bottom.y;
            if (t.left.y < min_y) min_y = t.left.y;
            if (t.left.y > max_y) max_y = t.left.y;
        }
    }
    
    int center_y = (min_y + max_y) / 2;
    
    // Flip all y coordinates around the center
    for (int row = 0; row < m_num_rows; row++) {
        for (int col = 0; col < m_num_cols; col++) {
            MapModelTile& t = tiles[row][col];
            t.top.y = center_y - (t.top.y - center_y);
            t.right.y = center_y - (t.right.y - center_y);
            t.bottom.y = center_y - (t.bottom.y - center_y);
            t.left.y = center_y - (t.left.y - center_y);
        }
    }
}

void MapView::__handleWndLimits () {
    int visible_w = m_wnd_w * 100 / m_zoom_level;
    int visible_h = m_wnd_h * 100 / m_zoom_level;
    int max_cx = m_map_w - visible_w;
    int max_cy = m_map_h - visible_h;
    
    max_cx = max_cx < 0 ? 0 : max_cx;
    max_cy = max_cy < 0 ? 0 : max_cy;
    
    m_cx = m_cx < 0 ? 0 : m_cx;
    m_cx = m_cx > max_cx ? max_cx : m_cx;
    m_cy = m_cy < 0 ? 0 : m_cy;
    m_cy = m_cy > max_cy ? max_cy : m_cy;
}


//================================================================================================================================
//=> - MapView test functions -
//================================================================================================================================

int MapView::testTileClickDetection () {
    MapModelTile** tiles = m_model->getTiles();
    int tests_run = 0, failures = 0;
    for (int row = 0; row < m_num_rows; row++) {
        for (int col = 0; col < m_num_cols; col++) {
            MapModelTile& expected_tile = tiles[row][col];
            int center_x = (expected_tile.left.x + expected_tile.right.x) / 2;
            int center_y = (expected_tile.top.y + expected_tile.bottom.y) / 2;
            int screen_x = (center_x - m_cx) * m_zoom_level / 100;
            int screen_y = (center_y - m_cy) * m_zoom_level / 100;
            SDL_Event fake_event;
            fake_event.type = SDL_MOUSEBUTTONDOWN;
            fake_event.button.button = SDL_BUTTON_LEFT;
            fake_event.button.x = screen_x;
            fake_event.button.y = screen_y;
            m_clicked_tile = nullptr;
            __clearHighlights();
            __handleMouseClick(fake_event);
            tests_run++;
            if (m_clicked_tile == nullptr || m_clicked_tile != &expected_tile) {
                std::cout << "Failure on row/col " << row << "/" << col << std::endl;
                failures++;
                std::cout << expected_tile.top.x << " "<< expected_tile.top.y << std::endl;
                std::cout << expected_tile.right.x << " "<< expected_tile.right.y << std::endl;
                std::cout << expected_tile.bottom.x << " "<< expected_tile.bottom.y << std::endl;
                std::cout << expected_tile.left.x << " "<< expected_tile.left.y << std::endl;

                if (failures > 10) {
                    break;
                }
            }
        }
        if (failures > 10) {
            break;
        }
    }
    std::cout << "Tile click detection test outcome: " << tests_run - failures << "/" << tests_run << std::endl;
    return failures;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
