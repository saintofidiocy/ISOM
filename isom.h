#ifndef H_ISOM
#define H_ISOM
#include "types.h"

typedef union {
  struct {
    u16 edited  : 1;  // only used during tile placement in editor?
    u16 dir     : 3;
    u16 type    : 11;
    u16 skipped : 1;  // only used during tile placement in editor?
  };
  u16 raw;
} ISOMTile;

typedef struct {
  ISOMTile left;
  ISOMTile up;
  ISOMTile right;
  ISOMTile down;
} ISOMRect;

typedef struct {
  u16 isomType;
  u16 flags;
  u16 domain;
} TileDomain;

typedef struct {
  u32 flags;
  Rect16 bounds;
  RGBA color;
} Domain;

bool initISOMData();
bool generateISOMData();

u16 getTileAt(u32 x, u32 y, RGBA* shading);

bool isISOMPartialEdgeSimple(u32 baseType, u32 cmpType);


// ISOM edge IDs
#define ISOM_EDGE_NW       0
#define ISOM_EDGE_NE       1
#define ISOM_EDGE_SE       2
#define ISOM_EDGE_SW       3
#define ISOM_CORNER_OUT_N  4
#define ISOM_CORNER_OUT_E  5
#define ISOM_CORNER_OUT_S  6
#define ISOM_CORNER_OUT_W  7
#define ISOM_CORNER_IN_E   8
#define ISOM_CORNER_IN_W   9
#define ISOM_CORNER_IN_S   10
#define ISOM_CORNER_IN_N   11
#define ISOM_VERTICAL      12
#define ISOM_HORIZONTAL    13

// ISOM grid coordinates
#define DIR_TOP_LEFT_H   0
#define DIR_TOP_LEFT_V   1
#define DIR_TOP_RIGHT_H  2
#define DIR_TOP_RIGHT_V  3
#define DIR_BOT_RIGHT_H  4
#define DIR_BOT_RIGHT_V  5
#define DIR_BOT_LEFT_V   6
#define DIR_BOT_LEFT_H   7


// Tile flags for domain finder
#define TILE_LEFT_TILE        0x0000 // even group
#define TILE_RIGHT_TILE       0x0001 // odd group
#define TILE_BASIC_GROUP      0x0002 // plain terrain type, which can safely ignore left/right distinction
#define TILE_DOODAD_TILE      0x0004 // tile reindexes through a doodad
#define TILE_BORDER_LEFT      0x0000 // ISOM edge from bottom left to top right
#define TILE_BORDER_RIGHT     0x0008 // ISOM edge from top left to bottom right
#define TILE_HORZ_MISALIGN    0x0010 // tile is horizontally misaligned to the default ISOM grid
#define TILE_VERT_MISALIGN    0x0020 // tile is vertically misaligned to the default ISOM grid
#define TILE_ID_IS_DOMAIN     0x0040 // set if ID points to a domain ID, otherwise recursively references another tile ID
#define TILE_INVALID_ISOM     0x0080 // tile does not match neighboring tiles on an ISOM grid, or has no ISOM type
#define TILE_MISMATCH_LEFT    0x0100 // tile neighbors don't match
#define TILE_MISMATCH_UP      0x0200
#define TILE_MISMATCH_RIGHT   0x0400
#define TILE_MISMATCH_DOWN    0x0800
#define TILE_ISOM_GRID_0      0x1000 // offset  0, 0
#define TILE_ISOM_GRID_1      0x2000 // offset -1, 0
#define TILE_ISOM_GRID_2      0x4000 // offset  0,-1
#define TILE_ISOM_GRID_3      0x8000 // offset -1,-1

// Tile flag masks
#define TILE_COLUMN           TILE_RIGHT_TILE
#define TILE_BORDER_TYPE      TILE_BORDER_RIGHT
#define TILE_MISALIGNED       (TILE_HORZ_MISALIGN|TILE_VERT_MISALIGN)
#define TILE_MISMATCHED       (TILE_MISMATCH_LEFT|TILE_MISMATCH_UP|TILE_MISMATCH_RIGHT|TILE_MISMATCH_DOWN)
#define TILE_ISOM_GRID        (TILE_ISOM_GRID_0|TILE_ISOM_GRID_1|TILE_ISOM_GRID_2|TILE_ISOM_GRID_3)


// Domain flags
#define DOM_SINGLE_GRID       1
#define DOM_ISOM_GRID_0       TILE_ISOM_GRID_0
#define DOM_ISOM_GRID_1       TILE_ISOM_GRID_1
#define DOM_ISOM_GRID_2       TILE_ISOM_GRID_2
#define DOM_ISOM_GRID_3       TILE_ISOM_GRID_3

#define DOMAIN_NONE           0xFFFF

#define LEFT  0
#define UP    1
#define RIGHT 2
#define DOWN  3
#define UP_LEFT 4
#define UP_RIGHT 5
#define DOWN_LEFT 6
#define DOWN_RIGHT 7

#define CLIFF_NONE  0
#define CLIFF_LEFT  1
#define CLIFF_RIGHT 2

// color shading
#define SHADING_MISMATCHED  COLOR(255,128,  0,  64)
#define SHADING_MISALIGNED  COLOR(255,  0,  0,  64)
#define SHADING_INVALID     COLOR(  0,  0,  0, 128)
#define SHADING_NO_SHADING  0

#endif
