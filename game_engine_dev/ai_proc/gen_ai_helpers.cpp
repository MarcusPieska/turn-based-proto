//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "gen_ai_helpers.h"

#include "assert_log.h"
#include "game_array_simple.h"
#include "gen_bottleneck_forts.h"
#include "gen_mtn_passes.h"
#include "gen_settlement_targets.h"

//================================================================================================================================
//=> - Local -
//================================================================================================================================

static u8 resolve_intent (u8 cur, u8 cand) {
    if (cur == AI_TILE_OV_INTENT_CITY) {
        return cur;
    }
    if (cand == AI_TILE_OV_INTENT_CITY) {
        return cand;
    }
    if (cand == AI_TILE_OV_INTENT_FORT) {
        return cand;
    }
    if (cur == AI_TILE_OV_INTENT_FORT) {
        return cur;
    }
    if (cand == AI_TILE_OV_INTENT_MTN_PASS) {
        return cand;
    }
    return cur;
}

static bool try_stamp (GameArraySimple& map, u16 x, u16 y, u8 cand) {
    const u8 cur = map.get_ai_ov_intent(x, y);
    const u8 nxt = resolve_intent(cur, cand);
    if (nxt == cur) {
        return false;
    }
    map.set_ai_ov_intent(x, y, nxt);
    return true;
}

static void clr_pass_fort (GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 iv = map.get_ai_ov_intent(x, y);
            if (iv == AI_TILE_OV_INTENT_MTN_PASS || iv == AI_TILE_OV_INTENT_FORT) {
                map.set_ai_ov_intent(x, y, AI_TILE_OV_INTENT_NONE);
            }
        }
    }
}

static void unmark_start_cities (GameArraySimple& map, const SpgCoordPair* starts, u32 start_n) {
    if (starts == nullptr) {
        return;
    }
    for (u32 i = 0; i < start_n; ++i) {
        const u16 x = starts[i].x;
        const u16 y = starts[i].y;
        if (x >= map.width() || y >= map.height()) {
            continue;
        }
        map.set_planned_city(x, y, 0u);
    }
}

static void stamp_mtn (GameArraySimple& map, const GenMtnPasses& mtn, GenAiHelpersRslt* out) {
    const Whiteboard_1B& pass = mtn.passes();
    const Whiteboard_1B& pft = mtn.pass_forts();
    const u16 w = map.width();
    const u16 h = map.height();
    u32 pass_n = 0u;
    u32 pass_fort_n = 0u;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (pft.rd(x, y) != 0u) {
                if (try_stamp(map, x, y, AI_TILE_OV_INTENT_FORT)) {
                    ++pass_fort_n;
                }
                continue;
            }
            if (pass.rd(x, y) != 0u) {
                if (try_stamp(map, x, y, AI_TILE_OV_INTENT_MTN_PASS)) {
                    ++pass_n;
                }
            }
        }
    }
    if (out != nullptr) {
        out->m_mtn_pass_n = pass_n;
        out->m_mtn_pass_fort_n = pass_fort_n;
    }
}

static u32 stamp_bn_forts (GameArraySimple& map, const GenBottleneckForts& bf) {
    const Whiteboard_1B& ft = bf.forts();
    const u16 w = map.width();
    const u16 h = map.height();
    u32 n = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (ft.rd(x, y) == 0u) {
                continue;
            }
            if (try_stamp(map, x, y, AI_TILE_OV_INTENT_FORT)) {
                ++n;
            }
        }
    }
    return n;
}

static u32 count_intent (const GameArraySimple& map, u8 intent) {
    u32 n = 0;
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_ai_ov_intent(x, y) == intent) {
                ++n;
            }
        }
    }
    return n;
}

static u32 order_site_n (const GenSettlementOrder& ord) {
    u32 n = 0;
    const u16 pn = ord.pn();
    for (u16 p = 0; p < pn; ++p) {
        n += ord.n(p);
    }
    return n;
}

//================================================================================================================================
//=> - GenAiHelpers -
//================================================================================================================================

GenAiHelpers::GenAiHelpers () :
    m_map(nullptr),
    m_ord(nullptr),
    m_jobs(nullptr),
    m_roads(nullptr),
    m_ok(false) {
}

GenAiHelpers::~GenAiHelpers () {
    delete m_roads;
    m_roads = nullptr;
    delete m_jobs;
    m_jobs = nullptr;
    delete m_ord;
    m_ord = nullptr;
}

bool GenAiHelpers::begin (GameArraySimple& map) {
    m_map = &map;
    if (m_ord == nullptr) {
        m_ord = new GenSettlementOrder();
    }
    if (m_jobs == nullptr) {
        m_jobs = new WorkerCityJobs();
    }
    if (m_roads == nullptr) {
        m_roads = new GenRoadNetwork();
    }
    m_ok = map.width() > 0u && map.height() > 0u && m_ord != nullptr && m_ord->ok()
        && m_jobs != nullptr && m_roads != nullptr && m_roads->begin(map);
    GAME_EXPECT(m_ok, "GenAiHelpers begin whiteboard checkout failed");
    return m_ok;
}

void GenAiHelpers::clr () {
    if (m_map != nullptr) {
        clr_pass_fort(*m_map);
    }
    if (m_ord != nullptr) {
        m_ord->clr();
    }
    if (m_jobs != nullptr) {
        m_jobs->clr();
    }
    if (m_roads != nullptr) {
        m_roads->clr();
    }
    delete m_roads;
    m_roads = nullptr;
    delete m_jobs;
    m_jobs = nullptr;
    delete m_ord;
    m_ord = nullptr;
    m_map = nullptr;
    m_ok = false;
}

bool GenAiHelpers::ok () const {
    return m_ok;
}

const GenSettlementOrder& GenAiHelpers::order () const {
    GAME_EXPECT(m_ord != nullptr, "GenAiHelpers order not begun");
    return *m_ord;
}

GenSettlementOrder& GenAiHelpers::order () {
    GAME_EXPECT(m_ord != nullptr, "GenAiHelpers order not begun");
    return *m_ord;
}

const WorkerCityJobs& GenAiHelpers::jobs () const {
    GAME_EXPECT(m_jobs != nullptr, "GenAiHelpers jobs not begun");
    return *m_jobs;
}

WorkerCityJobs& GenAiHelpers::jobs () {
    GAME_EXPECT(m_jobs != nullptr, "GenAiHelpers jobs not begun");
    return *m_jobs;
}

const GenRoadNetwork& GenAiHelpers::roads () const {
    GAME_EXPECT(m_roads != nullptr, "GenAiHelpers roads not begun");
    return *m_roads;
}

GenRoadNetwork& GenAiHelpers::roads () {
    GAME_EXPECT(m_roads != nullptr, "GenAiHelpers roads not begun");
    return *m_roads;
}

bool GenAiHelpers::build (const SpgCoordPair* starts, u32 start_n, GenAiHelpersRslt* out) {
    GAME_EXPECT_RET(m_ok && m_map != nullptr && m_ord != nullptr && m_jobs != nullptr && m_roads != nullptr,
        false, "GenAiHelpers build not begun");
    GenAiHelpersRslt local = {};
    GenAiHelpersRslt* r = out != nullptr ? out : &local;
    *r = {};
    clr_pass_fort(*m_map);
    GenSettlementTargetsRslt city = {};
    if (!GenSettlementTargets::generate(*m_map, starts, start_n, &city)) {
        return false;
    }
    unmark_start_cities(*m_map, starts, start_n);
    r->m_city_n = count_intent(*m_map, AI_TILE_OV_INTENT_CITY);
    if (start_n > 0u) {
        if (!m_ord->gen_excl(*m_map, starts, start_n)) {
            return false;
        }
        r->m_ord_n = order_site_n(*m_ord);
    } else {
        r->m_ord_n = 0u;
    }
    {
        GenMtnPasses mtn;
        if (!mtn.begin(*m_map)) {
            return false;
        }
        if (!mtn.index_patches(*m_map)) {
            return false;
        }
        if (!mtn.find_passes(*m_map)) {
            return false;
        }
        stamp_mtn(*m_map, mtn, r);
    }
    GenBottleneckForts bf;
    if (!bf.begin(*m_map)) {
        return false;
    }
    if (!bf.build()) {
        return false;
    }
    r->m_bn_fort_n = stamp_bn_forts(*m_map, bf);
    r->m_mtn_pass_n = count_intent(*m_map, AI_TILE_OV_INTENT_MTN_PASS);
    if (!m_jobs->build(*m_map)) {
        return false;
    }
    r->m_wcj_site_n = m_jobs->site_n();
    u32 jn = 0;
    for (u32 i = 0; i < m_jobs->site_n(); ++i) {
        jn += m_jobs->job_n(i);
    }
    r->m_wcj_job_n = jn;
    if (!m_roads->begin(*m_map)) {
        return false;
    }
    if (!m_roads->build(starts, start_n)) {
        return false;
    }
    r->m_road_term_n = m_roads->term_n();
    r->m_road_n = m_roads->road_n();
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
