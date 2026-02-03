//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>
#include "dat31_io.h"

//================================================================================================================================
//=> - Header struct -
//================================================================================================================================

struct dat31_hdr {
    unsigned int has_next : 1;
    unsigned int size : 31;
};

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

static void write_header (FILE* f, int size, bool has_next) {
    dat31_hdr hdr;
    hdr.size = size;
    hdr.has_next = has_next ? 1 : 0;
    fwrite(&hdr, sizeof(hdr), 1, f);
}

//================================================================================================================================
//=> - Read class -
//================================================================================================================================

Dat31Reader::Dat31Reader (const char* path) {
    FILE* m_ptr = fopen (path, "rb");
    if (!m_ptr) { 
        printf ("*** Error: Failed to open file: %s\n", path);
        return;
    }
    while (1) {
        dat31_hdr hdr;
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

Dat31Reader::~Dat31Reader () {
}

const void* Dat31Reader::get_item (int index, int* size_out) {
    if (m_items.empty()) {
        if (size_out) {
            *size_out = 0;
        }
        return nullptr;
    }
    int idx = index % (int)m_items.size();
    if (idx < 0) {
        idx += m_items.size();
    }
    if (size_out) {
        *size_out = m_items[idx].size();
    }
    return m_items[idx].data();
}

//================================================================================================================================
//=> - Write class -
//================================================================================================================================

Dat31Writer::Dat31Writer (const char* path) {
    m_ptr = fopen (path, "wb");
    m_pending.clear ();
}

Dat31Writer::~Dat31Writer () {
    if (m_ptr) {
        finish ();
        fclose (m_ptr);
    }
}

void Dat31Writer::flush_pending (bool has_next) {
    if (m_pending.empty ()) {
        return;
    }
    write_header (m_ptr, m_pending.size (), has_next);
    fwrite (m_pending.data (), 1, m_pending.size (), m_ptr);
    m_pending.clear ();
}

void Dat31Writer::write (const void* data, int size) {
    flush_pending (true);
    m_pending.resize (size);
    memcpy (m_pending.data (), data, size);
}

void Dat31Writer::finish () {
    flush_pending (false);
}

//================================================================================================================================
//=> - Static globals -
//================================================================================================================================

static Dat31Writer* g_writer = nullptr;
static Dat31Reader* g_reader = nullptr;

//================================================================================================================================
//=> - C wrapper functions -
//================================================================================================================================

extern "C" {

void dat31_start_writer (const char* path) {
    if (g_writer) {
        delete g_writer;
    }
    g_writer = new Dat31Writer(path);
}

void dat31_write_item (const void* data, int size) {
    if (g_writer) {
        g_writer->write(data, size);
    }
}

void dat31_finish () {
    if (g_writer) {
        g_writer->finish();
        delete g_writer;
        g_writer = nullptr;
    }
}

void dat31_start_reader (const char* path) {
    if (g_reader) {
        delete g_reader;
    }
    g_reader = new Dat31Reader(path);
}

const void* dat31_get_item (int index, int* size_out) {
    if (g_reader) {
        return g_reader->get_item(index, size_out);
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
