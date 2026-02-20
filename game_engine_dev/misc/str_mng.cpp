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
