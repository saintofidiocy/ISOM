#include "isom.h"
#include "terrain.h"
#include "chk.h"

#define ISOMCoords(x,y) ((y)*(mapw/2+1) + (x)/2)
#define DomCoords(x,y)  (((y)+1)*(mapw+2) + (x)+2)

// Look-Up Tables
#define EDGE_RSV_START   49  // 49-56 have special meanings
#define MAX_TABLE_COUNT  49  // 38 is the highest normally used
#define MAX_CLIFF_COUNT  8   // normally 2

// default CV5-->ISOM ID mappings
const CV5ISOM BadlandsISOMTypes[] = {{2,1},{4,9},{3,2},{5,3},{6,4},{7,7},{18,8},{14,5},{15,6},{22,0x6F},{34,0x0D},{35,0x1B},{20,0x29},{21,0x45},{31,0x61},{27,0x53},{28,0x37},0};
const CV5ISOM PlatformISOMTypes[] = {{2,1},{8,9},{9,10},{3,2},{11,14},{4,11},{7,8},{5,4},{6,12},{10,13},{20,0x18},{17,0x42},{18,0x50},{13,0x88},{14,0x5E},{16,0x34},{21,0x26},{15,0x6C},{19,0x7A},0};
const CV5ISOM InstallISOMTypes[]  = {{2,1},{3,2},{6,3},{4,4},{5,5},{8,6},{7,7},{12,0x16},{13,0x24},{10,0x32},{11,0x40},{14,0x4E},{15,0x5C},0};
const CV5ISOM AshworldISOMTypes[] = {{8,1},{2,2},{3,3},{6,4},{9,8},{4,5},{5,6},{7,7},{17,0x1B},{11,0x37},{13,0x53},{15,0x6F},{16,0x29},{12,0x45},{14,0x61},0};
const CV5ISOM JungleISOMTypes[]   = {{5,3},{2,1},{4,13},{8,4},{15,6},{11,7},{9,5},{16,8},{3,2},{10,9},{12,10},{13,11},{17,12},{35,0x1F},{22,0xAB},{23,0x2D},{28,0x3B},{25,0x57},{29,0x49},{32,0x65},{34,0x11},{24,0x73},{26,0x81},{30,0x8F},{33,0x9D},0};
// desert, ice, twilight are same as jungle
const CV5ISOM* CV5ISOMTypes[8] = {BadlandsISOMTypes, PlatformISOMTypes, InstallISOMTypes, AshworldISOMTypes, JungleISOMTypes, JungleISOMTypes, JungleISOMTypes, JungleISOMTypes};

// remaps cliff-specific edge types to the equivalent base terrain type, since ISOM placement ignores them
u8 EdgeTypes[MAX_TABLE_COUNT] = {0};

// maps CV5 ID property to ISOM base types -- edge tiles have additional values added to this type depending on direction and other adjacent tiles
u8 ISOMTypes[256] = {0};

// maps CV5 edge types to basic ISOM terrain types
u8 ISOMEdgeTypes[MAX_TABLE_COUNT] = {0};

// Remaps cliff edge types for tile pairing checks
u8 HorizCliffTypes[MAX_TABLE_COUNT] = {0};

// Edge IDs and group ranges for stacked cliff tiles
struct {
  u16 lowerID;
  u16 upperID;
  u16 firstGroup;
  u16 lastGroup;
} StackCliffs[MAX_CLIFF_COUNT] = {0};
u32 stackCliffCount = 0;

// ISOM edge types for edge tiles that can be cut off to look like basic types
struct {
  u8 basic;
  u8 edges[MAX_TABLE_COUNT];
} ISOMBasicEdges[MAX_TABLE_COUNT] = {0};


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

void generateTypeTables();
void addBasicEdge(u32 basic, u32 edge);

u16 getStackCliffUpper(u16 id);
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
            if(ISOMTypes[i] <= cmpType && ISOMTypes[i] > ISOMTypes[j]) j = i;
          }
          printf("(%3d,%3d) %03x (actual: %03x): %3d, %3d; %3d, %3d\t\t%03x + %d\n", x,y, id, cmpType, (y<0||x<0)?0: groups[tileIndex], (y<0||x>=mapw-2)?0: groups[tileIndex+2], (y>=maph-1||x<0)?0: groups[tileIndex+mapw], (y>=mapw-1||x>=mapw-2)?0: groups[tileIndex+mapw+2], ISOMTypes[j], cmpType-ISOMTypes[j]);
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
  
  for(i = 0; ISOMBasicEdges[i].basic != type; i++){
    if(ISOMBasicEdges[i].basic == 0) return false;
  }
  
  for(j = 0; ISOMBasicEdges[i].edges[j] != 0; j++){
    switch(rectSide){
      case LEFT:
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[i].edges[j] + ISOM_CORNER_OUT_W)) return true;
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[i].edges[j] + ISOM_CORNER_IN_W)) return true;
        break;
      case RIGHT:
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[i].edges[j] + ISOM_CORNER_OUT_E)) return true;
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[i].edges[j] + ISOM_CORNER_IN_E)) return true;
        break;
      case UP:
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[i].edges[j] + ISOM_CORNER_OUT_N)) return true;
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[i].edges[j] + ISOM_CORNER_IN_N)) return true;
        break;
      case DOWN:
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[i].edges[j] + ISOM_CORNER_OUT_S)) return true;
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[i].edges[j] + ISOM_CORNER_IN_S)) return true;
        break;
      case UP_LEFT:
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[i].edges[j] + ISOM_EDGE_NW)) return true;
        break;
      case UP_RIGHT:
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[i].edges[j] + ISOM_EDGE_NE)) return true;
        break;
      case DOWN_LEFT:
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[i].edges[j] + ISOM_EDGE_SW)) return true;
        break;
      case DOWN_RIGHT:
        if(doesISOMCellEqualType(x, y, ISOMBasicEdges[i].edges[j] + ISOM_EDGE_SE)) return true;
        break;
      default:
        return false;
    }
  }
  return false;
}

bool isISOMPartialEdgeSimple(u32 baseType, u32 cmpType){
  u32 i,j;
  if(baseType == cmpType) return true;
  for(i = 0; ISOMBasicEdges[i].basic != baseType; i++){
    if(ISOMBasicEdges[i].basic == 0) return false;
  }
  for(j = 0; ISOMBasicEdges[i].edges[j] != 0; j++){
    if(cmpType >= ISOMBasicEdges[i].edges[j] && cmpType <= ISOMBasicEdges[i].edges[j] + 12) return true;
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
      isomType = getISOMType(getStackCliffUpper(tiles[6]));
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
      isomType = getISOMType(getStackCliffUpper(tiles[4]));
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



void generateTypeTables(){
  memset(EdgeTypes, 0, sizeof(EdgeTypes));
  memset(ISOMTypes, 0, sizeof(ISOMTypes));
  memset(ISOMEdgeTypes, 0, sizeof(ISOMEdgeTypes));
  memset(HorizCliffTypes, 0, sizeof(HorizCliffTypes));
  memset(StackCliffs, 0, sizeof(StackCliffs));
  memset(ISOMBasicEdges, 0, sizeof(ISOMBasicEdges));
  stackCliffCount = 0;
  
  int i,j;
  u8 EdgeCV5Type[MAX_TABLE_COUNT] = {0}; // temporary look-up table for generating the others
  
  // pass 1 to get IDs
  for(i = 2; i < 1024; i += 2){
    if(cv5[i].id > 1){
      if(ISOMTypes[cv5[i].id] == 0){
        for(j = 0; CV5ISOMTypes[tileset][j].id != 0; j++){
          if(CV5ISOMTypes[tileset][j].id == cv5[i].id){
            ISOMTypes[cv5[i].id] = CV5ISOMTypes[tileset][j].ISOM;
            break;
          }
        }
      }
      
      // basic types
      if(cv5[i].group.edge.left == cv5[i].group.edge.up && cv5[i].group.edge.left == cv5[i].group.edge.right && cv5[i].group.edge.left == cv5[i].group.edge.down){
        EdgeTypes[cv5[i].group.edge.left] = cv5[i].group.edge.left;
        HorizCliffTypes[cv5[i].group.edge.left] = cv5[i].group.edge.left;
        ISOMEdgeTypes[cv5[i].group.edge.left] = ISOMTypes[cv5[i].id];
        EdgeCV5Type[cv5[i].group.edge.left] = cv5[i].id;
      }
      
      // cliff types
      if(cv5[i].group.edge.left < EDGE_RSV_START && cv5[i].group.edge.up < EDGE_RSV_START && cv5[i].group.edge.right < EDGE_RSV_START && cv5[i].group.edge.down < EDGE_RSV_START){
        if(EdgeTypes[cv5[i].group.edge.left] == cv5[i].group.edge.left && cv5[i].group.edge.left == cv5[i].group.edge.down && cv5[i].group.edge.right == cv5[i].group.edge.up){
          EdgeTypes[cv5[i].group.edge.right] = cv5[i].group.edge.left;
          HorizCliffTypes[cv5[i].group.edge.right] = cv5[i].group.edge.right - 1;
          ISOMEdgeTypes[cv5[i].group.edge.right] = ISOMEdgeTypes[cv5[i].group.edge.left];
          EdgeCV5Type[cv5[i].group.edge.right] = cv5[i].id;
        }
        if(EdgeTypes[cv5[i].group.edge.right] == cv5[i].group.edge.right && cv5[i].group.edge.left == cv5[i].group.edge.up && cv5[i].group.edge.right == cv5[i].group.edge.down){
          EdgeTypes[cv5[i].group.edge.left] = cv5[i].group.edge.right;
          HorizCliffTypes[cv5[i].group.edge.left] = cv5[i].group.edge.left;
          ISOMEdgeTypes[cv5[i].group.edge.left] = ISOMEdgeTypes[cv5[i].group.edge.right];
          EdgeCV5Type[cv5[i].group.edge.left] = cv5[i].id;
        }
      }
      
      // more cliff stuff
      if(cv5[i].group.edge.up == 53 && cv5[i].group.edge.right == 50 && cv5[i].group.edge.right != cv5[i].group.edge.down && HorizCliffTypes[cv5[i].group.edge.down] == cv5[i].group.edge.down - 1){
        EdgeTypes[cv5[i].group.edge.left] = EdgeTypes[cv5[i].group.edge.down];
        HorizCliffTypes[cv5[i].group.edge.left] = EdgeTypes[cv5[i].group.edge.down];
        ISOMEdgeTypes[cv5[i].group.edge.left] = ISOMEdgeTypes[cv5[i].group.edge.down];
        EdgeCV5Type[cv5[i].group.edge.left] = cv5[i].id;
      }
      if(cv5[i].group.edge.left == 52 && cv5[i].group.edge.up == 54 && cv5[i].group.edge.right != cv5[i].group.edge.down && HorizCliffTypes[cv5[i].group.edge.down] == cv5[i].group.edge.down){
        EdgeTypes[cv5[i].group.edge.right] = EdgeTypes[cv5[i].group.edge.down];
        HorizCliffTypes[cv5[i].group.edge.right] = EdgeTypes[cv5[i].group.edge.down];
        ISOMEdgeTypes[cv5[i].group.edge.right] = ISOMEdgeTypes[cv5[i].group.edge.down];
        EdgeCV5Type[cv5[i].group.edge.right] = cv5[i].id;
      }
    }
  }
  
  // pass 2 to get stacked cliffs and edges (because they can appear before the actual cliffs themselves)
  bool doingStackCliff = false;
  u16 addID = 0;
  u16 addA = 0;
  u16 addB = 0;
  for(i = 2; i < 1024; i += 2){
    if(doingStackCliff && (cv5[i].id != StackCliffs[stackCliffCount].lowerID || cv5[i].group.edge.up >= EDGE_RSV_START || EdgeTypes[cv5[i].group.edge.up] == HorizCliffTypes[cv5[i].group.edge.up] || EdgeCV5Type[cv5[i].group.edge.up] != StackCliffs[stackCliffCount].upperID)){
      StackCliffs[stackCliffCount].lastGroup = i;
      stackCliffCount++;
      doingStackCliff = false;
    }
    if(addID != 0 && addID != cv5[i].id){
      addBasicEdge(ISOMEdgeTypes[addA], ISOMTypes[addID]);
      addBasicEdge(ISOMEdgeTypes[addB], ISOMTypes[addID]);
      addA = 0;
      addB = 0;
      addID = 0;
    }
    
    if(cv5[i].id > 1){
      if(!doingStackCliff && cv5[i].group.edge.up < EDGE_RSV_START && EdgeTypes[cv5[i].group.edge.up] != HorizCliffTypes[cv5[i].group.edge.up] && EdgeCV5Type[cv5[i].group.edge.up] != cv5[i].id){
        StackCliffs[stackCliffCount].lowerID = cv5[i].id;
        StackCliffs[stackCliffCount].upperID = EdgeCV5Type[cv5[i].group.edge.up];
        StackCliffs[stackCliffCount].firstGroup = i;
        doingStackCliff = true;
      }
      
      if((cv5[i].group.edge.left == 51 || cv5[i].group.edge.right == 51) && cv5[i].group.edge.left == cv5[i].group.edge.up && cv5[i].group.edge.right == cv5[i].group.edge.down){
        if(cv5[i].group.edge.left < EDGE_RSV_START){
          addA = cv5[i].group.edge.left;
          addID = cv5[i].id;
        }
        if(cv5[i].group.edge.right < EDGE_RSV_START){
          addB = cv5[i].group.edge.right;
        }else if(cv5[i].group.edge.right == 55){
          addB = 0;
        }
      }
    }
  }
}

void addBasicEdge(u32 basic, u32 edge){
  int i,j;
  if(basic == 0 || edge == 0) return; // nothing to add
  for(i = 0; ISOMBasicEdges[i].basic != 0; i++){
    if(ISOMBasicEdges[i].basic == basic){ // basic already exists
      for(j = 0; ISOMBasicEdges[i].edges[j] != 0; j++){
        if(ISOMBasicEdges[i].edges[j] == edge) return; // already exists
      }
      ISOMBasicEdges[i].edges[j] = edge;
      return;
    }
  }
  ISOMBasicEdges[i].basic = basic;
  ISOMBasicEdges[i].edges[0] = edge;
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


u16 getStackCliffUpper(u16 id){
  int i;
  for(i = 0; i < stackCliffCount; i++){
    if(id >= StackCliffs[i].firstGroup && id < StackCliffs[i].lastGroup){
      return StackCliffs[i].upperID;
    }
  }
  return cv5[id].id;
}

u32 getEdgeType(u16 id){
  if(id >= EDGE_RSV_START) return id;
  return EdgeTypes[id];
}

u32 getISOMType(u16 id){
  if(id >= 36) return 0;
  return ISOMTypes[id];
}

u32 getISOMFromEdge(u16 id){
  if(id >= MAX_TABLE_COUNT) return 0;
  return ISOMEdgeTypes[id];
}

bool isBasicGroup(u16 group){
  if(group >= cv5count) return false;
  return cv5[group].group.edge.left == cv5[group].group.edge.up && cv5[group].group.edge.left == cv5[group].group.edge.right && cv5[group].group.edge.left == cv5[group].group.edge.down;
}


bool isBasicType(u16 id){
  if(id == 0 || id >= MAX_TABLE_COUNT || id >= EDGE_RSV_START) return false;
  if(id < 8) return true;
  return getCliffType(id) == CLIFF_NONE;
}

u32 getCliffType(u16 id){
  if(id == 0 || id >= MAX_TABLE_COUNT) return CLIFF_NONE;
  if(HorizCliffTypes[id] < 8) return CLIFF_NONE;
  u16 base = id;
  if(HorizCliffTypes[base] == base-1) base--;
  if(HorizCliffTypes[base] == base && HorizCliffTypes[base+1] == base){
    if(base == id) return CLIFF_LEFT;
    if(base+1 == id) return CLIFF_RIGHT;
  }
  return CLIFF_NONE;
}

