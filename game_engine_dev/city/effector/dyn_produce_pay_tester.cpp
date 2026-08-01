//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "booster_register_tester_shared.h"
#include "building_static_key.h"
#include "dyn_produce_register.h"
#include "item_effects.h"
#include "resource_static_key.h"
#include "runtime_static_loader.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static RuntimeStaticLoader g_rt_loader;
static int g_fails = 0;

static void note_fail (cstr msg) {
    ++g_fails;
    std::printf("FAIL: %s\n", msg);
}

static i16 coal_stock (const DynProduceRegister::Inventory& inv, u16 coal_dense) {
    if (inv.m_v == nullptr || coal_dense >= inv.m_n) {
        return 0;
    }
    return inv.m_v[coal_dense];
}

static i16 prod_stock (const DynProduceRegister::CityYields& yld) {
    return yld.m_v[static_cast<u16>(ItemProduceYield::PRODUCTION)];
}

static bool make_inv (DynProduceRegister::Inventory& inv, u16 n) {
    delete[] inv.m_v;
    inv.m_v = nullptr;
    inv.m_n = n;
    if (n == 0) {
        return true;
    }
    inv.m_v = new i16[n]();
    return inv.m_v != nullptr;
}

static void free_inv (DynProduceRegister::Inventory& inv) {
    delete[] inv.m_v;
    inv.m_v = nullptr;
    inv.m_n = 0;
}

static void run_case (cstr label, i16 start_coal, const DynProduceRegister& reg, BoosterRegisterToggleEnv& env,
    u16 coal_bld, u16 coal_dense) {
    DynProduceRegister::Inventory inv;
    DynProduceRegister::CityYields yld;
    if (!make_inv(inv, reg.inv_n())) {
        note_fail("make_inv");
        return;
    }
    for (u16 i = 0; i < inv.m_n; ++i) {
        inv.m_v[i] = 0;
    }
    if (coal_dense < inv.m_n) {
        inv.m_v[coal_dense] = start_coal;
    }
    reg.clear_yld(yld);

    env.clear_all();
    env.m_array.get_bld_bank()->set_flag(env.m_city_idx, coal_bld);
    const EffectCtx ctx = env.make_ctx();

    std::printf("%s  start coal=%d production=%d\n",
        label, static_cast<int>(coal_stock(inv, coal_dense)), static_cast<int>(prod_stock(yld)));
    for (u16 iter = 1; iter <= 10; ++iter) {
        const i16 coal_before = coal_stock(inv, coal_dense);
        const i16 prod_before = prod_stock(yld);
        reg.apply(inv, yld, ctx);
        const i16 coal_after = coal_stock(inv, coal_dense);
        const i16 prod_after = prod_stock(yld);
        std::printf("  iter %u  coal %d -> %d  production %d -> %d\n",
            static_cast<u32>(iter),
            static_cast<int>(coal_before), static_cast<int>(coal_after),
            static_cast<int>(prod_before), static_cast<int>(prod_after));
    }
    free_inv(inv);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!g_rt_loader.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        std::printf("statics failed\n");
        return 1;
    }
    RuntimeStatics& st = g_rt_loader.statics();
    const DynProduceRegister& reg = st.dyn_produce();

    u16 coal_bld = U16_KEY_NULL;
    for (u16 i = 0; i < st.building().get_item_count(); ++i) {
        cstr nm = st.building().get_name(BuildingStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, "Coal Plant") == 0) {
            coal_bld = i;
            break;
        }
    }
    if (coal_bld == U16_KEY_NULL) {
        note_fail("Coal Plant not found");
        return 1;
    }

    u16 coal_cat = U16_KEY_NULL;
    for (u16 i = 0; i < st.resource().get_item_count(); ++i) {
        cstr nm = st.resource().get_name(ResourceStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, "Coal") == 0) {
            coal_cat = i;
            break;
        }
    }
    if (coal_cat == U16_KEY_NULL) {
        note_fail("Coal resource not found");
        return 1;
    }
    const u16 coal_dense = reg.dense_of(coal_cat);
    if (coal_dense == U16_KEY_NULL) {
        note_fail("Coal not in dense produce inventory");
        return 1;
    }

    BoosterRegisterToggleEnv env;
    if (!env.bind(st)) {
        note_fail("toggle env bind");
        return 1;
    }

    std::printf("dyn_produce_pay  Coal Plant built, 10 applies per case\n");
    run_case("case 1 (coal=8)", 8, reg, env, coal_bld, coal_dense);
    run_case("case 2 (coal=4)", 4, reg, env, coal_bld, coal_dense);

    std::printf("fails=%d\n", g_fails);
    return g_fails == 0 ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
