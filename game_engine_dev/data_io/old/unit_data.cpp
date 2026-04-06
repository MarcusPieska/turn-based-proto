//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

#include "str_mng.h"
#include "bit_array.h"
#include "resource_data.h"
#include "tech_data.h"
#include "city_flags.h"
#include "civ_data.h"
#include "building_data.h"
#include "unit_types.h"

#include "unit_data.h"

//================================================================================================================================
//=> - Private structures and globals -
//================================================================================================================================

static UnitTypeStats* unit_data_array = nullptr;
static u32 unit_data_count = 0;

//================================================================================================================================
//=> - Public functions -
//================================================================================================================================

void UnitData::load_static_data (const std::string& filename) {
    if (unit_data_array != nullptr) {
        return;
    }
    parse_and_allocate(filename);
}

void UnitData::print_content () {
    printf("Unit Data (total=%u):\n", unit_data_count);
    for (u32 i = 0; i < unit_data_count; ++i) {
        const UnitTypeStats& stats = unit_data_array[i];
        printf("Unit: %s, Cost: %u, A/D/M: %u/%u/%u, Tech prerequisite idx: %u",
               stats.name.c_str(),
               static_cast<u32>(stats.cost),
               static_cast<u32>(stats.attack),
               static_cast<u32>(stats.defense),
               static_cast<u32>(stats.movement_speed),
               stats.tech_prereq_idx.get_idx());

        bool first_req = true;
        for (u32 r = 0; r < MAX_UNIT_REQS; ++r) {
            const UnitRequirement& req = stats.requirements[r];
            if (req.type == UNIT_REQ_NONE) {
                continue;
            }
            if (first_req) {
                printf(", Reqs: ");
                first_req = false;
            } else {
                printf(" : ");
            }

            switch (req.type) {
                case UNIT_REQ_RESOURCE:
                    printf("Resource (idx=%u)", static_cast<u32>(req.data.resource_req.resource_idx));
                    break;
                default:
                    printf("unknown");
                    break;
            }
        }

        printf("\n");
    }
}

void UnitData::print_content_by_unit_type () {
    printf("Unit Data (total=%u):\n", unit_data_count);
    for (u32 t = 0; t < UnitTypeData::get_unit_type_count(); ++t) {
        printf("Unit Type: %s\n", UnitTypeData::get_unit_type_name_array()[t].name.c_str());
        for (u32 i = 0; i < unit_data_count; ++i) {
            if (unit_data_array[i].unit_type != t) {
                continue;
            }
            const UnitTypeStats& stats = unit_data_array[i];
            printf("  Unit: %s, Cost: %u, A/D/M: %u/%u/%u, Tech prerequisite idx: %u\n",
                   stats.name.c_str(),
                   static_cast<u32>(stats.cost),
                   static_cast<u32>(stats.attack),
                   static_cast<u32>(stats.defense),
                   static_cast<u32>(stats.movement_speed),
                   stats.tech_prereq_idx.get_idx());
        }
    }
}

u16 UnitData::find_unit_index (const std::string& unit_name) {
    for (u32 i = 0; i < unit_data_count; ++i) {
        if (unit_data_array[i].name == unit_name) {
            return static_cast<u16>(i);
        }
    }
    printf("ERROR: Unit '%s' not found in UnitData\n", unit_name.c_str());
    print_content();
    exit(1);
    return 0; // Return 0 if not found (default)
}

u16 UnitData::get_unit_data_count () {
    return unit_data_count;
}

const UnitTypeStats* UnitData::get_unit_data_array () {
    return unit_data_array;
}

//================================================================================================================================
//=> - Private helper functions -
//================================================================================================================================

u16 UnitData::validate_and_count (const std::string& filename) {
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        printf("ERROR: No content found in %s\n", filename.c_str());
        exit(1);
    }

    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");

    u16 count = 0;
    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() >= 6) {
            bool valid = true;
            for (size_t j = 0; j < 6; j++) {
                std::string part = trimmer.trim(parts[j]);
                if (part.empty()) {
                    valid = false;
                    break;
                }
            }
            if (valid) {
                count++;
            }
        }
    }

    return count;
}

//================================================================================================================================
//=> - Local helpers (requirements parsing) -
//================================================================================================================================

static void init_requirements (UnitTypeStats& stats) {
    for (u32 r = 0; r < MAX_UNIT_REQS; ++r) {
        stats.requirements[r].type = UNIT_REQ_NONE;
        stats.requirements[r].data.resource_req.resource_idx = 0;
        stats.requirements[r].data.building_req.building_idx = 0;
        stats.requirements[r].data.building_req.count_required = 0;
        stats.requirements[r].data.civ_req.civ_idx = 0;
        stats.requirements[r].data.flag_req.flag_idx = 0;
    }
}

static bool parse_resource_requirement (const std::string& token, UnitRequirement& out_req) {
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
    out_req.type = UNIT_REQ_RESOURCE;
    out_req.data.resource_req.resource_idx = static_cast<u16>(res_idx.get_idx());
    return true;
}

static bool parse_building_requirement (const std::string& token, UnitRequirement& out_req) {
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
    if (parts.empty()) {
        return false;
    }

    std::string name_str  = trimmer.trim(parts[0]);
    u16 count_required    = 1;
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
    out_req.type = UNIT_REQ_BUILDING;
    out_req.data.building_req.building_idx = building_idx;
    out_req.data.building_req.count_required = count_required;
    return true;
}

static bool parse_flag_requirement (const std::string& token, UnitRequirement& out_req) {
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
    out_req.type = UNIT_REQ_FLAG;
    out_req.data.flag_req.flag_idx = idx;
    return true;
}

static bool parse_civ_requirement (const std::string& token, UnitRequirement& out_req) {
    const char* prefix = "civ(";
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

    u16 idx = CivData::find_civ_index(inside);
    out_req.type = UNIT_REQ_CIV;
    out_req.data.civ_req.civ_idx = idx;
    return true;
}

void UnitData::parse_and_allocate (const std::string& filename) {
    unit_data_count = UnitData::validate_and_count(filename);
    if (unit_data_count == 0) {
        printf("ERROR: No units found in data file '%s'\n", filename.c_str());
        exit(1);
    }
    unit_data_array = new UnitTypeStats[unit_data_count];
    if (unit_data_array == nullptr) {
        printf("ERROR: Failed to allocate memory for %u units\n", unit_data_count);
        exit(1);
    }
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        printf("ERROR: No content found in %s\n", filename.c_str());
        exit(1);
    }
    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");

    u32 idx = 0;
    for (size_t i = 0; i < lines.size() && idx < unit_data_count; ++i) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() < 7) {
            continue;
        }
        u16 part_idx = 0;
        std::string name = trimmer.trim(parts[part_idx++]);
        std::string unit_type = trimmer.trim(parts[part_idx++]);
        std::string cost_str = trimmer.trim(parts[part_idx++]);
        std::string atk_str = trimmer.trim(parts[part_idx++]);
        std::string def_str = trimmer.trim(parts[part_idx++]);
        std::string move_str = trimmer.trim(parts[part_idx++]);
        std::string tech_name  = trimmer.trim(parts[part_idx++]);

        UnitTypeStats& stats = unit_data_array[idx];
        stats.name = name;
        stats.unit_type = UnitTypeData::find_unit_type_index(unit_type);
        stats.cost = static_cast<u32>(std::atoi(cost_str.c_str()));
        stats.attack = static_cast<u16>(std::atoi(atk_str.c_str()));
        stats.defense = static_cast<u16>(std::atoi(def_str.c_str()));
        stats.movement_speed = static_cast<u16>(std::atoi(move_str.c_str()));
        stats.tech_prereq_idx = TechData::find_tech_index(tech_name);
        init_requirements(stats);

        u32 req_idx = 0;
        for (size_t j = part_idx; j < parts.size() && req_idx < MAX_UNIT_REQS; ++j) {
            std::string token = trimmer.trim(parts[j]);
            if (token.empty()) {
                continue;
            }

            UnitRequirement req;
            req.type = UNIT_REQ_NONE;

            bool parsed = false;
            if (!parsed) parsed = parse_resource_requirement(token, req);
            if (!parsed) parsed = parse_building_requirement(token, req);
            if (!parsed) parsed = parse_civ_requirement(token, req);
            if (!parsed) parsed = parse_flag_requirement(token, req);

            if (parsed && req.type != UNIT_REQ_NONE) {
                stats.requirements[req_idx++] = req;
            }
        }

        idx++;
    }
    if (idx != unit_data_count) {
        printf("ERROR: Expected %u units, but got %u\n", unit_data_count, idx);
        exit(1);
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
