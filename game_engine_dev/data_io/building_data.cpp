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
#include "city_flags.h"
#include "resource_data.h"
#include "tech_data.h"

#include "building_data.h"

//================================================================================================================================
//=> - Private structures and globals -
//================================================================================================================================

static BuildingTypeStats* building_data_array = nullptr;
static u32 building_data_count = 0;

//================================================================================================================================
//=> - Public functions -
//================================================================================================================================


void BuildingData::load_static_data (const std::string& filename) {
    if (building_data_array != nullptr) {
        return;
    }
    parse_and_allocate(filename);
}

void BuildingData::print_content () {
    printf("Building data count: (total=%u)\n", building_data_count);
    for (u32 i = 0; i < building_data_count; ++i) {
        const BuildingTypeStats& stats = building_data_array[i];
        printf("Building: %s, Cost: %u, Tech prerequisite idx: %u",
               stats.name.c_str(),
               stats.cost,
               stats.tech_prereq_idx.get_idx());

        bool first_req = true;
        for (u32 r = 0; r < MAX_BUILDING_REQS; ++r) {
            const BuildingRequirement& req = stats.requirements[r];
            if (req.type == BUILDING_REQ_NONE) {
                continue;
            }
            if (first_req) {
                printf(", Reqs: ");
                first_req = false;
            } else {
                printf(" : ");
            }

            switch (req.type) {
                case BUILDING_REQ_FLAG:
                    printf("Flag (idx=%u)", static_cast<u32>(req.data.flag_req.flag_idx));
                    break;
                case BUILDING_REQ_RESOURCE:
                    printf("Resource (idx=%u)", static_cast<u32>(req.data.resource_req.resource_idx));
                    break;
                case BUILDING_REQ_BUILDING: {
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

u16 BuildingData::find_building_index (const std::string& building_name) {
    for (u32 i = 0; i < building_data_count; ++i) {
        if (building_data_array[i].name == building_name) {
            return static_cast<u16>(i);
        }
    }
    print_content();
    printf("ERROR: Building '%s' not found in BuildingData\n", building_name.c_str());
    exit(1);
    return 0; // To make the compiler happy
}

u16 BuildingData::get_building_data_count () {
    return static_cast<u16>(building_data_count);
}

const BuildingTypeStats* BuildingData::get_building_data_array () {
    return building_data_array;
}

//================================================================================================================================
//=> - Private helper functions -
//================================================================================================================================

static void init_requirements (BuildingTypeStats& stats) {
    for (u32 r = 0; r < MAX_BUILDING_REQS; ++r) {
        stats.requirements[r].type = BUILDING_REQ_NONE;
        stats.requirements[r].data.flag_req.flag_idx = 0;
        stats.requirements[r].data.resource_req.resource_idx = 0;
        stats.requirements[r].data.building_req.building_idx = 0;
        stats.requirements[r].data.building_req.count_required = 0;
    }
}

static bool parse_flag_requirement (const std::string& token, BuildingRequirement& out_req) {
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
    out_req.type = BUILDING_REQ_FLAG;
    out_req.data.flag_req.flag_idx = idx;
    return true;
}

static bool parse_resource_requirement (const std::string& token, BuildingRequirement& out_req) {
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
    out_req.type = BUILDING_REQ_RESOURCE;
    out_req.data.resource_req.resource_idx = res_idx.get_idx();
    return true;
}

struct BuildingReqTemp {
    std::string building_name;
    u16 count_required;
    u32 req_index;
    u32 building_index;
};

static std::vector<BuildingReqTemp> deferred_building_reqs;

static bool parse_building_requirement (const std::string& token, BuildingRequirement& out_req, u32 building_idx, u32 req_idx) {
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

    BuildingReqTemp temp;
    temp.building_name = name_str;
    temp.count_required = count_required;
    temp.req_index = req_idx;
    temp.building_index = building_idx;
    deferred_building_reqs.push_back(temp);

    out_req.type = BUILDING_REQ_BUILDING;
    out_req.data.building_req.building_idx = 0; 
    out_req.data.building_req.count_required = count_required;
    return true;
}

u16 BuildingData::validate_and_count (const std::string& filename) {
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        printf("ERROR: Building data file '%s' is empty\n", filename.c_str());
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
        if (parts.size() >= 3) {
            bool valid = true;
            for (size_t j = 0; j < 3; j++) {
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

void BuildingData::parse_and_allocate (const std::string& filename) {
    building_data_count = BuildingData::validate_and_count(filename);
    if (building_data_count == 0) {
        printf("ERROR: No buildings found in data file '%s'\n", filename.c_str());
        exit(1);
    }
    building_data_array = new BuildingTypeStats[building_data_count];
    if (building_data_array == nullptr) {
        printf("ERROR: Failed to allocate memory for %u buildings\n", building_data_count);
        exit(1);
    }

    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        printf("ERROR: Building data file '%s' is empty\n", filename.c_str());
        exit(1);
    }

    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");

    u32 idx = 0;
    for (size_t i = 0; i < lines.size() && idx < building_data_count; ++i) {
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

        BuildingTypeStats& stats = building_data_array[idx];
        stats.name = name;
        stats.cost = static_cast<u32>(std::atoi(cost_str.c_str()));
        stats.tech_prereq_idx = TechData::find_tech_index(tech_name);
        init_requirements(stats);
        stats.effect_indices.indices[0] = 0;
        stats.effect_indices.indices[1] = 0;
        stats.effect_indices.indices[2] = 0;
        stats.effect_indices.indices[3] = 0;

        u32 req_idx = 0;
        for (size_t j = 3; j < parts.size() && req_idx < MAX_BUILDING_REQS; ++j) {
            std::string token = trimmer.trim(parts[j]);
            if (token.empty()) {
                continue;
            }

            BuildingRequirement req;
            req.type = BUILDING_REQ_NONE;

            bool parsed = false;
            if (!parsed) parsed = parse_flag_requirement(token, req);
            if (!parsed) parsed = parse_resource_requirement(token, req);
            if (!parsed) parsed = parse_building_requirement(token, req, idx, req_idx);

            if (parsed && req.type != BUILDING_REQ_NONE) {
                stats.requirements[req_idx++] = req;
            }
        }

        ++idx;
    }
    if (idx != building_data_count) {
        printf("ERROR: Did not parse %u buildings from data file '%s'\n", building_data_count, filename.c_str());
        exit(1);
    }
    for (size_t i = 0; i < deferred_building_reqs.size(); ++i) {
        const BuildingReqTemp& temp = deferred_building_reqs[i];
        u16 building_idx = BuildingData::find_building_index(temp.building_name);
        building_data_array[temp.building_index].requirements[temp.req_index].data.building_req.building_idx = building_idx;
    }
    deferred_building_reqs.clear();
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
