//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "str_mng.h"
#include "bit_array.h"
#include "city_flags.h"
#include "tech_data.h"
#include "building_data.h"
#include "resource_data.h"

#include "wonder_data.h"

//================================================================================================================================
//=> - Private structures and globals -
//================================================================================================================================

static WonderTypeStats* wonder_data_array = nullptr;
static u32 wonder_data_count = 0;

//================================================================================================================================
//=> - Public functions -
//================================================================================================================================

void WonderData::load_static_data (const std::string& filename) {
    if (wonder_data_array != nullptr) {
        return;
    }
    parse_and_allocate(filename);
}

void WonderData::print_content () {
    printf("Wonder data count: (total=%u)\n", wonder_data_count);
    for (u32 i = 0; i < wonder_data_count; ++i) {
        const WonderTypeStats& stats = wonder_data_array[i];
        printf("Wonder: %s, Cost: %u, Tech prerequisite idx: %u",
               stats.name.c_str(),
               stats.cost,
               stats.tech_prereq_idx.get_idx());

        bool first_req = true;
        for (u32 r = 0; r < MAX_WONDER_REQS; ++r) {
            const WonderRequirement& req = stats.requirements[r];
            if (req.type == WONDER_REQ_NONE) {
                continue;
            }
            if (first_req) {
                printf(", Reqs: ");
                first_req = false;
            } else {
                printf(" : ");
            }

            switch (req.type) {
                case WONDER_REQ_FLAG:
                    printf("Flag (idx=%u)", static_cast<u32>(req.data.flag_req.flag_idx));
                    break;
                case WONDER_REQ_RESOURCE:
                    printf("Resource (idx=%u)", static_cast<u32>(req.data.resource_req.resource_idx));
                    break;
                case WONDER_REQ_BUILDING: {
                    u32 idx = static_cast<u32>(req.data.building_req.building_idx);
                    u32 ct = static_cast<u32>(req.data.building_req.count_required);
                    if (idx != 0 && ct > 1) {
                        printf("Count (idx=%u, count=%u)", idx, ct);
                    } else {
                        printf("Building (idx=%u, count=%u)", idx, ct == 0 ? 1 : ct);
                    }
                    break;
                }
                default:
                    printf("unknown");
                    break;
            }
        }

        printf("\n");
    }
}

u16 WonderData::find_wonder_index (const std::string& wonder_name) {
    for (u32 i = 0; i < wonder_data_count; ++i) {
        if (wonder_data_array[i].name == wonder_name) {
            return static_cast<u16>(i);
        }
    }
    printf("ERROR: Wonder '%s' not found in WonderData\n", wonder_name.c_str());
    exit(1);
    return 0; // To make the compiler happy
}

u16 WonderData::get_wonder_data_count () {
    return static_cast<u16>(wonder_data_count);
}

const WonderTypeStats* WonderData::get_wonder_data_array () {
    return wonder_data_array;
}

//================================================================================================================================
//=> - Private helper functions -
//================================================================================================================================

u16 WonderData::validate_and_count (const std::string& filename) {
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        printf("ERROR: Wonder data file '%s' is empty\n", filename.c_str());
        exit(1);
    }
    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");
    u16 count = 0;

    for (size_t i = 0; i < lines.size(); ++i) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() >= 3) {
            bool valid = true;
            for (size_t j = 0; j < 3; ++j) {
                std::string part = trimmer.trim(parts[j]);
                if (part.empty()) {
                    valid = false;
                    break;
                }
            }
            if (valid) {
                ++count;
            }
        }
    }
    return count;
}

static void init_requirements (WonderTypeStats& stats) {
    for (u32 r = 0; r < MAX_WONDER_REQS; ++r) {
        stats.requirements[r].type = WONDER_REQ_NONE;
        stats.requirements[r].data.flag_req.flag_idx = 0;
        stats.requirements[r].data.resource_req.resource_idx = 0;
        stats.requirements[r].data.building_req.building_idx = 0;
        stats.requirements[r].data.building_req.count_required = 0;
    }
}

static bool parse_flag_requirement (const std::string& token, WonderRequirement& out_req) {
    const char* prefix = "flag(";
    const size_t prefix_len = std::strlen(prefix);
    if (token.compare(0, prefix_len, prefix) != 0) {
        return false;
    }
    if (token.size() < prefix_len + 2 || token.back() != ')') {
        return false;
    }
    std::string inside = token.substr(prefix_len, token.size() - prefix_len - 1);
    StringTrimmer trimmer(" \t\r\n");
    inside = trimmer.trim(inside);
    if (inside.empty()) {
        return false;
    }

    u16 idx = CityFlagData::find_flag_index(inside);
    out_req.type = WONDER_REQ_FLAG;
    out_req.data.flag_req.flag_idx = idx;
    return true;
}

static bool parse_resource_requirement (const std::string& token, WonderRequirement& out_req) {
    const char* prefix = "resource(";
    const size_t prefix_len = std::strlen(prefix);
    if (token.compare(0, prefix_len, prefix) != 0) {
        return false;
    }
    if (token.size() < prefix_len + 2 || token.back() != ')') {
        return false;
    }
    std::string inside = token.substr(prefix_len, token.size() - prefix_len - 1);
    StringTrimmer trimmer(" \t\r\n");
    inside = trimmer.trim(inside);
    if (inside.empty()) {
        return false;
    }
    ResourceIdx res_idx = ResourceData::find_resource_index(inside);
    out_req.type = WONDER_REQ_RESOURCE;
    out_req.data.resource_req.resource_idx = res_idx.get_idx();
    return true;
}

static bool parse_building_requirement (const std::string& token, WonderRequirement& out_req) {
    const char* prefix = "building(";
    const size_t prefix_len = std::strlen(prefix);
    if (token.compare(0, prefix_len, prefix) != 0) {
        return false;
    }
    if (token.size() < prefix_len + 2 || token.back() != ')') {
        return false;
    }
    std::string inside = token.substr(prefix_len, token.size() - prefix_len - 1);
    StringTrimmer trimmer(" \t\r\n");
    inside = trimmer.trim(inside);
    if (inside.empty()) {
        return false;
    }

    StringSplitter comma_splitter(",");
    std::vector<std::string> parts = comma_splitter.split(inside);
    std::string name_str;
    u16 count_required = 1;

    if (parts.empty()) {
        return false;
    }

    name_str = trimmer.trim(parts[0]);
    if (parts.size() >= 2) {
        std::string count_str = trimmer.trim(parts[1]);
        if (!count_str.empty()) {
            count_required = static_cast<u16>(std::atoi(count_str.c_str()));
            if (count_required == 0) {
                count_required = 1;
            }
        }
    }

    if (name_str.empty()) {
        return false;
    }

    u16 building_idx = BuildingData::find_building_index(name_str);
    out_req.type = WONDER_REQ_BUILDING;
    out_req.data.building_req.building_idx = building_idx;
    out_req.data.building_req.count_required = count_required;
    return true;
}

void WonderData::parse_and_allocate (const std::string& filename) {
    wonder_data_count = WonderData::validate_and_count(filename);
    if (wonder_data_count == 0) {
        printf("ERROR: No wonders found in data file '%s'\n", filename.c_str());
        exit(1);
    }
    wonder_data_array = new WonderTypeStats[wonder_data_count];
    if (wonder_data_array == nullptr) {
        printf("ERROR: Failed to allocate memory for %u wonders\n", wonder_data_count);
        exit(1);
    }

    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        printf("ERROR: Wonder data file '%s' is empty\n", filename.c_str());
        exit(1);
    }

    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");

    u32 idx = 0;
    for (size_t i = 0; i < lines.size() && idx < wonder_data_count; ++i) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }

        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() < 3) {
            continue;
        }

        std::string name = trimmer.trim(parts[0]);
        std::string cost_str = trimmer.trim(parts[1]);
        std::string tech_name = trimmer.trim(parts[2]);

        if (name.empty() || cost_str.empty() || tech_name.empty()) {
            continue;
        }

        WonderTypeStats& stats = wonder_data_array[idx];
        stats.name = name;
        stats.cost = static_cast<u32>(std::atoi(cost_str.c_str()));
        stats.tech_prereq_idx = TechData::find_tech_index(tech_name);
        init_requirements(stats);

        u32 req_idx = 0;
        for (size_t j = 3; j < parts.size() && req_idx < MAX_WONDER_REQS; ++j) {
            std::string token = trimmer.trim(parts[j]);
            if (token.empty()) {
                continue;
            }

            WonderRequirement req;
            req.type = WONDER_REQ_NONE;

            bool parsed = false;
            if (!parsed) parsed = parse_flag_requirement(token, req);
            if (!parsed) parsed = parse_resource_requirement(token, req);
            if (!parsed) parsed = parse_building_requirement(token, req);

            if (parsed && req.type != WONDER_REQ_NONE) {
                stats.requirements[req_idx++] = req;
            }
        }

        ++idx;
    }
    if (idx != wonder_data_count) {
        printf("ERROR: Did not parse %u wonders from data file '%s'\n", wonder_data_count, filename.c_str());
        exit(1);
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

