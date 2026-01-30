//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <algorithm>

#include "map_mini.h"

//================================================================================================================================
//=> - MapMini public methods -
//================================================================================================================================

MapMini::MapMini (SDL_Renderer* rend, const char* img_path, Size mini_size, Size canvas_size, Size map_size) : 
    m_rend(rend), 
    m_map_tex(nullptr), 
    m_cached_tex(nullptr),
    m_mini_size(mini_size), 
    m_canvas_size(canvas_size),
    m_map_size(map_size), 
    m_mini_pos(0, 0), 
    m_img_pos(0, 0), 
    m_img_size(0, 0), 
    m_scale(0.0f),
    m_box_rect(0, 0, 0, 0) {
    SDL_Surface* surf = IMG_Load(img_path);
    m_map_tex = SDL_CreateTextureFromSurface(m_rend, surf);
    SDL_FreeSurface(surf);
    m_mini_pos.x = 0;
    m_mini_pos.y = m_canvas_size.height - m_mini_size.height;
    float scale_w = (float)m_mini_size.width / (float)m_map_size.width;
    float scale_h = (float)m_mini_size.height / (float)m_map_size.height;
    m_scale = std::min(scale_w, scale_h);
    m_img_size.width = (int)(m_map_size.width * m_scale);
    m_img_size.height = (int)(m_map_size.height * m_scale);
    m_img_pos.x = m_mini_pos.x + (m_mini_size.width - m_img_size.width) / 2;
    m_img_pos.y = m_mini_pos.y + (m_mini_size.height - m_img_size.height) / 2;
    __buildCache();
}

MapMini::~MapMini () {
    if (m_cached_tex) SDL_DestroyTexture(m_cached_tex);
    if (m_map_tex) SDL_DestroyTexture(m_map_tex);
}

void MapMini::__buildCache () {
    int cache_w = m_mini_size.width;
    int cache_h = m_mini_size.height;
    m_cached_tex = SDL_CreateTexture(m_rend, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, cache_w, cache_h);
    SDL_SetTextureBlendMode(m_cached_tex, SDL_BLENDMODE_BLEND);
    SDL_Texture* prev_target = SDL_GetRenderTarget(m_rend);
    SDL_SetRenderTarget(m_rend, m_cached_tex);
    SDL_SetRenderDrawBlendMode(m_rend, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_rend, 0, 0, 0, 128);
    SDL_RenderFillRect(m_rend, nullptr);
    int dst_x = (m_mini_size.width - m_img_size.width) / 2;
    int dst_y = (m_mini_size.height - m_img_size.height) / 2;
    SDL_Rect src = {0, 0, m_map_size.width, m_map_size.height};
    SDL_Rect dst = {dst_x, dst_y, m_img_size.width, m_img_size.height};
    SDL_RenderCopy(m_rend, m_map_tex, &src, &dst);
    SDL_SetRenderTarget(m_rend, prev_target);
}

void MapMini::render (int cam_x, int cam_y, int vis_w, int vis_h) {
    if (!m_map_tex) return;
    SDL_Rect bg = {m_mini_pos.x, m_mini_pos.y, m_mini_size.width, m_mini_size.height};
    SDL_SetRenderDrawBlendMode(m_rend, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_rend, 0, 0, 0, 128);
    SDL_RenderFillRect(m_rend, &bg);
    SDL_Rect src = {0, 0, m_map_size.width, m_map_size.height};
    SDL_Rect dst = {m_img_pos.x, m_img_pos.y, m_img_size.width, m_img_size.height};
    SDL_RenderCopy(m_rend, m_map_tex, &src, &dst);
    int vp_x = m_img_pos.x + (int)(cam_x * m_scale);
    int vp_y = m_img_pos.y + (int)(cam_y * m_scale);
    int vp_w = std::min(m_img_size.width, (int)(vis_w * m_scale));
    int vp_h = std::min(m_img_size.height, (int)(vis_h * m_scale));
    vp_x = std::max(m_img_pos.x, std::min(m_img_pos.x + m_img_size.width - vp_w, vp_x));
    vp_y = std::max(m_img_pos.y, std::min(m_img_pos.y + m_img_size.height - vp_h, vp_y));
    SDL_Rect vp = {vp_x, vp_y, vp_w, vp_h};
    SDL_SetRenderDrawColor(m_rend, 0, 0, 0, 255);
    SDL_SetRenderDrawBlendMode(m_rend, SDL_BLENDMODE_NONE);
    SDL_RenderDrawRect(m_rend, &vp);
    if (m_box_rect.width > 0 && m_box_rect.height > 0) {
        int box_x = m_img_pos.x + (int)(m_box_rect.x * m_scale);
        int box_y = m_img_pos.y + (int)(m_box_rect.y * m_scale);
        int box_w = (int)(m_box_rect.width * m_scale);
        int box_h = (int)(m_box_rect.height * m_scale);
        box_x = std::max(m_img_pos.x, std::min(m_img_pos.x + m_img_size.width - 1, box_x));
        box_y = std::max(m_img_pos.y, std::min(m_img_pos.y + m_img_size.height - 1, box_y));
        box_w = std::max(1, std::min(m_img_size.width - (box_x - m_img_pos.x), box_w));
        box_h = std::max(1, std::min(m_img_size.height - (box_y - m_img_pos.y), box_h));
        SDL_Rect box = {box_x, box_y, box_w, box_h};
        SDL_SetRenderDrawColor(m_rend, 0, 0, 0, 255);
        SDL_RenderDrawRect(m_rend, &box);
    }
}

void MapMini::renderFast (int cam_x, int cam_y, int vis_w, int vis_h) {
    if (!m_cached_tex) return;
    SDL_Rect dst = {m_mini_pos.x, m_mini_pos.y, m_mini_size.width, m_mini_size.height};
    SDL_RenderCopy(m_rend, m_cached_tex, nullptr, &dst);
    int vp_x = m_img_pos.x + (int)(cam_x * m_scale);
    int vp_y = m_img_pos.y + (int)(cam_y * m_scale);
    int vp_w = std::min(m_img_size.width, (int)(vis_w * m_scale));
    int vp_h = std::min(m_img_size.height, (int)(vis_h * m_scale));
    vp_x = std::max(m_img_pos.x, std::min(m_img_pos.x + m_img_size.width - vp_w, vp_x));
    vp_y = std::max(m_img_pos.y, std::min(m_img_pos.y + m_img_size.height - vp_h, vp_y));
    SDL_Rect vp = {vp_x, vp_y, vp_w, vp_h};
    SDL_SetRenderDrawColor(m_rend, 0, 0, 0, 255);
    SDL_SetRenderDrawBlendMode(m_rend, SDL_BLENDMODE_NONE);
    SDL_RenderDrawRect(m_rend, &vp);
    if (m_box_rect.width > 0 && m_box_rect.height > 0) {
        int box_x = m_img_pos.x + (int)(m_box_rect.x * m_scale);
        int box_y = m_img_pos.y + (int)(m_box_rect.y * m_scale);
        int box_w = (int)(m_box_rect.width * m_scale);
        int box_h = (int)(m_box_rect.height * m_scale);
        box_x = std::max(m_img_pos.x, std::min(m_img_pos.x + m_img_size.width - 1, box_x));
        box_y = std::max(m_img_pos.y, std::min(m_img_pos.y + m_img_size.height - 1, box_y));
        box_w = std::max(1, std::min(m_img_size.width - (box_x - m_img_pos.x), box_w));
        box_h = std::max(1, std::min(m_img_size.height - (box_y - m_img_pos.y), box_h));
        SDL_Rect box = {box_x, box_y, box_w, box_h};
        SDL_SetRenderDrawColor(m_rend, 0, 0, 0, 255);
        SDL_RenderDrawRect(m_rend, &box);
    }
}

bool MapMini::isPointOnMinimap (Point scr_pt) const {
    if (scr_pt.x < m_mini_pos.x || scr_pt.x >= m_mini_pos.x + m_mini_size.width) {
        return false;
    }
    if (scr_pt.y < m_mini_pos.y || scr_pt.y >= m_mini_pos.y + m_mini_size.height) {
        return false;
    }
    return true;
}

void MapMini::minimapToMap (Point mini_pt, Point& map_pt) const {
    int rx = mini_pt.x - m_img_pos.x;
    int ry = mini_pt.y - m_img_pos.y;
    rx = std::max(0, std::min(m_img_size.width - 1, rx));
    ry = std::max(0, std::min(m_img_size.height - 1, ry));
    map_pt.x = (int)(rx / m_scale);
    map_pt.y = (int)(ry / m_scale);
    map_pt.x = std::max(0, std::min(m_map_size.width - 1, map_pt.x));
    map_pt.y = std::max(0, std::min(m_map_size.height - 1, map_pt.y));
}

void MapMini::setBox (Rect map_rect) {
    m_box_rect = map_rect;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
