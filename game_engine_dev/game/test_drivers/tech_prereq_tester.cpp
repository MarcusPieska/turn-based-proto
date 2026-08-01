//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "assert_log.h"
#include "bit_array.h"
#include "item_reqs.h"
#include "runtime_static_loader.h"
#include "tech_static_data.h"
#include "tech_static_key.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static RuntimeStaticLoader g_rt_loader;
static RuntimeStatics* g_rt_statics = nullptr;

//================================================================================================================================
//=> - Types -
//================================================================================================================================

struct PrereqInfo {
    u16 m_idx;
    u16 m_prereq_n;
    u32 m_own_cost;
    u32 m_total_cost;
    u16* m_prereqs;
};

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

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

static cstr tech_name (u16 idx) {
    cstr nm = g_rt_statics->tech().get_name(TechStaticDataKey::from_raw(idx));
    return nm != nullptr ? nm : "?";
}

static void collect_prereqs (u16 tech_idx, const TechStaticDataStruct* items, u16 n, BitArrayCL& seen) {
    GAME_EXPECT(tech_idx < n, "tech_prereq_tester collect tech_idx");
    const ItemReqsStruct& reqs = items[tech_idx].reqs;
    for (u8 i = 0; i < MAX_PREREQ_COUNT; ++i) {
        if (reqs.types[i] != ITEM_REQ_TYPE_TECH) {
            continue;
        }
        const u16 p = reqs.indices[i];
        GAME_EXPECT(p < n, "tech_prereq_tester collect prereq");
        if (seen.get_bit(p) != 0) {
            continue;
        }
        seen.set_bit(p);
        collect_prereqs(p, items, n, seen);
    }
}

static int cost_cmp (const void* a, const void* b) {
    const PrereqInfo* pa = static_cast<const PrereqInfo*>(a);
    const PrereqInfo* pb = static_cast<const PrereqInfo*>(b);
    if (pa->m_total_cost < pb->m_total_cost) {
        return -1;
    }
    if (pa->m_total_cost > pb->m_total_cost) {
        return 1;
    }
    if (pa->m_idx < pb->m_idx) {
        return -1;
    }
    if (pa->m_idx > pb->m_idx) {
        return 1;
    }
    return 0;
}

static int u16_cost_cmp (const void* a, const void* b) {
    const u16* ia = static_cast<const u16*>(a);
    const u16* ib = static_cast<const u16*>(b);
    const u32 ca = g_rt_statics->tech().get_item(TechStaticDataKey::from_raw(*ia)).cost;
    const u32 cb = g_rt_statics->tech().get_item(TechStaticDataKey::from_raw(*ib)).cost;
    if (ca < cb) {
        return -1;
    }
    if (ca > cb) {
        return 1;
    }
    if (*ia < *ib) {
        return -1;
    }
    if (*ia > *ib) {
        return 1;
    }
    return 0;
}

//================================================================================================================================
//=> - TechPrereqTester -
//================================================================================================================================

class TechPrereqTester {
public:
    TechPrereqTester () = delete;

    static int run (bool extensive);
};

int TechPrereqTester::run (bool extensive) {
    if (!ensure_statics()) {
        std::printf("statics failed\n");
        return 1;
    }
    const u16 n = g_rt_statics->tech().get_item_count();
    GAME_EXPECT(n > 0, "tech_prereq_tester tech_n");
    const TechStaticDataStruct* items = &g_rt_statics->tech().get_item(TechStaticDataKey::from_raw(0));

    PrereqInfo* infos = new PrereqInfo[n];
    for (u16 t = 0; t < n; ++t) {
        BitArrayCL seen(n);
        collect_prereqs(t, items, n, seen);
        u16 pn = 0;
        for (u16 i = 0; i < n; ++i) {
            if (seen.get_bit(i) != 0) {
                pn = static_cast<u16>(pn + 1u);
            }
        }
        u16* plist = nullptr;
        if (pn > 0) {
            plist = new u16[pn];
            u16 w = 0;
            for (u16 i = 0; i < n; ++i) {
                if (seen.get_bit(i) == 0) {
                    continue;
                }
                plist[w] = i;
                w = static_cast<u16>(w + 1u);
            }
            std::qsort(plist, pn, sizeof(u16), u16_cost_cmp);
        }
        u32 total = items[t].cost;
        for (u16 i = 0; i < pn; ++i) {
            total = total + items[plist[i]].cost;
        }
        infos[t].m_idx = t;
        infos[t].m_prereq_n = pn;
        infos[t].m_own_cost = items[t].cost;
        infos[t].m_total_cost = total;
        infos[t].m_prereqs = plist;
    }

    std::qsort(infos, n, sizeof(PrereqInfo), cost_cmp);

    std::printf("=== tech prereq detail (cheapest total first) ===\n");
    for (u16 i = 0; i < n; ++i) {
        const PrereqInfo& info = infos[i];
        std::printf("\n%s  prereqs=%u  own=%u  total=%u\n",
            tech_name(info.m_idx),
            static_cast<u32>(info.m_prereq_n),
            info.m_own_cost,
            info.m_total_cost);
        for (u16 p = 0; p < info.m_prereq_n; ++p) {
            const u16 pix = info.m_prereqs[p];
            std::printf("  %s  cost=%u\n", tech_name(pix), items[pix].cost);
        }
        if (!extensive) {
            continue;
        }
        u16* skip = new u16[n];
        u16 sn = 0;
        for (u16 j = 0; j < n; ++j) {
            if (j == info.m_idx) {
                continue;
            }
            bool need = false;
            for (u16 p = 0; p < info.m_prereq_n; ++p) {
                if (info.m_prereqs[p] == j) {
                    need = true;
                    break;
                }
            }
            if (need) {
                continue;
            }
            skip[sn] = j;
            sn = static_cast<u16>(sn + 1u);
        }
        std::qsort(skip, sn, sizeof(u16), u16_cost_cmp);
        for (u16 s = 0; s < sn; ++s) {
            const u16 six = skip[s];
            std::printf("  \033[38;5;174m%s  cost=%u\033[0m\n", tech_name(six), items[six].cost);
        }
        delete[] skip;
    }

    std::printf("\n=== tech prereq summary (cheapest total first) ===\n");
    for (u16 i = 0; i < n; ++i) {
        const PrereqInfo& info = infos[i];
        std::printf("%-28s  prereqs=%3u  total=%6u\n",
            tech_name(info.m_idx),
            static_cast<u32>(info.m_prereq_n),
            info.m_total_cost);
    }

    BitArrayCL used_as_prereq(n);
    for (u16 t = 0; t < n; ++t) {
        const ItemReqsStruct& reqs = items[t].reqs;
        for (u8 i = 0; i < MAX_PREREQ_COUNT; ++i) {
            if (reqs.types[i] != ITEM_REQ_TYPE_TECH) {
                continue;
            }
            const u16 p = reqs.indices[i];
            GAME_EXPECT(p < n, "tech_prereq_tester leaf prereq");
            used_as_prereq.set_bit(p);
        }
    }
    u16* leaves = new u16[n];
    u16 ln = 0;
    for (u16 t = 0; t < n; ++t) {
        if (used_as_prereq.get_bit(t) != 0) {
            continue;
        }
        leaves[ln] = t;
        ln = static_cast<u16>(ln + 1u);
    }
    std::qsort(leaves, ln, sizeof(u16), u16_cost_cmp);
    std::printf("\n=== techs not required by any other tech (cheapest first) ===\n");
    for (u16 i = 0; i < ln; ++i) {
        const u16 t = leaves[i];
        std::printf("%-28s  cost=%6u\n", tech_name(t), items[t].cost);
    }

    delete[] leaves;
    for (u16 i = 0; i < n; ++i) {
        delete[] infos[i].m_prereqs;
    }
    delete[] infos;
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    bool extensive = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "ext") == 0 || std::strcmp(argv[i], "extensive") == 0) {
            extensive = true;
        }
    }
    return TechPrereqTester::run(extensive);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
