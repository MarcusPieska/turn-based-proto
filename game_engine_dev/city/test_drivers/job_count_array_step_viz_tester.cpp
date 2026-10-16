//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "city_job_static_data.h"
#include "city_job_static_key.h"
#include "job_count_array.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static const u16 k_peers = 8;

static bool all_zero (u16 city_idx) {
    const u16 jn = JobCountArray::job_n();
    for (u16 j = 0; j < jn; ++j) {
        if (JobCountArray::get(city_idx, j) != 0) {
            return false;
        }
    }
    return true;
}

static bool peers_zero (const u16* peers, u16 tgt) {
    for (u16 p = 0; p < k_peers; ++p) {
        if (peers[p] == tgt) {
            continue;
        }
        if (!all_zero(peers[p])) {
            return false;
        }
    }
    return true;
}

//================================================================================================================================
//=> - Layout viz -
//================================================================================================================================

static void print_layout (u16 city_idx, u16 sel) {
    static const cstr k_red = "\033[31m";
    static const cstr k_grn = "\033[32m";
    static const cstr k_blu = "\033[34m";
    static const cstr k_rst = "\033[0m";
    const u16 jn = JobCountArray::job_n();
    const JobCountSlot* slots = JobCountArray::slots();
    u32 col = 0;
    for (u16 j = 0; j < jn; ++j) {
        const JobCountSlot& s = slots[j];
        const u8 val = JobCountArray::get(city_idx, j);
        const cstr tint = (j == sel) ? k_blu : ((j & 1u) ? k_grn : k_red);
        for (u8 bi = 0; bi < s.m_wid; ++bi) {
            if (col > 0) {
                if ((col % 80u) == 0u) {
                    std::printf("\n");
                } else if ((col % 8u) == 0u) {
                    std::printf(" ");
                }
            }
            std::printf("%s%u%s", tint, SC_U32((val >> bi) & 1u), k_rst);
            ++col;
        }
    }
    if (col > 0) {
        std::printf("\n");
    }
}

static bool sweep_job (u16 city_idx, u16 sel, const u16* peers) {
    const u16 jn = JobCountArray::job_n();
    u8 snap[64];
    if (jn > 64) {
        std::printf("*** FAILED job_n too large\n");
        return false;
    }
    for (u16 j = 0; j < jn; ++j) {
        JobCountArray::set(city_idx, j, 0);
        snap[j] = 0;
    }
    if (!peers_zero(peers, city_idx)) {
        std::printf("*** FAILED peers not zero before sweep job %u\n", SC_U32(sel));
        return false;
    }
    const u8 vmax = SC_U8(JobCountArray::max_n(sel) + 1u);
    for (u16 v = 0; v < vmax; ++v) {
        const u8 uv = SC_U8(v);
        JobCountArray::set(city_idx, sel, uv);
        snap[sel] = uv;
        if (JobCountArray::get(city_idx, sel) != uv) {
            std::printf("*** FAILED get job %u expect %u got %u\n",
                SC_U32(sel), SC_U32(uv), SC_U32(JobCountArray::get(city_idx, sel)));
            return false;
        }
        for (u16 o = 0; o < jn; ++o) {
            if (o == sel) {
                continue;
            }
            if (JobCountArray::get(city_idx, o) != snap[o]) {
                std::printf("*** FAILED side job %u touched writing job %u=%u\n",
                    SC_U32(o), SC_U32(sel), SC_U32(uv));
                return false;
            }
        }
        if (!peers_zero(peers, city_idx)) {
            std::printf("*** FAILED peer batch non-zero writing job %u=%u\n",
                SC_U32(sel), SC_U32(uv));
            return false;
        }
        print_layout(city_idx, sel);
    }
    JobCountArray::set(city_idx, sel, 0);
    snap[sel] = 0;
    if (!all_zero(city_idx) || !peers_zero(peers, city_idx)) {
        std::printf("*** FAILED not clear after sweep job %u\n", SC_U32(sel));
        return false;
    }
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    RuntimeStaticLoader loader;
    if (!loader.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        std::printf("*** FAILED statics load\n");
        return 1;
    }
    RuntimeStatics& st = loader.statics();
    if (!JobCountArray::setup(st)) {
        std::printf("*** FAILED JobCountArray::setup\n");
        return 1;
    }
    u16 peers[k_peers];
    for (u16 p = 0; p < k_peers; ++p) {
        peers[p] = JobCountArray::claim();
        if (peers[p] == U16_KEY_NULL) {
            std::printf("*** FAILED claim peer %u\n", SC_U32(p));
            JobCountArray::clear();
            return 1;
        }
        const u16 jn = JobCountArray::job_n();
        for (u16 j = 0; j < jn; ++j) {
            JobCountArray::set(peers[p], j, 0);
        }
    }
    const u16 city_idx = peers[0];
    const u16 jn = JobCountArray::job_n();
    bool ok = true;
    for (u16 j = 0; j < jn && ok; ++j) {
        cstr nm = st.city_job().get_name(CityJobStaticDataKey::from_raw(j));
        std::printf("=== %s (job %u, max %u, wid %u) ===\n",
            nm ? nm : "?", SC_U32(j), SC_U32(JobCountArray::max_n(j)),
            SC_U32(JobCountArray::slots()[j].m_wid));
        ok = sweep_job(city_idx, j, peers);
    }
    if (ok) {
        std::printf("*** PASSED step viz + member/peer checks (%u jobs, %u peer batches)\n",
            SC_U32(jn), SC_U32(k_peers - 1u));
    }
    JobCountArray::clear();
    return ok ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
