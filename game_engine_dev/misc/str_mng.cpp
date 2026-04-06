//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include <sstream>
#include <fstream>

#include "str_mng.h"

//================================================================================================================================
//=> - StringSplitter implementation -
//================================================================================================================================

std::vector<std::string> StringSplitter::split (const std::string& str) const {
    std::vector<std::string> result;
    if (delimiter.empty()) {
        result.push_back(str);
        return result;
    }
    size_t start = 0;
    size_t end = str.find(delimiter);
    while (end != std::string::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    result.push_back(str.substr(start));
    return result;
}

//================================================================================================================================
//=> - StringExtractor implementation -
//================================================================================================================================

bool StringExtractor::can_extract (const std::string& str) const {
    if (start_marker.empty() || end_marker.empty()) {
        return false;
    }
    size_t start_pos = str.find(start_marker);
    if (start_pos == std::string::npos) {
        return false;
    }
    size_t value_start_pos = start_pos + start_marker.size();
    size_t end_pos = str.find(end_marker, value_start_pos);
    if (end_pos == std::string::npos) {
        return false;
    }
    return value_start_pos <= end_pos;
}

std::string StringExtractor::extract (const std::string& str) const {
    if (!can_extract(str)) {
        return "";
    }
    size_t start_pos = str.find(start_marker);
    size_t value_start_pos = start_pos + start_marker.size();
    size_t end_pos = str.find(end_marker, value_start_pos);
    return str.substr(value_start_pos, end_pos - value_start_pos);
}

//================================================================================================================================
//=> - StringTrimmer implementation -
//================================================================================================================================

std::string StringTrimmer::trim (const std::string& str) const {
    if (str.empty() || trim_chars.empty()) {
        return str;
    }
    size_t start = 0;
    size_t end = str.length();
    while (start < end && trim_chars.find(str[start]) != std::string::npos) {
        start++;
    }
    while (end > start && trim_chars.find(str[end - 1]) != std::string::npos) {
        end--;
    }
    return str.substr(start, end - start);
}

//================================================================================================================================
//=> - StringReader implementation -
//================================================================================================================================

std::string StringReader::read () const {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

//================================================================================================================================
//=> - StringWriter implementation -
//================================================================================================================================

void StringWriter::append (const std::string& str) {
    content += str;
}

void StringWriter::write () const {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << content;
    }
    file.close();
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
