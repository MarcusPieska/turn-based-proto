//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef DAT_IO_H
#define DAT_IO_H

#include <cstdio>
#include <vector>

//================================================================================================================================
//=> - Read and write classes -
//================================================================================================================================

class Dat15Reader {
public:
    Dat15Reader (const char* path);
    ~Dat15Reader ();
    int get_count ();
    const void* get_item (int idx, int* size_out);
private:
    std::vector<std::vector<unsigned char> > m_items;
};
    
class Dat15Writer {
public:
    Dat15Writer (const char* path);
    ~Dat15Writer ();
    void write (const void* data, int size);
    void finish ();
private:
    FILE* m_ptr;
    std::vector<unsigned char> m_pending;
    void flush_pending (bool m_has_next);
};

//================================================================================================================================
//=> - C wrapper functions -
//================================================================================================================================

#ifdef __cplusplus
extern "C" {
#endif

void dat15_start_writer (const char* path);
void dat15_write_item (const void* data, int size);
void dat15_finish ();
void dat15_start_reader (const char* path);
int dat15_get_item_count ();
const void* dat15_get_item (int idx, int* size_out);

#ifdef __cplusplus
}
#endif

//================================================================================================================================
//=> - End -
//================================================================================================================================

#endif
