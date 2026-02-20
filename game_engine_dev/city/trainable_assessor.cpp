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
#include "unit_vector.h"
#include "resources_vector.h"

#include "trainable_assessor.h"

//================================================================================================================================
//=> - Private structures and globals -
//================================================================================================================================

typedef struct UnitReq {
    uint16_t req0;
    uint16_t req1;
    uint16_t req2;
    uint16_t req3;
} UnitReq;

static UnitReq* unit_reqs_array = nullptr;

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

static uint32_t unit_name_to_index (const std::string& unit_name) {
    const UnitTypeStats* unit_array = UnitVector::get_unit_data_array();
    uint32_t unit_count = UnitVector::get_unit_data_count();
    for (uint32_t i = 0; i < unit_count; i++) {
        if (unit_array[i].name == unit_name) {
            return i;
        }
    }
    printf("ERROR: Unit name not found: %s\n", unit_name.c_str());
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

static std::string unit_index_to_name (uint32_t unit_idx) {
    const UnitTypeStats* unit_array = UnitVector::get_unit_data_array();
    uint32_t unit_count = UnitVector::get_unit_data_count();
    if (unit_idx >= unit_count) {
        printf("ERROR: Unit index out of bounds: %u (max: %u)\n", unit_idx, unit_count);
        exit(1);
    }
    return unit_array[unit_idx].name;
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
//=> - TrainableAssessor implementation -
//================================================================================================================================

TrainableAssessor::TrainableAssessor () {
    if (unit_reqs_array == nullptr) {
        uint32_t unit_count = UnitVector::get_unit_data_count();
        if (unit_count > 0) {
            unit_reqs_array = new UnitReq[unit_count];
            for (uint32_t i = 0; i < unit_count; i++) {
                unit_reqs_array[i].req0 = 0;
                unit_reqs_array[i].req1 = 0;
                unit_reqs_array[i].req2 = 0;
                unit_reqs_array[i].req3 = 0;
            }
        }
    } 
}

TrainableAssessor::~TrainableAssessor () {
}

void TrainableAssessor::load_unit_resource_costs (const std::string& filename) {
    if (unit_reqs_array == nullptr) {
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
        
        std::string unit_name = trimmer.trim(parts[0]);
        if (unit_name.empty()) {
            continue;
        }
        
        uint32_t unit_idx = unit_name_to_index(unit_name);
        if (unit_idx == UINT32_MAX) {
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
                unit_reqs_array[unit_idx].req0 = static_cast<uint16_t>(resource_idx);
            } else if (req_count == 1) {
                unit_reqs_array[unit_idx].req1 = static_cast<uint16_t>(resource_idx);
            } else if (req_count == 2) {
                unit_reqs_array[unit_idx].req2 = static_cast<uint16_t>(resource_idx);
            } else if (req_count == 3) {
                unit_reqs_array[unit_idx].req3 = static_cast<uint16_t>(resource_idx);
            }
            req_count++;
        }
    }
}

void TrainableAssessor::print_unit_resource_costs () {
    if (unit_reqs_array == nullptr) {
        return;
    }
    
    uint32_t unit_count = UnitVector::get_unit_data_count();
    for (uint32_t i = 0; i < unit_count; i++) {
        std::string unit_name = unit_index_to_name(i);
        printf("%s", unit_name.c_str());
        
        if (unit_reqs_array[i].req0 != 0) {
            std::string res_name = resource_index_to_name(unit_reqs_array[i].req0);
            printf(" : %s", res_name.c_str());
        }
        if (unit_reqs_array[i].req1 != 0) {
            std::string res_name = resource_index_to_name(unit_reqs_array[i].req1);
            printf(" : %s", res_name.c_str());
        }
        if (unit_reqs_array[i].req2 != 0) {
            std::string res_name = resource_index_to_name(unit_reqs_array[i].req2);
            printf(" : %s", res_name.c_str());
        }
        if (unit_reqs_array[i].req3 != 0) {
            std::string res_name = resource_index_to_name(unit_reqs_array[i].req3);
            printf(" : %s", res_name.c_str());
        }
        printf("\n");
    }
}

void TrainableAssessor::determine_trainable_units (UnitVector* units, const ResourceVector& resources) {
    uint32_t unit_count = UnitVector::get_unit_data_count();
    for (uint32_t i = 0; i < unit_count; i++) {
        bool all_resources_available = true;
        
        if (resources.is_available(unit_reqs_array[i].req0) != 1) {
            all_resources_available = false;
        }
        if (resources.is_available(unit_reqs_array[i].req1) != 1) {
            all_resources_available = false;
        }
        if (resources.is_available(unit_reqs_array[i].req2) != 1) {
            all_resources_available = false;
        }
        if (resources.is_available(unit_reqs_array[i].req3) != 1) {
            all_resources_available = false;
        }
        
        if (all_resources_available) {
            units->set_trainable(i);
        }
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
