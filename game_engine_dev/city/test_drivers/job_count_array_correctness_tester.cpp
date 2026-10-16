//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "city_job_static_data.h"
#include "city_job_static_key.h"
#include "dyn_job_slot_register.h"
#include "item_effects.h"
#include "job_count_array.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static const u16 k_peers = 8;
static const u16 k_jobs_cap = 64;

static u8 s_wid (u8 nv) {
    if (nv <= 1) {
        return 0;
    }
    u8 w = 0;
    for (u8 x = SC_U8(nv - 1); x != 0; x = SC_U8(x >> 1)) {
        ++w;
    }
    return w;
}

static u8 s_theor_max (const DynJobSlotRegister& reg, u16 job_id, u16 base) {
    i32 count_sum = 0;
    i32 pct_sum = 0;
    const DynJobSlotEntry* ents = reg.entries();
    const u16 en = reg.entry_count();
    for (u16 i = 0; i < en; ++i) {
        if (ents[i].m_job_id != job_id) {
            continue;
        }
        if (ents[i].m_mode == ItemEffectAmountMode::PERCENTAGE) {
            pct_sum += SC_I32(ents[i].m_amount);
        } else {
            count_sum += SC_I32(ents[i].m_amount);
        }
    }
    i32 total = SC_I32(base) + count_sum + (SC_I32(base) * pct_sum) / 100;
    if (total < 0) {
        total = 0;
    }
    if (total > 255) {
        total = 255;
    }
    return SC_U8(total);
}

static u8 pattern_val (u16 city, u16 job, u8 vmax) {
    if (vmax == 0) {
        return 0;
    }
    return SC_U8((SC_U32(city) * 17u + SC_U32(job) * 3u) % (SC_U32(vmax) + 1u));
}

static bool city_eq (u16 city_idx, const u8* snap, u16 jn) {
    for (u16 j = 0; j < jn; ++j) {
        if (JobCountArray::get(city_idx, j) != snap[j]) {
            return false;
        }
    }
    return true;
}

static void take_snap (u16 city_idx, u8* snap, u16 jn) {
    for (u16 j = 0; j < jn; ++j) {
        snap[j] = JobCountArray::get(city_idx, j);
    }
}

//================================================================================================================================
//=> - Layout max -
//================================================================================================================================

static bool check_layout (const RuntimeStatics& st) {
    const CityJobStaticData& jobs = st.city_job();
    const DynJobSlotRegister& reg = st.dyn_job_slot();
    const u16 jn = JobCountArray::job_n();
    if (jn != jobs.get_item_count()) {
        std::printf("*** FAILED job_n %u vs catalog %u\n",
            SC_U32(jn), SC_U32(jobs.get_item_count()));
        return false;
    }
    const JobCountSlot* slots = JobCountArray::slots();
    u32 bits = 0;
    for (u16 j = 0; j < jn; ++j) {
        const CityJobStaticDataStruct& row = jobs.get_item(CityJobStaticDataKey::from_raw(j));
        const u8 expect = s_theor_max(reg, j, row.slots);
        const u8 got = JobCountArray::max_n(j);
        if (got != expect || slots[j].m_max != expect) {
            std::printf("*** FAILED layout max job %u expect %u got max_n=%u slot=%u (base %u)\n",
                SC_U32(j), SC_U32(expect), SC_U32(got), SC_U32(slots[j].m_max), SC_U32(row.slots));
            return false;
        }
        const u8 wid = s_wid(SC_U8(expect + 1u));
        if (slots[j].m_wid != wid) {
            std::printf("*** FAILED layout wid job %u expect %u got %u\n",
                SC_U32(j), SC_U32(wid), SC_U32(slots[j].m_wid));
            return false;
        }
        if (slots[j].m_off != SC_U16(bits)) {
            std::printf("*** FAILED layout off job %u expect %u got %u\n",
                SC_U32(j), bits, SC_U32(slots[j].m_off));
            return false;
        }
        bits += SC_U32(wid);
    }
    const u16 expect_bits = bits == 0 ? SC_U16(1) : SC_U16(bits);
    if (JobCountArray::bits() != expect_bits) {
        std::printf("*** FAILED batch bits expect %u got %u\n",
            SC_U32(expect_bits), SC_U32(JobCountArray::bits()));
        return false;
    }
    std::printf("*** PASSED layout max/off/wid (%u jobs, %u bits)\n",
        SC_U32(jn), SC_U32(expect_bits));
    return true;
}

//================================================================================================================================
//=> - Clamp -
//================================================================================================================================

static bool check_clamp (u16 city_idx) {
    const u16 jn = JobCountArray::job_n();
    for (u16 j = 0; j < jn; ++j) {
        JobCountArray::set(city_idx, j, 0);
    }
    for (u16 j = 0; j < jn; ++j) {
        const u8 mx = JobCountArray::max_n(j);
        JobCountArray::set(city_idx, j, mx);
        if (JobCountArray::get(city_idx, j) != mx) {
            std::printf("*** FAILED clamp exact max job %u\n", SC_U32(j));
            return false;
        }
        if (mx < 255) {
            JobCountArray::set(city_idx, j, SC_U8(mx + 1u));
            if (JobCountArray::get(city_idx, j) != mx) {
                std::printf("*** FAILED clamp max+1 job %u expect %u got %u\n",
                    SC_U32(j), SC_U32(mx), SC_U32(JobCountArray::get(city_idx, j)));
                return false;
            }
        }
        JobCountArray::set(city_idx, j, 255);
        if (JobCountArray::get(city_idx, j) != mx) {
            std::printf("*** FAILED clamp 255 job %u expect %u got %u\n",
                SC_U32(j), SC_U32(mx), SC_U32(JobCountArray::get(city_idx, j)));
            return false;
        }
        for (u16 o = 0; o < jn; ++o) {
            if (o == j) {
                continue;
            }
            if (JobCountArray::get(city_idx, o) != 0) {
                std::printf("*** FAILED clamp side job %u while clamping job %u\n",
                    SC_U32(o), SC_U32(j));
                return false;
            }
        }
        JobCountArray::set(city_idx, j, 0);
    }
    std::printf("*** PASSED clamp (%u jobs)\n", SC_U32(jn));
    return true;
}

//================================================================================================================================
//=> - Interference -
//================================================================================================================================

static bool check_interference (const u16* peers) {
    const u16 jn = JobCountArray::job_n();
    if (jn > k_jobs_cap) {
        std::printf("*** FAILED job_n too large\n");
        return false;
    }
    u8 snap[k_peers][k_jobs_cap];
    for (u16 p = 0; p < k_peers; ++p) {
        for (u16 j = 0; j < jn; ++j) {
            const u8 v = pattern_val(peers[p], j, JobCountArray::max_n(j));
            JobCountArray::set(peers[p], j, v);
        }
        take_snap(peers[p], snap[p], jn);
    }
    for (u16 p = 0; p < k_peers; ++p) {
        if (!city_eq(peers[p], snap[p], jn)) {
            std::printf("*** FAILED isolation baseline city %u\n", SC_U32(peers[p]));
            return false;
        }
    }
    for (u16 tgt = 0; tgt < k_peers; ++tgt) {
        for (u16 j = 0; j < jn; ++j) {
            const u8 vmax = SC_U8(JobCountArray::max_n(j) + 1u);
            for (u16 v = 0; v < vmax; ++v) {
                const u8 uv = SC_U8(v);
                JobCountArray::set(peers[tgt], j, uv);
                snap[tgt][j] = uv;
                if (JobCountArray::get(peers[tgt], j) != uv) {
                    std::printf("*** FAILED get city %u job %u expect %u got %u\n",
                        SC_U32(peers[tgt]), SC_U32(j), SC_U32(uv),
                        SC_U32(JobCountArray::get(peers[tgt], j)));
                    return false;
                }
                for (u16 o = 0; o < jn; ++o) {
                    if (o == j) {
                        continue;
                    }
                    if (JobCountArray::get(peers[tgt], o) != snap[tgt][o]) {
                        std::printf("*** FAILED intra city %u side job %u writing job %u=%u\n",
                            SC_U32(peers[tgt]), SC_U32(o), SC_U32(j), SC_U32(uv));
                        return false;
                    }
                }
                for (u16 p = 0; p < k_peers; ++p) {
                    if (p == tgt) {
                        continue;
                    }
                    if (!city_eq(peers[p], snap[p], jn)) {
                        std::printf("*** FAILED peer city %u touched by city %u job %u=%u\n",
                            SC_U32(peers[p]), SC_U32(peers[tgt]), SC_U32(j), SC_U32(uv));
                        return false;
                    }
                }
            }
            const u8 restore = pattern_val(peers[tgt], j, JobCountArray::max_n(j));
            JobCountArray::set(peers[tgt], j, restore);
            snap[tgt][j] = restore;
        }
    }
    std::printf("*** PASSED interference (%u cities, non-zero neighbors, full sweeps)\n",
        SC_U32(k_peers));
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
    bool ok = check_layout(st);
    u16 peers[k_peers];
    for (u16 p = 0; p < k_peers && ok; ++p) {
        peers[p] = JobCountArray::claim();
        if (peers[p] == U16_KEY_NULL) {
            std::printf("*** FAILED claim peer %u\n", SC_U32(p));
            ok = false;
        }
    }
    if (ok) {
        ok = check_clamp(peers[0]);
    }
    if (ok) {
        ok = check_interference(peers);
    }
    if (ok) {
        std::printf("*** PASSED job_count_array correctness\n");
    }
    JobCountArray::clear();
    return ok ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
