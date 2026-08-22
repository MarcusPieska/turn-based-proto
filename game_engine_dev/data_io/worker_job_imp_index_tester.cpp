//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "map_overlay_static_key.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "worker_job_imp_index.h"
#include "worker_job_imp_static_key.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_RT_LIB = "./runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../";

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 1;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

void note_result (bool cond, cstr msg) {
    test_count++;
    total_tests_run++;
    if (cond) {
        test_pass++;
        if (print_level > 1) {
            std::printf("*** TEST PASSED: %s\n", msg);
        }
    } else {
        total_test_fails++;
        if (print_level > 0) {
            std::printf("*** TEST FAILED: %s\n", msg);
        }
    }
}

static void print_index (const RuntimeStatics& st) {
    const WorkerJobImpIndex& idx = st.worker_job_imp_index();
    std::printf("WorkerJobImpIndex overlays=%u imps=%u\n",
        static_cast<unsigned>(idx.ov_n()),
        static_cast<unsigned>(idx.imp_total()));
    for (u16 j = 0; j < idx.ov_n(); ++j) {
        const u16 n = idx.imp_n(j);
        if (n == 0u) {
            continue;
        }
        cstr onm = st.map_overlay().get_name(MapOverlayStaticDataKey::from_raw(j));
        if (onm == nullptr) {
            onm = "?";
        }
        std::printf("%s (%u):\n", onm, static_cast<unsigned>(n));
        const u16* imps = idx.imps(j);
        for (u16 k = 0; k < n; ++k) {
            const u16 ii = imps[k];
            cstr inm = st.worker_job_imp().get_name(WorkerJobImpStaticDataKey::from_raw(ii));
            if (inm == nullptr) {
                inm = "?";
            }
            std::printf("  - [%u] %s\n", static_cast<unsigned>(ii), inm);
        }
    }
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    RuntimeStaticLoader loader;
    note_result(loader.load(G_RT_LIB, G_RT_DATA), "load runtime statics");
    if (total_test_fails > 0) {
        std::printf("=======================================================\n");
        std::printf(" TESTING WORKER JOB IMP INDEX: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
        std::printf("=======================================================\n");
        return 1;
    }

    RuntimeStatics& st = loader.statics();
    const WorkerJobImpIndex& idx = st.worker_job_imp_index();
    note_result(idx.ov_n() == st.map_overlay().get_item_count(), "index ov_n matches map_overlay catalog");
    note_result(idx.imp_total() == st.worker_job_imp().get_item_count(), "index imp_total matches worker_job_imp catalog");

    u32 seen = 0;
    for (u16 j = 0; j < idx.ov_n(); ++j) {
        seen += idx.imp_n(j);
    }
    note_result(seen == idx.imp_total(), "sum of per-overlay lists equals imp_total");

    print_index(st);

    loader.unload();
    note_result(!loader.is_loaded(), "unload SO while retaining statics");
    note_result(loader.statics().worker_job_imp_index().imp_total() == seen, "index survives SO unload");

    std::printf("=======================================================\n");
    std::printf(" TESTING WORKER JOB IMP INDEX: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return (total_test_fails > 0) ? 1 : 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
