#include "terrain.h"
#include "isom.h"
#include "files.h"

const char tilesets[8][10] = {"badlands","platform","install","ashworld","jungle","desert","ice","twilight"};


CV5* cv5 = NULL;
u32 cv5count = 0;
VX4EX* vx4 = NULL;
u32 vx4count = 0;
VR4* vr4 = NULL;
u32 vr4count = 0;
RGBA wpe[256];
u16 dddata[512][256];

void unloadTileset(){
  if(cv5 != NULL) free(cv5);
  if(vx4 != NULL) free(vx4);
  if(vr4 != NULL) free(vr4);
  cv5 = NULL;
  cv5count = 0;
  vx4 = NULL;
  vx4count = 0;
  vr4 = NULL;
  vr4count = 0;
}

bool loadTileset(u32 id){
  u32 size;
  char filename[32];
  
  unloadTileset();
  
  do {
    sprintf(filename, "tileset\\%s.cv5", tilesets[id]);
    cv5 = (CV5*)readFile(filename, &size, FILE_ARCHIVE);
    if(cv5 == NULL) break;
    cv5count = size / sizeof(CV5);
    
    sprintf(filename, "tileset\\%s.vx4ex", tilesets[id]);
    vx4 = (VX4EX*)readFile(filename, &size, FILE_ARCHIVE);
    if(vx4 == NULL) break;
    vx4count = size / sizeof(VX4EX);
    
    sprintf(filename, "tileset\\%s.vr4", tilesets[id]);
    vr4 = (VR4*)readFile(filename, &size, FILE_ARCHIVE);
    if(vr4 == NULL) break;
    vr4count = size / sizeof(VR4);
    
    sprintf(filename, "tileset\\%s.wpe", tilesets[id]);
    if(readFileFixed(filename, wpe, sizeof(wpe), FILE_ARCHIVE) == false) break;
    
    sprintf(filename, "tileset\\%s\\dddata.bin", tilesets[id]);
    if(readFileFixed(filename, dddata, sizeof(dddata), FILE_ARCHIVE) == false) break;
    
    return true;
  } while(false);
  
  puts("Error loading tileset.");
  unloadTileset();
  return false;
}

void copyPal(RGBA* pal){
  u32 i;
  for(i = 0; i < 256; i++){
    pal[i].b = wpe[i].r;
    pal[i].g = wpe[i].g;
    pal[i].r = wpe[i].b;
    pal[i].a = 0;
  }
}

void drawTile(u8* buf, s32 bufWidth, s32 bufHeight, s32 dstX, s32 dstY, u32 tileID, RGBA shading){
  u32 group = tileID >> 4;
  u32 tile = tileID & 0xF;
  bool flip;
  u32 i,x,y;
  
  if(group >= cv5count) group = 0;
  tileID = cv5[group].tiles[tile];
  if(tileID >= vx4count) tileID = 0;
  
  for(i = 0; i < 16; i++){
    x = (i & 3) * 8;
    y = (i >> 2) * 8;
    flip = vx4[tileID].tiles[i] & 1;
    tile = vx4[tileID].tiles[i] >> 1;
    drawMiniTile(buf, bufWidth, bufHeight, dstX + x, dstY + y, tile, flip, shading);
  }
}

void drawMiniTile(u8* buf, s32 bufWidth, s32 bufHeight, s32 dstX, s32 dstY, u32 tileID, bool flip, RGBA shading){
  if(dstX < -7 || dstY < -7 || dstX >= bufWidth/3 || dstY >= bufHeight) return;
  if(tileID >= vr4count) tileID = 0;
  
  s32 xmin = (dstX < 0) ? -dstX : 0;
  s32 ymin = (dstY < 0) ? -dstY : 0;
  s32 xmax = (dstX+7 >= bufWidth/3) ? bufWidth/3 - dstX : 8;
  s32 ymax = (dstY+7 >= bufHeight) ? bufHeight - dstY : 8;
  s32 x,y;
  s32 bufoffs;
  u32 pal;
  
  for(y = ymin; y < ymax; y++){
    //bufoffs = (bufHeight - dstY - y - 1) * bufWidth + dstX;
    bufoffs = (bufHeight - dstY - y - 1) * bufWidth + dstX*3;
    for(x = xmin; x < xmax; x++){
      if(flip){
        pal = vr4[tileID].bmp[y*8 + 7 - x];
      }else{
        pal = vr4[tileID].bmp[y*8 + x];
      }
      if(shading.a == 0){
        buf[bufoffs + x*3 +0] = wpe[pal].b;
        buf[bufoffs + x*3 +1] = wpe[pal].g;
        buf[bufoffs + x*3 +2] = wpe[pal].r;
      }else{
        buf[bufoffs + x*3 +0] = (shading.b*shading.a + wpe[pal].b*(255-shading.a))/255;
        buf[bufoffs + x*3 +1] = (shading.g*shading.a + wpe[pal].g*(255-shading.a))/255;
        buf[bufoffs + x*3 +2] = (shading.r*shading.a + wpe[pal].r*(255-shading.a))/255;
      }
    }
  }
}

void drawMinimap(u8* buf, s32 bufw, s32 bufh, u32 width, u32 height, u32 scale){
  u32 x,y;
  u32 bufoffs;
  u32 tile,group;
  for(y = 0; y < height; y++){
    switch(scale){
      case MINIMAP_64:
        bufoffs = (y*2)*bufw;
        break;
      case MINIMAP_128:
        bufoffs = y*bufw;
        break;
      case MINIMAP_256:
        if(y & 1) continue;
        bufoffs = (y/2)*bufw;
        break;
    }
    for(x = 0; x < width; x++){
      // MTXM tile
      tile = getTileAt(x, y, NULL);
      
      // CV5 tile
      group = tile >> 4;
      tile = tile & 0xF;
      if(group >= cv5count) group = 0;
      
      // VX4 tile
      tile = cv5[group].tiles[tile];
      
      switch(scale){
        case MINIMAP_64:
          buf[bufoffs + x*2] = vr4[vx4[tile].tiles[0] >> 1].bmp[55];
          buf[bufoffs + x*2+1] = vr4[vx4[tile].tiles[1] >> 1].bmp[55];
          buf[bufoffs+bufw + x*2] = vr4[vx4[tile].tiles[4] >> 1].bmp[55];
          buf[bufoffs+bufw + x*2+1] = vr4[vx4[tile].tiles[5] >> 1].bmp[55];
          break;
        case MINIMAP_128:
          buf[bufoffs + x] = vr4[vx4[tile].tiles[0] >> 1].bmp[55];
          break;
        case MINIMAP_256:
          buf[bufoffs + x/2] = vr4[vx4[tile].tiles[0] >> 1].bmp[55];
          x++;
          break;
      }
    }
  }
}
