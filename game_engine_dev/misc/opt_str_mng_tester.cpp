//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "opt_str_mng.h"

typedef const char* cstr;

int g_tc = 0;
int g_tp = 0;
int g_tf = 0;
int g_pl = 0;
cstr g_fn = "DEL_content_for_tester";

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

void nt(bool ok, cstr msg) {
    ++g_tc;
    if (ok) {
        ++g_tp;
        if (g_pl > 1) std::printf("*** TEST PASSED: %s\n", msg);
    } else {
        ++g_tf;
        if (g_pl > 0) std::printf("*** TEST FAILED: %s\n", msg);
    }
}

bool wr(cstr fn, cstr txt) {
    if (!fn || !txt) {
        return false;
    }
    FILE* f = std::fopen(fn, "wb");
    if (!f) {
        return false;
    }
    std::size_t len = std::strlen(txt);
    std::size_t n = std::fwrite(txt, 1, len, f);
    std::fclose(f);
    if (n != len) {
        return false;
    }
    return true;
}

bool eq(cstr a, cstr b) {
    if (!a) a = "";
    if (!b) b = "";
    return std::strcmp(a, b) == 0;
}

void p3_orig(cstr txt) {
    if (g_pl >= 3) {
        std::printf("\n*** Original content: [%s]\n", txt ? txt : "");
    }
}

void p3_split(char c) {
    if (g_pl >= 3) {
        std::printf("*** Split on: '%c'\n", c);
    }
}

void p3_res(StringManager& m) {
    if (g_pl < 3) return;
    std::printf("*** Resulting content:\n");
    for (u32 i = 0; i < m.get_string_count(); ++i) {
        cstr s = m.get_string_content(i);
        std::printf("[%s]\n", s ? s : "");
    }
}

typedef void (StringManager::*step_fn)(u32, char);
typedef void (StringManager::*step_fn0)();

typedef struct TestStep {
    step_fn fn;
    step_fn0 fn0;
    u32 i;
    char c;
} TestStep;

void p3_step(const TestStep& s) {
    if (s.fn0 == &StringManager::cull_empty_strings && g_pl >= 3) {
        std::printf("*** Cull empty strings\n");
        return;
    }
    if (s.fn == &StringManager::split_string_by_char) {
        p3_split(s.c);
        return;
    }
    if (s.fn == &StringManager::trim_head_char && g_pl >= 3) {
        std::printf("*** Trim head for: '%c'\n", s.c);
        return;
    }
    if (s.fn == &StringManager::trim_tail_char && g_pl >= 3) {
        std::printf("*** Trim tail for: '%c'\n", s.c);
    }
}

bool ck_all(StringManager& m, const cstr* exp, u32 exp_n) {
    if (m.get_string_count() != exp_n) return false;
    for (u32 i = 0; i < exp_n; ++i) {
        if (!eq(m.get_string_content(i), exp[i])) return false;
    }
    return true;
}

class TestCase {
public:
    TestCase(cstr name, cstr txt, const cstr* exp, u32 exp_n, const TestStep* steps, u32 step_n)
        : m_name(name), m_txt(txt), m_exp(exp), m_exp_n(exp_n), m_steps(steps), m_step_n(step_n) {}

    void run() const {
        StringManager m;
        nt(wr(g_fn, m_txt), "write test source");
        p3_orig(m_txt);
        nt(m.load_file_content(g_fn), "load test source");
        for (u32 i = 0; i < m_step_n; ++i) {
            p3_step(m_steps[i]);
            if (m_steps[i].fn0) {
                (m.*(m_steps[i].fn0))();
            } else {
                (m.*(m_steps[i].fn))(m_steps[i].i, m_steps[i].c);
            }
            p3_res(m);
        }
        nt(ck_all(m, m_exp, m_exp_n), m_name);
    }

private:
    cstr m_name;
    cstr m_txt;
    const cstr* m_exp;
    u32 m_exp_n;
    const TestStep* m_steps;
    u32 m_step_n;
};

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void ts_basic_split_trim() {
    const TestStep s1[] = {
        {&StringManager::split_string_by_char, 0, 0, ','}
    };
    const cstr e1[] = {"aa", "bb", "cc"};
    TestCase("split basic", "aa,bb,cc", e1, 3, s1, 1).run();

    const TestStep s2[] = {
        {&StringManager::split_string_by_char, 0, 0, ','},
        {&StringManager::trim_head_char, 0, 0, 'a'},
        {&StringManager::trim_tail_char, 0, 0, 'a'}
    };
    const cstr e2[] = {"", "bb", "cc"};
    TestCase("trim basic", "aa,bb,cc", e2, 3, s2, 3).run();
}

void ts_spaces_and_edges() {
    const TestStep s[] = {
        {&StringManager::split_string_by_char, 0, 0, ';'},
        {&StringManager::trim_head_char, 0, 0, ' '},
        {&StringManager::trim_tail_char, 0, 0, ' '},
        {&StringManager::trim_head_char, 0, 1, ' '},
        {&StringManager::trim_tail_char, 0, 1, ' '},
        {&StringManager::trim_head_char, 0, 2, ' '},
        {&StringManager::trim_tail_char, 0, 2, ' '}
    };
    const cstr e[] = {"one", "two", "three", ""};
    TestCase("trim spaced values", "  one ; two ;  three  ;", e, 4, s, 7).run();
}

void ts_reload_and_reuse() {
    const TestStep s[] = {
        {&StringManager::split_string_by_char, 0, 0, '|'}
    };
    const cstr e1[] = {"x", "y"};
    TestCase("reload split one", "x|y", e1, 2, s, 1).run();
    const cstr e2[] = {"q", "", "r"};
    TestCase("reload split two", "q||r", e2, 3, s, 1).run();
}

void ts_cull_empty_strings() {
    const TestStep head[] = {
        {&StringManager::split_string_by_char, 0, 0, ','},
        {0, &StringManager::cull_empty_strings, 0, 0}
    };
    const cstr head_e[] = {"mid", "tail"};
    TestCase("cull empty head", ",mid,tail", head_e, 2, head, 2).run();

    const TestStep mid[] = {
        {&StringManager::split_string_by_char, 0, 0, ','},
        {0, &StringManager::cull_empty_strings, 0, 0}
    };
    const cstr mid_e[] = {"head", "tail"};
    TestCase("cull empty mid", "head,,tail", mid_e, 2, mid, 2).run();

    const TestStep tail[] = {
        {&StringManager::split_string_by_char, 0, 0, ','},
        {0, &StringManager::cull_empty_strings, 0, 0}
    };
    const cstr tail_e[] = {"head", "mid"};
    TestCase("cull empty tail", "head,mid,", tail_e, 2, tail, 2).run();

    const TestStep all[] = {
        {&StringManager::split_string_by_char, 0, 0, ','},
        {0, &StringManager::cull_empty_strings, 0, 0}
    };
    TestCase("cull all empty", ",,", 0, 0, all, 2).run();

    const TestStep none[] = {
        {&StringManager::split_string_by_char, 0, 0, ','},
        {0, &StringManager::cull_empty_strings, 0, 0}
    };
    const cstr none_e[] = {"head", "mid", "tail"};
    TestCase("cull no empty", "head,mid,tail", none_e, 3, none, 2).run();
}

void ts_direct_cstr_content() {
    {
        StringManager m;
        nt(m.load_cstr_content("a,b,c"), "load cstr basic");
        m.split_string_by_char(0, ',');
        bool ok = (m.get_string_count() == 3);
        ok = ok && eq(m.get_string_content(0), "a");
        ok = ok && eq(m.get_string_content(1), "b");
        ok = ok && eq(m.get_string_content(2), "c");
        nt(ok, "split cstr basic");
    }
    {
        StringManager m;
        nt(m.load_cstr_content("  x  ; y ;  z  "), "load cstr trim");
        m.split_string_by_char(0, ';');
        m.trim_head_char(0, ' ');
        m.trim_tail_char(0, ' ');
        m.trim_head_char(1, ' ');
        m.trim_tail_char(1, ' ');
        m.trim_head_char(2, ' ');
        m.trim_tail_char(2, ' ');
        bool ok = (m.get_string_count() == 3);
        ok = ok && eq(m.get_string_content(0), "x");
        ok = ok && eq(m.get_string_content(1), "y");
        ok = ok && eq(m.get_string_content(2), "z");
        nt(ok, "trim cstr basic");
    }
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main(int argc, char** argv) {
    if (argc > 1) g_pl = std::atoi(argv[1]);
    ts_basic_split_trim();
    ts_spaces_and_edges();
    ts_reload_and_reuse();
    ts_cull_empty_strings();
    ts_direct_cstr_content();
    std::remove(g_fn);
    std::printf("=======================================================\n");
    std::printf(" TESTING STRING MANAGEMENT: TOTAL FAILURES: %d/%d\n", g_tf, g_tc);
    std::printf("=======================================================\n");
    return g_tf;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
