//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "opt_str_mng.h"
#include "str_pool.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

void trim_all(StringManager& m) {
    for (u32 i = 0; i < m.get_string_count(); ++i) {
        m.trim_head_char(i, ' ');
        m.trim_tail_char(i, ' ');
        m.trim_head_char(i, '\t');
        m.trim_tail_char(i, '\t');
        m.trim_head_char(i, '\r');
        m.trim_tail_char(i, '\r');
    }
}

void print_pool_chars(cstr label, const StringPool& p) {
    std::printf("%s char index: %u, char limit: %u\n", label, p.get_char_count(), p.get_char_capacity());
    std::printf("%s full char iteration:\n", label);
    cstr base = p.get_string_count() > 0 ? p.get_string(0) : "";
    for (u32 i = 0; i < p.get_char_capacity(); ++i) {
        char c = base[i];
        if (c == '\0') {
            std::printf("\\0");
        } else {
            std::printf("%c", c);
        }
    }
    std::printf("\n");
}

void run_build_cost_pool_test() {
    StringManager lines;
    if (!lines.load_file_content("../game_config.buildings")) {
        std::printf("Failed to load ../game_config.buildings\n");
        return;
    }
    lines.split_string_by_char(0, '\n');
    trim_all(lines);
    lines.cull_empty_strings();

    u32 name_len = 0;
    u32 cost_len = 0;
    u16 pair_n = 0;
    for (u32 i = 0; i < lines.get_string_count(); ++i) {
        StringManager parts;
        if (!parts.load_cstr_content(lines.get_string_content(i))) {
            continue;
        }
        parts.split_string_by_char(0, ':');
        trim_all(parts);
        parts.cull_empty_strings();
        if (parts.get_string_count() < 2) {
            continue;
        }
        name_len += (u32)std::strlen(parts.get_string_content(0));
        cost_len += (u32)std::strlen(parts.get_string_content(1));
        ++pair_n;
    }

    StringPool name_pool(name_len + (u32)pair_n, pair_n);
    StringPool cost_pool(cost_len + (u32)pair_n, pair_n);

    for (u32 i = 0; i < lines.get_string_count(); ++i) {
        StringManager parts;
        if (!parts.load_cstr_content(lines.get_string_content(i))) {
            continue;
        }
        parts.split_string_by_char(0, ':');
        trim_all(parts);
        parts.cull_empty_strings();
        if (parts.get_string_count() < 2) {
            continue;
        }
        u16 id_a = 0;
        u16 id_b = 0;
        if (!name_pool.push_string(parts.get_string_content(0), &id_a)) {
            continue;
        }
        if (!cost_pool.push_string(parts.get_string_content(1), &id_b)) {
            continue;
        }
    }

    for (u16 i = 0; i < name_pool.get_string_count() && i < cost_pool.get_string_count(); ++i) {
        std::printf("build %s has a cost of %s\n", name_pool.get_string(i), cost_pool.get_string(i));
    }

    print_pool_chars("name pool", name_pool);
    print_pool_chars("cost pool", cost_pool);
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main() {
    StringManager m;
    if (!m.load_file_content("../game_config.buildings")) {
        std::printf("Failed to load ../game_config.buildings\n");
        return 1;
    }

    m.split_string_by_char(0, '\n');
    trim_all(m);

    for (u32 i = 0; i < m.get_string_count(); ++i) {
        m.split_string_by_char(i, ':');
    }
    trim_all(m);
    m.cull_empty_strings();

    u32 total_len = 0;
    for (u32 i = 0; i < m.get_string_count(); ++i) {
        total_len += (u32)std::strlen(m.get_string_content(i));
    }

    u16 str_n = (u16)m.get_string_count();
    u32 char_n = total_len + (u32)str_n;
    StringPool p(char_n, str_n);

    for (u32 i = 0; i < m.get_string_count(); ++i) {
        u16 id = 0;
        if (!p.push_string(m.get_string_content(i), &id)) {
            std::printf("push_string failed at %u\n", i);
            return 1;
        }
    }

    std::printf("Manager string count: %u\n", m.get_string_count());
    std::printf("Total payload length: %u\n", total_len);
    std::printf("Pool char capacity: %u\n", p.get_char_capacity());
    std::printf("Pool char used: %u\n", p.get_char_count());
    std::printf("Pool string capacity: %u\n", p.get_string_capacity());
    std::printf("Pool string used: %u\n", p.get_string_count());
    std::printf("Pool content:\n");
    for (u16 i = 0; i < p.get_string_count(); ++i) {
        std::printf("[%u] %s\n", i, p.get_string(i));
    }
    print_pool_chars("single pool", p);

    run_build_cost_pool_test();

    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
