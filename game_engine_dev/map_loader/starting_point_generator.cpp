//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include "starting_point_generator.h"

#include "generator_constants.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static const u16 k_u16_inf = 0xFFFFu;

static f32 rnd_f32 (f32 lo, f32 hi) {
    const f32 t = static_cast<f32>(std::rand()) / static_cast<f32>(RAND_MAX);
    return lo + (hi - lo) * t;
}

static u32 rnd_u32 (u32 lo, u32 hi) {
    if (hi <= lo) {
        return lo;
    }
    return lo + static_cast<u32>(std::rand() % (hi - lo + 1u));
}

static void shuffle_idx (u32* idx, u32 n) {
    for (u32 i = n; i > 1u; --i) {
        const u32 j = static_cast<u32>(std::rand() % i);
        const u32 t = idx[i - 1u];
        idx[i - 1u] = idx[j];
        idx[j] = t;
    }
}

static f32 dist_sq (const SpgPt& a, const SpgPt& b) {
    const f32 dx = a.x - b.x;
    const f32 dy = a.y - b.y;
    return dx * dx + dy * dy;
}

//================================================================================================================================
//=> - StartingPointGenerator -
//================================================================================================================================

StartingPointGenerator::StartingPointGenerator (const StartingPointGeneratorParams& par) :
    m_p(par),
    m_w(0),
    m_h(0),
    m_cls(nullptr),
    m_dist(nullptr),
    m_gray(nullptr),
    m_dist_max(0),
    m_latt(nullptr),
    m_latt_n(0),
    m_land(nullptr),
    m_land_n(0),
    m_cand(nullptr),
    m_cand_n(0),
    m_pick(nullptr),
    m_pick_n(0),
    m_pick_skip(false),
    m_ok(false) {
    m_latt = new SpgPt[SPG_MAX_LATT_PTS];
    m_land = new SpgPt[SPG_MAX_LATT_PTS];
    m_cand = new SpgPt[SPG_MAX_LATT_PTS];
    m_pick = new SpgPt[SPG_MAX_PICK_PTS];
}

StartingPointGenerator::~StartingPointGenerator () {
    free_bufs();
    delete[] m_latt;
    m_latt = nullptr;
    delete[] m_land;
    m_land = nullptr;
    delete[] m_cand;
    m_cand = nullptr;
    delete[] m_pick;
    m_pick = nullptr;
}

u32 StartingPointGenerator::lattice_count () const {
    return m_latt_n;
}

u32 StartingPointGenerator::land_count () const {
    return m_land_n;
}

u32 StartingPointGenerator::candidate_count () const {
    return m_cand_n;
}

u32 StartingPointGenerator::pick_count () const {
    return m_pick_n;
}

u16 StartingPointGenerator::dist_max () const {
    return m_dist_max;
}

bool StartingPointGenerator::pick_skipped () const {
    return m_pick_skip;
}

SpgPickCoords StartingPointGenerator::picks_coords () const {
    SpgPickCoords out = {};
    out.n = m_pick_n;
    for (u32 i = 0; i < m_pick_n; ++i) {
        out.pts[i].x = static_cast<u16>(m_pick[i].x);
        out.pts[i].y = static_cast<u16>(m_pick[i].y);
    }
    return out;
}

u16 StartingPointGenerator::pick_target () const {
    return m_p.pick_n;
}

const SpgPt* StartingPointGenerator::candidates () const {
    return m_cand;
}

const SpgPt* StartingPointGenerator::picks () const {
    return m_pick;
}

u16 StartingPointGenerator::map_width () const {
    return m_w;
}

u16 StartingPointGenerator::map_height () const {
    return m_h;
}

const u8* StartingPointGenerator::terrain () const {
    return m_cls;
}

const u8* StartingPointGenerator::dist_gray () const {
    return m_gray;
}

bool StartingPointGenerator::picks_are_start_land () const {
    if (!m_ok || m_cls == nullptr || m_w == 0 || m_h == 0) {
        return false;
    }
    const SpgPickCoords c = picks_coords();
    for (u32 i = 0; i < c.n; ++i) {
        const u16 px = c.pts[i].x;
        const u16 py = c.pts[i].y;
        if (px >= m_w || py >= m_h) {
            return false;
        }
        const u8 cls = m_cls[static_cast<u32>(py) * static_cast<u32>(m_w) + static_cast<u32>(px)];
        if (!is_start_land(cls)) {
            return false;
        }
    }
    return true;
}

void StartingPointGenerator::free_bufs () {
    delete[] m_dist;
    m_dist = nullptr;
    delete[] m_gray;
    m_gray = nullptr;
}

bool StartingPointGenerator::chk_par () const {
    if (m_p.map == nullptr) {
        return false;
    }
    if (m_p.map->data() == nullptr || m_p.map->width() == 0 || m_p.map->height() == 0) {
        return false;
    }
    if (m_p.latt_rows == 0 || m_p.latt_cols == 0) {
        return false;
    }
    const u32 latt_n = static_cast<u32>(m_p.latt_rows) * static_cast<u32>(m_p.latt_cols);
    if (latt_n == 0 || latt_n > SPG_MAX_LATT_PTS) {
        return false;
    }
    if (m_p.pick_n == 0 || m_p.pick_n > SPG_MAX_PICK_PTS) {
        return false;
    }
    return true;
}

bool StartingPointGenerator::is_water (u8 cls) const {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

bool StartingPointGenerator::is_land (u8 cls) const {
    return !is_water(cls);
}

bool StartingPointGenerator::is_start_land (u8 cls) const {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0];
}

bool StartingPointGenerator::adj_water (u16 px, u16 py) const {
    const u16 wi = m_w;
    const u32 i = static_cast<u32>(py) * static_cast<u32>(wi) + static_cast<u32>(px);
    if (px > 0 && is_water(m_cls[i - 1u])) {
        return true;
    }
    if (static_cast<u32>(px) + 1u < static_cast<u32>(wi) && is_water(m_cls[i + 1u])) {
        return true;
    }
    if (py > 0 && is_water(m_cls[i - static_cast<u32>(wi)])) {
        return true;
    }
    if (static_cast<u32>(py) + 1u < static_cast<u32>(m_h) && is_water(m_cls[i + static_cast<u32>(wi)])) {
        return true;
    }
    return false;
}

bool StartingPointGenerator::bfs_dist () {
    const u32 n = static_cast<u32>(m_w) * static_cast<u32>(m_h);
    m_dist = new u16[n];
    u32* q = new u32[n];
    u32 qh = 0;
    u32 qt = 0;
    m_dist_max = 0;
    for (u32 i = 0; i < n; ++i) {
        m_dist[i] = k_u16_inf;
    }
    for (u16 py = 0; py < m_h; ++py) {
        for (u16 px = 0; px < m_w; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(m_w) + static_cast<u32>(px);
            if (!is_land(m_cls[i]) || !adj_water(px, py)) {
                continue;
            }
            m_dist[i] = 1;
            q[qt++] = i;
            if (m_dist_max < 1) {
                m_dist_max = 1;
            }
        }
    }
    while (qh < qt) {
        const u32 i = q[qh++];
        const u16 cur = m_dist[i];
        if (cur >= k_u16_inf - 1u) {
            continue;
        }
        const u16 nxt = static_cast<u16>(cur + 1u);
        const u16 py = static_cast<u16>(i / static_cast<u32>(m_w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(m_w));
        if (px > 0) {
            const u32 j = i - 1u;
            if (is_land(m_cls[j]) && m_dist[j] == k_u16_inf) {
                m_dist[j] = nxt;
                if (nxt > m_dist_max) {
                    m_dist_max = nxt;
                }
                q[qt++] = j;
            }
        }
        if (static_cast<u32>(px) + 1u < static_cast<u32>(m_w)) {
            const u32 j = i + 1u;
            if (is_land(m_cls[j]) && m_dist[j] == k_u16_inf) {
                m_dist[j] = nxt;
                if (nxt > m_dist_max) {
                    m_dist_max = nxt;
                }
                q[qt++] = j;
            }
        }
        if (py > 0) {
            const u32 j = i - static_cast<u32>(m_w);
            if (is_land(m_cls[j]) && m_dist[j] == k_u16_inf) {
                m_dist[j] = nxt;
                if (nxt > m_dist_max) {
                    m_dist_max = nxt;
                }
                q[qt++] = j;
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(m_h)) {
            const u32 j = i + static_cast<u32>(m_w);
            if (is_land(m_cls[j]) && m_dist[j] == k_u16_inf) {
                m_dist[j] = nxt;
                if (nxt > m_dist_max) {
                    m_dist_max = nxt;
                }
                q[qt++] = j;
            }
        }
    }
    delete[] q;
    return true;
}

u8 StartingPointGenerator::flip_norm (u16 d) const {
    if (d <= 0 || d >= k_u16_inf) {
        return 0;
    }
    if (d == 1) {
        return 255;
    }
    if (m_dist_max <= 1) {
        return 255;
    }
    const u16 flip = static_cast<u16>(m_dist_max - d + 1u);
    return static_cast<u8>(1 + (static_cast<u32>(flip) - 1u) * 254u / (static_cast<u32>(m_dist_max) - 1u));
}

void StartingPointGenerator::mk_gray () {
    const u32 n = static_cast<u32>(m_w) * static_cast<u32>(m_h);
    m_gray = new u8[n];
    for (u32 i = 0; i < n; ++i) {
        if (is_water(m_cls[i]) || !is_land(m_cls[i])) {
            m_gray[i] = 0;
            continue;
        }
        m_gray[i] = flip_norm(m_dist[i]);
    }
}

u8 StartingPointGenerator::gray_at (f32 x, f32 y) const {
    const i32 px = static_cast<i32>(x);
    const i32 py = static_cast<i32>(y);
    if (px < 0 || py < 0 || px >= static_cast<i32>(m_w) || py >= static_cast<i32>(m_h)) {
        return 0;
    }
    return m_gray[static_cast<u32>(py) * static_cast<u32>(m_w) + static_cast<u32>(px)];
}

void StartingPointGenerator::mk_lattice () {
    const f32 dx = static_cast<f32>(m_w) / static_cast<f32>(m_p.latt_cols);
    const f32 dy = static_cast<f32>(m_h) / static_cast<f32>(m_p.latt_rows);
    const f32 jx = dx * m_p.jitter_frac;
    const f32 jy = dy * m_p.jitter_frac;
    m_latt_n = 0;
    for (u16 row = 0; row < m_p.latt_rows; ++row) {
        for (u16 col = 0; col < m_p.latt_cols; ++col) {
            SpgPt& p = m_latt[m_latt_n++];
            p.x = (static_cast<f32>(col) + 0.5f) * dx + rnd_f32(-jx, jx);
            p.y = (static_cast<f32>(row) + 0.5f) * dy + rnd_f32(-jy, jy);
        }
    }
}

void StartingPointGenerator::filt_land () {
    m_land_n = 0;
    for (u32 i = 0; i < m_latt_n; ++i) {
        const SpgPt& p = m_latt[i];
        const i32 px = static_cast<i32>(p.x);
        const i32 py = static_cast<i32>(p.y);
        if (px < 0 || py < 0 || px >= static_cast<i32>(m_w) || py >= static_cast<i32>(m_h)) {
            continue;
        }
        const u8 cls = m_cls[static_cast<u32>(py) * static_cast<u32>(m_w) + static_cast<u32>(px)];
        if (is_start_land(cls)) {
            m_land[m_land_n++] = p;
        }
    }
}

void StartingPointGenerator::filt_prob () {
    const u32 n = m_land_n;
    const u16 min_n = m_p.pick_n;
    if (n <= static_cast<u32>(min_n)) {
        for (u32 i = 0; i < n; ++i) {
            m_cand[i] = m_land[i];
        }
        m_cand_n = n;
        return;
    }
    u32* idx = new u32[n];
    for (u32 i = 0; i < n; ++i) {
        idx[i] = i;
    }
    shuffle_idx(idx, n);
    m_cand_n = 0;
    for (u32 i = 0; i < n; ++i) {
        const u32 rest = n - i;
        const SpgPt& p = m_land[idx[i]];
        const u32 roll = rnd_u32(0, 255);
        const u8 d = gray_at(p.x, p.y);
        if (roll <= static_cast<u32>(d)) {
            m_cand[m_cand_n++] = p;
        } else if (m_cand_n + rest - 1u >= static_cast<u32>(min_n)) {
        } else {
            m_cand[m_cand_n++] = p;
        }
    }
    delete[] idx;
}

void StartingPointGenerator::pick_spaced () {
    const u32 n = m_cand_n;
    const u32 k = static_cast<u32>(m_p.pick_n);
    if (n == 0 || k == 0) {
        return;
    }
    const u32 use_k = n < k ? n : k;
    for (u32 si = 0; si < use_k; ++si) {
        const u32 a = (si * n) / use_k;
        const u32 b = ((si + 1u) * n) / use_k;
        f32 best_d = -1.0f;
        u32 best_i = a;
        bool has = false;
        for (u32 i = a; i < b; ++i) {
            f32 md = 1.0e30f;
            for (u32 j = 0; j < m_pick_n; ++j) {
                const f32 d = dist_sq(m_cand[i], m_pick[j]);
                if (d < md) {
                    md = d;
                }
            }
            if (!has || md > best_d) {
                best_d = md;
                best_i = i;
                has = true;
            }
        }
        if (has) {
            m_pick[m_pick_n++] = m_cand[best_i];
        }
    }
}

bool StartingPointGenerator::generate () {
    m_ok = false;
    m_latt_n = 0;
    m_land_n = 0;
    m_cand_n = 0;
    m_pick_n = 0;
    m_pick_skip = false;
    free_bufs();
    if (!chk_par()) {
        return false;
    }
    std::srand(m_p.seed);
    m_w = m_p.map->width();
    m_h = m_p.map->height();
    m_cls = m_p.map->data();
    if (m_cls == nullptr || m_w == 0 || m_h == 0) {
        return false;
    }
    if (!bfs_dist()) {
        return false;
    }
    mk_gray();
    mk_lattice();
    filt_land();
    filt_prob();
    if (m_cand_n <= static_cast<u32>(m_p.pick_n)) {
        m_pick_skip = true;
        for (u32 i = 0; i < m_cand_n; ++i) {
            m_pick[m_pick_n++] = m_cand[i];
        }
    } else {
        m_pick_skip = false;
        pick_spaced();
    }
    m_ok = true;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
