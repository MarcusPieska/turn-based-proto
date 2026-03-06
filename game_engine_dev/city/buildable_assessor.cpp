//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "str_mng.h"
#include "building_vector.h"
#include "resources_vector.h"

#include "buildable_assessor.h"

//================================================================================================================================
//=> - Private structures and globals -
//================================================================================================================================

typedef struct BuildingReq {
    uint16_t req0;
    uint16_t req1;
    uint16_t req2;
    uint16_t req3;
} BuildingReq;

static BuildingReq* building_reqs_array = nullptr;

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

static uint32_t building_name_to_index (const std::string& building_name) {
    const BuildingTypeStats* building_array = BuildingVector::get_building_data_array();
    uint32_t building_count = BuildingVector::get_building_data_count();
    if (building_array == nullptr) {
        printf("ERROR: Building data array is null\n");
        exit(1);
    }
    for (uint32_t i = 0; i < building_count; i++) {
        if (building_array[i].name == building_name) {
            return i;
        }
    }
    printf("ERROR: Building name not found: %s\n", building_name.c_str());
    exit(1);
}

static uint32_t resource_name_to_index (const std::string& resource_name) {
    const ResourceTypeStats* resource_array = ResourceVector::get_resource_data_array();
    uint32_t resource_count = ResourceVector::get_resource_data_count();
    for (uint32_t i = 0; i < resource_count; i++) {
        if (resource_array[i].name == resource_name) {
            return i;
        }
    }
    printf("ERROR: Resource name not found: %s\n", resource_name.c_str());
    exit(1);
}

static std::string building_index_to_name (uint32_t building_idx) {
    const BuildingTypeStats* building_array = BuildingVector::get_building_data_array();
    uint32_t building_count = BuildingVector::get_building_data_count();
    if (building_array == nullptr) {
        printf("ERROR: Building data array is null\n");
        exit(1);
    }
    if (building_idx >= building_count) {
        printf("ERROR: Building index out of bounds: %u (max: %u)\n", building_idx, building_count);
        exit(1);
    }
    return building_array[building_idx].name;
}

static std::string resource_index_to_name (uint32_t resource_idx) {
    const ResourceTypeStats* resource_array = ResourceVector::get_resource_data_array();
    uint32_t resource_count = ResourceVector::get_resource_data_count();
    if (resource_idx >= resource_count) {
        printf("ERROR: Resource index out of bounds: %u (max: %u)\n", resource_idx, resource_count);
        exit(1);
    }
    return resource_array[resource_idx].name;
}

//================================================================================================================================
//=> - BuildableAssessor implementation -
//================================================================================================================================

BuildableAssessor::BuildableAssessor () {
    if (building_reqs_array == nullptr) {
        uint32_t building_count = BuildingVector::get_building_data_count();
        if (building_count > 0) {
            building_reqs_array = new BuildingReq[building_count];
            for (uint32_t i = 0; i < building_count; i++) {
                building_reqs_array[i].req0 = 0;
                building_reqs_array[i].req1 = 0;
                building_reqs_array[i].req2 = 0;
                building_reqs_array[i].req3 = 0;
            }
        }
    } 
}

BuildableAssessor::~BuildableAssessor () {
}

void BuildableAssessor::load_building_resource_costs (const std::string& filename) {
    if (building_reqs_array == nullptr) {
        return;
    }
    
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        return;
    }
    
    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");
    
    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() < 2) {
            continue;
        }
        
        std::string building_name = trimmer.trim(parts[0]);
        if (building_name.empty()) {
            continue;
        }
        
        uint32_t building_idx = building_name_to_index(building_name);
        if (building_idx == UINT32_MAX) {
            continue;
        }
        
        uint16_t req_count = 0;
        for (size_t j = 1; j < parts.size() && req_count < 4; j++) {
            std::string resource_name = trimmer.trim(parts[j]);
            if (resource_name.empty()) {
                continue;
            }
            
            uint32_t resource_idx = resource_name_to_index(resource_name);
            if (resource_idx == UINT32_MAX) {
                continue;
            }
            
            if (req_count == 0) {
                building_reqs_array[building_idx].req0 = static_cast<uint16_t>(resource_idx);
            } else if (req_count == 1) {
                building_reqs_array[building_idx].req1 = static_cast<uint16_t>(resource_idx);
            } else if (req_count == 2) {
                building_reqs_array[building_idx].req2 = static_cast<uint16_t>(resource_idx);
            } else if (req_count == 3) {
                building_reqs_array[building_idx].req3 = static_cast<uint16_t>(resource_idx);
            }
            req_count++;
        }
    }
}

void BuildableAssessor::print_building_resource_costs () {
    if (building_reqs_array == nullptr) {
        return;
    }
    
    uint32_t building_count = BuildingVector::get_building_data_count();
    for (uint32_t i = 0; i < building_count; i++) {
        std::string building_name = building_index_to_name(i);
        printf("%s", building_name.c_str());
        
        if (building_reqs_array[i].req0 != 0) {
            std::string res_name = resource_index_to_name(building_reqs_array[i].req0);
            printf(" : %s", res_name.c_str());
        }
        if (building_reqs_array[i].req1 != 0) {
            std::string res_name = resource_index_to_name(building_reqs_array[i].req1);
            printf(" : %s", res_name.c_str());
        }
        if (building_reqs_array[i].req2 != 0) {
            std::string res_name = resource_index_to_name(building_reqs_array[i].req2);
            printf(" : %s", res_name.c_str());
        }
        if (building_reqs_array[i].req3 != 0) {
            std::string res_name = resource_index_to_name(building_reqs_array[i].req3);
            printf(" : %s", res_name.c_str());
        }
        printf("\n");
    }
}

void BuildableAssessor::determine_buildable_buildings (BuildingVector* buildings, const ResourceVector& resources) {
    uint32_t building_count = BuildingVector::get_building_data_count();
    for (uint32_t i = 0; i < building_count; i++) {
        bool all_resources_available = true;
        
        if (resources.is_available(building_reqs_array[i].req0) != 1) {
            all_resources_available = false;
        }
        if (resources.is_available(building_reqs_array[i].req1) != 1) {
            all_resources_available = false;
        }
        if (resources.is_available(building_reqs_array[i].req2) != 1) {
            all_resources_available = false;
        }
        if (resources.is_available(building_reqs_array[i].req3) != 1) {
            all_resources_available = false;
        }
        
        if (all_resources_available) {
            buildings->set_buildable(i);
        }
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
