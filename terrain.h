#ifndef H_TERRAIN
#define H_TERRAIN
#include "types.h"

typedef struct {
  u16 id;
  u16 flags;
  union {
    struct {
      Rect16 edge;
      Rect16 piece;
    } group;
    struct {
      u16 overlayID;
      u16 isRemaster;
      u16 groupStr;
      u16 unused5;
      u16 doodadID;
      u16 width;
      u16 height;
      u16 unused9;
    } doodad;
  };
  u16 tiles[16];
} CV5;

typedef struct {
  u16 tiles[16];
} VF4;

typedef struct {
  u32 tiles[16];
} VX4EX;

typedef struct {
  u8 bmp[64];
} VR4;


void unloadTileset();
bool loadTileset(u32 tileset);

void copyPal(RGBA* pal);
void drawTile(u8* buf, s32 bufWidth, s32 bufHeight, s32 dstX, s32 dstY, u32 tileID, RGBA shading);
void drawMiniTile(u8* buf, s32 bufWidth, s32 bufHeight, s32 dstX, s32 dstY, u32 tileID, bool flip, RGBA shading);
void drawMinimap(u8* buf, s32 bufw, s32 bufh, u32 width, u32 height, u32 scale);


// Minimap scale sizes
#define MINIMAP_64   0  // 4 pixels per tile
#define MINIMAP_128  1  // 1 pixel per tile
#define MINIMAP_256  2  // 1 pixel per 4 tiles

#define CV5_DOODAD_ID 1

#endif
