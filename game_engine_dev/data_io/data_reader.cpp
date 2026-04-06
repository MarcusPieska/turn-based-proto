//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdlib>
#include <string>

#include "str_mng.h"
#include "data_reader.h"

//================================================================================================================================
//=> - Private helper functions -
//================================================================================================================================

static bool is_comment_line (const std::string& line) {
    return line.size() >= 2 && line[0] == '/' && line[1] == '/';
}

//================================================================================================================================
//=> - DataReader implementation -
//================================================================================================================================

DataReader::DataReader (const std::string& filepath) :
    m_filepath(filepath) {
    read_and_parse();
}

const std::vector<RawItem>& DataReader::get_raw_items () const {
    return m_raw_items;
}

void DataReader::read_and_parse () {
    StringReader reader(m_filepath);
    std::string content = reader.read();
    if (content.empty()) {
        printf("ERROR: DataReader could not read file '%s'\n", m_filepath.c_str());
        std::exit(1);
    }

    StringSplitter line_splitter("\n");
    StringSplitter colon_splitter(":");
    StringTrimmer trimmer(" \t\r\n");

    std::vector<std::string> lines = line_splitter.split(content);
    m_raw_items.clear();

    for (size_t i = 0; i < lines.size(); ++i) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        if (is_comment_line(line)) {
            continue;
        }

        std::vector<std::string> parts = colon_splitter.split(line);
        std::string name = "";
        if (!parts.empty()) {
            name = trimmer.trim(parts[0]);
        }

        RawItem item;
        item.name = name;
        item.raw_line = line;
        m_raw_items.push_back(item);
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

