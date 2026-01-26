//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <cstdlib>
#include <ctime>
#include "img_mng.h"
#include "get_sectors.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

inline bool colors_match (u8* a, u8* b, int ch) {
    for (int i = 0; i < ch; i++) if (a[i] != b[i]) return false;
    return true;
}

//================================================================================================================================
//=> - Main public functions -
//================================================================================================================================

extern "C" {

struct QueueItem {
    int dsq;
    int x, y;
    bool operator<(const QueueItem& o) const {
        return dsq < o.dsq;
    }
};

struct Sector {
    int id;
    int cx, cy;
    std::vector<std::pair<int, int>> coords;
    u8 clr[3];
    std::vector<int> adj;
    std::vector<int> conn;
    bool coastal;
    int river_id;
    Sector(int i) : id(i), coastal(false), river_id(-1) {
        clr[0] = clr[1] = clr[2] = 0;
    }
};

void draw_thick_line(u8* img, int w, int h, int ch, int x1, int y1, int x2, int y2, u8* clr, int thick) {
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    int x = x1, y = y1;
    while (true) {
        for (int ty = -thick; ty <= thick; ty++) {
            for (int tx = -thick; tx <= thick; tx++) {
                if (tx * tx + ty * ty <= thick * thick) {
                    int px = x + tx;
                    int py = y + ty;
                    if (px >= 0 && px < w && py >= 0 && py < h) {
                        int pos = (py * w + px) * ch;
                        std::memcpy(&img[pos], clr, ch * sizeof(u8));
                    }
                }
            }
        }
        if (x == x2 && y == y2) {
            break;
        }
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

void gen_sect (u8* img_in, size img_size, int ch, u8* color_ocean, u8* color_mtn, int dist, u8* img_sect_out, u8* img_riv_out) {
    std::srand(std::time(nullptr));
    int zoom = 2;
    int ow = img_size.width;
    int oh = img_size.height;
    int zw = ow * zoom;
    int zh = oh * zoom;
    u8* img_z = new u8[zw * zh * ch];
    for (int y = 0; y < zh; y++) {
        for (int x = 0; x < zw; x++) {
            int ox = x / zoom;
            int oy = y / zoom;
            int op = (oy * ow + ox) * ch;
            int zp = (y * zw + x) * ch;
            std::memcpy(&img_z[zp], &img_in[op], ch * sizeof(u8));
        }
    }
    std::vector<Sector> secs;
    std::vector<std::vector<QueueItem>> qs;
    std::vector<std::set<std::pair<int, int>>> qsets;
    bool* clm = new bool[zh * zw];
    int* own = new int[zh * zw];
    std::memset(clm, 0, zh * zw * sizeof(bool));
    std::memset(own, -1, zh * zw * sizeof(int));
    for (int ly = 0; ly < zh; ly += dist) {
        for (int lx = 0; lx < zw; lx += dist) {
            int ox = (std::rand() % 11) - 5;
            int oy = (std::rand() % 11) - 5;
            int zx = lx + ox;
            int zy = ly + oy;
            if (zx < 0 || zx >= zw || zy < 0 || zy >= zh) {
                continue;
            }
            int ox_orig = zx / zoom;
            int oy_orig = zy / zoom;
            if (ox_orig >= ow || oy_orig >= oh) {
                continue;
            }
            int op = (oy_orig * ow + ox_orig) * ch;
            if (colors_match(&img_in[op], color_ocean, ch) || colors_match(&img_in[op], color_mtn, ch)) {
                continue;
            }
            Sector s(secs.size());
            s.cx = zx;
            s.cy = zy;
            secs.push_back(s);
        }
    }
    qs.resize(secs.size());
    qsets.resize(secs.size());
    for (size_t i = 0; i < secs.size(); i++) {
        int px = secs[i].cx;
        int py = secs[i].cy;
        if (px < 0 || px >= zw || py < 0 || py >= zh) {
            continue;
        }
        int zp = (py * zw + px) * ch;
        if (colors_match(&img_z[zp], color_ocean, ch) || colors_match(&img_z[zp], color_mtn, ch)) {
            continue;
        }
        clm[py * zw + px] = true;
        own[py * zw + px] = i;
        secs[i].coords.push_back({px, py});
        int dx[] = {-1, 1, 0, 0};
        int dy[] = {0, 0, -1, 1};
        for (int d = 0; d < 4; d++) {
            int nx = px + dx[d];
            int ny = py + dy[d];
            if (nx >= 0 && nx < zw && ny >= 0 && ny < zh && !clm[ny * zw + nx]) {
                int nzp = (ny * zw + nx) * ch;
                if (!colors_match(&img_z[nzp], color_ocean, ch) && !colors_match(&img_z[nzp], color_mtn, ch)) {
                    int dsq = (nx - px) * (nx - px) + (ny - py) * (ny - py);
                    std::pair<int, int> c = {nx, ny};
                    if (qsets[i].find(c) == qsets[i].end()) {
                        QueueItem e;
                        e.dsq = dsq;
                        e.x = nx;
                        e.y = ny;
                        qs[i].push_back(e);
                        std::sort(qs[i].begin(), qs[i].end());
                        qsets[i].insert(c);
                    }
                }
            }
        }
    }
    std::vector<int> act;
    for (size_t i = 0; i < secs.size(); i++) {
        act.push_back(i);
    }
    int rw = 3;
    while (!act.empty()) {
        int min_dsq = -1;
        for (int i : act) {
            if (qs[i].empty()) {
                continue;
            }
            for (auto& e : qs[i]) {
                if (!clm[e.y * zw + e.x]) {
                    min_dsq = (min_dsq == -1 || e.dsq < min_dsq) ? e.dsq : min_dsq;
                    break;
                }
            }
        }
        if (min_dsq == -1) {
            break;
        }
        int min_d = static_cast<int>(std::sqrt(min_dsq));
        int max_d = min_d + rw;
        int max_dsq = max_d * max_d;
        std::vector<int> new_act;
        int claimed_cnt = 0;
        for (int i : act) {
            if (qs[i].empty()) {
                continue;
            }
            int px = secs[i].cx;
            int py = secs[i].cy;
            bool claimed_one = false;
            size_t qidx = 0;
            while (qidx < qs[i].size()) {
                QueueItem e = qs[i][qidx];
                int x = e.x;
                int y = e.y;
                if (clm[y * zw + x]) {
                    std::pair<int, int> c = {x, y};
                    qsets[i].erase(c);
                    qs[i].erase(qs[i].begin() + qidx);
                    continue;
                }
                if (e.dsq < min_dsq || e.dsq > max_dsq) {
                    qidx++;
                    continue;
                }
                int zp = (y * zw + x) * ch;
                if (colors_match(&img_z[zp], color_ocean, ch) || colors_match(&img_z[zp], color_mtn, ch)) {
                    std::pair<int, int> c = {x, y};
                    qsets[i].erase(c);
                    qs[i].erase(qs[i].begin() + qidx);
                    continue;
                }
                std::pair<int, int> c = {x, y};
                qsets[i].erase(c);
                qs[i].erase(qs[i].begin() + qidx);
                clm[y * zw + x] = true;
                own[y * zw + x] = i;
                secs[i].coords.push_back({x, y});
                claimed_cnt++;
                claimed_one = true;
                int dx[] = {-1, 1, 0, 0};
                int dy[] = {0, 0, -1, 1};
                for (int d = 0; d < 4; d++) {
                    int nx = x + dx[d];
                    int ny = y + dy[d];
                    if (nx >= 0 && nx < zw && ny >= 0 && ny < zh) {
                        int nzp = (ny * zw + nx) * ch;
                        if (colors_match(&img_z[nzp], color_ocean, ch) || colors_match(&img_z[nzp], color_mtn, ch)) {
                            continue;
                        }
                        if (clm[ny * zw + nx]) {
                            int oid = own[ny * zw + nx];
                            if (oid != -1 && oid != i) {
                                if (std::find(secs[i].adj.begin(), secs[i].adj.end(), oid) == secs[i].adj.end()) {
                                    secs[i].adj.push_back(oid);
                                    secs[oid].adj.push_back(i);
                                    secs[i].conn.push_back(oid);
                                    secs[oid].conn.push_back(i);
                                }
                            }
                        } else {
                            c = {nx, ny};
                            if (qsets[i].find(c) == qsets[i].end()) {
                                int ndsq = (nx - px) * (nx - px) + (ny - py) * (ny - py);
                                QueueItem ne;
                                ne.dsq = ndsq;
                                ne.x = nx;
                                ne.y = ny;
                                qs[i].push_back(ne);
                                std::sort(qs[i].begin(), qs[i].end());
                                qsets[i].insert(c);
                            }
                        }
                    }
                }
                break;
            }
            bool has_unclaimed = false;
            for (auto& e : qs[i]) {
                if (!clm[e.y * zw + e.x]) {
                    has_unclaimed = true;
                    break;
                }
            }
            if (has_unclaimed) {
                new_act.push_back(i);
            }
        }
        if (claimed_cnt == 0) {
            break;
        }
        act = new_act;
    }
    std::memcpy(img_sect_out, img_z, zw * zh * ch * sizeof(u8));
    std::set<std::tuple<int, int, int>> used;
    for (size_t i = 0; i < secs.size(); i++) {
        if (secs[i].coords.empty()) {
            continue;
        }
        int cr, cg, cb;
        while (true) {
            cr = 50 + (std::rand() % 151);
            cg = 50 + (std::rand() % 151);
            cb = 50 + (std::rand() % 151);
            auto t = std::make_tuple(cr, cg, cb);
            if (used.find(t) == used.end()) {
                used.insert(t);
                break;
            }
        }
        secs[i].clr[0] = cr;
        secs[i].clr[1] = cg;
        secs[i].clr[2] = cb;
        for (auto& c : secs[i].coords) {
            int x = c.first;
            int y = c.second;
            int p = (y * zw + x) * ch;
            std::memcpy(&img_sect_out[p], secs[i].clr, ch * sizeof(u8));
        }
    }
    std::memcpy(img_riv_out, img_sect_out, zw * zh * ch * sizeof(u8));
    std::vector<int> coast;
    for (size_t i = 0; i < secs.size(); i++) {
        if (secs[i].coords.empty()) {
            continue;
        }
        bool is_c = false;
        for (auto& c : secs[i].coords) {
            int x = c.first;
            int y = c.second;
            int dx[] = {-1, 1, 0, 0};
            int dy[] = {0, 0, -1, 1};
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d];
                int ny = y + dy[d];
                if (nx >= 0 && nx < zw && ny >= 0 && ny < zh) {
                    int zp = (ny * zw + nx) * ch;
                    if (colors_match(&img_z[zp], color_ocean, ch)) {
                        is_c = true;
                        break;
                    }
                }
            }
            if (is_c) {
                break;
            }
        }
        if (is_c) {
            secs[i].coastal = true;
            secs[i].river_id = i;
            coast.push_back(i);
        }
    }
    if (!coast.empty()) {
        std::map<int, std::vector<u8>> rclrs;
        for (int cid : coast) {
            std::vector<u8> clr(3);
            clr[0] = 50 + (std::rand() % 151);
            clr[1] = 50 + (std::rand() % 151);
            clr[2] = 50 + (std::rand() % 151);
            rclrs[cid] = clr;
        }
        std::set<int> clmd;
        std::vector<int> q = coast;
        std::set<std::pair<int, int>> rconns;
        for (int id : coast) {
            clmd.insert(id);
        }
        while (!q.empty()) {
            std::vector<int> nq;
            for (int sidx : q) {
                bool any = false;
                for (int aid : secs[sidx].conn) {
                    if (clmd.find(aid) == clmd.end()) {
                        clmd.insert(aid);
                        secs[aid].river_id = secs[sidx].river_id;
                        rconns.insert({sidx, aid});
                        nq.push_back(aid);
                        any = true;
                    }
                }
                if (any) {
                    nq.push_back(sidx);
                }
            }
            q = nq;
        }
        u8 rclr[3] = {0, 100, 255};
        int thick = 1;
        for (auto& conn : rconns) {
            int i = conn.first;
            int j = conn.second;
            draw_thick_line(img_sect_out, zw, zh, ch, secs[i].cx, secs[i].cy, secs[j].cx, secs[j].cy, rclr, thick);
            draw_thick_line(img_riv_out, zw, zh, ch, secs[i].cx, secs[i].cy, secs[j].cx, secs[j].cy, rclr, thick);
        }
        for (size_t i = 0; i < secs.size(); i++) {
            int rid = secs[i].river_id;
            if (rid != -1 && rclrs.find(rid) != rclrs.end()) {
                for (auto& c : secs[i].coords) {
                    int x = c.first;
                    int y = c.second;
                    int p = (y * zw + x) * ch;
                    std::memcpy(&img_riv_out[p], &rclrs[rid][0], ch * sizeof(u8));
                }
            }
        }
        for (auto& conn : rconns) {
            int i = conn.first;
            int j = conn.second;
            draw_thick_line(img_riv_out, zw, zh, ch, secs[i].cx, secs[i].cy, secs[j].cx, secs[j].cy, rclr, thick);
        }
    }
    delete[] clm;
    delete[] own;
    delete[] img_z;
}

}

//================================================================================================================================
//=> - End -
//================================================================================================================================
