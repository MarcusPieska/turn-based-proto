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

class Dat31Reader {
public:
    Dat31Reader (const char* path);
    ~Dat31Reader ();
    const void* get_item (int index, int* size_out);
private:
    std::vector<std::vector<unsigned char> > m_items;
};
    
class Dat31Writer {
public:
    Dat31Writer (const char* path);
    ~Dat31Writer ();
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

void dat31_start_writer (const char* path);
void dat31_write_item (const void* data, int size);
void dat31_finish ();
void dat31_start_reader (const char* path);
const void* dat31_get_item (int index, int* size_out);

#ifdef __cplusplus
}
#endif

//================================================================================================================================
//=> - End -
//================================================================================================================================

#endif
