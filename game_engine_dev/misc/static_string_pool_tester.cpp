//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "../static_state/resource_static_key.h"
#include "opt_str_mng.h"
#include "static_string_pool.h"

static int g_tc = 0;
static int g_tp = 0;
static int g_tf = 0;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void nt (bool ok, cstr msg) {
    ++g_tc;
    if (ok) {
        ++g_tp;
    } else {
        ++g_tf;
        std::printf("*** TEST FAILED: %s\n", msg ? msg : "");
    }
}

static bool eq (cstr a, cstr b) {
    if (!a) a = "";
    if (!b) b = "";
    return std::strcmp(a, b) == 0;
}

static void trim_ln (StringManager& m) {
    for (u32 i = 0; i < m.get_string_count(); ++i) {
        m.trim_head_char(i, ' ');
        m.trim_tail_char(i, ' ');
        m.trim_head_char(i, '\t');
        m.trim_tail_char(i, '\t');
        m.trim_head_char(i, '\r');
        m.trim_tail_char(i, '\r');
    }
}

static u32 payload_len (const StringManager& m) {
    u32 n = 0;
    for (u32 i = 0; i < m.get_string_count(); ++i) {
        n += (u32)std::strlen(m.get_string_content(i));
    }
    return n;
}

static void pr_pool_stats (cstr label, const StaticStringPool& pool) {
    std::printf("%s pool stats: str_cap=%u str_n=%u char_cap=%u char_n=%u\n",
        label, pool.get_str_cap(), pool.get_str_n(), pool.get_char_cap(), pool.get_char_n());
}

static void run_line_test () {
    StringManager lines;
    if (!lines.load_file_content("../notes.statics_pipeline")) {
        std::printf("ERROR: failed to load ../notes.statics_pipeline\n");
        ++g_tf;
        return;
    }
    lines.split_string_by_char(0, '\n');
    trim_ln(lines);
    lines.cull_empty_strings();

    const u16 str_cap = (u16)lines.get_string_count();
    const u32 char_n = payload_len(lines);
    StaticStringPool pool(str_cap, char_n);

    nt(pool.get_str_cap() == str_cap, "line str cap");
    nt(pool.get_char_cap() == char_n + (u32)str_cap, "line char cap incl nulls");

    for (u32 i = 0; i < lines.get_string_count(); ++i) {
        u16 k = 0;
        nt(pool.add(lines.get_string_content(i), &k), "line add");
        nt(k == (u16)i, "line key equals row index");
    }

    nt(pool.get_str_n() == str_cap, "line str count");
    nt(pool.get_char_n() <= pool.get_char_cap(), "line char used within cap");

    for (u16 i = 0; i < pool.get_str_n(); ++i) {
        ResourceStaticDataKey key = ResourceStaticDataKey::from_raw(i);
        cstr got = pool.get(key.value());
        cstr want = lines.get_string_content(i);
        nt(eq(got, want), "line roundtrip via key.value()");
    }

    for (u16 i = 0; i < pool.get_str_n(); ++i) {
        std::printf("[%u] %s\n", i, pool.get(i));
    }
}

static void run_space_test () {
    StringManager words;
    if (!words.load_file_content("../notes.statics_pipeline")) {
        std::printf("ERROR: failed to load ../notes.statics_pipeline for space test\n");
        ++g_tf;
        return;
    }
    words.split_string_by_char(0, ' ');
    words.cull_empty_strings();

    const u16 str_cap = (u16)words.get_string_count();
    const u32 char_n = payload_len(words);
    StaticStringPool pool(str_cap, char_n);

    for (u32 i = 0; i < words.get_string_count(); ++i) {
        u16 k = 0;
        if (!pool.add(words.get_string_content(i), &k)) {
            std::printf("ERROR: space test add failed at %u\n", i);
            ++g_tf;
            return;
        }
    }

    for (u16 i = 0; i < pool.get_str_n(); ++i) {
        std::printf(" | %s", pool.get(i));
    }

    pr_pool_stats("space", pool);
}

//================================================================================================================================
//=> - Driver -
//================================================================================================================================

int main () {
    run_line_test();
    run_space_test();

    std::printf("=======================================================\n");
    std::printf(" TESTING STATIC STRING POOL: TOTAL FAILURES: %d/%d\n", g_tf, g_tc);
    std::printf("=======================================================\n");
    return g_tf > 0 ? 1 : 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
