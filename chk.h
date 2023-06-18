#ifndef H_CHK
#define H_CHK
#include "types.h"
#include "isom.h"

typedef struct {
  u32 name;
  u32 size;
  union {
    u16 era;
    struct {
      u16 width;
      u16 height;
    } dim;
    u16 tiles[1];
    u8 data[1];
    u32 data32[1];
  };
} CHK;

bool loadMap(const char* path);
bool writeMap(const char* path);

void unloadCHK();
bool parseCHK(u8* data, u32 size);
void setCHKData(u32 section, void* data);
u8* getCHK(u32* size);

u32  getMapEra();
void getMapDim(u32* width, u32* height);
void getMapTILE(u16* buffer);
void getMapMTXM(u16* buffer);
void getMapISOM(ISOMRect* buffer);
void clearMapISOM();
bool hasTILEData();
bool hasMTXMData();
bool hasISOMData();

u16 getMTXMTile(u32 x, u32 y);
u16 getTILETile(u32 x, u32 y);

// CHK Sections
#define CHK_ERA  0x20415245
#define CHK_DIM  0x204D4944
#define CHK_MTXM 0x4D58544D
#define CHK_TILE 0x454C4954
#define CHK_ISOM 0x4D4F5349


#define MAX_MAP_DIM      256
#define MAX_ISOM_WIDTH  (MAX_MAP_DIM/2+1)
#define MAX_ISOM_HEIGHT (MAX_MAP_DIM+1)

#define MAX_TILE_SIZE (MAX_MAP_DIM * MAX_MAP_DIM * sizeof(u16))
#define MAX_ISOM_SIZE (MAX_ISOM_WIDTH * MAX_ISOM_HEIGHT * sizeof(ISOMRect))

#endif
