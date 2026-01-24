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

#include "map_view.h"
#include "map_tiler.h"

#define ADJ_CX(x) ((x - m_cx) * m_zoom_level / 100)
#define ADJ_CY(y) ((y - m_cy) * m_zoom_level / 100)
#define HEIGHT_COLOR_MAX 255
#define U8_MAX 255

//================================================================================================================================
//=> - MapView public methods -
//================================================================================================================================

MapView::MapView (int wnd_width, int wnd_height, int map_width, int map_height, int tile_width, int tile_height, MapModel *model) :
    m_wnd (nullptr),
    m_rend (nullptr),
    m_model (model),
    m_wnd_w (wnd_width),
    m_wnd_h (wnd_height),
    m_wnd_w_orig (wnd_width),
    m_wnd_h_orig (wnd_height),
    m_map_w (map_width),
    m_map_h (map_height),
    m_tile_w (tile_width),
    m_tile_h (tile_height),
    m_running (false),
    m_cx (0),
    m_cy (0),
    m_scroll_add (0),
    m_clicked_tile (nullptr),
    m_zoom_level (ZOOM_DEFAULT) {
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
    return true;
}

void MapView::render_old () {
    SDL_SetRenderDrawColor (m_rend, U8_MAX, U8_MAX, U8_MAX, U8_MAX);
    SDL_RenderClear (m_rend);
    SDL_SetRenderDrawColor (m_rend, 0, 0, 0, U8_MAX);
    std::vector<std::vector<MapTile>> tiles = m_model->getTiles ();
    for (const auto &row : tiles) {
        for (const auto &t : row) {
            for (int i = 0; i < 4; i++) {
                int j = (i + 1) % 4;
                SDL_RenderDrawLine (m_rend, ADJ_CX(t.pts[i].x), ADJ_CY(t.pts[i].y), ADJ_CX(t.pts[j].x), ADJ_CY(t.pts[j].y));
            }
        }
    }
    SDL_RenderPresent (m_rend);
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
    if (surface == nullptr || col < 0 || row < 0 || col >= surface->w || row >= surface->h) {
        return 0.0f;
    }
    
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

void MapView::render_opt_pre (SDL_Surface* img_surface_height, float factor) {
    m_factor = factor;
    int num_rows = m_model->getWidth();
    int num_cols = m_model->getHeight();
    int** z_values = new int*[num_rows];
    for (int row_idx = 0; row_idx < num_rows; row_idx++) {
        z_values[row_idx] = new int[num_cols];
        for (int col_idx = 0; col_idx < num_cols; col_idx++) {
            float height = getHeightFromPixel(img_surface_height, col_idx, row_idx);
            z_values[row_idx][col_idx] = static_cast<int>(height * factor);
        }
    }   
    m_model->setTileElevations(z_values);
    for (int row_idx = 0; row_idx < num_rows; row_idx++) {
        delete[] z_values[row_idx];
    }
    delete[] z_values;
}

void MapView::render_opt () {
    SDL_SetRenderDrawColor (m_rend, U8_MAX, U8_MAX, U8_MAX, U8_MAX);
    SDL_RenderClear (m_rend);
    SDL_SetRenderDrawColor (m_rend, 0, 0, 0, U8_MAX);
    std::vector<std::vector<MapTile>>& tiles = m_model->getTilesRef ();
    int visible_h = m_wnd_h * 100 / m_zoom_level + HEIGHT_COLOR_MAX * m_factor;
    int visible_w = m_wnd_w * 100 / m_zoom_level;
    int half_h = m_tile_h / 2;
    int y_min = std::max(0, m_cy - 3 * half_h - int(HEIGHT_COLOR_MAX * m_factor));
    int y_max = std::min(m_map_h, m_cy + visible_h + 3 * half_h);
    
    size_t min_col = (m_cx) / m_tile_w;
    size_t max_col = (m_cx + visible_w) / m_tile_w;
    min_col = min_col < 0 ? 0 : min_col;
    max_col = max_col >= tiles[0].size() ? tiles[0].size() - 1 : max_col;

    size_t min_row = y_min / (m_tile_h/2);
    size_t max_row = y_max / (m_tile_h/2);
    min_row = min_row < 0 ? 0 : min_row;
    max_row = max_row >= tiles.size() ? tiles.size() - 1 : max_row;

    for (size_t row_idx = min_row; row_idx <= max_row && row_idx < tiles.size(); row_idx++) {
        for (size_t col_idx = min_col; col_idx <= max_col && col_idx < tiles[row_idx].size(); col_idx++) {
            const MapTile& t = tiles[row_idx][col_idx];
            SDL_RenderDrawLine (m_rend, ADJ_CX(t.pts[3].x), ADJ_CY(t.pts[3].y), ADJ_CX(t.pts[0].x), ADJ_CY(t.pts[0].y));
            SDL_RenderDrawLine (m_rend, ADJ_CX(t.pts[2].x), ADJ_CY(t.pts[2].y), ADJ_CX(t.pts[3].x), ADJ_CY(t.pts[3].y));
        }
    }
    if (min_row == 0) {
        for (size_t col_idx = 0; col_idx < tiles[0].size(); col_idx++) {
            const MapTile& t1 = tiles[0][col_idx];
            SDL_RenderDrawLine (m_rend, ADJ_CX(t1.pts[0].x), ADJ_CY(t1.pts[0].y), ADJ_CX(t1.pts[1].x), ADJ_CY(t1.pts[1].y));
        }
    }
    if (max_row == tiles.size() - 1) {
        for (size_t col_idx = 0; col_idx < tiles[tiles.size() - 1].size(); col_idx++) {
            const MapTile& t2 = tiles[tiles.size() - 1][col_idx];
            SDL_RenderDrawLine (m_rend, ADJ_CX(t2.pts[1].x), ADJ_CY(t2.pts[1].y), ADJ_CX(t2.pts[2].x), ADJ_CY(t2.pts[2].y));
        }
    }
    if (max_col == tiles[0].size() - 1) {
        for (size_t row_idx = min_row; row_idx <= max_row && row_idx < tiles.size(); row_idx++) {
            if (max_col < tiles[row_idx].size()) {
                const MapTile& t = tiles[row_idx][max_col];
                SDL_RenderDrawLine (m_rend, ADJ_CX(t.pts[0].x), ADJ_CY(t.pts[0].y), ADJ_CX(t.pts[1].x), ADJ_CY(t.pts[1].y));
                SDL_RenderDrawLine (m_rend, ADJ_CX(t.pts[1].x), ADJ_CY(t.pts[1].y), ADJ_CX(t.pts[2].x), ADJ_CY(t.pts[2].y));
            }
        }
    }
    
    // Draw highlights on top
    if (m_clicked_tile != nullptr) {
        __highlightCurrentTile(m_clicked_tile, 0, U8_MAX, 0);
        for (const auto &near_tile : m_near_tiles) {
            __highlightTile(near_tile, 0, U8_MAX, 0);
        }
        for (const auto &diagonal_tile : m_diagonal_tiles) {
            __highlightTile(diagonal_tile, 0, static_cast<Uint8>(U8_MAX / 2), 0);
        }
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

bool MapView::__inBounds (int x, int y, const MapTile &tile) {
    bool all_negative = true, all_positive = true;
    for (int i = 0; i < 4; i++) {
        int j = (i + 1) % 4;
        int edge_x = tile.pts[j].x - tile.pts[i].x;
        int edge_y = tile.pts[j].y - tile.pts[i].y;
        int to_point_x = x - tile.pts[i].x;
        int to_point_y = y - tile.pts[i].y;
        int cross = edge_x * to_point_y - edge_y * to_point_x;
        all_negative = cross > 0 ? all_negative : false;
        all_positive = cross < 0 ? all_positive : false;
    }
    return all_negative || all_positive;
}

std::tuple<MapTile*, size_t, size_t> MapView::__searchTileFromPt (int map_x, int map_y, size_t start_row, size_t start_col) {
    std::vector<std::vector<MapTile>>& tiles = m_model->getTilesRef();
    if (tiles.size() == 0 || tiles[0].size() == 0) {
        return std::make_tuple(nullptr, 0, 0);
    }
    std::queue<std::pair<size_t, size_t>> search_queue;
    std::set<std::pair<size_t, size_t>> visited;
    if (start_row < tiles.size() && start_col < tiles[start_row].size()) {
        search_queue.push(std::make_pair(start_row, start_col));
        visited.insert(std::make_pair(start_row, start_col));
    }
    int iteration_budget = HEIGHT_COLOR_MAX * m_factor / m_tile_h * 4 + 1; // A tile could be of beacuse of the elevation factor
    iteration_budget *= iteration_budget;
    while (!search_queue.empty() && iteration_budget-- > 0) {
        size_t row = search_queue.front().first;
        size_t col = search_queue.front().second;
        search_queue.pop();
        if (row >= tiles.size() || col >= tiles[row].size()) {
            continue;
        }
        const MapTile& tile = tiles[row][col];
        if (__inBounds(map_x, map_y, tile)) {
            return std::make_tuple(&tiles[row][col], row, col);
        }
        int offsets[][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
        for (int i = 0; i < 8; i++) {
            size_t new_row = row + offsets[i][0];
            size_t new_col = col + offsets[i][1];
            std::pair<size_t, size_t> key = std::make_pair(new_row, new_col);
            if (visited.find(key) == visited.end() && 
                new_row < tiles.size() && 
                new_col < tiles[new_row].size()) {
                search_queue.push(key);
                visited.insert(key);
            }
        }
    }
    return std::make_tuple(nullptr, 0, 0);
}

void MapView::__handleMouseClick (SDL_Event &e) {
    switch (e.button.button) {
        case SDL_BUTTON_LEFT: {
            int screen_x = e.button.x;
            int screen_y = e.button.y;
            int map_x = (screen_x * 100 / m_zoom_level) + m_cx;
            int map_y = (screen_y * 100 / m_zoom_level) + m_cy;
            std::vector<std::vector<MapTile>>& tiles = m_model->getTilesRef();
            if (tiles.size() > 0 && tiles[0].size() > 0) {
                int first_row_y = tiles[0][0].pts[0].y;
                int first_col_x = tiles[0][0].pts[3].x;
                int half_h = m_tile_h / 2;
                size_t start_row = std::max(0, (map_y - first_row_y) / half_h);
                size_t start_col = std::max(0, (map_x - first_col_x) / m_tile_w);
                
                auto result = __searchTileFromPt(map_x, map_y, start_row, start_col);
                MapTile *tile = std::get<0>(result);
                size_t row = std::get<1>(result);
                size_t col = std::get<2>(result);
                if (tile != nullptr) {
                    __clearHighlights ();
                    m_clicked_tile = tile;
                    m_near_tiles = m_model->m_tiler->tileIdxToNearTiles(row, col);
                    m_diagonal_tiles = m_model->m_tiler->tileIdxToDiagonalTiles(row, col);
                }
            }
            break;
        }
        default: break;
    }
}

void MapView::__highlightTile (MapTile *tile, Uint8 r, Uint8 g, Uint8 b) {
    if (tile == nullptr) {
        return;
    }
    int center_x = 0;
    int center_y = 0;
    for (int i = 0; i < 4; i++) {
        center_x += tile->pts[i].x;
        center_y += tile->pts[i].y;
    }
    center_x /= 4;
    center_y /= 4;
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

void MapView::__highlightCurrentTile (MapTile *tile, Uint8 r, Uint8 g, Uint8 b) {
    if (tile == nullptr) {
        return;
    }
    SDL_SetRenderDrawColor (m_rend, r, g, b, U8_MAX);
    int thickness = 3;
    for (int offset = -thickness/2; offset <= thickness/2; offset++) {
        for (int i = 0; i < 4; i++) {
            int j = (i + 1) % 4;
            int x1 = ADJ_CX(tile->pts[i].x);
            int y1 = ADJ_CY(tile->pts[i].y);
            int x2 = ADJ_CX(tile->pts[j].x);
            int y2 = ADJ_CY(tile->pts[j].y);
            int dx = x2 - x1;
            int dy = y2 - y1;
            float len = sqrtf(dx*dx + dy*dy);
            if (len > 0) {
                int perp_x = -dy * offset / len;
                int perp_y = dx * offset / len;
                SDL_RenderDrawLine(m_rend, x1 + perp_x, y1 + perp_y, x2 + perp_x, y2 + perp_y);
            }
        }
    }
}

void MapView::__clearHighlights () {
    m_clicked_tile = nullptr;
    m_near_tiles.clear ();
    m_diagonal_tiles.clear ();
}

void MapView::__centerOnTile (MapTile *tile) {
    if (tile == nullptr) {
        return;
    }
    int center_x = 0;
    int center_y = 0;
    for (int i = 0; i < 4; i++) {
        center_x += tile->pts[i].x;
        center_y += tile->pts[i].y;
    }
    center_x /= 4;
    center_y /= 4;
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
    std::vector<std::vector<MapTile>>& tiles = m_model->getTilesRef();
    int min_y = INT_MAX;
    int max_y = INT_MIN;
     
    // Find the min and max y values
    for (size_t row = 0; row < tiles.size(); row++) {
        for (size_t col = 0; col < tiles[row].size(); col++) {
            for (int i = 0; i < 4; i++) {
                int y = tiles[row][col].pts[i].y;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
            }
        }
    }
    
    int center_y = (min_y + max_y) / 2;
    
    // Flip all y coordinates around the center
    for (size_t row = 0; row < tiles.size(); row++) {
        for (size_t col = 0; col < tiles[row].size(); col++) {
            for (int i = 0; i < 4; i++) {
                tiles[row][col].pts[i].y = center_y - (tiles[row][col].pts[i].y - center_y);
            }
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
    std::vector<std::vector<MapTile>>& tiles = m_model->getTilesRef();
    int tests_run = 0, failures = 0;
    for (size_t row = 0; row < tiles.size(); row++) {
        for (size_t col = 0; col < tiles[row].size(); col++) {
            MapTile& expected_tile = tiles[row][col];
            int center_x = (expected_tile.pts[3].x + expected_tile.pts[1].x) / 2;
            int center_y = (expected_tile.pts[0].y + expected_tile.pts[2].y) / 2;
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
                std::cout << expected_tile.pts[0].x << " "<< expected_tile.pts[0].y << std::endl;
                std::cout << expected_tile.pts[1].x << " "<< expected_tile.pts[1].y << std::endl;
                std::cout << expected_tile.pts[2].x << " "<< expected_tile.pts[2].y << std::endl;
                std::cout << expected_tile.pts[3].x << " "<< expected_tile.pts[3].y << std::endl;

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
