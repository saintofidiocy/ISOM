#include "chk.h"

u8* chk = NULL;
u32 chkSize = 0;
CHK* chkERA  = NULL;
CHK* chkDIM  = NULL;
CHK* chkTILE = NULL;
CHK* chkMTXM = NULL;
CHK* chkISOM = NULL;
bool validTILEChunk = false;
bool validMTXMChunk = false;
bool validISOMChunk = false;
u32 tileSize = 0;
u32 isomSize = 0;

void addCHKSection(u32 section, u32 size, void* data);

void unloadCHK(){
  if(chk != NULL) free(chk);
  chk = NULL;
  chkSize = 0;
  chkERA = NULL;
  chkDIM = NULL;
  chkTILE = NULL;
  chkMTXM = NULL;
  chkISOM = NULL;
  validTILEChunk = false;
  validMTXMChunk = false;
  validISOMChunk = false;
  tileSize = 0;
  isomSize = 0;
}

bool parseCHK(u8* data, u32 size){
  CHK* chunk;
  u32 position = 0;
  
  unloadCHK();
  
  while(position + 8 <= size){
    chunk = (CHK*)(&data[position]);
    switch(chunk->name){
      case CHK_ERA:
        if(chunk->size == 2){
          chkERA = chunk;
        }
        break;
      case CHK_DIM:
        if(chunk->size == 4){
          chkDIM = chunk;
        }
        break;
      case CHK_MTXM:
        if(chunk->size > 0 && chunk->size <= MAX_TILE_SIZE){
          if(chkMTXM != NULL) puts("WARNING: Duplicate MTXM section is not supported.");
          chkMTXM = chunk;
        }
        break;
      case CHK_TILE:
        if(chunk->size > 0 && chunk->size <= MAX_TILE_SIZE){
          if(chkTILE != NULL) puts("WARNING: Duplicate TILE section is not supported.");
          chkTILE = chunk;
        }
        break;
      case CHK_ISOM:
        if(chunk->size > 0 && chunk->size <= MAX_ISOM_SIZE){
          if(chkISOM != NULL) puts("WARNING: Duplicate ISOM section is not supported.");
          chkISOM = chunk;
        }
        break;
    }
    position += 8 + chunk->size;
  }
  
  if(chkERA == NULL){
    puts("ERROR: \"ERA \" section not found.");
    return false;
  }
  if(chkDIM == NULL){
    puts("ERROR: \"DIM \" section not found.");
    return false;
  }
  if(chkDIM->dim.width == 0 || chkDIM->dim.height == 0 || chkDIM->dim.width > 256 || chkDIM->dim.height > 256){
    puts("ERROR: Invalid map dimensions.");
    return false;
  }
  
  printf("dim: %d x %d\nera: %d\n", chkDIM->dim.width, chkDIM->dim.height, chkERA->era);
  
  tileSize = chkDIM->dim.width * chkDIM->dim.height * sizeof(u16);
  if(chkTILE != NULL){
    if(chkTILE->size != tileSize){
      printf("WARNING: TILE section size %d, expected %d\n", chkTILE->size, tileSize);
    }else{
      puts("TILE found");
      validTILEChunk = true;
    }
  }
  if(chkMTXM != NULL){
    if(chkMTXM->size != tileSize){
      printf("ERROR: MTXM section size %d, expected %d\n", chkMTXM->size, tileSize);
    }else{
      puts("MTXM found");
      validMTXMChunk = true;
    }
  }
  if(!validTILEChunk && !validMTXMChunk){
    puts("ERROR: Valid \"TILE\" or \"MTXM\" sections not found.");
    return false;
  }
  
  isomSize = (chkDIM->dim.width/2+1) * (chkDIM->dim.height+1) * sizeof(ISOMRect);
  if(chkISOM != NULL){
    if(chkISOM->size != isomSize){
      printf("WARNING: ISOM section size %d, expected %d\n", chkISOM->size, isomSize);
    }else{
      u32 i;
      // only consider data valid if it is non-null
      validISOMChunk = false;
      for(i = 0; i < isomSize/4; i++){
        if(chkISOM->data32[i] != 0){
          validISOMChunk = true;
          break;
        }
      }
    }
  }
  
  chk = data;
  chkSize = size;
  
  return true;
}

void setCHKData(u32 section, void* data){
  switch(section){
    case CHK_MTXM:
      if(validMTXMChunk){
        memcpy(chkMTXM->data, data, tileSize);
      }else{
        addCHKSection(section, tileSize, data);
      }
      break;
    case CHK_TILE:
      if(validTILEChunk){
        memcpy(chkTILE->data, data, tileSize);
      }else{
        addCHKSection(section, tileSize, data);
      }
      break;
    case CHK_ISOM:
      if(validISOMChunk){
        memcpy(chkISOM->data, data, isomSize);
      }else{
        addCHKSection(section, isomSize, data);
      }
      break;
    default:
      puts("Error adding unsupported section");
      return;
  }
}

u8* getCHK(u32* size){
  if(size != NULL) *size = chkSize;
  return chk;
}


u32 getMapEra(){
  if(chkERA == NULL) return 0;
  return chkERA->era & 7;
}

void getMapDim(u32* width, u32* height){
  if(chkDIM == NULL) return;
  if(width  != NULL) *width  = chkDIM->dim.width;
  if(height != NULL) *height = chkDIM->dim.height;
}

void getMapTILE(u16* buffer){
  if(chkDIM == NULL || buffer == NULL) return;
  if(validTILEChunk){
    memcpy(buffer, chkTILE->data, tileSize);
  }else if(validMTXMChunk){
    memcpy(buffer, chkMTXM->data, tileSize);
  }else{
    memset(buffer, 0, tileSize);
  }
}

void getMapMTXM(u16* buffer){
  if(chkDIM == NULL || buffer == NULL) return;
  if(validMTXMChunk){
    memcpy(buffer, chkMTXM->data, tileSize);
  }else if(validTILEChunk){
    memcpy(buffer, chkTILE->data, tileSize);
  }else{
    memset(buffer, 0, tileSize);
  }
}

void getMapISOM(ISOMRect* buffer){
  if(chkDIM == NULL || chkISOM == NULL || buffer == NULL) return;
  if(validISOMChunk){
    memcpy(buffer, chkISOM->data, isomSize);
  }else{
    memset(buffer, 0, isomSize);
  }
}


bool hasTILEData(){
  return chkDIM != NULL && chkTILE != NULL && validTILEChunk;
}

bool hasMTXMData(){
  return chkDIM != NULL && chkMTXM != NULL && validMTXMChunk;
}

bool hasISOMData(){
  return chkDIM != NULL && chkISOM != NULL && validISOMChunk;
}

u16 getMTXMTile(u32 x, u32 y){
  if(x >= chkDIM->dim.width || y >= chkDIM->dim.height) return 0;
  if(validMTXMChunk){
    return chkMTXM->tiles[y*chkDIM->dim.width + x];
  }else if(validTILEChunk){
    return chkTILE->tiles[y*chkDIM->dim.width + x];
  }else{
    return 0;
  }
}

u16 getTILETile(u32 x, u32 y){
  if(!validTILEChunk || x >= chkDIM->dim.width || y >= chkDIM->dim.height) return 0;
  return chkTILE->tiles[y*chkDIM->dim.width + x];
}


void addCHKSection(u32 section, u32 size, void* data){
  u8* newCHK = NULL;
  u32 newSize = chkSize + size + 8;
  CHK* newSect = NULL;
  
  switch(section){
    case CHK_MTXM:
      if(size != tileSize){
        printf("Error: Invalid MTXM size %d (expected %d)\n", size, tileSize);
        return;
      }
      if(validMTXMChunk){
        setCHKData(section, data);
        return;
      }
      break;
    case CHK_TILE:
      if(size != tileSize){
        printf("Error: Invalid TILE size %d (expected %d)\n", size, tileSize);
        return;
      }
      if(validTILEChunk){
        setCHKData(section, data);
        return;
      }
      break;
    case CHK_ISOM:
      if(size != isomSize){
        printf("Error: Invalid ISOM size %d (expected %d)\n", size, isomSize);
        return;
      }
      if(validISOMChunk){
        setCHKData(section, data);
        return;
      }
      break;
    default:
      puts("Error adding unsupported section");
      return;
  }
  
  newCHK = malloc(newSize);
  if(newCHK == NULL){
    puts("Could not allocate memory :(");
    return;
  }
  
  memcpy(newCHK, chk, chkSize);
  
  if(chkERA != NULL) chkERA = (CHK*)((u32)chkERA - (u32)chk + (u32)newCHK);
  if(chkDIM != NULL) chkDIM = (CHK*)((u32)chkDIM - (u32)chk + (u32)newCHK);
  if(chkTILE != NULL) chkTILE = (CHK*)((u32)chkTILE - (u32)chk + (u32)newCHK);
  if(chkMTXM != NULL) chkMTXM = (CHK*)((u32)chkMTXM - (u32)chk + (u32)newCHK);
  if(chkISOM != NULL) chkISOM = (CHK*)((u32)chkISOM - (u32)chk + (u32)newCHK);
  newSect = (CHK*)((u32)newCHK + chkSize);
  free(chk);
  chk = newCHK;
  chkSize = newSize;
  
  newSect->name = section;
  newSect->size = size;
  memcpy(newSect->data, data, size);
  
  switch(section){
    case CHK_MTXM:
      chkMTXM = newSect;
      validMTXMChunk = true;
      break;
    case CHK_TILE:
      chkTILE = newSect;
      validTILEChunk = true;
      break;
    case CHK_ISOM:
      chkISOM = newSect;
      validISOMChunk = true;
      break;
  }
}
