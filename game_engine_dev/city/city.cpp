//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "bit_array.h"
#include "assert_log.h"
#include "general_assessor.h"
#include "general_bit_bank.h"
#include "runtime_statics.h"
#include "building_static_key.h"
#include "civ_static_data.h"
#include "civ_static_key.h"
#include "toggle_city_static_key.h"
#include "small_wonder_static_key.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"
#include "wonder_static_key.h"

#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "player_ledger.h"
#include "game_state.h"

#include "unit_add_struct.h"
#include "unit_type_action_map.h"
#include "worker_helper.h"

#include "effect_ctx.h"
#include "booster_apply.h"
#include "local_unit_exp_booster_register.h"
#include "civ_ship_training_booster_register.h"
#include "city_commerce_booster_register.h"
#include "city_production_booster_register.h"
#include "city_pop_growth_booster_register.h"
#include "civ_pop_growth_booster_register.h"
#include "city_culture_booster_register.h"
#include "local_commerce_booster_register.h"
#include "local_pop_growth_booster_register.h"
#include "local_production_booster_register.h"
#include "local_culture_booster_register.h"
#include "local_sanitation_booster_register.h"
#include "city_sanitation_booster_register.h"
#include "civ_sanitation_booster_register.h"
#include "dyn_booster_register.h"
#include "city_border.h"
#include "city_tracer.h"
#include "tile_yields.h"

#include <cstring>

#include "city.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

static const RuntimeStatics* s_statics = nullptr;
static GeneralBitBank* s_flag_bank = nullptr;
static GeneralBitBank* s_res_bank = nullptr;
static GeneralBitBank* s_bld_bank = nullptr;
static UnitAddVector* s_units = nullptr;
static u16* s_wonder_city = nullptr;
static PlayerState* s_player_states = nullptr;
static u16 s_player_n = 0;
static BitArrayCL* s_scratch_bld = nullptr;
static BitArrayCL* s_scratch_wonder = nullptr;
static BitArrayCL* s_scratch_sw = nullptr;
static BitArrayCL* s_scratch_unit = nullptr;
static BitArrayCL* s_civ_trait_scratch = nullptr;
static u16 s_flag_has_wonder = U16_KEY_NULL;
static u16 s_flag_has_wonder_small = U16_KEY_NULL;
static u16 s_wonder_n = 0;
static u16 s_small_wonder_n = 0;

static const u16 k_act_is_land = 0u;
static const u16 k_act_is_sea = 2u; 

//================================================================================================================================
//=> - BuildType -
//================================================================================================================================

typedef enum BuildType : u8 {
    BUILD_TYPE_NONE = 0,
    BUILD_TYPE_BUILDING = 1,
    BUILD_TYPE_WONDER = 2,
    BUILD_TYPE_SMALL_WONDER = 3,
    BUILD_TYPE_UNIT = 4,
    ACCUMULATE_COMMERCE = 5 
} BuildType;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void fill_civ_trait_scratch (BitArrayCL* civ) {
    if (s_civ_trait_scratch == nullptr) {
        return;
    }
    s_civ_trait_scratch->clear_all();
    if (s_statics == nullptr || civ == nullptr) {
        return;
    }
    for (u32 i = 0; i < civ->get_count(); ++i) {
        if (civ->get_bit(i) == 0) {
            continue;
        }
        const CivTraitStruct& tr = s_statics->civ().get_item(CivStaticDataKey::from_raw(i)).traits;
        for (u32 t = 0; t < MAX_CIV_TRAIT_COUNT; ++t) {
            const u16 tix = tr.indices[t];
            if (tix != U16_KEY_NULL && tix < s_civ_trait_scratch->get_count()) {
                s_civ_trait_scratch->set_bit(tix);
            }
        }
        return;
    }
}

static AssessorCtx make_city_ctx (const City& city, u16 city_idx, BitArrayCL* techs, BitArrayCL* civ) {
    fill_civ_trait_scratch(civ);
    AssessorCtx ctx = {};
    ctx.m_tech = techs;
    ctx.m_civ = civ;
    ctx.m_city_idx = city_idx;
    ctx.m_resource_bank = s_res_bank;
    ctx.m_building_bank = s_bld_bank;
    ctx.m_toggle_city_bank = s_flag_bank;
    ctx.m_civ_trait = s_civ_trait_scratch;
    ctx.m_map = CityBorder::map();
    ctx.m_x = city.get_x();
    ctx.m_y = city.get_y();
    return ctx;
}

static u16 find_toggle_city_idx (cstr nm) {
    if (s_statics == nullptr || nm == nullptr) {
        return U16_KEY_NULL;
    }
    const u16 n = s_statics->toggle_city().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        cstr flag_nm = s_statics->toggle_city().get_name(ToggleCityStaticDataKey::from_raw(i));
        if (flag_nm != nullptr && std::strcmp(flag_nm, nm) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static void set_toggle_city (u16 city_idx, u16 flag_idx) {
    if (s_flag_bank == nullptr || flag_idx == U16_KEY_NULL) {
        return;
    }
    s_flag_bank->set_flag(city_idx, flag_idx);
}

static u16* small_wonder_row (u16 owner) {
    if (s_player_states == nullptr || owner >= s_player_n) {
        return nullptr;
    }
    return s_player_states[owner].m_small_wonder_city;
}

static EffectCtx make_city_effect_ctx (const City& city, u16 city_idx) {
    EffectCtx ctx = {};
    ctx.m_owner = city.get_owner();
    ctx.m_city_idx = city_idx;
    if (s_player_states != nullptr && ctx.m_owner < s_player_n) {
        ctx.m_tech = s_player_states[ctx.m_owner].m_techs_researched;
    }
    ctx.m_bld_bank = s_bld_bank;
    ctx.m_wonder_city = s_wonder_city;
    ctx.m_wonder_n = s_wonder_n;
    ctx.m_small_wonder_city = small_wonder_row(ctx.m_owner);
    ctx.m_small_wonder_n = s_small_wonder_n;
    return ctx;
}

static bool unit_is_land (u16 unit_idx) {
    if (s_statics == nullptr || unit_idx == U16_KEY_NULL) {
        return false;
    }
    const UnitStaticDataStruct& u = s_statics->unit().get_item(UnitStaticDataKey::from_raw(unit_idx));
    return s_statics->unit_type_action_map().unit_type_can_do(u.type, k_act_is_land);
}

static bool unit_is_sea (u16 unit_idx) {
    if (s_statics == nullptr || unit_idx == U16_KEY_NULL) {
        return false;
    }
    const UnitStaticDataStruct& u = s_statics->unit().get_item(UnitStaticDataKey::from_raw(unit_idx));
    return s_statics->unit_type_action_map().unit_type_can_do(u.type, k_act_is_sea);
}

static bool unit_is_settler (u16 unit_idx) {
    if (s_statics == nullptr || unit_idx == U16_KEY_NULL) {
        return false;
    }
    const UnitStaticDataStruct& u = s_statics->unit().get_item(UnitStaticDataKey::from_raw(unit_idx));
    cstr nm = s_statics->unit_type().get_name(UnitTypeStaticDataKey::from_raw(u.type));
    return nm != nullptr && std::strcmp(nm, "LAND_SETTLER") == 0;
}

static bool unit_is_worker (u16 unit_idx) {
    if (s_statics == nullptr || unit_idx == U16_KEY_NULL) {
        return false;
    }
    const UnitStaticDataStruct& u = s_statics->unit().get_item(UnitStaticDataKey::from_raw(unit_idx));
    cstr nm = s_statics->unit_type().get_name(UnitTypeStaticDataKey::from_raw(u.type));
    return nm != nullptr && std::strcmp(nm, "LAND_WORKER") == 0;
}

static u8 spawn_unit_level (const City& city, u16 city_idx, u16 unit_idx) {
    const EffectCtx ctx = make_city_effect_ctx(city, city_idx);
    if (unit_is_sea(unit_idx)) {
        return apply_unit_level_boost(GREEN, CivShipTrainingBoosterRegister::determine_effect(ctx));
    }
    if (unit_is_land(unit_idx)) {
        return apply_unit_level_boost(GREEN, LocalUnitExpBoosterRegister::determine_effect(ctx));
    }
    return GREEN;
}

//================================================================================================================================
//=> - City implementation -
//================================================================================================================================

City::City () : m_owner(U16_KEY_NULL),
    m_x(U16_KEY_NULL),
    m_y(U16_KEY_NULL),
    m_bld_idx(U16_KEY_NULL),
    m_pop_count(0),
    m_build_cost(0),
    m_accumulated_production(0),
    m_culture(0),
    m_conn_city_nw(U16_KEY_NULL),
    m_conn_city_ne(U16_KEY_NULL),
    m_conn_city_sw(U16_KEY_NULL),
    m_conn_city_se(U16_KEY_NULL),
    m_accumulated_food(0),
    m_build_type(BUILD_TYPE_NONE),
    m_is_frontier_city(0),
    m_road_conn(0),
    m_misc{} {
}

City::~City () {
}

void City::init (u16 owner, u16 x, u16 y) {
    m_owner = owner;
    m_x = x;
    m_y = y;
    m_accumulated_food = 0;
    m_accumulated_production = 0;
    m_culture = 0;
    m_bld_idx = U16_KEY_NULL;
    m_build_cost = 0;
    m_build_type = BUILD_TYPE_NONE;
    m_pop_count = 1;
    m_is_frontier_city = 1; // Presumed true until proven otherwise by AI settler sensing logic
    m_conn_city_nw = U16_KEY_NULL;
    m_conn_city_ne = U16_KEY_NULL;
    m_conn_city_sw = U16_KEY_NULL;
    m_conn_city_se = U16_KEY_NULL;
    m_road_conn = 0;
    m_misc.m_city_has_worker = 0;
    m_misc.m_city_defense_deduction = 0;
    m_misc.m_tile_imp_count = 0;
}

void City::bind_statics (const RuntimeStatics& st) {
    clear_assess_scratch();
    s_statics = &st;
    const u16 bld_n = st.building().get_item_count();
    const u16 wonder_n = st.wonder().get_item_count();
    const u16 sw_n = st.small_wonder().get_item_count();
    const u16 unit_n = st.unit().get_item_count();
    if (bld_n > 0) {
        s_scratch_bld = new BitArrayCL(bld_n);
    }
    if (wonder_n > 0) {
        s_scratch_wonder = new BitArrayCL(wonder_n);
    }
    if (sw_n > 0) {
        s_scratch_sw = new BitArrayCL(sw_n);
    }
    if (unit_n > 0) {
        s_scratch_unit = new BitArrayCL(unit_n);
    }
    const u16 trait_n = st.civ_trait().get_item_count();
    if (trait_n > 0) {
        s_civ_trait_scratch = new BitArrayCL(trait_n);
    }
    s_flag_has_wonder = find_toggle_city_idx("HAS_WONDER");
    s_flag_has_wonder_small = find_toggle_city_idx("HAS_WONDER_SMALL");
    s_wonder_n = wonder_n;
    s_small_wonder_n = sw_n;
}

void City::clear_assess_scratch () {
    delete s_scratch_bld;
    delete s_scratch_wonder;
    delete s_scratch_sw;
    delete s_scratch_unit;
    delete s_civ_trait_scratch;
    s_scratch_bld = nullptr;
    s_scratch_wonder = nullptr;
    s_scratch_sw = nullptr;
    s_scratch_unit = nullptr;
    s_civ_trait_scratch = nullptr;
}

void City::bind_banks (GeneralBitBank* flags, GeneralBitBank* resources, GeneralBitBank* buildings) {
    s_flag_bank = flags;
    s_res_bank = resources;
    s_bld_bank = buildings;
}

void City::bind_units (UnitAddVector* units) {
    s_units = units;
}

void City::bind_wonder_cities (u16* wonder_city) {
    s_wonder_city = wonder_city;
}

void City::bind_player_states (PlayerState* states, u16 player_n) {
    s_player_states = states;
    s_player_n = player_n;
}

BitArrayCL* City::get_buildable_buildings (u16 city_idx, BitArrayCL* techs, BitArrayCL* civ) const {
    BitArrayCL* r = s_scratch_bld;
    GAME_EXPECT_RET(r != nullptr, nullptr, "City building assess scratch");
    GAME_EXPECT_RET(s_statics != nullptr, nullptr, "City statics");
    r->clear_all();
    const u16 n = static_cast<u16>(r->get_count());
    AssessorCtx ctx = make_city_ctx(*this, city_idx, techs, civ);
    for (u16 i = 0; i < n; ++i) {
        const BuildingStaticDataStruct& item = s_statics->building().get_item(BuildingStaticDataKey::from_raw(i));
        if (GeneralAssessor::chk(item.reqs, ctx)) {
            r->set_bit(i);
        }
    }
    return r;
}

BitArrayCL* City::get_buildable_wonders (u16 city_idx, BitArrayCL* techs, BitArrayCL* civ) const {
    BitArrayCL* r = s_scratch_wonder;
    GAME_EXPECT_RET(r != nullptr, nullptr, "City wonder assess scratch");
    GAME_EXPECT_RET(s_statics != nullptr, nullptr, "City statics");
    r->clear_all();
    const u16 n = static_cast<u16>(r->get_count());
    AssessorCtx ctx = make_city_ctx(*this, city_idx, techs, civ);
    for (u16 i = 0; i < n; ++i) {
        if (s_wonder_city != nullptr && s_wonder_city[i] != U16_KEY_NULL) {
            continue;
        }
        const WonderStaticDataStruct& item = s_statics->wonder().get_item(WonderStaticDataKey::from_raw(i));
        if (GeneralAssessor::chk(item.reqs, ctx)) {
            r->set_bit(i);
        }
    }
    return r;
}

BitArrayCL* City::get_buildable_small_wonders (u16 city_idx, BitArrayCL* techs, BitArrayCL* civ) const {
    BitArrayCL* r = s_scratch_sw;
    GAME_EXPECT_RET(r != nullptr, nullptr, "City small wonder assess scratch");
    GAME_EXPECT_RET(s_statics != nullptr, nullptr, "City statics");
    const u16* built = small_wonder_row(m_owner);
    r->clear_all();
    const u16 n = static_cast<u16>(r->get_count());
    AssessorCtx ctx = make_city_ctx(*this, city_idx, techs, civ);
    for (u16 i = 0; i < n; ++i) {
        if (built != nullptr && built[i] != U16_KEY_NULL) {
            continue;
        }
        const SmallWonderStaticDataStruct& item = s_statics->small_wonder().get_item(SmallWonderStaticDataKey::from_raw(i));
        if (GeneralAssessor::chk(item.reqs, ctx)) {
            r->set_bit(i);
        }
    }
    return r;
}

BitArrayCL* City::get_trainable_units (u16 city_idx, BitArrayCL* techs, BitArrayCL* civ) const {
    BitArrayCL* r = s_scratch_unit;
    GAME_EXPECT_RET(r != nullptr, nullptr, "City unit assess scratch");
    GAME_EXPECT_RET(s_statics != nullptr, nullptr, "City statics");
    r->clear_all();
    const u16 n = static_cast<u16>(r->get_count());
    AssessorCtx ctx = make_city_ctx(*this, city_idx, techs, civ);
    for (u16 i = 0; i < n; ++i) {
        const UnitStaticDataStruct& item = s_statics->unit().get_item(UnitStaticDataKey::from_raw(i));
        if (GeneralAssessor::chk(item.reqs, ctx)) {
            r->set_bit(i);
        }
    }
    return r;
}

//================================================================================================================================
//=> - City build instructions -
//================================================================================================================================

static cstr cur_bld_name (u8 build_type, u16 bld_idx) {
    if (build_type == ACCUMULATE_COMMERCE) {
        return "Wealth";
    }
    if (s_statics == nullptr || bld_idx == U16_KEY_NULL) {
        return "";
    }
    if (build_type == BUILD_TYPE_BUILDING) {
        return s_statics->building().get_name(BuildingStaticDataKey::from_raw(bld_idx));
    }
    if (build_type == BUILD_TYPE_WONDER) {
        return s_statics->wonder().get_name(WonderStaticDataKey::from_raw(bld_idx));
    }
    if (build_type == BUILD_TYPE_SMALL_WONDER) {
        return s_statics->small_wonder().get_name(SmallWonderStaticDataKey::from_raw(bld_idx));
    }
    if (build_type == BUILD_TYPE_UNIT) {
        return s_statics->unit().get_name(UnitStaticDataKey::from_raw(bld_idx));
    }
    return "";
}

void City::build_building (u16 building_idx) {
    m_build_type = BUILD_TYPE_BUILDING;
    m_bld_idx = building_idx;
    m_build_cost = static_cast<u16>(s_statics->building().get_item(BuildingStaticDataKey::from_raw(building_idx)).cost);
    LOG_CITY_BUILD((cur_bld_name(m_build_type, m_bld_idx), 1, 0));
}

void City::build_wonder (u16 wonder_idx) {
    m_build_type = BUILD_TYPE_WONDER;
    m_bld_idx = wonder_idx;
    m_build_cost = static_cast<u16>(s_statics->wonder().get_item(WonderStaticDataKey::from_raw(wonder_idx)).cost);
    LOG_CITY_BUILD((cur_bld_name(m_build_type, m_bld_idx), 1, 0));
}

void City::build_small_wonder (u16 small_wonder_idx) {
    m_build_type = BUILD_TYPE_SMALL_WONDER;
    m_bld_idx = small_wonder_idx;
    m_build_cost = static_cast<u16>(s_statics->small_wonder().get_item(SmallWonderStaticDataKey::from_raw(small_wonder_idx)).cost);
    LOG_CITY_BUILD((cur_bld_name(m_build_type, m_bld_idx), 1, 0));
}

void City::build_unit (u16 unit_idx) {
    m_build_type = BUILD_TYPE_UNIT;
    m_bld_idx = unit_idx;
    m_build_cost = static_cast<u16>(s_statics->unit().get_item(UnitStaticDataKey::from_raw(unit_idx)).cost);
    if (unit_is_worker(unit_idx)) {
        m_misc.m_city_has_worker = 1;
    }
    LOG_CITY_BUILD((cur_bld_name(m_build_type, m_bld_idx), 1, 0));
}

void City::accumulate_commerce () {
    m_build_type = ACCUMULATE_COMMERCE;
    m_bld_idx = U16_KEY_NULL;
    m_build_cost = 0;
    LOG_CITY_BUILD((cur_bld_name(m_build_type, m_bld_idx), 1, 0));
}

//================================================================================================================================
//=> - City add yields in furtherance of builds / bank -
//================================================================================================================================

i16 City::add_food (u16 city_idx, u16 amount, i16 net_sanitation) {
    const EffectCtx ctx = make_city_effect_ctx(*this, city_idx);
    LOG_CITY_SAN((net_sanitation));
    LOG_CITY_SCI((0));
    LOG_CITY_REL((0));

    // Pull local tile food yield (no POP_GROWTH on gross food)
    const TileYield cy = TileYields::get(m_x, m_y);
    amount = static_cast<u16>(cy.m_food + amount);
    const DynBoosterRegister& food_reg = s_statics->dyn_booster();
    amount = apply_booster_u16(amount, food_reg.determine(ItemEffectBoosterType::FOOD, ItemEffectsScope::LOCAL, ctx));
    amount = apply_booster_u16(amount, food_reg.determine(ItemEffectBoosterType::FOOD, ItemEffectsScope::CITY, ctx));
    amount = apply_booster_u16(amount, food_reg.determine(ItemEffectBoosterType::FOOD, ItemEffectsScope::CIV, ctx));
    LOG_CITY_FOOD((amount));

    // Feed the population; 2 food per pop per turn; bank toward growth at 20; starve at most 1 per turn
    i16 food_surplus = static_cast<i16>(amount) - static_cast<i16>(m_pop_count * 2);
    if (net_sanitation < 0) {
        food_surplus = static_cast<i16>(food_surplus + net_sanitation);
    }
    
    // If we now have a surplus, apply growth boosters
    if (food_surplus > 0) {
        u16 scaled = static_cast<u16>(food_surplus);
        scaled = apply_booster_u16(scaled, LocalPopGrowthBoosterRegister::determine_effect(ctx));
        scaled = apply_booster_u16(scaled, CityPopGrowthBoosterRegister::determine_effect(ctx));
        scaled = apply_booster_u16(scaled, CivPopGrowthBoosterRegister::determine_effect(ctx));
        food_surplus = static_cast<i16>(scaled);
    }

    // Bank the food, and determine the population change
    i16 bank = static_cast<i16>(m_accumulated_food) + food_surplus;
    i16 pop_change = 0;
    if (bank < 0) {
        if (m_pop_count > 1) {
            m_pop_count--;
            pop_change = -1;
        }
        bank = 0;
    } else {
        while (bank >= 20) {
            bank = static_cast<i16>(bank - 20);
            m_pop_count++;
            pop_change++;
        }
    }
    m_accumulated_food = static_cast<i8>(bank);
    LOG_CITY_POP((m_pop_count));
    return pop_change;
}

bool City::add_production (u16 city_idx, u16 amount) {
    const EffectCtx ctx = make_city_effect_ctx(*this, city_idx);

    // Pull local tile production yield, then PRODUCTION boosters
    const TileYield cy = TileYields::get(m_x, m_y);
    amount = static_cast<u16>(cy.m_production + amount);
    const DynBoosterRegister& prod_reg = s_statics->dyn_booster();
    amount = apply_booster_u16(amount, prod_reg.determine(ItemEffectBoosterType::PRODUCTION, ItemEffectsScope::LOCAL, ctx));
    amount = apply_booster_u16(amount, prod_reg.determine(ItemEffectBoosterType::PRODUCTION, ItemEffectsScope::CITY, ctx));
    amount = apply_booster_u16(amount, prod_reg.determine(ItemEffectBoosterType::PRODUCTION, ItemEffectsScope::CIV, ctx));
    m_accumulated_production = static_cast<u16>(m_accumulated_production + amount);
    LOG_CITY_PROD((amount));
    
    // Do not trigger build-is-done if we are not building a building, or if we are converting production to commerce
    if (m_build_type == BUILD_TYPE_NONE || m_build_type == ACCUMULATE_COMMERCE) {
        return false;
    }

    // Trigger build-is-done if we are missing a building, or if we have enough production to build the building
    return m_bld_idx == U16_KEY_NULL || m_accumulated_production >= m_build_cost;
}

void City::add_commerce (u16 city_idx, u16 amount) {
    const EffectCtx ctx = make_city_effect_ctx(*this, city_idx);

    // Pull local tile commerce yield, and apply local boosters
    const TileYield cy = TileYields::get(m_x, m_y);
    const u16 local = apply_booster_u16(cy.m_commerce, LocalCommerceBoosterRegister::determine_effect(ctx));
    amount = static_cast<u16>(amount + local);

    // Handle the remaining commerce, and the non-local boosters
    const BoosterRegisterResult boost = CityCommerceBoosterRegister::determine_effect(ctx);
    const u16 commerce = apply_booster_u16(amount, boost);

    // Add the commerce to the player ledger
    if (!PlayerLedger::add_commerce(m_owner, commerce)) {
        GAME_EXPECT(false, "City commerce ledger");
    }
    LOG_CITY_COM((commerce));
}

void City::add_culture (u16 city_idx, u16 amount) {
    GAME_EXPECT(s_statics != nullptr, "City add_culture statics");
    const EffectCtx ctx = make_city_effect_ctx(*this, city_idx);

    // Pull local tile culture yield, and apply local boosters
    const u16 local = apply_booster_u16(amount, LocalCultureBoosterRegister::determine_effect(ctx));
    amount = static_cast<u16>(amount + local);
    amount = static_cast<u16>(amount + s_statics->config().get_default_culture_per_turn());

    // Handle the remaining culture, and the non-local boosters
    const u16 boosted = apply_booster_u16(amount, CityCultureBoosterRegister::determine_effect(ctx));
    const u16 new_culture = static_cast<u16>(m_culture + boosted);
    LOG_CITY_CULT((boosted));
    
    // Check if the city will expand its borders with the new culture amount
    if (CityBorder::will_expand(m_culture, new_culture)) {
        CityBorder::claim_expand(m_x, m_y, m_culture, new_culture, static_cast<u8>(m_owner));
    }
    m_culture = new_culture;
}

//================================================================================================================================
//=> - City getters -
//================================================================================================================================

i16 City::get_city_net_sanitation (u16 sanitation_boost) const {
    const i16 net = static_cast<i16>(static_cast<i32>(sanitation_boost) - static_cast<i32>(m_pop_count));
    if (net >= 0) {
        return net;
    }
    if (net >= -5) {
        const i32 mag = static_cast<i32>(-net);
        return static_cast<i16>(-(mag * mag));
    }
    return static_cast<i16>(net * 5);
}

u16 City::get_city_sanitation_boost (u16 city_idx) const {
    const EffectCtx ctx = make_city_effect_ctx(*this, city_idx);
    u16 amount = apply_booster_u16(3, LocalSanitationBoosterRegister::determine_effect(ctx));
    amount = apply_booster_u16(amount, CitySanitationBoosterRegister::determine_effect(ctx));
    amount = apply_booster_u16(amount, CivSanitationBoosterRegister::determine_effect(ctx));
    return amount;
}

u16 City::get_current_food_store () const {
    return m_accumulated_food;
}

u16 City::get_current_production_store () const {
    return m_accumulated_production;
}

u16 City::get_current_culture () const {
    return m_culture;
}

void City::set_culture (u16 culture) {
    m_culture = culture;
}

u16 City::get_current_population () const {
    return m_pop_count;
}

void City::set_population (u16 pop) {
    m_pop_count = pop;
}

u16 City::get_owner () const {
    return m_owner;
}

void City::set_owner (u16 owner) {
    m_owner = owner;
}

u16 City::get_x () const {
    return m_x;
}

u16 City::get_y () const {
    return m_y;
}

bool City::is_frontier () const {
    return m_is_frontier_city != 0;
}

void City::city_no_longer_frontier () {
    m_is_frontier_city = 0;
}

bool City::city_has_worker () const {
    return m_misc.m_city_has_worker != 0;
}

u8 City::get_tile_imp_count () const {
    return static_cast<u8>(m_misc.m_tile_imp_count);
}

void City::set_tile_imp_count (u8 n) {
    m_misc.m_tile_imp_count = n;
}

void City::add_tile_imp_count (i16 d) {
    i32 n = static_cast<i32>(m_misc.m_tile_imp_count) + static_cast<i32>(d);
    if (n < 0) {
        n = 0;
    }
    if (n > 255) {
        n = 255;
    }
    m_misc.m_tile_imp_count = static_cast<u8>(n);
}

void City::refresh_unit_support (u16 city_idx) {
    GAME_EXPECT(s_statics != nullptr, "City::refresh_unit_support null statics");
    GAME_EXPECT(s_player_states != nullptr, "City::refresh_unit_support null player states");
    GAME_EXPECT(m_owner < s_player_n, "City::refresh_unit_support owner out of bounds");
    const EffectCtx ctx = make_city_effect_ctx(*this, city_idx);
    const DynBoosterRegister& reg = s_statics->dyn_booster();
    u16 city_land = apply_booster_u16(0, reg.determine(ItemEffectBoosterType::CITY_LAND_UNIT_SUPPORT, ItemEffectsScope::CITY, ctx));
    u16 city_naval = apply_booster_u16(0, reg.determine(ItemEffectBoosterType::CITY_NAVAL_UNIT_SUPPORT, ItemEffectsScope::CITY, ctx));
    if (city_land > 127u) {
        city_land = 127u;
    }
    if (city_naval > 127u) {
        city_naval = 127u;
    }
    m_misc.m_free_land_unit_support = city_land;
    m_misc.m_free_naval_unit_support = city_naval;
    const u16 civ_land = apply_booster_u16(0, reg.determine(ItemEffectBoosterType::CIV_LAND_UNIT_SUPPORT, ItemEffectsScope::CIV, ctx));
    const u16 civ_naval = apply_booster_u16(0, reg.determine(ItemEffectBoosterType::CIV_NAVAL_UNIT_SUPPORT, ItemEffectsScope::CIV, ctx));
    PlayerState& ps = s_player_states[m_owner];
    const u32 land_sum = static_cast<u32>(ps.m_free_land_unit_support) + static_cast<u32>(civ_land);
    const u32 naval_sum = static_cast<u32>(ps.m_free_naval_unit_support) + static_cast<u32>(civ_naval);
    ps.m_free_land_unit_support = land_sum > 65535u ? 65535u : static_cast<u16>(land_sum);
    ps.m_free_naval_unit_support = naval_sum > 65535u ? 65535u : static_cast<u16>(naval_sum);
}

u16 City::calc_city_land_unit_support (u16 city_idx) const {
    GAME_EXPECT(s_statics != nullptr, "City::calc_city_land_unit_support null statics");
    const EffectCtx ctx = make_city_effect_ctx(*this, city_idx);
    const DynBoosterRegister& reg = s_statics->dyn_booster();
    u16 city_land = apply_booster_u16(0, reg.determine(ItemEffectBoosterType::CITY_LAND_UNIT_SUPPORT, ItemEffectsScope::CITY, ctx));
    if (city_land > 127u) {
        city_land = 127u;
    }
    return city_land;
}

u16 City::calc_city_naval_unit_support (u16 city_idx) const {
    GAME_EXPECT(s_statics != nullptr, "City::calc_city_naval_unit_support null statics");
    const EffectCtx ctx = make_city_effect_ctx(*this, city_idx);
    const DynBoosterRegister& reg = s_statics->dyn_booster();
    u16 city_naval = apply_booster_u16(0, reg.determine(ItemEffectBoosterType::CITY_NAVAL_UNIT_SUPPORT, ItemEffectsScope::CITY, ctx));
    if (city_naval > 127u) {
        city_naval = 127u;
    }
    return city_naval;
}

void City::count_unit_build_support (PlayerState* ps) {
    GAME_EXPECT(ps != nullptr, "City::count_unit_build_support null player state");
    if (m_build_type != BUILD_TYPE_UNIT || m_bld_idx == U16_KEY_NULL) {
        return;
    }
    const u16 cost = 1u;
    if (unit_is_settler(m_bld_idx)) {
        const u32 sn = static_cast<u32>(ps->m_this_turn_settler_build_n) + 1u;
        ps->m_this_turn_settler_build_n = sn > 65535u ? 65535u : static_cast<u16>(sn);
    }
    if (unit_is_land(m_bld_idx)) {
        const u32 sum = static_cast<u32>(ps->m_this_turn_new_land_unit_build_support) + static_cast<u32>(cost);
        ps->m_this_turn_new_land_unit_build_support = sum > 65535u ? 65535u : static_cast<u16>(sum);
        return;
    }
    if (unit_is_sea(m_bld_idx)) {
        const u32 sum = static_cast<u32>(ps->m_this_turn_new_naval_unit_build_support) + static_cast<u32>(cost);
        ps->m_this_turn_new_naval_unit_build_support = sum > 65535u ? 65535u : static_cast<u16>(sum);
    }
}

void City::refund_land_unit_upkeep (const UnitAddStruct& unit, PlayerState* ps) {
    (void)unit;
    GAME_EXPECT(ps != nullptr, "City::refund_land_unit_upkeep null player state");
    const u16 cost = 1u;
    u16 free_n = static_cast<u16>(m_misc.m_free_land_unit_support);
    u16 covered = cost;
    if (covered > free_n) {
        covered = free_n;
    }
    m_misc.m_free_land_unit_support = static_cast<u16>(free_n - covered);
    const u16 left = static_cast<u16>(cost - covered);
    if (left == 0) {
        return;
    }
    const u32 sum = static_cast<u32>(ps->m_land_unit_upkeep_needed) + static_cast<u32>(left);
    ps->m_land_unit_upkeep_needed = sum > 65535u ? 65535u : static_cast<u16>(sum);
}

void City::refund_naval_unit_upkeep (const UnitAddStruct& unit, PlayerState* ps) {
    (void)unit;
    GAME_EXPECT(ps != nullptr, "City::refund_naval_unit_upkeep null player state");
    const u16 cost = 1u;
    u16 free_n = static_cast<u16>(m_misc.m_free_naval_unit_support);
    u16 covered = cost;
    if (covered > free_n) {
        covered = free_n;
    }
    m_misc.m_free_naval_unit_support = static_cast<u16>(free_n - covered);
    const u16 left = static_cast<u16>(cost - covered);
    if (left == 0) {
        return;
    }
    const u32 sum = static_cast<u32>(ps->m_naval_unit_upkeep_needed) + static_cast<u32>(left);
    ps->m_naval_unit_upkeep_needed = sum > 65535u ? 65535u : static_cast<u16>(sum);
}

u16 City::get_free_land_unit_support () const {
    return static_cast<u16>(m_misc.m_free_land_unit_support);
}

u16 City::get_free_naval_unit_support () const {
    return static_cast<u16>(m_misc.m_free_naval_unit_support);
}

u16 City::get_defense_deduction () const {
    return static_cast<u16>(m_misc.m_city_defense_deduction);
}

void City::set_defense_deduction (u16 v) {
    m_misc.m_city_defense_deduction = v;
}

void City::restore_defense () {
    u16 d = static_cast<u16>(m_misc.m_city_defense_deduction);
    if (d == 0u) {
        return;
    }
    u16 cut = static_cast<u16>((static_cast<u32>(d) * 20u) / 100u);
    if (cut < 10u) {
        cut = 10u;
    }
    if (cut > d) {
        cut = d;
    }
    m_misc.m_city_defense_deduction = static_cast<u16>(d - cut);
}

void City::refresh_city_worker_flags (GameState& state, u16 player) {
    GAME_EXPECT(state.m_player_states != nullptr, "City::refresh_city_worker_flags null player states");
    GAME_EXPECT(player < state.m_player_n, "City::refresh_city_worker_flags player out of bounds");
    PlayerState& ps = state.m_player_states[player];
    if (ps.m_tech_just_researched == 0) {
        return;
    }
    ps.m_tech_just_researched = 0;
}

//================================================================================================================================
//=> - City finish build -
//================================================================================================================================

bool City::finish_if_ready (u16 city_idx) {
    if (m_build_type == BUILD_TYPE_NONE) {
        return false;
    }
    if (m_build_type != ACCUMULATE_COMMERCE) {
        if (m_bld_idx == U16_KEY_NULL) {
            return false;
        }
        if (m_accumulated_production < m_build_cost) {
            return false;
        }
    }
    switch (m_build_type) {
        case BUILD_TYPE_BUILDING: {
            GAME_EXPECT_RET(s_bld_bank != nullptr, false, "City building bank");
            LOG_CITY_BUILD((cur_bld_name(m_build_type, m_bld_idx), 0, 1));
            m_accumulated_production = static_cast<u16>(m_accumulated_production - m_build_cost);
            s_bld_bank->set_flag(city_idx, m_bld_idx);
            m_build_type = BUILD_TYPE_NONE;
            m_bld_idx = U16_KEY_NULL;
            m_build_cost = 0;
            return true;
        }
        case BUILD_TYPE_WONDER: {
            GAME_EXPECT_RET(s_wonder_city != nullptr, false, "City wonder cities");
            LOG_CITY_BUILD((cur_bld_name(m_build_type, m_bld_idx), 0, 1));
            m_accumulated_production = static_cast<u16>(m_accumulated_production - m_build_cost);
            s_wonder_city[m_bld_idx] = city_idx;
            set_toggle_city(city_idx, s_flag_has_wonder);
            m_build_type = BUILD_TYPE_NONE;
            m_bld_idx = U16_KEY_NULL;
            m_build_cost = 0;
            return true;
        }
        case BUILD_TYPE_SMALL_WONDER: {
            u16* built = small_wonder_row(m_owner);
            GAME_EXPECT_RET(built != nullptr, false, "City small wonder cities");
            LOG_CITY_BUILD((cur_bld_name(m_build_type, m_bld_idx), 0, 1));
            m_accumulated_production = static_cast<u16>(m_accumulated_production - m_build_cost);
            built[m_bld_idx] = city_idx;
            set_toggle_city(city_idx, s_flag_has_wonder_small);
            m_build_type = BUILD_TYPE_NONE;
            m_bld_idx = U16_KEY_NULL;
            m_build_cost = 0;
            return true;
        }
        case BUILD_TYPE_UNIT: {
            GAME_EXPECT_RET(s_units != nullptr, false, "City units");
            const UnitAddKey unit_key = s_units->get_next_new_unit_add_key();
            GAME_EXPECT_RET(unit_key.is_valid(), false, "City unit pool");
            UnitAddStruct* unit = s_units->get_unit_add(unit_key);
            if (unit == nullptr) {
                s_units->return_unit_add(unit_key);
                GAME_EXPECT(false, "City unit add");
                return false;
            }
            const u16 typ = m_bld_idx;
            LOG_CITY_BUILD((cur_bld_name(m_build_type, m_bld_idx), 0, 1));
            unit->m_x = m_x;
            unit->m_y = m_y;
            unit->m_player_idx = m_owner;
            unit->m_unit_typ_idx = typ;
            unit->m_next_unit_on_tile = U16_KEY_NULL;
            unit->m_next_unit_in_group = U16_KEY_NULL;
            unit->m_mvt_points = 0;
            unit->m_health = UNIT_HEALTH;
            unit->m_level = spawn_unit_level(*this, city_idx, typ);
            m_accumulated_production = static_cast<u16>(m_accumulated_production - m_build_cost);
            m_build_type = BUILD_TYPE_NONE;
            m_bld_idx = U16_KEY_NULL;
            m_build_cost = 0;
            UnitMovementMng::finish_unit_spawn(unit_key, m_x, m_y, m_owner);
            if (unit_is_worker(typ)) {
                m_misc.m_city_has_worker = 1;
                WorkerHelper::set_data(unit, city_idx);
            }
            return true;
        }
        case ACCUMULATE_COMMERCE: {
            if (m_accumulated_production < 2) {
                return false;
            }
            const u16 base = static_cast<u16>(m_accumulated_production / 2);
            const EffectCtx ctx = make_city_effect_ctx(*this, city_idx);
            const BoosterRegisterResult boost = CityCommerceBoosterRegister::determine_effect(ctx);
            const u16 commerce = apply_booster_u16(base, boost);
            if (commerce == 0) {
                m_accumulated_production = 0;
                return false;
            }
            if (!PlayerLedger::add_commerce(m_owner, commerce)) {
                GAME_EXPECT(false, "City commerce ledger");
            }
            m_accumulated_production = 0;
            return false;
        }
        default: {
            return false;
        }
    }
    return false;
}

bool City::has_building (u16 city_idx, u16 building_idx) const {
    GAME_EXPECT_RET(s_bld_bank != nullptr, false, "City building bank");
    return s_bld_bank->is_flagged(city_idx, building_idx);
}

bool City::need_prod_pick () const {
    return m_build_type == BUILD_TYPE_NONE || m_build_type == ACCUMULATE_COMMERCE;
}

void City::set_conn_city (u16 city_idx, u8 dir) {
    GAME_EXPECT(dir < 4u, "City::set_conn_city bad dir");
    const u8 sh = static_cast<u8>(dir * 2u);
    if (city_idx == U16_KEY_NULL) {
        m_road_conn = static_cast<u8>(m_road_conn & ~(0x3u << sh));
    }
    if (dir == 0u) {
        m_conn_city_ne = city_idx;
    } else if (dir == 1u) {
        m_conn_city_nw = city_idx;
    } else if (dir == 2u) {
        m_conn_city_se = city_idx;
    } else {
        m_conn_city_sw = city_idx;
    }
}

u16 City::get_conn_city (u8 dir) const {
    GAME_EXPECT_RET(dir < 4u, U16_KEY_NULL, "City::get_conn_city bad dir");
    if (dir == 0u) {
        return m_conn_city_ne;
    }
    if (dir == 1u) {
        return m_conn_city_nw;
    }
    if (dir == 2u) {
        return m_conn_city_se;
    }
    return m_conn_city_sw;
}

bool City::is_conn_city_locked (u8 dir) const {
    GAME_EXPECT_RET(dir < 4u, false, "City::is_conn_city_locked bad dir");
    return ((m_road_conn >> (dir * 2u)) & 0x1u) != 0u;
}

bool City::is_conn_city_built (u8 dir) const {
    GAME_EXPECT_RET(dir < 4u, false, "City::is_conn_city_built bad dir");
    return ((m_road_conn >> (dir * 2u + 1u)) & 0x1u) != 0u;
}

void City::conn_city_is_locked (u8 dir) {
    GAME_EXPECT(dir < 4u, "City::conn_city_is_locked bad dir");
    m_road_conn = static_cast<u8>(m_road_conn | (0x1u << (dir * 2u)));
}

void City::conn_city_is_built (u8 dir) {
    GAME_EXPECT(dir < 4u, "City::conn_city_is_built bad dir");
    m_road_conn = static_cast<u8>(m_road_conn | (0x2u << (dir * 2u)));
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
