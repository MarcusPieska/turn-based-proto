//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include "bit_array.h"
#include "civ_static_data.h"
#include "civ_static_key.h"
#include "civ_trait_enum.h"
#include "game_state.h"
#include "research_turn_handler.h"
#include "runtime_static_loader.h"
#include "tech_static_key.h"
#include "tech_trait_attribution.h"
#include "tech_trait_orderings.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/single-city-dev-test";
static const char* G_OUT_FILE = "tech_order_by_trait.txt";
static const u16 G_TRAIT_N = 7u;
static const u16 G_ORD_MAX = 512u;
static const u16 G_NAME_MAX = 64u;
static const u32 G_TURNS = 400u;
static const u16 G_IDLE_STOP = 8u;

static char g_ord[G_TRAIT_N][G_ORD_MAX][G_NAME_MAX];
static u16 g_ord_n[G_TRAIT_N];

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static RuntimeStaticLoader g_rt_loader;
static RuntimeStatics* g_rt_statics = nullptr;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static cstr trait_name (CivTrait t) {
    switch (t) {
    case CivTrait::Agricultural: return "Agricultural";
    case CivTrait::Industrious: return "Industrious";
    case CivTrait::Expansionist: return "Expansionist";
    case CivTrait::Religious: return "Religious";
    case CivTrait::Militaristic: return "Militaristic";
    case CivTrait::Scientific: return "Scientific";
    case CivTrait::Commercial: return "Commercial";
    }
    return "?";
}

static bool ensure_out_dir () {
    return ::mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST;
}

static bool ensure_statics () {
    if (g_rt_statics != nullptr) {
        return true;
    }
    if (!g_rt_loader.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        return false;
    }
    g_rt_statics = &g_rt_loader.statics();
    return g_rt_statics != nullptr;
}

static u16 find_civ_for_trait (const RuntimeStatics& st, CivTrait want) {
    const u16 n = st.civ().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        const CivStaticDataStruct& civ = st.civ().get_item(CivStaticDataKey::from_raw(i));
        if (static_cast<CivTrait>(civ.traits.indices[0]) == want) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static bool push_ord (u16 trait_i, cstr nm) {
    if (trait_i >= G_TRAIT_N || g_ord_n[trait_i] >= G_ORD_MAX) {
        return false;
    }
    char* dst = g_ord[trait_i][g_ord_n[trait_i]];
    if (nm == nullptr) {
        nm = "?";
    }
    std::snprintf(dst, G_NAME_MAX, "%s", nm);
    g_ord_n[trait_i] = static_cast<u16>(g_ord_n[trait_i] + 1u);
    return true;
}

static u16 col_width () {
    u16 w = 0;
    for (u16 t = 0; t < G_TRAIT_N; ++t) {
        const u16 tn = static_cast<u16>(std::strlen(trait_name(static_cast<CivTrait>(t))));
        if (tn > w) {
            w = tn;
        }
        for (u16 i = 0; i < g_ord_n[t]; ++i) {
            const u16 bn = static_cast<u16>(std::strlen(g_ord[t][i]));
            if (bn > w) {
                w = bn;
            }
        }
    }
    return w;
}

static void write_pad (std::FILE* f, cstr s, u16 w) {
    const u16 n = static_cast<u16>(std::strlen(s));
    std::fputs(s, f);
    for (u16 i = n; i < w; ++i) {
        std::fputc(' ', f);
    }
}

static void write_table (std::FILE* f) {
    const u16 w = col_width();
    for (u16 t = 0; t < G_TRAIT_N; ++t) {
        if (t > 0) {
            std::fputc(' ', f);
        }
        write_pad(f, trait_name(static_cast<CivTrait>(t)), w);
    }
    std::fputc('\n', f);
    u16 rows = 0;
    for (u16 t = 0; t < G_TRAIT_N; ++t) {
        if (g_ord_n[t] > rows) {
            rows = g_ord_n[t];
        }
    }
    for (u16 r = 0; r < rows; ++r) {
        for (u16 t = 0; t < G_TRAIT_N; ++t) {
            if (t > 0) {
                std::fputc(' ', f);
            }
            const cstr cell = (r < g_ord_n[t]) ? g_ord[t][r] : "";
            write_pad(f, cell, w);
        }
        std::fputc('\n', f);
    }
}

static void run_trait (CivTrait trait, u16 trait_i) {
    const u16 civ_idx = find_civ_for_trait(*g_rt_statics, trait);
    if (civ_idx == U16_KEY_NULL) {
        std::printf("no primary-trait civ for %s\n", trait_name(trait));
        return;
    }

    GameState state;
    state.m_statics = g_rt_statics;
    state.m_player_n = 1;
    state.m_player_states = new PlayerState[1];
    PlayerState& ps = state.m_player_states[0];
    ps.m_civ_index = civ_idx;
    ps.m_research_spending_perc = 100;
    ps.m_research = 0;
    ps.m_commerce = 0;
    ps.m_commerce_from_turn = 0;
    ps.m_current_research_target_idx = U16_KEY_NULL;
    ps.m_techs_researched = nullptr;

    const u16 tech_n = g_rt_statics->tech().get_item_count();
    BitArrayCL seen(tech_n);
    ResearchTurnHandler::begin(state);

    u16 idle = 0;
    for (u32 turn = 0; turn < G_TURNS; ++turn) {
        if (ps.m_current_research_target_idx != U16_KEY_NULL
            && ps.m_current_research_target_idx < tech_n) {
            const u32 cost = g_rt_statics->tech()
                .get_item(TechStaticDataKey::from_raw(ps.m_current_research_target_idx)).cost;
            if (ps.m_research < cost) {
                ps.m_research = cost;
            }
        }
        ResearchTurnHandler::handle(state, 0);
        bool finished = false;
        if (ps.m_techs_researched != nullptr) {
            for (u16 i = 0; i < tech_n; ++i) {
                if (ps.m_techs_researched->get_bit(i) == 0 || seen.get_bit(i) != 0) {
                    continue;
                }
                seen.set_bit(i);
                cstr nm = g_rt_statics->tech().get_name(TechStaticDataKey::from_raw(i));
                push_ord(trait_i, nm);
                finished = true;
            }
        }
        if (finished) {
            idle = 0;
        } else {
            idle = static_cast<u16>(idle + 1u);
            if (idle >= G_IDLE_STOP) {
                break;
            }
        }
        state.m_current_turn = turn + 1;
    }

    std::printf("%s civ=%u techs=%u\n", trait_name(trait), static_cast<u32>(civ_idx), static_cast<u32>(g_ord_n[trait_i]));
    state.clear();
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!ensure_out_dir()) {
        std::printf("out dir failed\n");
        return 1;
    }
    if (!ensure_statics()) {
        std::printf("statics failed\n");
        return 1;
    }
    for (u16 t = 0; t < G_TRAIT_N; ++t) {
        g_ord_n[t] = 0;
    }

    if (!TechTraitOrderings::begin(g_rt_statics->tech(), g_rt_statics->building())) {
        std::printf("TechTraitOrderings::begin failed\n");
        return 1;
    }

    for (u16 t = 0; t < G_TRAIT_N; ++t) {
        run_trait(static_cast<CivTrait>(t), t);
    }

    char path[320];
    std::snprintf(path, sizeof(path), "%s/%s", G_OUT_DIR, G_OUT_FILE);
    std::FILE* f = std::fopen(path, "w");
    if (f == nullptr) {
        std::printf("open failed: %s\n", path);
        TechTraitOrderings::clear();
        TechTraitAttribution::clear();
        return 1;
    }
    write_table(f);
    std::fclose(f);
    std::printf("wrote %s\n", path);

    TechTraitOrderings::clear();
    TechTraitAttribution::clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
