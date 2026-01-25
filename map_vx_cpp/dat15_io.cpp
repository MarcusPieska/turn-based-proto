//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>
#include <iostream>
#include "dat15_io.h"

//================================================================================================================================
//=> - Header struct -
//================================================================================================================================

struct dat15_top_hdr {
    unsigned int num_rows : 16;
    unsigned int num_cols : 16;
};

struct dat15_hdr {
    unsigned int has_next : 1;
    unsigned int size : 15;
};

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

static void write_header (FILE* f, int size, bool has_next) {
    dat15_hdr hdr;
    hdr.size = size;
    hdr.has_next = has_next ? 1 : 0;
    fwrite(&hdr, sizeof(hdr), 1, f);
}

//================================================================================================================================
//=> - Read class -
//================================================================================================================================

Dat15Reader::Dat15Reader (const char* path) {
    FILE* m_ptr = fopen (path, "rb");
    if (!m_ptr) { 
        std::cerr << "*** Error: Failed to open file: " << path << std::endl;
        return;
    }
    dat15_top_hdr top_hdr;
    if (fread (&top_hdr, sizeof (top_hdr), 1, m_ptr) != 1) {
        std::cerr << "*** Error: Failed to read top header: " << path << std::endl;
        return;
    }
    m_num_rows = top_hdr.num_rows;
    m_num_cols = top_hdr.num_cols;
    std::cout << "*** Dat15Reader: " << m_num_rows << " x " << m_num_cols << std::endl;
    while (1) {
        dat15_hdr hdr;
        if (fread (&hdr, sizeof (hdr), 1, m_ptr) != 1) {
            break;
        }
        std::vector<unsigned char> item (hdr.size);
        if (fread (item.data (), 1, hdr.size, m_ptr) != hdr.size) {
            break;
        }
        m_items.push_back (item);
        if (!hdr.has_next) {
            break;
        }
    }
    fclose(m_ptr);
}

Dat15Reader::~Dat15Reader () {
}

const void* Dat15Reader::get_item (int row, int col, int* size_out) {
    row = row % m_num_rows;
    col = col % m_num_cols;
    int idx = row * m_num_cols + col;
    *size_out = m_items[idx].size();
    return m_items[idx].data();
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
