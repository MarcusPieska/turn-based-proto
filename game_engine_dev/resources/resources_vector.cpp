//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "str_mng.h"
#include "bit_array.h"

#include "resources_vector.h"

//================================================================================================================================
//=> - Private structures and globals -
//================================================================================================================================

static ResourceTypeStats* resource_data_array = nullptr;
static uint32_t resource_data_count = 0;

const ResourceTypeStats* ResourceVector::get_resource_data_array () {
    return resource_data_array;
}

uint32_t ResourceVector::get_resource_data_count () {
    return resource_data_count;
}

//================================================================================================================================
//=> - ResourceIO implementation -
//================================================================================================================================

int ResourceIO::validate_and_count () const {
    StringReader reader(m_filename);
    std::string content = reader.read();
    if (content.empty()) {
        return 0;
    }
    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");
    int count = 0;
    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() == 4) {
            bool valid = true;
            for (size_t j = 0; j < parts.size(); j++) {
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

void ResourceIO::print_content () const {
    StringReader reader(m_filename);
    std::string content = reader.read();
    if (content.empty()) {
        printf("File is empty or could not be read.\n");
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
        if (parts.size() == 4) {
            std::string name = trimmer.trim(parts[0]);
            std::string food = trimmer.trim(parts[1]);
            std::string shields = trimmer.trim(parts[2]);
            std::string income = trimmer.trim(parts[3]);
            if (!name.empty() && !food.empty() && !shields.empty() && !income.empty()) {
                printf("Resource: %s, Food: %s, Shields: %s, Income: %s\n", 
                       name.c_str(), food.c_str(), shields.c_str(), income.c_str());
            }
        }
    }
}

void ResourceIO::parse_and_allocate () const {
    if (resource_data_array != nullptr) {
        return;
    }
    int count = validate_and_count();
    if (count == 0) {
        return;
    }
    resource_data_count = static_cast<uint32_t>(count);
    resource_data_array = new ResourceTypeStats[resource_data_count];
    StringReader reader(m_filename);
    std::string content = reader.read();
    if (content.empty()) {
        return;
    }
    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");
    uint32_t index = 0;
    for (size_t i = 0; i < lines.size() && index < resource_data_count; i++) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() == 4) {
            std::string name = trimmer.trim(parts[0]);
            std::string food_str = trimmer.trim(parts[1]);
            std::string shields_str = trimmer.trim(parts[2]);
            std::string income_str = trimmer.trim(parts[3]);
            if (!name.empty() && !food_str.empty() && !shields_str.empty() && !income_str.empty()) {
                resource_data_array[index].name = name;
                resource_data_array[index].food = static_cast<uint16_t>(std::atoi(food_str.c_str()));
                resource_data_array[index].shields = static_cast<uint16_t>(std::atoi(shields_str.c_str()));
                resource_data_array[index].income = static_cast<uint16_t>(std::atoi(income_str.c_str()));
                index++;
            }
        }
    }
}

//================================================================================================================================
//=> - ResourceVector implementation -
//================================================================================================================================

ResourceVector::ResourceVector (uint32_t num_resources, BitArrayCL* resources_available) : 
    m_resources_available (resources_available) {
}

ResourceVector::~ResourceVector () {
}

ResourceData ResourceVector::get_resource (uint32_t index) const {
    ResourceData result{resource_data_array[index]};
    return result;
}

bool ResourceVector::is_available (uint32_t index) const {
    return m_resources_available->get_bit(index);
}

uint32_t ResourceVector::get_count () const {
    return m_resources_available->get_count();
}

void ResourceVector::set_available (uint32_t index) {
    if (index >= m_resources_available->get_count()) {
        return;
    }
    m_resources_available->set_bit(index);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
