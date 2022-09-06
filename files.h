#ifndef H_FILES
#define H_FILES
#include "types.h"

#define FILE_DISK      0
#define FILE_CASC      1
#define FILE_MPQ       2
#define FILE_ARCHIVE   3
#define FILE_MAP_FILE  4

void initArchiveData();
void closeArchiveData();

u8* readFile(const char* path, u32* filesize, u32 source);
bool readFileFixed(const char* path, void* buffer, u32 filesize, u32 source);
bool writeFile(const char* path, u8* data, u32 filesize, u32 destination);

#endif
