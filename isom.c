#include "isom.h"
#include "terrain.h"
#include "chk.h"

#define ISOMCoords(x,y) ((y)*(mapw/2+1) + (x)/2)
#define DomCoords(x,y)  (((y)+1)*(mapw+2) + (x)+2)

// Look-Up Tables

// remaps cliff-specific edge types to the equivalent base terrain type, since ISOM placement ignores them
const u8 EdgeTypes[5][38] = {
//    index:    0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37
/* badlands: */{0, 1, 2, 3, 4, 5, 0, 0, 0, 0,10,11, 0, 0,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,10,10,10,10, 1, 1, 1, 1, 5, 5, 5, 5},
/* platform: */{0, 1, 2, 3, 4, 5, 6, 7, 8, 2, 2, 2, 2,13,13,13,13,13,18,18,18,18,18, 1, 1, 1, 1, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0},
/*  install: */{0, 1, 2, 3, 4, 5, 6, 7, 1, 1, 1, 1, 2, 2, 2, 2, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
/* ashworld: */{0, 1, 2, 3, 4, 5, 6, 7, 8, 7, 7, 7, 7, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
/*   jungle: */{0, 1, 0, 0, 4, 5, 6, 7, 8, 9, 0,11,12,13, 0,15,16,17, 8, 8, 8, 8, 9, 9, 9, 9, 0, 0, 0, 0, 1, 1, 1, 1, 5, 5, 5, 5}
// desert, ice, twilight are same as jungle
};

// maps CV5 ID property to ISOM base types -- edge tiles have additional values added to this type depending on direction and other adjacent tiles
const u8 ISOMTypes[5][36] = {
//    index:    0 1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16   17   18   19   20   21   22   23   24   25   26   27   28   29   30   31   32   33   34   35
/* badlands: */{0,0,0x01,0x02,0x09,0x03,0x04,0x07,   0,   0,   0,   0,   0,   0,0x05,0x06,   0,   0,0x08,   0,0x29,0x45,0x6F,   0,   0,   0,   0,0x53,0x37,   0,   0,0x61,   0,   0,0x0D,0x1B},
/* platform: */{0,0,0x01,0x02,0x0B,0x04,0x0C,0x08,0x09,0x0A,0x0D,0x0E,   0,0x88,0x5E,0x6C,0x34,0x42,0x50,0x7A,0x18,0x26,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
/*  install: */{0,0,0x01,0x02,0x04,0x05,0x03,0x07,0x06,   0,0x32,0x40,0x16,0x24,0x4E,0x5C,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
/* ashworld: */{0,0,0x02,0x03,0x05,0x06,0x04,0x07,0x01,0x08,   0,0x37,0x45,0x53,0x61,0x6F,0x29,0x1B,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
/*   jungle: */{0,0,0x01,0x02,0x0D,0x03,   0,   0,0x04,0x05,0x09,0x07,0x0A,0x0B,   0,0x06,0x08,0x0C,   0,   0,   0,   0,0xAB,0x2D,0x73,0x57,0x81,   0,0x3B,0x49,0x8F,   0,0x65,0x9D,0x11,0x1F}
// desert, ice, twilight are same as jungle
};

// maps CV5 edge types to basic ISOM terrain types
const u8 ISOMEdgeTypes[5][38] = {
//    index:    0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37
/* badlands: */{0, 1, 4, 7, 2, 3, 0, 0, 0, 0, 5, 6, 0, 0, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 5, 5, 1, 1, 1, 1, 3, 3, 3, 3},
/* platform: */{0, 1, 2,11, 4,12, 8,14,13, 2, 2, 2, 2, 9, 9, 9, 9, 9,10,10,10,10,10, 1, 1, 1, 1, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0},
/*  install: */{0, 1, 2, 3, 4, 5, 7, 6, 1, 1, 1, 1, 2, 2, 2, 2, 7, 7, 7, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
/* ashworld: */{0, 2, 3, 5, 6, 4, 7, 1, 8, 1, 1, 1, 1, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
/*   jungle: */{0, 1, 0, 0, 2, 3, 7,10, 4, 9, 0, 6, 5,11, 0,13, 8,12, 4, 4, 4, 4, 9, 9, 9, 9, 0, 0, 0, 0, 1, 1, 1, 1, 3, 3, 3, 3}
// desert, ice, twilight are same as jungle
};

// Remaps cliff edge types for tile pairing checks
const u8 HorizCliffTypes[5][38] = {
//    index:    0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37
/* badlands: */{0, 1, 2, 3, 4, 5, 0, 0, 0, 0,10,11, 0, 0,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,26,26,10,10,30,30, 1, 1,34,34, 5, 5},
/* platform: */{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 2, 2,13,14,14,13,13,18,19,19,18,18,23,23, 1, 1,27,27, 2, 2, 0, 0, 0, 0, 0, 0, 0},
/*  install: */{0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 1, 1,12,12, 2, 2,16,16, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
/* ashworld: */{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 7, 7,13,13, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
/*   jungle: */{0, 1, 0, 0, 4, 5, 6, 7, 8, 9, 0,11,12,13, 0,15,16,17,18,18, 8, 8,22,22, 9, 9, 0, 0, 0, 0,30,30, 1, 1,34,34, 5, 5}
// desert, ice, twilight are same as jungle
};

// Edge IDs and group ranges for stacked cliff tiles
const u16 StackCliffs[5][8] = {
           // {lower cliff ID, upper cliff ID, first group ID, last group ID}
/*badlands: */{35,34,432,448, 0},
/*platform: */{17,21,214,230, 20,21,374,390},
/*install:  */{12,13,408,424, 15,12,424,440},
/*ashworld: */{17,16,368,384, 0},
/*jungle:   */{35,34,560,576, 0}
// desert, ice, twilight are same as jungle
};

// ISOM edge types for edge tiles that can be cut off to look like basic types
const struct {
  u8 basic;
  u8 edges[6];
} ISOMBasicEdges[5][11] = {
// badlands:
{ { 3, {0x1B, 0}},                     // water -> dirt
  { 1, {0x6F,0x0D,0x29,0x53,0x37, 0}}, // dirt  - mud, high dirt, grass, asphalt, rocky ground
  { 2, {0x29, 0}},                     // high dirt -> high grass
  { 5, {0x61, 0}},                     // asphalt -> structure
  { 6, {0x37, 0}},                     // rocky ground -> dirt
  0},
// platform:
{ { 1, {0x18, 0}},                     // space -> platform
  { 9, {0x42, 0}},                     // low platform -> platform
  {10, {0x50, 0}},                     // rusty pit -> platform
  { 2, {0x88,0x5E,0x34,0x26,0x7A, 0}}, // platform -> dark platform, plating, solar array, high platform, elevated catwalk
  { 4, {0x6C, 0}},                     // high platform -> high plating
  0},
// install:
{ { 1, {0x16,0x32,0x4E, 0}},           // substructure -> floor, substructure plating, substructure panels
  { 2, {0x24,0x40, 0}},                // floor -> roof, plating
  { 7, {0x5C, 0}},                     // bottomless pit -> substructure
  0},
// ashworld:
{ { 1, {0x1B, 0}},                     // magma -> dirt
  { 2, {0x37,0x53,0x6F,0x29, 0}},      // dirt -> lava, shale, broken rock, high dirt
  { 4, {0x53, 0}},                     // shale -> dirt
  { 5, {0x45,0x61, 0}},                // high dirt -> high lava, high shale
  { 7, {0x61, 0}},                     // high shale -> high dirt
  0},
// jungle, desert, ice, twilight:
{ { 3, {0x1F, 0}},                     // water -> dirt
  { 1, {0xAB,0x2D,0x3B,0x11, 0}},      // dirt -> mud, jungle, rocky ground, high dirt
  { 4, {0x57,0x49,0x65, 0}},           // jungle -> ruins, raised jungle, temple
  { 6, {0x3B, 0}},                     // rocky ground -> dirt
  { 7, {0x57, 0}},                     // ruins -> jungle
  { 5, {0x49, 0}},                     // raised jungle -> jungle
  { 2, {0x73, 0}},                     // high dirt -> high jungle
  { 9, {0x81,0x8F,0x9D, 0}},           // high jungle -> high ruins, high raised jungle, high temple
  {10, {0x81, 0}},                     // high ruins -> high jungle
  {11, {0x8F, 0}},                     // high raised jungle -> high jungle
  0},
};


// terrain values
extern CV5* cv5;
extern u32 cv5count;
extern u16 dddata[512][256];

u32 tileset = 0;
s32 mapw = 0;
s32 maph = 0;
u16 maptiles[MAX_MAP_DIM*MAX_MAP_DIM] = {0};
ISOMRect isom[MAX_ISOM_WIDTH*MAX_ISOM_HEIGHT] = {0};

u16 groups[MAX_MAP_DIM*MAX_MAP_DIM] = {0};
u16 edges[MAX_ISOM_WIDTH*MAX_ISOM_HEIGHT*2] = {0};

TileDomain tileDoms[(MAX_MAP_DIM+1)*(MAX_MAP_DIM+2)] = {0};
Domain domains[65536] = {0};
u32 domainCount = 0;


void parseTiles();
bool validateTILE();
bool validateISOM(u32* cellsChecked, u32* cellsValid);

void generateISOMGrids();

u32 getISOMTypeAt(s32 x, s32 y);
bool isISOMCellAt(s32 x, s32 y);
bool doesISOMCellEqualType(s32 x, s32 y, u32 type);
bool isISOMPartialEdge(s32 x, s32 y, u32 type, u32 rectSide);
void setISOMCellAt(s32 x, s32 y, u32 type);

bool checkISOMTiles(u16 tiles[], u16 flags[]);
u32 getRectISOMType(u16 tiles[]);

u32 getCV5Index(u16 tile);
u32 getEdgeType(u16 id);
u32 getISOMType(u16 id);
u32 getISOMFromEdge(u16 id);

bool isBasicGroup(u16 group);
bool isBasicType(u16 id);
u32 getCliffType(u16 id);



bool initISOMData(){
  bool hasTILE = false;
  bool validTILE = false;
  bool hasISOM = false;
  bool validISOM = false;
  u32 cellsChecked = 0;
  u32 cellsValid = 0;
  char statusText[260] = "File loaded successfully!";
  char* textAppend = statusText + strlen(statusText);
  
  // get map data
  tileset = getMapEra();
  getMapDim(&mapw, &maph);
  getMapMTXM(maptiles);
  getMapISOM(isom);
  
  // desert, ice, twilight are equivalent to jungle
  if(tileset > 4) tileset = 4;
  
  // clear buffers
  memset(groups, 0, sizeof(groups));
  memset(edges, 0, sizeof(edges));
  memset(tileDoms, 0, sizeof(tileDoms));
  memset(domains, 0, sizeof(domains));
  domainCount = 0;
  
  // set tile flags & determine valid ISOM regions
  parseTiles();
  
  // check if TILE and MTXM match
  if(hasTILEData() && hasMTXMData()){
    hasTILE = true;
    validTILE = validateTILE();
    if(validTILE){
      strcpy(textAppend, " -- TILE and MTXM match");
    }else{
      strcpy(textAppend, " -- TILE and MTXM mismatch");
    }
    textAppend += strlen(textAppend);
  }
  
  if(hasISOMData()){
    hasISOM = true;
    validISOM = validateISOM(&cellsChecked, &cellsValid);
    if(cellsChecked != 0){
      sprintf(textAppend, " -- %d of %d cells match -- Map ISOM data %s", cellsValid, cellsChecked, validISOM?"valid!":"invalid");
    }else{
      sprintf(textAppend, " -- Map ISOM data invalid");
      validISOM = false;
    }
  }else{
    strcpy(textAppend, " -- No ISOM data found");
  }
  textAppend += strlen(textAppend);
  
  if(!validISOM){
    puts("Generate w/e");
    generateISOMGrids();
    
    // find ISOM domains
    
    if(domainCount == 1){
      strcpy(textAppend, " -- Single grid domain");
      textAppend += strlen(textAppend);
      if(((domains[0].flags & TILE_ISOM_GRID) & (DOM_ISOM_GRID_1|DOM_ISOM_GRID_3)) == 0){
        strcpy(textAppend, " (valid ISOM)");
      }else{
        strcpy(textAppend, " (horizontal misalignment)");
      }
    }
  }
  
  setStatusText(statusText);
  return validISOM;
}



bool generateISOMData(){
  s32 x,y;
  u32 gridID;
  u32 gridFlag;
  s32 domIndex;
  u32 domain;
  
  puts("generate ISOM data");
  
  // memset(isom, 0, sizeof(isom)); ?
  
  if((domains[0].flags & DOM_SINGLE_GRID) && (domains[0].flags & (DOM_ISOM_GRID_1|DOM_ISOM_GRID_3))){
    puts("impossible ISOM grid");
    return false;
  }
  
  for(y = -1; y < maph; y++){
    for(x = -2; x < mapw; x++){
      domIndex = DomCoords(x,y);
      gridID = (4 - 2*(y&1) - x) & 3;
      gridFlag = DOM_ISOM_GRID_0 << gridID;
      domain = tileDoms[domIndex].domain;
      
      if(domain == DOMAIN_NONE) continue;
      
      if((domains[domain].flags & DOM_SINGLE_GRID) && (domains[domain].flags & TILE_ISOM_GRID) == gridFlag){
        setISOMCellAt(x, y, tileDoms[domIndex].isomType);
      }
      
      // multiple domains ???
    }
  }
  
  setCHKData(CHK_ISOM, isom);
  
  return true;
}


// returns MTXM tile and tile drawing properties
u16 getTileAt(u32 x, u32 y, RGBA* shading){
  if(x >= mapw || y >= maph) return 0;
  
  if(shading != NULL){
    u32 flags = tileDoms[DomCoords(x,y)].flags;
    
    // get shading
    if(flags & TILE_INVALID_ISOM){
      shading->raw = SHADING_INVALID;
    }else if(flags & TILE_MISMATCHED){
      if(flags & TILE_HORZ_MISALIGN){
        shading->raw = BLEND(SHADING_MISMATCHED, SHADING_MISALIGNED);
      }else{
        shading->raw = SHADING_MISMATCHED;
      }
    }else if(flags & TILE_HORZ_MISALIGN){
      shading->raw = SHADING_MISALIGNED;
    }else{
      switch(flags & TILE_ISOM_GRID){
        case 0:
          shading->raw = COLOR(  0,  0,  0, 32);
          break;
        case TILE_ISOM_GRID_0:
          shading->raw = COLOR(0,255,0, 32);
          break;
        case TILE_ISOM_GRID_1:
          shading->raw = COLOR(255,0,255, 32);
          break;
        case TILE_ISOM_GRID_2:
          shading->raw = COLOR(0,255,255, 32);
          break;
        case TILE_ISOM_GRID_3:
          shading->raw = COLOR(0,0,255,32);
          break;
        case TILE_ISOM_GRID:
          shading->raw = COLOR(255,255,255, 128);
          break;
        default:
          shading->raw = SHADING_NO_SHADING;
      }
    }
  }
  
  return getMTXMTile(x,y);
}







void parseTiles(){
  s32 x,y;
  u32 i;
  u32 group;
  u32 mismatchMask;
  u32 domIndex;
  
  // pass 1: get group IDs, set basic flags
  for(y = 0, i = 0; y < maph; y++){
    for(x = 0; x < mapw; x++,i++){
      groups[i] = getCV5Index(maptiles[i]);
      group = groups[i];
      
      domIndex = i+(mapw+2)+2;
      
      // is null?
      if(cv5[group].id == 0 || ((maptiles[i] & 0xF) != 0 && cv5[maptiles[i] >> 4].tiles[maptiles[i] & 0xF] == 0)){
        tileDoms[domIndex].flags = TILE_INVALID_ISOM;
        continue;
      }
      
      // basic flags
      tileDoms[domIndex].flags |= group & TILE_COLUMN; // TILE_LEFT_TILE, TILE_RIGHT_TILE
      if(isBasicGroup(group)) {
        tileDoms[domIndex].flags |= TILE_BASIC_GROUP;
      }
      if(group != 0 && group != (maptiles[i] >> 4)){
        tileDoms[domIndex].flags |= TILE_DOODAD_TILE;
      }
      
      // alignment flags probably not useful since grid domains account for this?
      
      // misalign flags
      if((group & TILE_COLUMN) != (x & TILE_COLUMN)){
        //printf("%d %d\n", group, x);
        tileDoms[domIndex].flags |= TILE_HORZ_MISALIGN;
      }
      // TILE_VERT_MISALIGN?
    }
  }
  
  // pass 2: set neighbor flags
  for(y = 0, i = 0; y < maph; y++){
    for(x = 0; x < mapw; x++,i++){
      group = groups[i];
      
      // skip invalid/null tiles
      if(tileDoms[domIndex].flags & TILE_INVALID_ISOM){
        continue;
      }
      
      // mismatch flags
      if(x < mapw-1){
        if(group == 0 || groups[i+1] == 0 // tile is null
          || ((group & TILE_COLUMN) == TILE_LEFT_TILE && groups[i+1] != group+1) // right tile does not match left tile
          //|| ((group & TILE_COLUMN) == TILE_RIGHT_TILE && !isValidISOMPair(group, groups[i+1], RIGHT)) // right tile has an invalid neighbor
        ){
          tileDoms[domIndex].flags |= TILE_MISMATCH_RIGHT;
          tileDoms[domIndex+1].flags |= TILE_MISMATCH_LEFT;
          printf("(%3d,%3d) LR mismatch %3d,%3d : {%2d,%2d,%2d,%2d}, {%2d,%2d,%2d,%2d}\n", x,y, group, groups[i+1], cv5[group].group.edge.left, cv5[group].group.edge.up, cv5[group].group.edge.right, cv5[group].group.edge.down, cv5[groups[i+1]].group.edge.left, cv5[groups[i+1]].group.edge.up, cv5[groups[i+1]].group.edge.right, cv5[groups[i+1]].group.edge.down);
        }
      }
      if(y < maph-1){
        /*if(groups[i] == 0 || groups[i+mapw] == 0 || !isValidISOMPair(group, groups[i+mapw], DOWN)){
          tileDoms[domIndex].flags |= TILE_MISMATCH_DOWN;
          tileDoms[domIndex+mapw].flags |= TILE_MISMATCH_UP;
          printf("UD mismatch %3d,%3d : {%2d,%2d,%2d,%2d}, {%2d,%2d,%2d,%2d}\n", group, groups[i+mapw], cv5[group].group.edge.left, cv5[group].group.edge.up, cv5[group].group.edge.right, cv5[group].group.edge.down, cv5[groups[i+mapw]].group.edge.left, cv5[groups[i+mapw]].group.edge.up, cv5[groups[i+mapw]].group.edge.right, cv5[groups[i+mapw]].group.edge.down);
        }*/
      }
      
      // get valid mismatch flags
      mismatchMask = 0;
      if(x > 0) mismatchMask |= TILE_MISMATCH_LEFT;
      if(y > 0) mismatchMask |= TILE_MISMATCH_UP;
      if(x < mapw-1) mismatchMask |= TILE_MISMATCH_RIGHT;
      if(y < maph-1) mismatchMask |= TILE_MISMATCH_DOWN;
      
      // no matching neighbors; not a valid tile
      if((tileDoms[domIndex].flags & TILE_MISMATCHED) == mismatchMask || // all valid neighbors are mismatched
         (tileDoms[domIndex].flags & (TILE_COLUMN | TILE_BASIC_GROUP | TILE_MISMATCH_RIGHT)) == (TILE_LEFT_TILE | TILE_MISMATCH_RIGHT) || // left tile does not match right tile (unless a basic tile)
         (tileDoms[domIndex].flags & (TILE_COLUMN | TILE_BASIC_GROUP | TILE_MISMATCH_LEFT)) == (TILE_RIGHT_TILE | TILE_MISMATCH_LEFT)) // right tile does not match left tile (unless a basic tile)
      {
        tileDoms[domIndex].flags |= TILE_INVALID_ISOM;
      }
      
      /*
      // can shift
      if(x < 2 || groups[i-2] == 0 || isValidISOMPair(group, groups[i-2], LEFT)){
        tileDoms[domIndex].flags |= TILE_CAN_SHIFT_LEFT;
      }
      if(x >= mapw-2 || groups[i+2] == 0 || isValidISOMPair(group, groups[i+2], RIGHT)){
        tileDoms[domIndex].flags |= TILE_CAN_SHIFT_RIGHT;
      }
      if(y < 2 || groups[i-2*mapw] == 0 || isValidISOMPair(group, groups[i-2*mapw], UP)){
        tileDoms[domIndex].flags |= TILE_CAN_SHIFT_UP;
      }
      if(y >= maph-2 || groups[i+2*maph] == 0 || isValidISOMPair(group, groups[i+2*mapw], DOWN)){
        tileDoms[domIndex].flags |= TILE_CAN_SHIFT_DOWN;
      }
      */
    }
  }
}


bool validateTILE(){
  u32 x,y;
  for(y = 0; y < maph; y++){
    for(x = 0; x < mapw; x++){
      if(groups[y*mapw + x] != getCV5Index(getTILETile(x,y))){
        return false;
      }
    }
  }
  return true;
}


bool validateISOM(u32* cellsChecked, u32* cellsValid){
  if(!hasISOMData()) return false;
  
  s32 x,y;
  s32 isomIndex;
  s32 tileIndex;
  s32 domIndex;
  u32 id;
  u32 i,j;
  u32 cmpType;
  
  bool isValid;
  u32 validISOM = 0;
  u32 checkISOM = 0;
  
  for(y = -1; y < maph; y++){
    for(x = -2; x < mapw; x += 2){
      tileIndex = y*mapw + x;
      domIndex = tileIndex + (mapw+2) + 2;
      
      if(isISOMCellAt(x,y)){
        checkISOM++;
        id = getISOMTypeAt(x, y);
        
        if(x < 0){
          if(y < 0) cmpType = isom[ISOMCoords(x+2,y+1)].left.type;
          else cmpType = isom[ISOMCoords(x+2,y)].left.type;
        }else{
          if(y < 0) cmpType = isom[ISOMCoords(x,y+1)].right.type;
          else cmpType = isom[ISOMCoords(x,y)].right.type;
        }
        
        isValid = doesISOMCellEqualType(x,y, id);
        if(!isValid){
          if(y < 0){
            if(x < 0){
              isValid = isISOMPartialEdge(x,y, id, DOWN_RIGHT);
            }else if(x >= mapw-2){
              isValid = isISOMPartialEdge(x,y, id, DOWN_LEFT);
            }else{
              isValid = isISOMPartialEdge(x,y, id, DOWN);
            }
          }else if(y >= maph-1){
            if(x < 0){
              isValid = isISOMPartialEdge(x,y, id, UP_RIGHT);
            }else if(x >= mapw-2){
              isValid = isISOMPartialEdge(x,y, id, UP_LEFT);
            }else{
              isValid = isISOMPartialEdge(x,y, id, UP);
            }
          }else{
            if(x < 0){
              isValid = isISOMPartialEdge(x,y, id, RIGHT);
            }else if(x >= mapw-2){
              isValid = isISOMPartialEdge(x,y, id, LEFT);
            }
          }
        }
        if(isValid){
          validISOM++;
        }else{
          j = 0;
          for(i = 0; i < 36; i++){
            if(ISOMTypes[tileset][i] <= cmpType && ISOMTypes[tileset][i] > ISOMTypes[tileset][j]) j = i;
          }
          printf("(%3d,%3d) %03x (actual: %03x): %3d, %3d; %3d, %3d\t\t%03x + %d\n", x,y, id, cmpType, (y<0||x<0)?0: groups[tileIndex], (y<0||x>=mapw-2)?0: groups[tileIndex+2], (y>=maph-1||x<0)?0: groups[tileIndex+mapw], (y>=mapw-1||x>=mapw-2)?0: groups[tileIndex+mapw+2], ISOMTypes[tileset][j], cmpType-ISOMTypes[tileset][j]);
          if(y >= 0){
            if(x >= 0){
              tileDoms[domIndex].flags |= TILE_MISMATCHED;
              tileDoms[domIndex+1].flags |= TILE_MISMATCHED;
            }
            if(x < mapw-2){
              tileDoms[domIndex+2].flags |= TILE_MISMATCHED;
              tileDoms[domIndex+3].flags |= TILE_MISMATCHED;
            }
          }
          if(y < maph-1){
            if(x >= 0){
              tileDoms[domIndex+mapw].flags |= TILE_MISMATCHED;
              tileDoms[domIndex+mapw+1].flags |= TILE_MISMATCHED;
            }
            if(x < mapw-2){
              tileDoms[domIndex+mapw+2].flags |= TILE_MISMATCHED;
              tileDoms[domIndex+mapw+3].flags |= TILE_MISMATCHED;
            }
          }
        }
      }
    }
  }
  
  printf("Valid: %d of %d\n", validISOM, checkISOM);
  if(cellsChecked != NULL) *cellsChecked = checkISOM;
  if(cellsValid != NULL) *cellsValid = validISOM;
  return validISOM == checkISOM;
}



void generateISOMGrids(){
  s32 x,y;
  u32 type;
  u32 i;
  
  //u32 gridCount[4] = {0};
  u32 gridID;
  s32 domIndex;
  
  u32 gridSet;
  u32 singleGrids = TILE_ISOM_GRID;
  u32 singleGridID;
  u8 hasGridSet[16] = {0};
  
  for(y = -1; y < maph; y++){
    for(x = -2; x < mapw; x++){
      gridID = (4 - 2*(y&1) - x) & 3;
      domIndex = DomCoords(x,y);
      
      type = getISOMTypeAt(x,y);
      tileDoms[domIndex].isomType = type;
      tileDoms[domIndex].domain = DOMAIN_NONE;
      if(type != 0){
        //gridCount[gridID]++;
        for(i = 0; i < 4; i++){
          if(x+i < mapw){
            tileDoms[domIndex+i].flags |= TILE_ISOM_GRID_0 << gridID;
            if(y < maph-1) tileDoms[domIndex+mapw+2+i].flags |= TILE_ISOM_GRID_0 << gridID;
          }
        }
      }
    }
  }
  
  for(y = 0; y < maph; y++){
    for(x = 0; x < mapw; x++){
      gridID = (4 - 2*(y&1) - x) & 3;
      domIndex = DomCoords(x,y);
      
      gridSet = tileDoms[domIndex].flags & TILE_ISOM_GRID;
      if((tileDoms[domIndex].flags & TILE_INVALID_ISOM) == 0){
        singleGrids &= gridSet;
        hasGridSet[gridSet / TILE_ISOM_GRID_0] = 1;
      }
    }
  }
  
  printf("%X, ", singleGrids / TILE_ISOM_GRID_0);
  for(i = 0; i < 16; i++){
    printf("%d", hasGridSet[i]);
  }
  puts("");
  
  // if multiple grids exist then select only one -- preference is grid 0, grid 2, then whatever
  if(singleGrids & TILE_ISOM_GRID_0){
    singleGrids &= TILE_ISOM_GRID_0;
    singleGridID = 0;
  }else if(singleGrids & TILE_ISOM_GRID_2){
    singleGrids &= TILE_ISOM_GRID_2;
    singleGridID = 2;
  }else if(singleGrids & TILE_ISOM_GRID_1){
    singleGrids &= TILE_ISOM_GRID_1;
    singleGridID = 1;
  }else if(singleGrids & TILE_ISOM_GRID_3){
    singleGrids &= TILE_ISOM_GRID_3;
    singleGridID = 3;
  }
  
  // if a single grid exists, then clear all others and set as a singular domain
  if(singleGrids){
    printf("single grid: %d\n", singleGridID);
    for(y = -1; y < maph; y++){
      for(x = -2; x < mapw; x++){
        gridID = (4 - 2*(y&1) - x) & 3;
        domIndex = DomCoords(x,y);
        
        tileDoms[domIndex].flags &= singleGrids | ~TILE_ISOM_GRID; // clear all grid flags other than single grid
        if(singleGridID == gridID){
          tileDoms[domIndex].domain = 0;
        }
      }
    }
    domains[0].color.raw = SHADING_NO_SHADING;
    domains[0].bounds.left = -2;
    domains[0].bounds.up = -1;
    domains[0].bounds.right = mapw;
    domains[0].bounds.down = maph;
    domains[0].flags = singleGrids | DOM_SINGLE_GRID;
    domainCount = 1;
    return;
  }
  
  // multiple domains ???
}






u32 getISOMTypeAt(s32 x, s32 y){
  u16 tiles[8] = {0};
  u16 flags[8] = {TILE_INVALID_ISOM, TILE_INVALID_ISOM, TILE_INVALID_ISOM, TILE_INVALID_ISOM,
                  TILE_INVALID_ISOM, TILE_INVALID_ISOM, TILE_INVALID_ISOM, TILE_INVALID_ISOM};
  s32 i;
  
  // rectangle fully out of bounds -- all tiles undefined
  if(x < -3 || x >= mapw || y < -1 || y >= maph){
    printf("out of bounds %d,%d -- %d,%d,%d,%d\n", x,y, x < -3 , x >= mapw , y < -1 , y >= maph);
    return 0;
  }
  
  // check custom patterns ?
  
  
  
  s32 tileIndex = y*mapw + x;
  s32 domIndex = tileIndex + (mapw+2) + 2;
  //s32 isomIndex = ISOMCoords(x0,y0);
  
  // copy tiles
  for(i = 0; i < 4; i++){
    if(i+x < 0) continue;
    if(i+x >= mapw) break;
    if(y >= 0){
      tiles[i] = groups[tileIndex+i];
      flags[i] = tileDoms[domIndex+i].flags;
    }
    if(y+1 < maph){
      tiles[4+i] = groups[tileIndex+mapw+i];
      flags[4+i] = tileDoms[domIndex+mapw+i].flags;
    }
  }
  
  // validate tiles
  if(checkISOMTiles(tiles, flags) == false){
    puts("invalid ISOM tiles");
    // invalid ISOM tiles
    return 0;
  }
  
  //printf("tiles: %4d %4d %4d %4d\n       %4d %4d %4d %4d\n", tiles[0], tiles[1], tiles[2], tiles[3], tiles[4], tiles[5], tiles[6], tiles[7]);
  
  return getRectISOMType(tiles);
}



bool isISOMCellAt(s32 x, s32 y){
  s32 isomIndex = ISOMCoords(x,y);
  if(y >= 0){
    if(x >= 0){
      if(isom[isomIndex].right.dir != DIR_TOP_LEFT_H) return false;
      if(isom[isomIndex].down.dir != DIR_TOP_LEFT_V) return false;
    }
    if(x < mapw-2){
      if(isom[isomIndex+1].left.dir != DIR_TOP_RIGHT_H) return false;
      if(isom[isomIndex+1].down.dir != DIR_TOP_RIGHT_V) return false;
    }
  }
  if(y < maph-1){
    if(x >= 0){
      if(isom[isomIndex+mapw/2+1].right.dir != DIR_BOT_LEFT_H) return false;
      if(isom[isomIndex+mapw/2+1].up.dir != DIR_BOT_LEFT_V) return false;
    }
    if(x < mapw-2){
      if(isom[isomIndex+mapw/2+2].left.dir != DIR_BOT_RIGHT_H) return false;
      if(isom[isomIndex+mapw/2+2].up.dir != DIR_BOT_RIGHT_V) return false;
    }
  }
  return true;
}

bool doesISOMCellEqualType(s32 x, s32 y, u32 type){
  s32 isomIndex = ISOMCoords(x,y);
  if(y >= 0){
    if(x >= 0){
      if(isom[isomIndex].right.type != type) return false;
      if(isom[isomIndex].down.type != type) return false;
    }
    if(x < mapw-2){
      if(isom[isomIndex+1].left.type != type) return false;
      if(isom[isomIndex+1].down.type != type) return false;
    }
  }
  if(y < maph-1){
    if(x >= 0){
      if(isom[isomIndex+mapw/2+1].right.type != type) return false;
      if(isom[isomIndex+mapw/2+1].up.type != type) return false;
    }
    if(x < mapw-2){
      if(isom[isomIndex+mapw/2+2].left.type != type) return false;
      if(isom[isomIndex+mapw/2+2].up.type != type) return false;
    }
  }
  return true;
}

bool isISOMPartialEdge(s32 x, s32 y, u32 type, u32 rectSide){
  u32 i,j;
  
  //printf("partial (%d,%d), %d, %s\n", x,y,type, rectSide?"Right":"Left");
  
  for(i = 0; ISOMBasicEdges[tileset][i].basic != type; i++){
    if(ISOMBasicEdges[tileset][i].basic == 0) return false;
  }
  
  for(j = 0; ISOMBasicEdges[tileset][i].edges[j] != 0; j++){
    switch(rectSide){
      case LEFT:
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[tileset][i].edges[j] + ISOM_CORNER_OUT_W)) return true;
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[tileset][i].edges[j] + ISOM_CORNER_IN_W)) return true;
        break;
      case RIGHT:
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[tileset][i].edges[j] + ISOM_CORNER_OUT_E)) return true;
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[tileset][i].edges[j] + ISOM_CORNER_IN_E)) return true;
        break;
      case UP:
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[tileset][i].edges[j] + ISOM_CORNER_OUT_N)) return true;
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[tileset][i].edges[j] + ISOM_CORNER_IN_N)) return true;
        break;
      case DOWN:
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[tileset][i].edges[j] + ISOM_CORNER_OUT_S)) return true;
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[tileset][i].edges[j] + ISOM_CORNER_IN_S)) return true;
        break;
      case UP_LEFT:
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[tileset][i].edges[j] + ISOM_EDGE_NW)) return true;
        break;
      case UP_RIGHT:
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[tileset][i].edges[j] + ISOM_EDGE_NE)) return true;
        break;
      case DOWN_LEFT:
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[tileset][i].edges[j] + ISOM_EDGE_SW)) return true;
        break;
      case DOWN_RIGHT:
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[tileset][i].edges[j] + ISOM_EDGE_SE)) return true;
        break;
      default:
        return false;
    }
  }
  return false;
}

void setISOMCellAt(s32 x, s32 y, u32 type){
  s32 isomIndex = ISOMCoords(x,y);
  if(y >= 0){
    if(x >= 0){
      isom[isomIndex].right.dir = DIR_TOP_LEFT_H;
      isom[isomIndex].right.type = type;
      isom[isomIndex].down.dir = DIR_TOP_LEFT_V;
      isom[isomIndex].down.type = type;
    }
    if(x < mapw-2){
      isom[isomIndex+1].left.dir = DIR_TOP_RIGHT_H;
      isom[isomIndex+1].left.type = type;
      isom[isomIndex+1].down.dir = DIR_TOP_RIGHT_V;
      isom[isomIndex+1].down.type = type;
    }
  }
  if(y < maph-1){
    if(x >= 0){
      isom[isomIndex+mapw/2+1].right.dir = DIR_BOT_LEFT_H;
      isom[isomIndex+mapw/2+1].right.type = type;
      isom[isomIndex+mapw/2+1].up.dir = DIR_BOT_LEFT_V;
      isom[isomIndex+mapw/2+1].up.type = type;
    }
    if(x < mapw-2){
      isom[isomIndex+mapw/2+2].left.dir = DIR_BOT_RIGHT_H;
      isom[isomIndex+mapw/2+2].left.type = type;
      isom[isomIndex+mapw/2+2].up.dir = DIR_BOT_RIGHT_V;
      isom[isomIndex+mapw/2+2].up.type = type;
    }
  }
}



//u32 getCustomISOM(s32 x, s32 y){}



bool checkISOMTiles(u16 tiles[], u16 flags[]){
  u32 i;
  u32 mask;
  u32 id;
  
  // return false if all tiles are undefined
  for(i = 0; i < 8; i++){
    if(tiles[i] != 0) break;
  }
  if(i == 8){
    printf("all tiles are undefined; ");
    return false;
  }
  
  // fill in pairs
  for(i = 0; i < 8; i++){
    if(flags[i] & TILE_BASIC_GROUP){
      // basic group ignores left/right
      flags[i] = (flags[i] & ~TILE_COLUMN) | (i & TILE_COLUMN);
      tiles[i] = (tiles[i] & ~TILE_COLUMN) | (i & TILE_COLUMN);
      if(((flags[i^1] & TILE_BASIC_GROUP) && (tiles[i] | TILE_COLUMN) == tiles[i^1] | TILE_COLUMN) || tiles[i^1] == 0){
        flags[i^1] = flags[i] ^ TILE_COLUMN;
        tiles[i^1] = tiles[i] ^ TILE_COLUMN;
      }
    }else if(tiles[i] != 0 && (flags[i] & TILE_COLUMN) == (i & TILE_COLUMN)){
      // non-basic types must be correctly paired
      if((flags[i^1] & TILE_COLUMN) == ((i^1) & TILE_COLUMN) && tiles[i^1] == 0){
        flags[i^1] = flags[i] ^ TILE_COLUMN;
        tiles[i^1] = tiles[i] ^ TILE_COLUMN;
      }
      if((tiles[i]^1) != tiles[i^1]){
        printf("[%d]==%d && [%d]==%d; ", i, tiles[i], i^1, tiles[i^1]);
        // invalid pair
        return false;
      }
    }else if(tiles[i] != 0){
      printf("invalid nonzero tile %d (%04X); ", tiles[i], flags[i]);
      // invalid pair
      return false;
    }
  }
  
  return true;
}

u32 getRectISOMType(u16 tiles[]){
  u32 id;
  u32 i;
  u32 edgeTypes[8];
  
  u32 isomType = 0;
  u32 isomSubtype = 0;
  
  // are all nonzero tiles the same ID?
  id = 0;
  for(i = 0; i < 8; i += 2){
    if(tiles[i] != 0){
      if(id == 0){
        id = tiles[i];
      }else if(tiles[i] != id){
        break;
      }
    }
  }
  if(id == 0) return 0;
  if(i == 8 && isBasicGroup(id)){ // all nonzero tiles are a basic group
    return getISOMType(cv5[id].id);
  }
  
  edgeTypes[DIR_TOP_LEFT_V] = getEdgeType(cv5[tiles[0]].group.edge.down);
  edgeTypes[DIR_BOT_LEFT_V] = getEdgeType(cv5[tiles[4]].group.edge.up);
  edgeTypes[DIR_TOP_LEFT_H] = getEdgeType(cv5[tiles[0]].group.edge.right);
  edgeTypes[DIR_TOP_RIGHT_H] = getEdgeType(cv5[tiles[2]].group.edge.left);
  edgeTypes[DIR_TOP_RIGHT_V] = getEdgeType(cv5[tiles[2]].group.edge.down);
  edgeTypes[DIR_BOT_RIGHT_V] = getEdgeType(cv5[tiles[6]].group.edge.up);
  edgeTypes[DIR_BOT_LEFT_H] = getEdgeType(cv5[tiles[4]].group.edge.right);
  edgeTypes[DIR_BOT_RIGHT_H] = getEdgeType(cv5[tiles[6]].group.edge.left);
  /*edgeTypes[DIR_TOP_LEFT_V] = cv5[tiles[0]].group.edge.down;
  edgeTypes[DIR_BOT_LEFT_V] = cv5[tiles[4]].group.edge.up;
  edgeTypes[DIR_TOP_LEFT_H] = cv5[tiles[0]].group.edge.right;
  edgeTypes[DIR_TOP_RIGHT_H] = cv5[tiles[2]].group.edge.left;
  edgeTypes[DIR_TOP_RIGHT_V] = cv5[tiles[2]].group.edge.down;
  edgeTypes[DIR_BOT_RIGHT_V] = cv5[tiles[6]].group.edge.up;
  edgeTypes[DIR_BOT_LEFT_H] = cv5[tiles[4]].group.edge.right;
  edgeTypes[DIR_BOT_RIGHT_H] = cv5[tiles[6]].group.edge.left;
  
  // are all nonzero edges the same ID?
  id = 0;
  for(i = 0; i < 8; i++){
    if(edgeTypes[i] != 0){
      if(id == 0){
        id = edgeTypes[i];
      }else if(edgeTypes[i] != id){
        break;
      }
      
      // turn raw edge type into usable edge type
      edgeTypes[i] = getEdgeType(edgeTypes[i]);
    }
  }
  if(id == 0) return 0; // all edges are 0
  if(i == 8 && isBasicType(id)){ // all edges are basic types
    return getISOMFromEdge(id);
  }*/
  
  // undefined values always match
  if(edgeTypes[DIR_TOP_LEFT_V] == 0) edgeTypes[DIR_TOP_LEFT_V] = edgeTypes[DIR_BOT_LEFT_V];
  if(edgeTypes[DIR_BOT_LEFT_V] == 0) edgeTypes[DIR_BOT_LEFT_V] = edgeTypes[DIR_TOP_LEFT_V];
  if(edgeTypes[DIR_TOP_LEFT_H] == 0) edgeTypes[DIR_TOP_LEFT_H] = edgeTypes[DIR_TOP_RIGHT_H];
  if(edgeTypes[DIR_TOP_RIGHT_H] == 0) edgeTypes[DIR_TOP_RIGHT_H] = edgeTypes[DIR_TOP_LEFT_H];
  if(edgeTypes[DIR_TOP_RIGHT_V] == 0) edgeTypes[DIR_TOP_RIGHT_V] = edgeTypes[DIR_BOT_RIGHT_V];
  if(edgeTypes[DIR_BOT_RIGHT_V] == 0) edgeTypes[DIR_BOT_RIGHT_V] = edgeTypes[DIR_TOP_RIGHT_V];
  if(edgeTypes[DIR_BOT_LEFT_H] == 0) edgeTypes[DIR_BOT_LEFT_H] = edgeTypes[DIR_BOT_RIGHT_H];
  if(edgeTypes[DIR_BOT_RIGHT_H] == 0) edgeTypes[DIR_BOT_RIGHT_H] = edgeTypes[DIR_BOT_LEFT_H];
  
  
  //printf("(%d,%d,%d,%d) -- L:%d,%d, U:%d,%d, R:%d,%d, D:%d,%d\n", TL, TR, BL, BR, edgeTypes[DIR_TOP_LEFT_V],edgeTypes[DIR_BOT_LEFT_V], edgeTypes[DIR_TOP_LEFT_H],edgeTypes[DIR_TOP_RIGHT_H], edgeTypes[DIR_TOP_RIGHT_V],edgeTypes[DIR_BOT_RIGHT_V], edgeTypes[DIR_BOT_LEFT_H],edgeTypes[DIR_BOT_RIGHT_H]);
  
  
  // clean these up to put similar checks together ?
  
  // north west
  if(edgeTypes[DIR_BOT_LEFT_H] == 51 && edgeTypes[DIR_BOT_RIGHT_H] == 51 && edgeTypes[DIR_TOP_RIGHT_V] == 51 && edgeTypes[DIR_BOT_RIGHT_V] == 51){
    if(edgeTypes[DIR_TOP_LEFT_H] == edgeTypes[DIR_TOP_RIGHT_H] && edgeTypes[DIR_TOP_LEFT_V] == edgeTypes[DIR_BOT_LEFT_V]){
      isomType = getISOMType(cv5[tiles[6]].id);
      isomSubtype = ISOM_EDGE_NW;
    }
  }
  if(edgeTypes[DIR_BOT_LEFT_H] == 51 && edgeTypes[DIR_BOT_RIGHT_H] == 51 && edgeTypes[DIR_TOP_RIGHT_V] == 0 && edgeTypes[DIR_BOT_RIGHT_V] == 0){
    if(edgeTypes[DIR_TOP_LEFT_H] == edgeTypes[DIR_TOP_RIGHT_H] && edgeTypes[DIR_TOP_LEFT_V] == edgeTypes[DIR_BOT_LEFT_V]){
      isomType = getISOMType(cv5[tiles[4]].id);
      isomSubtype = ISOM_EDGE_NW;
    }
  }
  if(edgeTypes[DIR_BOT_LEFT_H] == 0 && edgeTypes[DIR_BOT_RIGHT_H] == 0 && edgeTypes[DIR_TOP_RIGHT_V] == 51 && edgeTypes[DIR_BOT_RIGHT_V] == 51){
    if(edgeTypes[DIR_TOP_LEFT_H] == edgeTypes[DIR_TOP_RIGHT_H] && edgeTypes[DIR_TOP_LEFT_V] == edgeTypes[DIR_BOT_LEFT_V]){
      isomType = getISOMType(cv5[tiles[2]].id);
      isomSubtype = ISOM_EDGE_NW;
    }
  }
  // north east
  if(edgeTypes[DIR_BOT_LEFT_H] == 49 && edgeTypes[DIR_BOT_RIGHT_H] == 49 && edgeTypes[DIR_TOP_LEFT_V] == 49 && edgeTypes[DIR_BOT_LEFT_V] == 49){
    if(edgeTypes[DIR_TOP_LEFT_H] == edgeTypes[DIR_TOP_RIGHT_H] && edgeTypes[DIR_TOP_RIGHT_V] == edgeTypes[DIR_BOT_RIGHT_V]){
      isomType = getISOMType(cv5[tiles[4]].id);
      isomSubtype = ISOM_EDGE_NE;
    }
  }
  if(edgeTypes[DIR_BOT_LEFT_H] == 49 && edgeTypes[DIR_BOT_RIGHT_H] == 49 && edgeTypes[DIR_TOP_LEFT_V] == 0 && edgeTypes[DIR_BOT_LEFT_V] == 0){
    if(edgeTypes[DIR_TOP_LEFT_H] == edgeTypes[DIR_TOP_RIGHT_H] && edgeTypes[DIR_TOP_RIGHT_V] == edgeTypes[DIR_BOT_RIGHT_V]){
      isomType = getISOMType(cv5[tiles[6]].id);
      isomSubtype = ISOM_EDGE_NE;
    }
  }
  if(edgeTypes[DIR_BOT_LEFT_H] == 0 && edgeTypes[DIR_BOT_RIGHT_H] == 0 && edgeTypes[DIR_TOP_LEFT_V] == 49 && edgeTypes[DIR_BOT_LEFT_V] == 49){
    if(edgeTypes[DIR_TOP_LEFT_H] == edgeTypes[DIR_TOP_RIGHT_H] && edgeTypes[DIR_TOP_RIGHT_V] == edgeTypes[DIR_BOT_RIGHT_V]){
      isomType = getISOMType(cv5[tiles[0]].id);
      isomSubtype = ISOM_EDGE_NE;
    }
  }
  // south east
  if(edgeTypes[DIR_TOP_LEFT_H] == 52 && edgeTypes[DIR_TOP_RIGHT_H] == 52 && edgeTypes[DIR_TOP_LEFT_V] == 52 && edgeTypes[DIR_BOT_LEFT_V] == 52){
    if(edgeTypes[DIR_BOT_LEFT_H] == edgeTypes[DIR_BOT_RIGHT_H] && edgeTypes[DIR_TOP_RIGHT_V] == edgeTypes[DIR_BOT_RIGHT_V]){
      isomType = getISOMType(cv5[tiles[0]].id);
      isomSubtype = ISOM_EDGE_SE;
    }
  }
  if(edgeTypes[DIR_TOP_LEFT_H] == 0 && edgeTypes[DIR_TOP_RIGHT_H] == 0 && edgeTypes[DIR_TOP_LEFT_V] == 52 && edgeTypes[DIR_BOT_LEFT_V] == 52){
    if(edgeTypes[DIR_BOT_LEFT_H] == edgeTypes[DIR_BOT_RIGHT_H] && edgeTypes[DIR_TOP_RIGHT_V] == edgeTypes[DIR_BOT_RIGHT_V]){
      isomType = getISOMType(cv5[tiles[4]].id);
      isomSubtype = ISOM_EDGE_SE;
    }
  }
  if(edgeTypes[DIR_TOP_LEFT_H] == 52 && edgeTypes[DIR_TOP_RIGHT_H] == 52 && edgeTypes[DIR_TOP_LEFT_V] == 0 && edgeTypes[DIR_BOT_LEFT_V] == 0){
    if(edgeTypes[DIR_BOT_LEFT_H] == edgeTypes[DIR_BOT_RIGHT_H] && edgeTypes[DIR_TOP_RIGHT_V] == edgeTypes[DIR_BOT_RIGHT_V]){
      isomType = getISOMType(cv5[tiles[2]].id);
      isomSubtype = ISOM_EDGE_SE;
    }
  }
  if(edgeTypes[DIR_TOP_LEFT_H] == 0 && edgeTypes[DIR_TOP_RIGHT_H] == 0 && edgeTypes[DIR_TOP_LEFT_V] == 0 && edgeTypes[DIR_BOT_LEFT_V] == 0){
    if(getCliffType(cv5[tiles[6]].group.edge.left) != CLIFF_NONE && getCliffType(cv5[tiles[6]].group.edge.up) != CLIFF_NONE){
      if(tiles[6] >= StackCliffs[tileset][2] && tiles[6] < StackCliffs[tileset][3]){
        isomType = getISOMType(StackCliffs[tileset][1]);
      }else if(tiles[6] >= StackCliffs[tileset][6] && tiles[6] < StackCliffs[tileset][7]){
        isomType = getISOMType(StackCliffs[tileset][5]);
      }else{
        isomType = getISOMType(cv5[tiles[6]].id);
      }
      isomSubtype = ISOM_EDGE_SE;
    }
  }
  // south west
  if(edgeTypes[DIR_TOP_LEFT_H] == 50 && edgeTypes[DIR_TOP_RIGHT_H] == 50 && edgeTypes[DIR_TOP_RIGHT_V] == 50 && edgeTypes[DIR_BOT_RIGHT_V] == 50){
    if(edgeTypes[DIR_BOT_LEFT_H] == edgeTypes[DIR_BOT_RIGHT_H] && edgeTypes[DIR_TOP_LEFT_V] == edgeTypes[DIR_BOT_LEFT_V]){
      isomType = getISOMType(cv5[tiles[2]].id);
      isomSubtype = ISOM_EDGE_SW;
    }
  }
  if(edgeTypes[DIR_TOP_LEFT_H] == 0 && edgeTypes[DIR_TOP_RIGHT_H] == 0 && edgeTypes[DIR_TOP_RIGHT_V] == 50 && edgeTypes[DIR_BOT_RIGHT_V] == 50){
    if(edgeTypes[DIR_BOT_LEFT_H] == edgeTypes[DIR_BOT_RIGHT_H] && edgeTypes[DIR_TOP_LEFT_V] == edgeTypes[DIR_BOT_LEFT_V]){
      isomType = getISOMType(cv5[tiles[6]].id);
      isomSubtype = ISOM_EDGE_SW;
    }
  }
  if(edgeTypes[DIR_TOP_LEFT_H] == 50 && edgeTypes[DIR_TOP_RIGHT_H] == 50 && edgeTypes[DIR_TOP_RIGHT_V] == 0 && edgeTypes[DIR_BOT_RIGHT_V] == 0){
    if(edgeTypes[DIR_BOT_LEFT_H] == edgeTypes[DIR_BOT_RIGHT_H] && edgeTypes[DIR_TOP_LEFT_V] == edgeTypes[DIR_BOT_LEFT_V]){
      isomType = getISOMType(cv5[tiles[0]].id);
      isomSubtype = ISOM_EDGE_SW;
    }
  }
  if(edgeTypes[DIR_TOP_LEFT_H] == 0 && edgeTypes[DIR_TOP_RIGHT_H] == 0 && edgeTypes[DIR_TOP_RIGHT_V] == 0 && edgeTypes[DIR_BOT_RIGHT_V] == 0){
    if(getCliffType(cv5[tiles[4]].group.edge.right) != CLIFF_NONE && getCliffType(cv5[tiles[4]].group.edge.up) != CLIFF_NONE){
      if(tiles[4] >= StackCliffs[tileset][2] && tiles[4] < StackCliffs[tileset][3]){
        isomType = getISOMType(StackCliffs[tileset][1]);
      }else if(tiles[4] >= StackCliffs[tileset][6] && tiles[4] < StackCliffs[tileset][7]){
        isomType = getISOMType(StackCliffs[tileset][5]);
      }else{
        isomType = getISOMType(cv5[tiles[4]].id);
      }
      isomSubtype = ISOM_EDGE_SW;
    }
  }
  
  // north outer corner
  if(edgeTypes[DIR_BOT_LEFT_H] == 51 && edgeTypes[DIR_BOT_RIGHT_H] == 49){
    if(edgeTypes[DIR_TOP_LEFT_H] == edgeTypes[DIR_TOP_RIGHT_H] && edgeTypes[DIR_TOP_LEFT_V] == edgeTypes[DIR_BOT_LEFT_V] && edgeTypes[DIR_TOP_RIGHT_V] == edgeTypes[DIR_BOT_RIGHT_V]){
      isomType = getISOMType(cv5[tiles[4]].id);
      isomSubtype = ISOM_CORNER_OUT_N;
    }
  }
  // east outer corner
  if(edgeTypes[DIR_TOP_LEFT_V] == 54 && edgeTypes[DIR_BOT_LEFT_V] == 54){
    if(edgeTypes[DIR_TOP_LEFT_H] == edgeTypes[DIR_TOP_RIGHT_H] && edgeTypes[DIR_BOT_LEFT_H] == edgeTypes[DIR_BOT_RIGHT_H] && edgeTypes[DIR_TOP_RIGHT_V] == edgeTypes[DIR_BOT_RIGHT_V]){
      isomType = getISOMType(cv5[tiles[0]].id);
      isomSubtype = ISOM_CORNER_OUT_E;
    }
  }
  // south outer corner
  if(edgeTypes[DIR_TOP_LEFT_H] == 50 && edgeTypes[DIR_TOP_RIGHT_H] == 52){
    if(edgeTypes[DIR_BOT_LEFT_H] == edgeTypes[DIR_BOT_RIGHT_H] && edgeTypes[DIR_TOP_LEFT_V] == edgeTypes[DIR_BOT_LEFT_V] && edgeTypes[DIR_TOP_RIGHT_V] == edgeTypes[DIR_BOT_RIGHT_V]){
      isomType = getISOMType(cv5[tiles[0]].id);
      isomSubtype = ISOM_CORNER_OUT_S;
    }
  }
  // west outer corner
  if(edgeTypes[DIR_TOP_RIGHT_V] == 53 && edgeTypes[DIR_BOT_RIGHT_V] == 53){
    if(edgeTypes[DIR_TOP_LEFT_H] == edgeTypes[DIR_TOP_RIGHT_H] && edgeTypes[DIR_BOT_LEFT_H] == edgeTypes[DIR_BOT_RIGHT_H] && edgeTypes[DIR_TOP_LEFT_V] == edgeTypes[DIR_BOT_LEFT_V]){
      isomType = getISOMType(cv5[tiles[2]].id);
      isomSubtype = ISOM_CORNER_OUT_W;
    }
  }
  
  // east inner corner
  if(edgeTypes[DIR_TOP_RIGHT_V] == 56 && edgeTypes[DIR_BOT_RIGHT_V] == 56){
    if(/*edgeTypes[DIR_TOP_LEFT_H] == edgeTypes[DIR_TOP_RIGHT_H] && edgeTypes[DIR_BOT_LEFT_H] == edgeTypes[DIR_BOT_RIGHT_H] &&*/ edgeTypes[DIR_TOP_LEFT_V] == edgeTypes[DIR_BOT_LEFT_V]){
      isomType = getISOMType(cv5[tiles[2]].id);
      isomSubtype = ISOM_CORNER_IN_E;
    }
  }
  if(edgeTypes[DIR_TOP_LEFT_H] == 50 && edgeTypes[DIR_BOT_LEFT_H] == 51){
    if(edgeTypes[DIR_BOT_LEFT_V] == edgeTypes[DIR_TOP_LEFT_V] && edgeTypes[DIR_TOP_RIGHT_H] == edgeTypes[DIR_TOP_RIGHT_V] && edgeTypes[DIR_BOT_RIGHT_H] == edgeTypes[DIR_BOT_RIGHT_V] && edgeTypes[DIR_TOP_RIGHT_H] == edgeTypes[DIR_BOT_RIGHT_H]){
      isomType = getISOMType(cv5[tiles[0]].id);
      isomSubtype = ISOM_CORNER_IN_E;
    }
    if(edgeTypes[DIR_BOT_LEFT_V] == edgeTypes[DIR_TOP_LEFT_V] && edgeTypes[DIR_TOP_RIGHT_V] == 0 && edgeTypes[DIR_BOT_RIGHT_V] == 0 && edgeTypes[DIR_TOP_RIGHT_H] == 50 && edgeTypes[DIR_BOT_RIGHT_H] == 51){
      isomType = getISOMType(cv5[tiles[0]].id);
      isomSubtype = ISOM_CORNER_IN_E;
    }
  }
  // west inner corner
  if(edgeTypes[DIR_TOP_LEFT_V] == 55 && edgeTypes[DIR_BOT_LEFT_V] == 55){
    if(/*edgeTypes[DIR_TOP_LEFT_H] == edgeTypes[DIR_TOP_RIGHT_H] && edgeTypes[DIR_BOT_LEFT_H] == edgeTypes[DIR_BOT_RIGHT_H] &&*/ edgeTypes[DIR_TOP_RIGHT_V] == edgeTypes[DIR_BOT_RIGHT_V]){
      isomType = getISOMType(cv5[tiles[0]].id);
      isomSubtype = ISOM_CORNER_IN_W;
    }
  }
  if(edgeTypes[DIR_TOP_RIGHT_H] == 52 && edgeTypes[DIR_BOT_RIGHT_H] == 49){
    if(edgeTypes[DIR_BOT_RIGHT_V] == edgeTypes[DIR_TOP_RIGHT_V] && edgeTypes[DIR_TOP_LEFT_H] == edgeTypes[DIR_TOP_LEFT_V] && edgeTypes[DIR_BOT_LEFT_H] == edgeTypes[DIR_BOT_LEFT_V] && edgeTypes[DIR_TOP_LEFT_H] == edgeTypes[DIR_BOT_LEFT_H]){
      isomType = getISOMType(cv5[tiles[2]].id);
      isomSubtype = ISOM_CORNER_IN_W;
    }
    if(edgeTypes[DIR_BOT_RIGHT_V] == edgeTypes[DIR_TOP_RIGHT_V] && edgeTypes[DIR_TOP_LEFT_V] == 0 && edgeTypes[DIR_BOT_LEFT_V] == 0 && edgeTypes[DIR_TOP_LEFT_H] == 52 && edgeTypes[DIR_BOT_LEFT_H] == 49){
      isomType = getISOMType(cv5[tiles[2]].id);
      isomSubtype = ISOM_CORNER_IN_W;
    }
  }
  // south inner corner
  if(edgeTypes[DIR_BOT_LEFT_H] == 49 && edgeTypes[DIR_BOT_RIGHT_H] == 51){
    if(edgeTypes[DIR_TOP_LEFT_H] == edgeTypes[DIR_TOP_RIGHT_H] && edgeTypes[DIR_TOP_LEFT_V] == edgeTypes[DIR_BOT_LEFT_V] && edgeTypes[DIR_TOP_RIGHT_V] == edgeTypes[DIR_BOT_RIGHT_V]){
      isomType = getISOMType(cv5[tiles[4]].id);
      isomSubtype = ISOM_CORNER_IN_S;
    }
  }
  if(edgeTypes[DIR_BOT_LEFT_H] == 0 && edgeTypes[DIR_BOT_RIGHT_H] == 0 && edgeTypes[DIR_TOP_LEFT_V] == 49 && edgeTypes[DIR_TOP_RIGHT_V] == 51){
    if(edgeTypes[DIR_TOP_LEFT_H] == edgeTypes[DIR_TOP_RIGHT_H]){
      isomType = getISOMType(cv5[tiles[0]].id);
      isomSubtype = ISOM_CORNER_IN_S;
    }
  }
  // north inner corner
  if(edgeTypes[DIR_TOP_LEFT_H] == 52 && edgeTypes[DIR_TOP_RIGHT_H] == 50){
    if(edgeTypes[DIR_BOT_LEFT_H] == edgeTypes[DIR_BOT_RIGHT_H] && edgeTypes[DIR_TOP_LEFT_V] == edgeTypes[DIR_BOT_LEFT_V] && edgeTypes[DIR_TOP_RIGHT_V] == edgeTypes[DIR_BOT_RIGHT_V]){
      isomType = getISOMType(cv5[tiles[0]].id);
      isomSubtype = ISOM_CORNER_IN_N;
    }
  }
  if(edgeTypes[DIR_TOP_LEFT_H] == 0 && edgeTypes[DIR_TOP_RIGHT_H] == 0 && edgeTypes[DIR_BOT_LEFT_V] == 52 && edgeTypes[DIR_TOP_RIGHT_V] == 50){
    if(edgeTypes[DIR_BOT_LEFT_H] == edgeTypes[DIR_BOT_RIGHT_H]){
      isomType = getISOMType(cv5[tiles[4]].id);
      isomSubtype = ISOM_CORNER_IN_N;
    }
  }
  
  // top, bottom
  if(edgeTypes[DIR_TOP_LEFT_H] == 50 && edgeTypes[DIR_TOP_RIGHT_H] == 52 && edgeTypes[DIR_BOT_LEFT_H] == 51 && edgeTypes[DIR_BOT_RIGHT_H] == 49){
    if(edgeTypes[DIR_TOP_LEFT_V] == edgeTypes[DIR_BOT_LEFT_V] && edgeTypes[DIR_TOP_RIGHT_V] == edgeTypes[DIR_BOT_RIGHT_V]){
      isomType = getISOMType(cv5[tiles[0]].id);
      isomSubtype = ISOM_VERTICAL;
    }
  }
  // left, right
  if(edgeTypes[DIR_TOP_LEFT_V] == 54 && edgeTypes[DIR_BOT_LEFT_V] == 54 && edgeTypes[DIR_TOP_RIGHT_V] == 53 && edgeTypes[DIR_BOT_RIGHT_V] == 53){
    if(edgeTypes[DIR_TOP_LEFT_H] == edgeTypes[DIR_TOP_RIGHT_H] && edgeTypes[DIR_BOT_LEFT_H] == edgeTypes[DIR_BOT_RIGHT_H]){
      isomType = getISOMType(cv5[tiles[0]].id);
      isomSubtype = ISOM_HORIZONTAL;
    }
  }
  
  if(isomType == 0){
    // are all nonzero edges the same ID?
    id = 0;
    for(i = 0; i < 8; i++){
      if(edgeTypes[i] != 0){
        if(id == 0){
          id = edgeTypes[i];
        }else if(edgeTypes[i] != id){
          break;
        }
      }
    }
    if(id == 0) return 0; // all edges are 0
    if(i == 8 && isBasicType(id)){ // all edges are basic types
      return getISOMFromEdge(id);
    }
    
    printf("\t%03x (%d) egde IDs: %d,%d, %d,%d, %d,%d, %d,%d\n", isomType+isomSubtype, isomSubtype, edgeTypes[DIR_TOP_LEFT_V],edgeTypes[DIR_BOT_LEFT_V],edgeTypes[DIR_TOP_LEFT_H],edgeTypes[DIR_TOP_RIGHT_H],edgeTypes[DIR_TOP_RIGHT_V],edgeTypes[DIR_BOT_RIGHT_V],edgeTypes[DIR_BOT_LEFT_H],edgeTypes[DIR_BOT_RIGHT_H]);
  }
  
  return isomType+isomSubtype;
}

u32 getCV5Index(u16 tile){
  u16 id = tile >> 4;
  tile &= 0xF;
  if(id == 0 || id >= cv5count) return 0;
  if(tile != 0 && cv5[id].tiles[tile] == 0) return 0;
  if(cv5[id].id != CV5_DOODAD_ID) return id;
  
  u16 x = tile;
  u16 y = 0;
  if(x >= cv5[id].doodad.width) return 0;
//  printf("doodad tile: %d.%d --> ", id, tile);
  while(cv5[id-1].id == CV5_DOODAD_ID && cv5[id-1].doodad.doodadID == cv5[id].doodad.doodadID){
    id--;
    y++;
  }
//  printf("%d (%d,%d)\n", dddata[cv5[id].doodad.doodadID][y*cv5[id].doodad.width+x], x,y);
  return dddata[cv5[id].doodad.doodadID][y*cv5[id].doodad.width+x];
}




u32 getEdgeType(u16 id){
  if(id > 48) return id;
  if(id >= 38) return 0;
  return EdgeTypes[tileset][id];
}

u32 getISOMType(u16 id){
  if(id >= 36) return 0;
  return ISOMTypes[tileset][id];
}

u32 getISOMFromEdge(u16 id){
  if(id >= 38) return 0;
  return ISOMEdgeTypes[tileset][id];
}

bool isBasicGroup(u16 group){
  if(group >= cv5count) return false;
  return cv5[group].group.edge.left == cv5[group].group.edge.up && cv5[group].group.edge.left == cv5[group].group.edge.right && cv5[group].group.edge.left == cv5[group].group.edge.down;
}


bool isBasicType(u16 id){
  if(id == 0 || id >= 38) return false;
  if(id < 8) return true;
  return getCliffType(id) == CLIFF_NONE;
}

u32 getCliffType(u16 id){
  if(id == 0 || id >= 38) return CLIFF_NONE;
  if(HorizCliffTypes[tileset][id] < 8) return CLIFF_NONE;
  u16 base = id;
  if(HorizCliffTypes[tileset][base] == base-1) base--;
  if(HorizCliffTypes[tileset][base] == base && HorizCliffTypes[tileset][base+1] == base){
    if(base == id) return CLIFF_LEFT;
    if(base+1 == id) return CLIFF_RIGHT;
  }
  return CLIFF_NONE;
}

