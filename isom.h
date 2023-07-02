#ifndef H_ISOM
#define H_ISOM
#include "types.h"


bool initISOMData();
bool generateISOMData();

u16 getTileAt(u32 x, u32 y, RGBA* shading);

bool isISOMPartialEdgeSimple(u32 baseType, u32 cmpType, u32 rectSide);

// debug thing
void printPatterns();


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

typedef struct {
  u16 id;
  u16 ISOM;
} CV5ISOM;



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
#define ISOM_EDGE_COUNT    14

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

#define LEFT        0
#define UP          1
#define RIGHT       2
#define DOWN        3
#define UP_LEFT     4
#define UP_RIGHT    5
#define DOWN_LEFT   6
#define DOWN_RIGHT  7

#define CLIFF_NONE  0
#define CLIFF_LEFT  1
#define CLIFF_RIGHT 2

// color shading
#define SHADING_MISMATCHED  COLOR(255,128,  0,  64)
#define SHADING_MISALIGNED  COLOR(255,  0,  0,  64)
#define SHADING_INVALID     COLOR(  0,  0,  0, 128)
#define SHADING_NO_SHADING  0


// Terrain transition types
#define PATTERN_TYPE_NORMAL   0
#define PATTERN_TYPE_SIMPLE   1 // similar to normal, but with fewer transition tiles
#define PATTERN_TYPE_CLIFFS   2
#define PATTERN_TYPE_STACK    3 // same as cliff, but has tiles to transition directly to a different cliff type

// A = base terrain
// B = terrain being transitioned to
// C0,C1,C2,C3 = unique cliff edge values -- for non-cliff types, these evaluate to whatever "A" is
#define MATCH_A       1
#define MATCH_B       2
#define MATCH_C0      4 // Only exist for cliff types
#define MATCH_C1      8
#define MATCH_C2     12
#define MATCH_C3     16
#define MATCH_A_OR_C0 (MATCH_C0 | MATCH_A)
#define MATCH_A_OR_C1 (MATCH_C1 | MATCH_A)
#define MATCH_A_OR_C2 (MATCH_C2 | MATCH_A)
#define MATCH_A_OR_C3 (MATCH_C3 | MATCH_A)


// Quadrant indexes
#define PATTERN_TOP_LEFT     0
#define PATTERN_TOP_RIGHT    1
#define PATTERN_BOT_LEFT     2
#define PATTERN_BOT_RIGHT    3


// Types but as flags
#define SEL_NORMAL  1
#define SEL_SIMPLE  2
#define SEL_CLIFFS  4
#define SEL_STACK   8
#define SEL_ALL     0x0F
#define SEL_DEFAULT 0xF0  // rule applies to any terrain type not yet defined

// Restricts terrain group types for each quadrant
#define GROUP_BASIC  1
#define GROUP_EDGE   2
#define GROUP_STACK  4
#define GROUP_ANY    (GROUP_BASIC | GROUP_EDGE | GROUP_STACK)


// somewhat arbitrarily, the argument order starts with the top left horizontal value and goes clockwise:
//     1   2
//  0         3
//  7         4
//     6   5
// (which can't be any more arbitrary than the actual order, but makes it easier to copy the drawings)
#define PATTERN_EDGES(TL_V,TL_H, TR_H,TR_V, BR_V,BR_H, BL_H,BL_V)  {TL_H,TL_V, TR_H,TR_V, BR_H,BR_V, BL_V,BL_H}

// macro memes
#define _PAT_DNE(flags) ((((flags)>>4)^(flags))&15)
#define _PAT_SEL(arg,index,flags) ((((arg)|(((arg)>>4)&(_PAT_DNE(flags))))&(1<<(index)))==(1<<(index)))
#define _PAT_TILES(a,a0,a1,a2,a3, b,b0,b1,b2,b3, c,c0,c1,c2,c3, d,d0,d1,d2,d3) \
           {{_PAT_SEL(a,0,a|b|c|d)*(a0) | _PAT_SEL(b,0,a|b|c|d)*(b0) | _PAT_SEL(c,0,a|b|c|d)*(c0) | _PAT_SEL(d,0,a|b|c|d)*(d0),  \
             _PAT_SEL(a,0,a|b|c|d)*(a1) | _PAT_SEL(b,0,a|b|c|d)*(b1) | _PAT_SEL(c,0,a|b|c|d)*(c1) | _PAT_SEL(d,0,a|b|c|d)*(d1),  \
             _PAT_SEL(a,0,a|b|c|d)*(a2) | _PAT_SEL(b,0,a|b|c|d)*(b2) | _PAT_SEL(c,0,a|b|c|d)*(c2) | _PAT_SEL(d,0,a|b|c|d)*(d2),  \
             _PAT_SEL(a,0,a|b|c|d)*(a3) | _PAT_SEL(b,0,a|b|c|d)*(b3) | _PAT_SEL(c,0,a|b|c|d)*(c3) | _PAT_SEL(d,0,a|b|c|d)*(d3)}, \
            {_PAT_SEL(a,1,a|b|c|d)*(a0) | _PAT_SEL(b,1,a|b|c|d)*(b0) | _PAT_SEL(c,1,a|b|c|d)*(c0) | _PAT_SEL(d,1,a|b|c|d)*(d0),  \
             _PAT_SEL(a,1,a|b|c|d)*(a1) | _PAT_SEL(b,1,a|b|c|d)*(b1) | _PAT_SEL(c,1,a|b|c|d)*(c1) | _PAT_SEL(d,1,a|b|c|d)*(d1),  \
             _PAT_SEL(a,1,a|b|c|d)*(a2) | _PAT_SEL(b,1,a|b|c|d)*(b2) | _PAT_SEL(c,1,a|b|c|d)*(c2) | _PAT_SEL(d,1,a|b|c|d)*(d2),  \
             _PAT_SEL(a,1,a|b|c|d)*(a3) | _PAT_SEL(b,1,a|b|c|d)*(b3) | _PAT_SEL(c,1,a|b|c|d)*(c3) | _PAT_SEL(d,1,a|b|c|d)*(d3)}, \
            {_PAT_SEL(a,2,a|b|c|d)*(a0) | _PAT_SEL(b,2,a|b|c|d)*(b0) | _PAT_SEL(c,2,a|b|c|d)*(c0) | _PAT_SEL(d,2,a|b|c|d)*(d0),  \
             _PAT_SEL(a,2,a|b|c|d)*(a1) | _PAT_SEL(b,2,a|b|c|d)*(b1) | _PAT_SEL(c,2,a|b|c|d)*(c1) | _PAT_SEL(d,2,a|b|c|d)*(d1),  \
             _PAT_SEL(a,2,a|b|c|d)*(a2) | _PAT_SEL(b,2,a|b|c|d)*(b2) | _PAT_SEL(c,2,a|b|c|d)*(c2) | _PAT_SEL(d,2,a|b|c|d)*(d2),  \
             _PAT_SEL(a,2,a|b|c|d)*(a3) | _PAT_SEL(b,2,a|b|c|d)*(b3) | _PAT_SEL(c,2,a|b|c|d)*(c3) | _PAT_SEL(d,2,a|b|c|d)*(d3)}, \
            {_PAT_SEL(a,3,a|b|c|d)*(a0) | _PAT_SEL(b,3,a|b|c|d)*(b0) | _PAT_SEL(c,3,a|b|c|d)*(c0) | _PAT_SEL(d,3,a|b|c|d)*(d0),  \
             _PAT_SEL(a,3,a|b|c|d)*(a1) | _PAT_SEL(b,3,a|b|c|d)*(b1) | _PAT_SEL(c,3,a|b|c|d)*(c1) | _PAT_SEL(d,3,a|b|c|d)*(d1),  \
             _PAT_SEL(a,3,a|b|c|d)*(a2) | _PAT_SEL(b,3,a|b|c|d)*(b2) | _PAT_SEL(c,3,a|b|c|d)*(c2) | _PAT_SEL(d,3,a|b|c|d)*(d2),  \
             _PAT_SEL(a,3,a|b|c|d)*(a3) | _PAT_SEL(b,3,a|b|c|d)*(b3) | _PAT_SEL(c,3,a|b|c|d)*(c3) | _PAT_SEL(d,3,a|b|c|d)*(d3)}}
#define _PAT_TILES_1(a,a0,a1,a2,a3) _PAT_TILES(a,a0,a1,a2,a3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
#define _PAT_TILES_2(a,a0,a1,a2,a3, b,b0,b1,b2,b3) _PAT_TILES(a,a0,a1,a2,a3, b,b0,b1,b2,b3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
#define _PAT_TILES_3(a,a0,a1,a2,a3, b,b0,b1,b2,b3, c,c0,c1,c2,c3) _PAT_TILES(a,a0,a1,a2,a3, b,b0,b1,b2,b3, c,c0,c1,c2,c3, 0, 0, 0, 0, 0)
#define _PAT_TILES_4(a,a0,a1,a2,a3, b,b0,b1,b2,b3, c,c0,c1,c2,c3, d,d0,d1,d2,d3) _PAT_TILES(a,a0,a1,a2,a3, b,b0,b1,b2,b3, c,c0,c1,c2,c3, d,d0,d1,d2,d3)
#define _PAT_TILES_SEL(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,lmao,...) lmao

// macro for rules specifying tile group types allowed in each quadrant
#define PATTERN_TILES(...) _PAT_TILES_SEL(__VA_ARGS__,_PAT_TILES_4,,,,,_PAT_TILES_3,,,,,_PAT_TILES_2,,,,,_PAT_TILES_1)(__VA_ARGS__)

// specifies a rule for the specified terrain type(s)
#define PATTERN_TILES_DEF(sel_types, TL, TR, BL, BR) sel_types,TL,TR,BL,BR

// shorter macro specifying the same rule for all terrain types -- equivalent to PATTERN_TILES( PATTERN_TILES_DEF(SEL_ALL, TL, TR, BL, BR) )
#define PATTERN_TILES_ALL(TL, TR, BL, BR) {{TL,TR,BL,BR},{TL,TR,BL,BR},{TL,TR,BL,BR},{TL,TR,BL,BR}}

#endif
