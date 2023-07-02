#include "isom.h"
#include "terrain.h"
#include "chk.h"

#define ISOMCoords(x,y) ((y)*(mapw/2+1) + (x)/2)
#define DomCoords(x,y)  (((y)+1)*(mapw+2) + (x)+2)

#define EDGE_RSV_START   48  // 49-56 have special meanings
#define MAX_TABLE_COUNT  48  // 38 is the highest normally used


/* ----- Look-up table stuff ----- */

// default CV5-->ISOM ID mappings, as a list of {CV5 id, ISOM type} pairs
const CV5ISOM BadlandsISOMTypes[] = {{2,1},{4,9},{3,2},{5,3},{6,4},{7,7},{18,8},{14,5},{15,6},{22,0x6F},{34,0x0D},{35,0x1B},{20,0x29},{21,0x45},{31,0x61},{27,0x53},{28,0x37},0};
const CV5ISOM PlatformISOMTypes[] = {{2,1},{8,9},{9,10},{3,2},{11,14},{4,11},{7,8},{5,4},{6,12},{10,13},{20,0x18},{17,0x42},{18,0x50},{13,0x88},{14,0x5E},{16,0x34},{21,0x26},{15,0x6C},{19,0x7A},0};
const CV5ISOM InstallISOMTypes[]  = {{2,1},{3,2},{6,3},{4,4},{5,5},{8,6},{7,7},{12,0x16},{13,0x24},{10,0x32},{11,0x40},{14,0x4E},{15,0x5C},0};
const CV5ISOM AshworldISOMTypes[] = {{8,1},{2,2},{3,3},{6,4},{9,8},{4,5},{5,6},{7,7},{17,0x1B},{11,0x37},{13,0x53},{15,0x6F},{16,0x29},{12,0x45},{14,0x61},0};
const CV5ISOM JungleISOMTypes[]   = {{5,3},{2,1},{4,13},{8,4},{15,6},{11,7},{9,5},{16,8},{3,2},{10,9},{12,10},{13,11},{17,12},{35,0x1F},{22,0xAB},{23,0x2D},{28,0x3B},{25,0x57},{29,0x49},{32,0x65},{34,0x11},{24,0x73},{26,0x81},{30,0x8F},{33,0x9D},0};
// desert, ice, twilight are same as jungle
const CV5ISOM* CV5ISOMTypes[8] = {BadlandsISOMTypes, PlatformISOMTypes, InstallISOMTypes, AshworldISOMTypes, JungleISOMTypes, JungleISOMTypes, JungleISOMTypes, JungleISOMTypes};

// ISOM edge subtype definitions
const struct {
  u16 edges[8];         // cv5 edge values to match for each ISOM subtype
  u8 tileTypes[4][4];   // group type matching flags (basic/plain tiles, edge/transition tiles, stack cliff types, or any/don't care)
  u8 tileOrder[4];      // specifies the priority of each quadrant for determining the ISOM ID (since some patterns can include basic types or unrelated tiles)
} ISOMPatterns[ISOM_EDGE_COUNT] = {
  { 
    // Type 0: ISOM_EDGE_NW
    PATTERN_EDGES(MATCH_A,MATCH_A, MATCH_A,51, 51,51, 51,MATCH_A),
    PATTERN_TILES_ALL(GROUP_ANY, GROUP_EDGE, GROUP_EDGE, GROUP_EDGE),
    {PATTERN_BOT_RIGHT, PATTERN_BOT_LEFT, PATTERN_TOP_RIGHT, PATTERN_TOP_LEFT}
  },{
    // Type 1: ISOM_EDGE_NE
    PATTERN_EDGES(49,MATCH_A, MATCH_A,MATCH_A, MATCH_A,49, 49,49),
    PATTERN_TILES_ALL(GROUP_EDGE, GROUP_ANY, GROUP_EDGE, GROUP_EDGE),
    {PATTERN_BOT_LEFT, PATTERN_BOT_RIGHT, PATTERN_TOP_LEFT, PATTERN_TOP_RIGHT}
  },{
    // Type 2: ISOM_EDGE_SE
    PATTERN_EDGES(52,52, 52,MATCH_A_OR_C0, MATCH_A_OR_C0,MATCH_A_OR_C0, MATCH_A_OR_C0,52),
    PATTERN_TILES(
      PATTERN_TILES_DEF(SEL_CLIFFS|SEL_STACK,  GROUP_EDGE, GROUP_EDGE, GROUP_EDGE, GROUP_EDGE|GROUP_STACK),
      //PATTERN_TILES_DEF(SEL_STACK,   GROUP_EDGE, GROUP_EDGE, GROUP_EDGE, GROUP_EDGE),
      PATTERN_TILES_DEF(SEL_DEFAULT, GROUP_EDGE, GROUP_EDGE, GROUP_EDGE, GROUP_ANY)
    ),
    {PATTERN_TOP_LEFT, PATTERN_TOP_RIGHT, PATTERN_BOT_LEFT, PATTERN_BOT_RIGHT}
  },{
    // Type 3: ISOM_EDGE_SW
    PATTERN_EDGES(MATCH_A_OR_C1,50, 50,50, 50,MATCH_A_OR_C1, MATCH_A_OR_C1,MATCH_A_OR_C1),
    PATTERN_TILES(
      PATTERN_TILES_DEF(SEL_CLIFFS|SEL_STACK,  GROUP_EDGE, GROUP_EDGE, GROUP_EDGE|GROUP_STACK, GROUP_EDGE),
      //PATTERN_TILES_DEF(SEL_STACK,   GROUP_EDGE, GROUP_EDGE, GROUP_EDGE, GROUP_EDGE),
      PATTERN_TILES_DEF(SEL_DEFAULT, GROUP_EDGE, GROUP_EDGE, GROUP_ANY, GROUP_EDGE)
    ),
    {PATTERN_TOP_RIGHT, PATTERN_TOP_LEFT, PATTERN_BOT_RIGHT, PATTERN_BOT_LEFT}
  },{
    // Type 4: ISOM_CORNER_OUT_N
    PATTERN_EDGES(MATCH_A,MATCH_A, MATCH_A,MATCH_A, MATCH_A,49, 51,MATCH_A),
    PATTERN_TILES_ALL(GROUP_ANY, GROUP_ANY, GROUP_EDGE, GROUP_EDGE),
    {PATTERN_BOT_LEFT, PATTERN_BOT_RIGHT, PATTERN_TOP_LEFT, PATTERN_TOP_RIGHT}
  },{
    // Type 5: ISOM_CORNER_OUT_E
    PATTERN_EDGES(54,MATCH_A, MATCH_A,MATCH_A, MATCH_A,MATCH_A, MATCH_A_OR_C3,54),
    PATTERN_TILES_ALL(GROUP_EDGE, GROUP_ANY, GROUP_EDGE, GROUP_ANY),
    {PATTERN_TOP_LEFT, PATTERN_BOT_LEFT, PATTERN_TOP_RIGHT, PATTERN_BOT_RIGHT}
  },{
    // Type 6: ISOM_CORNER_OUT_S
    PATTERN_EDGES(MATCH_A_OR_C1,50, 52,MATCH_A_OR_C0, MATCH_A_OR_C0,MATCH_A_OR_C0, MATCH_A_OR_C1,MATCH_A_OR_C1),
    PATTERN_TILES(
      PATTERN_TILES_DEF(SEL_CLIFFS|SEL_STACK,  GROUP_EDGE, GROUP_EDGE, GROUP_EDGE|GROUP_STACK, GROUP_EDGE|GROUP_STACK),
      //PATTERN_TILES_DEF(SEL_STACK,   GROUP_EDGE, GROUP_EDGE, GROUP_EDGE, GROUP_EDGE),
      PATTERN_TILES_DEF(SEL_DEFAULT, GROUP_EDGE, GROUP_EDGE, GROUP_ANY, GROUP_ANY)
    ),
    {PATTERN_TOP_LEFT, PATTERN_TOP_RIGHT, PATTERN_BOT_LEFT, PATTERN_BOT_RIGHT}
  },{
    // Type 7: ISOM_CORNER_OUT_W
    PATTERN_EDGES(MATCH_A,MATCH_A, MATCH_A,53, 53,MATCH_A_OR_C2, MATCH_A,MATCH_A),
    PATTERN_TILES_ALL(GROUP_ANY, GROUP_EDGE, GROUP_ANY, GROUP_EDGE),
    {PATTERN_TOP_RIGHT, PATTERN_BOT_RIGHT, PATTERN_TOP_LEFT, PATTERN_BOT_LEFT}
  },{
    // Type 8: ISOM_CORNER_IN_E
    PATTERN_EDGES(MATCH_A_OR_C1,50, 56,56, 56,56, 51,MATCH_A_OR_C1),
    PATTERN_TILES(
      PATTERN_TILES_DEF(SEL_SIMPLE,  GROUP_EDGE, GROUP_EDGE|GROUP_BASIC, GROUP_EDGE, GROUP_EDGE|GROUP_BASIC),
      PATTERN_TILES_DEF(SEL_DEFAULT, GROUP_EDGE, GROUP_EDGE, GROUP_EDGE, GROUP_EDGE)
    ),
    {PATTERN_TOP_LEFT, PATTERN_BOT_LEFT, PATTERN_TOP_RIGHT, PATTERN_BOT_RIGHT}
  },{
    // Type 9: ISOM_CORNER_IN_W
    PATTERN_EDGES(55,55, 52,MATCH_A_OR_C0, MATCH_A_OR_C0,49, 55,55),
    PATTERN_TILES(
      PATTERN_TILES_DEF(SEL_SIMPLE,  GROUP_EDGE|GROUP_BASIC, GROUP_EDGE, GROUP_EDGE|GROUP_BASIC, GROUP_EDGE),
      PATTERN_TILES_DEF(SEL_DEFAULT, GROUP_EDGE, GROUP_EDGE, GROUP_EDGE, GROUP_EDGE)
    ),
    {PATTERN_TOP_RIGHT, PATTERN_BOT_RIGHT, PATTERN_TOP_LEFT, PATTERN_BOT_LEFT}
  },{
    // Type 10: ISOM_CORNER_IN_S
    PATTERN_EDGES(49,MATCH_A, MATCH_A,51, 51,51, 49,49),
    PATTERN_TILES_ALL(GROUP_EDGE, GROUP_EDGE, GROUP_EDGE, GROUP_EDGE),
    {PATTERN_BOT_LEFT, PATTERN_BOT_RIGHT, PATTERN_TOP_LEFT, PATTERN_TOP_RIGHT}
  },{
    // Type 11: ISOM_CORNER_IN_N
    PATTERN_EDGES(52,52, 50,50, 50,MATCH_A_OR_C1, MATCH_A_OR_C0,52),
    PATTERN_TILES_ALL(GROUP_EDGE, GROUP_EDGE, GROUP_EDGE, GROUP_EDGE),
    {PATTERN_TOP_LEFT, PATTERN_TOP_RIGHT, PATTERN_BOT_LEFT, PATTERN_BOT_RIGHT}
  },{
    // Type 12: ISOM_VERTICAL
    PATTERN_EDGES(MATCH_A_OR_C1,50, 52,MATCH_A_OR_C0, MATCH_A_OR_C0,49, 51,MATCH_A_OR_C1),
    PATTERN_TILES_ALL(GROUP_EDGE, GROUP_EDGE, GROUP_EDGE, GROUP_EDGE),
    {PATTERN_TOP_LEFT, PATTERN_TOP_RIGHT, PATTERN_BOT_LEFT, PATTERN_BOT_RIGHT}
  },{
    // Type 13: ISOM_HORIZONTAL
    PATTERN_EDGES(54,MATCH_A, MATCH_A,53, 53,MATCH_A_OR_C2, MATCH_A_OR_C3,54),
    PATTERN_TILES_ALL(GROUP_EDGE, GROUP_EDGE, GROUP_EDGE, GROUP_EDGE),
    {PATTERN_TOP_LEFT, PATTERN_TOP_RIGHT, PATTERN_BOT_LEFT, PATTERN_BOT_RIGHT}
  }
};

// Terrain data
struct {
  u8  groupType;   // basic, edge, stack
  u8  patternType; // normal, simple, cliff, stackable cliff
  u8  edgeA;       // plain or source CV5 edge type
  u8  edgeB;       // final CV5 edge type
  u8  edgeC[4];    // unique cliff IDs
  u16 cliffUpper;  // stacked cliffs -- upper cliff CV5 id
  u16 firstGroup;  // stacked cliffs -- first cv5 group (inclusive)
  u16 lastGroup;   // stacked cliffs -- last cv5 group (exclusive)
  u16 ISOMType;
} TerrainTypes[MAX_TABLE_COUNT] = {0};

/* ----- End Look-up table stuff ----- */


// terrain values
extern CV5* cv5;
extern u32 cv5count;
extern u16 dddata[512][256];


// chk data
u32 tileset = 0;
s32 mapw = 0;
s32 maph = 0;
u16 maptiles[MAX_MAP_DIM*MAX_MAP_DIM] = {0};
ISOMRect isom[MAX_ISOM_WIDTH*MAX_ISOM_HEIGHT] = {0};


// isom parsing data
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
bool isEmptyISOMCellAt(s32 x, s32 y);
bool doesISOMCellEqualType(s32 x, s32 y, u32 type);
bool isISOMPartialEdge(s32 x, s32 y, u32 type, u32 rectSide);
void setISOMCellAt(s32 x, s32 y, u32 type);

//u32 getCustomISOM(s32 x, s32 y);
bool checkISOMTiles(u16 tiles[], u16 flags[]);
u32 getRectISOMType(u16 tiles[]);

void generateTypeTables();

bool edgeMatches(u32 edge, u32 pattern, u32 id);
bool edgeMatchesType(u16 edge, u32 id);
u32 getCV5Index(u16 tile);
u32 getISOMFromBasicEdge(u16 id);
bool isBasicGroup(u16 group);



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
  
  // generate look-up tables
  generateTypeTables();
  
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
        //case 0:
        //  shading->raw = COLOR(  0,  0,  0, 32);
        //  break;
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
          shading->raw = COLOR(255,255,255, 64);
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
      
      domIndex = DomCoords(x,y);
      
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
          tileDoms[domIndex+mapw+2].flags |= TILE_MISMATCH_UP;
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
      domIndex = DomCoords(x,y);
      
      if(isISOMCellAt(x,y) || isEmptyISOMCellAt(x,y)){
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
          for(i = 0; i < MAX_TABLE_COUNT; i++){
            //if(ISOMTypes[i] <= cmpType && ISOMTypes[i] > ISOMTypes[j]) j = i;
            if(TerrainTypes[i].ISOMType <= cmpType && TerrainTypes[i].ISOMType > TerrainTypes[j].ISOMType) j = i;
          }
          printf("(%3d,%3d) %03x (actual: %03x): %3d, %3d; %3d, %3d\t\t%03x + %d\n", x,y, id, cmpType, (y<0||x<0)?0: groups[tileIndex], (y<0||x>=mapw-2)?0: groups[tileIndex+2], (y>=maph-1||x<0)?0: groups[tileIndex+mapw], (y>=mapw-1||x>=mapw-2)?0: groups[tileIndex+mapw+2], TerrainTypes[j].ISOMType, cmpType-TerrainTypes[j].ISOMType);
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
              tileDoms[domIndex+mapw+2].flags |= TILE_MISMATCHED;
              tileDoms[domIndex+mapw+2+1].flags |= TILE_MISMATCHED;
            }
            if(x < mapw-2){
              tileDoms[domIndex+mapw+2+2].flags |= TILE_MISMATCHED;
              tileDoms[domIndex+mapw+2+3].flags |= TILE_MISMATCHED;
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
  s32 domIndex = DomCoords(x,y);
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
      flags[4+i] = tileDoms[domIndex+mapw+2+i].flags;
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

bool isEmptyISOMCellAt(s32 x, s32 y){
  s32 isomIndex = ISOMCoords(x,y);
  u32 isomType = 0;
  if(y >= 0){
    if(x >= 0){
      if(isom[isomIndex].right.dir != 0) return false;
      if(isom[isomIndex].down.dir != 0) return false;
      if(isom[isomIndex].right.type != isom[isomIndex].down.type) return false;
      isomType = isom[isomIndex].right.type;
    }
    if(x < mapw-2){
      if(isom[isomIndex+1].left.dir != 0) return false;
      if(isom[isomIndex+1].down.dir != 0) return false;
      if(isom[isomIndex+1].left.type != isom[isomIndex+1].down.type) return false;
      if(isomType != 0 && isom[isomIndex+1].left.type != isomType) return false;
      if(isomType == 0) isomType = isom[isomIndex+1].left.type;
    }
  }
  if(y < maph-1){
    if(x >= 0){
      if(isom[isomIndex+mapw/2+1].right.dir != 0) return false;
      if(isom[isomIndex+mapw/2+1].up.dir != 0) return false;
      if(isom[isomIndex+mapw/2+1].right.type != isom[isomIndex+mapw/2+1].up.type) return false;
      if(isomType != 0 && isom[isomIndex+mapw/2+1].right.type != isomType) return false;
      if(isomType == 0) isomType = isom[isomIndex+mapw/2+1].right.type;
    }
    if(x < mapw-2){
      if(isom[isomIndex+mapw/2+2].left.dir != 0) return false;
      if(isom[isomIndex+mapw/2+2].up.dir != 0) return false;
      if(isom[isomIndex+mapw/2+2].left.type != isom[isomIndex+mapw/2+2].up.type) return false;
      if(isomType != 0 && isom[isomIndex+mapw/2+2].left.type != isomType) return false;
      if(isomType == 0) isomType = isom[isomIndex+mapw/2+2].left.type;
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
  u16 edgeType = 0;
  u32 patType;
  
  for(i = 0; i < MAX_TABLE_COUNT; i++){
    if(TerrainTypes[i].groupType == GROUP_BASIC && TerrainTypes[i].ISOMType == type){
      edgeType = TerrainTypes[i].edgeA;
      break;
    }
  }
  if(edgeType == 0) return false;
  
  for(i = 0; i < MAX_TABLE_COUNT; i++){
    if(TerrainTypes[i].groupType <= GROUP_BASIC) continue; // skip basic types
    patType = TerrainTypes[i].patternType;
    if(TerrainTypes[i].edgeA != edgeType && (patType != PATTERN_TYPE_SIMPLE || TerrainTypes[i].edgeB != edgeType)) continue; // skip irrelevant edges
    
    for(j = 0; j < ISOM_EDGE_COUNT; j++){
      switch(rectSide){
        case LEFT:
          if((ISOMPatterns[j].tileTypes[patType][PATTERN_TOP_LEFT] & GROUP_BASIC) && (ISOMPatterns[j].tileTypes[patType][PATTERN_BOT_LEFT] & GROUP_BASIC)){
            if(doesISOMCellEqualType(x, y, TerrainTypes[i].ISOMType + j)) return true;
          }
          break;
        case RIGHT:
          if((ISOMPatterns[j].tileTypes[patType][PATTERN_TOP_RIGHT] & GROUP_BASIC) && (ISOMPatterns[j].tileTypes[patType][PATTERN_BOT_RIGHT] & GROUP_BASIC)){
            if(doesISOMCellEqualType(x, y, TerrainTypes[i].ISOMType + j)) return true;
          }
          break;
        case UP:
          if((ISOMPatterns[j].tileTypes[patType][PATTERN_TOP_LEFT] & GROUP_BASIC) && (ISOMPatterns[j].tileTypes[patType][PATTERN_TOP_RIGHT] & GROUP_BASIC)){
            if(doesISOMCellEqualType(x, y, TerrainTypes[i].ISOMType + j)) return true;
          }
          break;
        case DOWN:
          if((ISOMPatterns[j].tileTypes[patType][PATTERN_BOT_LEFT] & GROUP_BASIC) && (ISOMPatterns[j].tileTypes[patType][PATTERN_BOT_RIGHT] & GROUP_BASIC)){
            if(doesISOMCellEqualType(x, y, TerrainTypes[i].ISOMType + j)) return true;
          }
          break;
        case UP_LEFT:
          if(ISOMPatterns[j].tileTypes[patType][PATTERN_TOP_LEFT] & GROUP_BASIC){
            if(doesISOMCellEqualType(x, y, TerrainTypes[i].ISOMType + j)) return true;
          }
          break;
        case UP_RIGHT:
          if(ISOMPatterns[j].tileTypes[patType][PATTERN_TOP_RIGHT] & GROUP_BASIC){
            if(doesISOMCellEqualType(x, y, TerrainTypes[i].ISOMType + j)) return true;
          }
          break;
        case DOWN_LEFT:
          if(ISOMPatterns[j].tileTypes[patType][PATTERN_BOT_LEFT] & GROUP_BASIC){
            if(doesISOMCellEqualType(x, y, TerrainTypes[i].ISOMType + j)) return true;
          }
          break;
        case DOWN_RIGHT:
          if(ISOMPatterns[j].tileTypes[patType][PATTERN_BOT_RIGHT] & GROUP_BASIC){
            if(doesISOMCellEqualType(x, y, TerrainTypes[i].ISOMType + j)) return true;
          }
          break;
      }
    }
  }
  
  return false;
}

bool isISOMPartialEdgeSimple(u32 baseType, u32 cmpType, u32 rectSide){
  u32 i,j;
  u16 edgeType = 0;
  u32 patType;
  
  if(baseType == cmpType) return true;
  
  for(i = 0; i < MAX_TABLE_COUNT; i++){
    if(TerrainTypes[i].groupType == GROUP_BASIC && TerrainTypes[i].ISOMType == baseType){
      edgeType = TerrainTypes[i].edgeA;
      break;
    }
  }
  if(edgeType == 0) return false;
  
  for(i = 0; i < MAX_TABLE_COUNT; i++){
    if(TerrainTypes[i].groupType <= GROUP_BASIC) continue; // skip basic types
    patType = TerrainTypes[i].patternType;
    if(TerrainTypes[i].edgeA != edgeType && (patType != PATTERN_TYPE_SIMPLE || TerrainTypes[i].edgeB != edgeType)) continue; // skip irrelevant edges
    
    for(j = 0; j < ISOM_EDGE_COUNT; j++){
      switch(rectSide){
        case LEFT:
          if((ISOMPatterns[j].tileTypes[patType][PATTERN_TOP_LEFT] & GROUP_BASIC) || (ISOMPatterns[j].tileTypes[patType][PATTERN_BOT_LEFT] & GROUP_BASIC)){
            if(cmpType == TerrainTypes[i].ISOMType + j) return true;
          }
          break;
        case RIGHT:
          if((ISOMPatterns[j].tileTypes[patType][PATTERN_TOP_RIGHT] & GROUP_BASIC) || (ISOMPatterns[j].tileTypes[patType][PATTERN_BOT_RIGHT] & GROUP_BASIC)){
            if(cmpType == TerrainTypes[i].ISOMType + j) return true;
          }
          break;
        case UP:
          if((ISOMPatterns[j].tileTypes[patType][PATTERN_TOP_LEFT] & GROUP_BASIC) || (ISOMPatterns[j].tileTypes[patType][PATTERN_TOP_RIGHT] & GROUP_BASIC)){
            if(cmpType == TerrainTypes[i].ISOMType + j) return true;
          }
          break;
        case DOWN:
          if((ISOMPatterns[j].tileTypes[patType][PATTERN_BOT_LEFT] & GROUP_BASIC) || (ISOMPatterns[j].tileTypes[patType][PATTERN_BOT_RIGHT] & GROUP_BASIC)){
            if(cmpType == TerrainTypes[i].ISOMType + j) return true;
          }
          break;
      }
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
    if(x < mapw){
      isom[isomIndex+1].left.dir = DIR_TOP_RIGHT_H;
      isom[isomIndex+1].left.type = type;
      isom[isomIndex+1].down.dir = DIR_TOP_RIGHT_V;
      isom[isomIndex+1].down.type = type;
    }
  }
  if(y < maph){
    if(x >= 0){
      isom[isomIndex+mapw/2+1].right.dir = DIR_BOT_LEFT_H;
      isom[isomIndex+mapw/2+1].right.type = type;
      isom[isomIndex+mapw/2+1].up.dir = DIR_BOT_LEFT_V;
      isom[isomIndex+mapw/2+1].up.type = type;
    }
    if(x < mapw){
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
  u32 i,j,k;
  u32 edgeTypes[8];
  
  u32 isomType = 0;
  u32 isomSubtype = 0;
  u32 patType;
  u32 typeMask;
  s32 match;
  s32 bestMatch = -1;
  
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
    return TerrainTypes[cv5[id].id].ISOMType;
  }
  
  edgeTypes[DIR_TOP_LEFT_V] = cv5[tiles[0]].group.edge.down;
  edgeTypes[DIR_BOT_LEFT_V] = cv5[tiles[4]].group.edge.up;
  edgeTypes[DIR_TOP_LEFT_H] = cv5[tiles[0]].group.edge.right;
  edgeTypes[DIR_TOP_RIGHT_H] = cv5[tiles[2]].group.edge.left;
  edgeTypes[DIR_TOP_RIGHT_V] = cv5[tiles[2]].group.edge.down;
  edgeTypes[DIR_BOT_RIGHT_V] = cv5[tiles[6]].group.edge.up;
  edgeTypes[DIR_BOT_LEFT_H] = cv5[tiles[4]].group.edge.right;
  edgeTypes[DIR_BOT_RIGHT_H] = cv5[tiles[6]].group.edge.left;
  
  for(i = 0; i < ISOM_EDGE_COUNT; i++){
    for(j = 0; j < 4; j++){
      id = cv5[tiles[ISOMPatterns[i].tileOrder[j]*2]].id;
      if(id == 0 || TerrainTypes[id].groupType == GROUP_BASIC) continue; // not a transition type
      patType = TerrainTypes[id].patternType;
      
      // is top row *only* null and bottom row *only* cliff stacking tiles?
      if(patType == PATTERN_TYPE_STACK && tiles[0] == 0 && tiles[2] == 0){
        for(k = 4; k < 8; k+=2){ // use a loop with continues/breaks rather than a massive "if"
          if(tiles[k] == 0) continue; // null is valid
          if(TerrainTypes[cv5[tiles[k]].id].groupType != GROUP_STACK) break;
          if(tiles[k] < TerrainTypes[cv5[tiles[k]].id].firstGroup) break;
          if(tiles[k] >= TerrainTypes[cv5[tiles[k]].id].lastGroup) break;
          if(k == 6 && tiles[4] != 0 && cv5[tiles[4]].id != cv5[tiles[6]].id) break; // left and right tiles don't match -- this isn't a valid case
        }
        if(k == 8){ // yes -- set id to the upper ID
          k = tiles[4] ? 4 : 6; // select whichever isn't null
          id = TerrainTypes[cv5[tiles[k]].id].cliffUpper;
          //patType = PATTERN_TYPE_CLIFFS;
        }
      }
      
      // validate tile types
      for(k = 0; k < 4; k++){
        if(tiles[k*2] == 0) continue;
        
        typeMask = TerrainTypes[cv5[tiles[k*2]].id].groupType;
        if(typeMask == GROUP_STACK){
          // if it's not inside the stack range then it's regular cliff tiles
          if(TerrainTypes[cv5[tiles[k*2]].id].cliffUpper != id || tiles[k*2] < TerrainTypes[cv5[tiles[k*2]].id].firstGroup || tiles[k*2] >= TerrainTypes[cv5[tiles[k*2]].id].lastGroup){
            typeMask = GROUP_EDGE;
          }
        }
        
        typeMask &= ISOMPatterns[i].tileTypes[patType][k];
        if(typeMask == 0) break;
        if((ISOMPatterns[i].tileTypes[patType][k] & GROUP_BASIC) == 0){ // not basic and not "any"
          if(typeMask & GROUP_EDGE){
            // is it the same type of edge?
            if(cv5[tiles[k*2]].id == id) continue;
          }
          if(typeMask & GROUP_STACK){
            // is it it the correct type of cliff?
            if(TerrainTypes[cv5[tiles[k*2]].id].cliffUpper == id) continue;
          }
          break;
        }
      }
      if(k != 4) continue;
      
      
      match = 0;
      for(k = 0; k < 8; k++){
        if(edgeTypes[k] == 0) continue;
        if(edgeMatches(edgeTypes[k], ISOMPatterns[i].edges[k], id) == false){
          break;
        }
        match++;
      }
      if(k != 8) continue;
      if(match == 8){
        return TerrainTypes[id].ISOMType + i;
      }
      
      if(match > bestMatch){
        bestMatch = match;
        isomType = TerrainTypes[id].ISOMType;
        isomSubtype = i;
      }
    }
  }
  
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
  if(i == 8){ // all edges are the same
    i = getISOMFromBasicEdge(id);
    if(i != 0) return i;
  }
  
  if(isomType == 0){
    printf("\t%03x (%d) egde IDs: %d,%d, %d,%d, %d,%d, %d,%d\n", isomType+isomSubtype, isomSubtype, edgeTypes[DIR_TOP_LEFT_V],edgeTypes[DIR_BOT_LEFT_V],edgeTypes[DIR_TOP_LEFT_H],edgeTypes[DIR_TOP_RIGHT_H],edgeTypes[DIR_TOP_RIGHT_V],edgeTypes[DIR_BOT_RIGHT_V],edgeTypes[DIR_BOT_LEFT_H],edgeTypes[DIR_BOT_RIGHT_H]);
  }
  
  return isomType+isomSubtype;
}



void generateTypeTables(){
  u32 i,j,k;
  u32 id;
  
  u8 typeMask;
  bool doingStackCliff;
  
  memset(TerrainTypes, 0, sizeof(TerrainTypes));
  
  // pass 1: get edge types
  for(i = 2; i < cv5count; i += 2){
    id = cv5[i].id;
    if(id <= 1) continue;
    
    if(id >= MAX_TABLE_COUNT){
      printf("wtf %d\n", id);
      continue;
    }
    
    for(j = 0; CV5ISOMTypes[tileset][j].id != 0; j++){
      if(CV5ISOMTypes[tileset][j].id == id){
        TerrainTypes[id].ISOMType = CV5ISOMTypes[tileset][j].ISOM;
        break;
      }
    }
    if(CV5ISOMTypes[tileset][j].id == 0){
      printf("Unrecognized CV5 type %d at index %d\n", id, i);
      continue;
    }
    
    // basic types
    if(cv5[i].group.edge.left == cv5[i].group.edge.up && cv5[i].group.edge.left == cv5[i].group.edge.right && cv5[i].group.edge.left == cv5[i].group.edge.down){
      TerrainTypes[id].groupType = GROUP_BASIC;
      TerrainTypes[id].edgeA = cv5[i].group.edge.left;
      continue;
    }
    
    // definitions can be split, most notably stacked cliffs
    if(TerrainTypes[id].patternType != 0){
      typeMask = TerrainTypes[id].patternType;
    }else{
      typeMask = SEL_ALL;
    }
    
    for( ; i < cv5count && cv5[i].id == id; i += 2){
      if(cv5[i].group.edge.right == 51 && cv5[i].group.edge.down == 51){
        if(cv5[i].group.edge.left == cv5[i].group.edge.up){
          TerrainTypes[id].edgeA = cv5[i].group.edge.left;
        }else{
          TerrainTypes[id].edgeA = cv5[i].group.edge.left;
          TerrainTypes[id].edgeC[1] = cv5[i].group.edge.up;
          typeMask &= (SEL_CLIFFS | SEL_STACK);
        }
      }
      if(cv5[i].group.edge.left == 51 && cv5[i].group.edge.up == 51){
        if(cv5[i].group.edge.right == cv5[i].group.edge.down){
          if(cv5[i].group.edge.right < EDGE_RSV_START) {
            TerrainTypes[id].edgeB = cv5[i].group.edge.right;
          }else if(cv5[i].group.edge.right == 55){
            // can't be simple
            typeMask &= ~SEL_SIMPLE;
          }
        }
      }
      if(cv5[i].group.edge.up == 53 && cv5[i].group.edge.right == 50){
        if(cv5[i].group.edge.left != cv5[i].group.edge.down){
          TerrainTypes[id].edgeC[2] = cv5[i].group.edge.left;
          TerrainTypes[id].edgeC[1] = cv5[i].group.edge.down;
          typeMask &= (SEL_CLIFFS | SEL_STACK);
        }else{
          typeMask &= (SEL_NORMAL | SEL_SIMPLE);
        }
      }
      if(cv5[i].group.edge.left == 52 && cv5[i].group.edge.up == 54){
        if(cv5[i].group.edge.right != cv5[i].group.edge.down){
          TerrainTypes[id].edgeC[3] = cv5[i].group.edge.right;
          TerrainTypes[id].edgeC[0] = cv5[i].group.edge.down;
          typeMask &= (SEL_CLIFFS | SEL_STACK);
        }else{
          typeMask &= (SEL_NORMAL | SEL_SIMPLE);
        }
      }
    }
    
    // preliminary type
    TerrainTypes[id].patternType = typeMask;
    
    // back up one step
    i -= 2;
  }
  
  // pass 2: solidify types and get stacked cliff ids
  for(i = 2; i < cv5count; i += 2){
    id = cv5[i].id;
    if(id <= 1) continue;
    
    if(TerrainTypes[id].ISOMType == 0) continue; // unknown type
    if(TerrainTypes[id].groupType == GROUP_BASIC) continue; // basic types don't have anything left to do
    
    typeMask = TerrainTypes[id].patternType;
    
    doingStackCliff = cv5[i].group.edge.up < EDGE_RSV_START && !edgeMatchesType(cv5[i].group.edge.up, id);
    
    for(j = i;  j < cv5count && cv5[j].id == id; j += 2){
      if(doingStackCliff){
        if(cv5[j].group.edge.up >= EDGE_RSV_START || edgeMatchesType(cv5[j].group.edge.up, id)){
          break;
        }
      }else{
        if(cv5[j].group.edge.up < EDGE_RSV_START && !edgeMatchesType(cv5[j].group.edge.up, cv5[j].id)){
          break;
        }
      }
    }
    
    if(doingStackCliff){
      if(j-i != 16) printf("Invalid stacked cliff range? cv5:%d, %d to %d (%d)\n", id, i, j, j-i);
      TerrainTypes[id].patternType = PATTERN_TYPE_STACK;
      TerrainTypes[id].groupType = GROUP_STACK;
      for(k = 0; k < MAX_TABLE_COUNT; k++){
        if(TerrainTypes[k].ISOMType != 0){
          if(TerrainTypes[k].edgeC[0] == cv5[i].group.edge.up || TerrainTypes[k].edgeC[1] == cv5[i].group.edge.up){
            TerrainTypes[id].cliffUpper = k;
            break;
          }
        }
      }
      TerrainTypes[id].firstGroup = i;
      TerrainTypes[id].lastGroup = j;
      printf("cliff thing %d->%d, %d-%d\n", id, TerrainTypes[id].cliffUpper, i, j);
    }else if(TerrainTypes[id].groupType == 0){ // don't assign a type if it already has one
      TerrainTypes[id].groupType = GROUP_EDGE;
      if(typeMask & SEL_CLIFFS){
        TerrainTypes[id].patternType = PATTERN_TYPE_CLIFFS;
      }else if(typeMask & SEL_SIMPLE){
        TerrainTypes[id].patternType = PATTERN_TYPE_SIMPLE;
      }else if(typeMask & SEL_NORMAL){
        TerrainTypes[id].patternType = PATTERN_TYPE_NORMAL;
      }else{
        printf("Type doesn't exist? cv5:%d\n", id);
        TerrainTypes[id].patternType = 0;
      }
    }
    
    i = j-2;
  }
  
  // debug stuff
  /*for(i = 0; i < MAX_TABLE_COUNT; i++){
    if(TerrainTypes[i].ISOMType == 0) continue;
    printf("%2d => 0x%2X -- %2d,%2d %2d,%2d,{%2d,%2d,%2d,%2d}\n", i, TerrainTypes[i].ISOMType, TerrainTypes[i].groupType, TerrainTypes[i].patternType, TerrainTypes[i].edgeA, TerrainTypes[i].edgeB, TerrainTypes[i].edgeC[0], TerrainTypes[i].edgeC[1], TerrainTypes[i].edgeC[2], TerrainTypes[i].edgeC[3]);
  }*/
}




bool edgeMatches(u32 edge, u32 pattern, u32 id){
  // special cases for each pattern type
  switch(TerrainTypes[id].patternType){
    case PATTERN_TYPE_SIMPLE:
      if(pattern == 55 || pattern == 56){
        pattern = MATCH_B;
      }
      // fall through
    case PATTERN_TYPE_NORMAL:
      if(pattern < EDGE_RSV_START){
        // remove cliff IDs
        pattern &= (MATCH_A | MATCH_B);
      }
      break;
    
    case PATTERN_TYPE_CLIFFS:
    case PATTERN_TYPE_STACK:
      if(pattern < EDGE_RSV_START && pattern >= MATCH_C0){
        // keep only cliff IDs
        pattern &= ~(MATCH_A | MATCH_B);
      }
      break;
  }
  
  switch(pattern){
    case MATCH_A:
      return edge == TerrainTypes[id].edgeA;
    case MATCH_B:
      return edge == TerrainTypes[id].edgeB;
    case MATCH_C0:
      return edge == TerrainTypes[id].edgeC[0];
    case MATCH_C1:
      return edge == TerrainTypes[id].edgeC[1];
    case MATCH_C2:
      return edge == TerrainTypes[id].edgeC[2];
    case MATCH_C3:
      return edge == TerrainTypes[id].edgeC[3];
    default:
      return edge == pattern;
  }
}

bool edgeMatchesType(u16 edge, u32 id){
  if(edge == 0) return false;
  if(edge >= EDGE_RSV_START) return false;
  if(TerrainTypes[id].edgeA == edge) return true;
  if(TerrainTypes[id].edgeB == edge) return true;
  int i;
  for(i = 0; i < 4; i++){
    if(TerrainTypes[id].edgeC[i] == edge) return true;
  }
  return false;
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

u32 getISOMFromBasicEdge(u16 id){
  if(id == 0 || id >= MAX_TABLE_COUNT || id >= EDGE_RSV_START) return 0;
  int i;
  for(i = 0; i < MAX_TABLE_COUNT; i++){
    if(TerrainTypes[i].groupType == GROUP_BASIC && TerrainTypes[i].edgeA == id) return TerrainTypes[i].ISOMType;
  }
  return 0;
}

bool isBasicGroup(u16 group){
  if(group >= cv5count) return false;
  return TerrainTypes[cv5[group].id].groupType == GROUP_BASIC;
}


// debug stuff

u32 matchVal(u32 a){
  switch(a){
    case MATCH_A_OR_C0: return 0xC0;
    case MATCH_A_OR_C1: return 0xC1;
    case MATCH_A_OR_C2: return 0xC2;
    case MATCH_A_OR_C3: return 0xC3;
    case MATCH_A:    return 0xA;
    case MATCH_B:    return 0xB;
    default:
      return ((a/10)<<4)|(a%10);
  }
}

void printPatterns(){
  u32 i,j;
  
  char typestrs[4][32] = {"Normal", "Simple", "Cliff", "Stack"};
  char groupstrs[8][32] = {"None", "Basic", "Edge", "Basic/Edge", "Stack", "Basic/Stack", "Edge/Stack", "Any"};
  
  for(i = 0; i < ISOM_EDGE_COUNT; i++){
    if(ISOMPatterns[i].edges[0] == 0) break;
    printf("[ Type %d ]\n", i);
    printf("  %2X %2X\n"
           "%2X     %2X\n"
           "%2X     %2X\n"
           "  %2X %2X\n\n", matchVal(ISOMPatterns[i].edges[DIR_TOP_LEFT_H]),matchVal(ISOMPatterns[i].edges[DIR_TOP_RIGHT_H]),
                            matchVal(ISOMPatterns[i].edges[DIR_TOP_LEFT_V]),matchVal(ISOMPatterns[i].edges[DIR_TOP_RIGHT_V]),
                            matchVal(ISOMPatterns[i].edges[DIR_BOT_LEFT_V]),matchVal(ISOMPatterns[i].edges[DIR_BOT_RIGHT_V]),
                            matchVal(ISOMPatterns[i].edges[DIR_BOT_LEFT_H]),matchVal(ISOMPatterns[i].edges[DIR_BOT_RIGHT_H]));
    for(j = 0; j < 4; j++){
      printf("%s Types: {%s, %s, %s, %s}\n", typestrs[j], groupstrs[ISOMPatterns[i].tileTypes[j][0]], groupstrs[ISOMPatterns[i].tileTypes[j][1]], groupstrs[ISOMPatterns[i].tileTypes[j][2]], groupstrs[ISOMPatterns[i].tileTypes[j][3]]);
    }
    puts("\n");
  }
}



