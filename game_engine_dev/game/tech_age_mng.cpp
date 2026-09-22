//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "tech_age_mng.h" 

#include "assert_log.h"
#include "runtime_statics.h"
#include "tech_static_data.h"
#include "tech_static_key.h"

//================================================================================================================================
//=> - Static members -
//================================================================================================================================

u8* TechAgeMng::m_cat = nullptr;
u16 TechAgeMng::m_age_n = 0;
u16 TechAgeMng::m_pct = 0;
const RuntimeStatics* TechAgeMng::m_st = nullptr;
bool TechAgeMng::m_ready = false;

//================================================================================================================================
//=> - Static API -
//================================================================================================================================

u32 TechAgeMng::need (u8 catalog_n) {
    if (catalog_n == 0) {
        return 0;
    }
    return (static_cast<u32>(catalog_n) * m_pct + 99u) / 100u;
}

bool TechAgeMng::setup (const RuntimeStatics& st) {
    clear();
    const u16 age_n = st.tech_age().get_item_count();
    const u16 tech_n = st.tech().get_item_count();
    const u16 pct = st.config().get_tech_age_unlock_pct();
    GAME_EXPECT(age_n > 0, "TechAgeMng setup age_n");
    GAME_EXPECT(tech_n > 0, "TechAgeMng setup tech_n");
    GAME_EXPECT(pct > 0 && pct <= 100, "TechAgeMng setup pct");
    m_cat = new u8[age_n]();
    m_age_n = age_n;
    m_pct = pct;
    m_st = &st;
    for (u16 t = 0; t < tech_n; ++t) {
        const u16 age = st.tech().get_item(TechStaticDataKey::from_raw(t)).tier;
        GAME_EXPECT(age < age_n, "TechAgeMng setup tech.tier");
        GAME_EXPECT(m_cat[age] < 255u, "TechAgeMng setup cat overflow");
        m_cat[age] = static_cast<u8>(m_cat[age] + 1u);
    }
    m_ready = true;
    return true;
}

void TechAgeMng::clear () {
    delete[] m_cat;
    m_cat = nullptr;
    m_age_n = 0;
    m_pct = 0;
    m_st = nullptr;
    m_ready = false;
}

bool TechAgeMng::ready () {
    return m_ready;
}

u16 TechAgeMng::age_n () {
    return m_age_n;
}

u8 TechAgeMng::cat (u16 age) {
    GAME_EXPECT(m_ready, "TechAgeMng cat ready");
    GAME_EXPECT(age < m_age_n, "TechAgeMng cat age");
    return m_cat[age];
}

u16 TechAgeMng::pct () {
    GAME_EXPECT(m_ready, "TechAgeMng pct ready");
    return m_pct;
}

//================================================================================================================================
//=> - Instance -
//================================================================================================================================

TechAgeMng::TechAgeMng () : m_done(nullptr), m_head(0) {
    GAME_EXPECT(m_ready, "TechAgeMng ctor ready");
    m_done = new u8[m_age_n]();
    reset();
}

TechAgeMng::~TechAgeMng () {
    delete[] m_done;
    m_done = nullptr;
}

void TechAgeMng::adv () {
    while (m_head + 1u < m_age_n && m_done[m_head] >= need(m_cat[m_head])) {
        m_head = static_cast<u16>(m_head + 1u);
    }
}

void TechAgeMng::reset () {
    GAME_EXPECT(m_done != nullptr, "TechAgeMng reset done");
    for (u16 i = 0; i < m_age_n; ++i) {
        m_done[i] = 0;
    }
    m_head = 0;
    adv();
}

bool TechAgeMng::log_tech (u16 tech_idx) {
    GAME_EXPECT(m_ready && m_st != nullptr, "TechAgeMng log_tech ready");
    GAME_EXPECT(tech_idx < m_st->tech().get_item_count(), "TechAgeMng log_tech idx");
    const u16 age = m_st->tech().get_item(TechStaticDataKey::from_raw(tech_idx)).tier;
    GAME_EXPECT(age < m_age_n, "TechAgeMng log_tech age");
    if (m_done[age] < 255u) {
        m_done[age] = static_cast<u8>(m_done[age] + 1u);
    }
    adv();
    return true;
}

bool TechAgeMng::is_available (u16 tech_idx) const {
    GAME_EXPECT(m_ready && m_st != nullptr, "TechAgeMng is_available ready");
    GAME_EXPECT(tech_idx < m_st->tech().get_item_count(), "TechAgeMng is_available idx");
    const u16 age = m_st->tech().get_item(TechStaticDataKey::from_raw(tech_idx)).tier;
    GAME_EXPECT(age < m_age_n, "TechAgeMng is_available age");
    return age <= m_head;
}

u16 TechAgeMng::head_age () const {
    return m_head;
}

u8 TechAgeMng::done (u16 age) const {
    GAME_EXPECT(m_done != nullptr, "TechAgeMng done ptr");
    GAME_EXPECT(age < m_age_n, "TechAgeMng done age");
    return m_done[age];
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
