//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>
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
        printf ("*** Error: Failed to open file: %s\n", path);
        return;
    }
    dat15_top_hdr top_hdr;
    if (fread (&top_hdr, sizeof (top_hdr), 1, m_ptr) != 1) {
        printf ("*** Error: Failed to read top header: %s\n", path);
        return;
    }
    m_num_rows = top_hdr.num_rows;
    m_num_cols = top_hdr.num_cols;
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
    if (idx < 0 || idx >= (int)m_items.size()) {
        if (size_out) {
            *size_out = 0;
        }
        return nullptr;
    }
    if (size_out) {
        *size_out = m_items[idx].size();
    }
    return m_items[idx].data();
}

//================================================================================================================================
//=> - Write class -
//================================================================================================================================

Dat15Writer::Dat15Writer (const char* path, int num_rows, int num_cols) {
    m_ptr = fopen (path, "wb");
    m_pending.clear ();
    dat15_top_hdr top_hdr;
    top_hdr.num_rows = num_rows;
    top_hdr.num_cols = num_cols;
    fwrite(&top_hdr, sizeof(top_hdr), 1, m_ptr);
}

Dat15Writer::~Dat15Writer () {
    if (m_ptr) {
        finish ();
        fclose (m_ptr);
    }
}

void Dat15Writer::flush_pending (bool has_next) {
    if (m_pending.empty ()) {
        return;
    }
    write_header (m_ptr, m_pending.size (), has_next);
    fwrite (m_pending.data (), 1, m_pending.size (), m_ptr);
    m_pending.clear ();
}

void Dat15Writer::write (const void* data, int size) {
    flush_pending (true);
    m_pending.resize (size);
    memcpy (m_pending.data (), data, size);
}

void Dat15Writer::finish () {
    flush_pending (false);
}

//================================================================================================================================
//=> - Static globals -
//================================================================================================================================

static Dat15Writer* g_writer = nullptr;
static Dat15Reader* g_reader = nullptr;

//================================================================================================================================
//=> - C wrapper functions -
//================================================================================================================================

extern "C" {

void dat15_start_writer (const char* path, int num_rows, int num_cols) {
    if (g_writer) {
        delete g_writer;
    }
    g_writer = new Dat15Writer(path, num_rows, num_cols);
}

void dat15_write_item (const void* data, int size) {
    if (g_writer) {
        g_writer->write(data, size);
    }
}

void dat15_finish () {
    if (g_writer) {
        g_writer->finish();
        delete g_writer;
        g_writer = nullptr;
    }
}

void dat15_start_reader (const char* path) {
    if (g_reader) {
        delete g_reader;
    }
    g_reader = new Dat15Reader(path);
}

const void* dat15_get_item (int row, int col, int* size_out) {
    if (g_reader) {
        return g_reader->get_item(row, col, size_out);
    }
    if (size_out) {
        *size_out = 0;
    }
    return nullptr;
}

}

//================================================================================================================================
//=> - End -
//================================================================================================================================
