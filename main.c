#include <dirent.h>
#include "types.h"
#include "gui.h"
#include "chk.h"
#include "terrain.h"
#include "isom.h"

// test mode buffers
ISOMRect mapIsom[MAX_ISOM_WIDTH*MAX_ISOM_HEIGHT] = {0};
ISOMRect genIsom[MAX_ISOM_WIDTH*MAX_ISOM_HEIGHT] = {0};

void testMaps();
bool compareGen(const char* file, FILE* log);

int main(int argc, char *argv[]){
  u32 openArg = 0;
  u32 saveArg = 0;
  bool testArg = false;
  bool testDir = false;
  bool forceGen = false;
  bool forceWindow = false;
  
  // parse command line options
  if(argc > 1){
    int i;
    for(i = 1; i < argc; i++){
      if(argv[i][0] != '-'){
        openArg = i;
      }else{
        switch(argv[i][1]){
          case 'i':
            i++;
            openArg = i;
            break;
          case 'o':
          case 's':
            i++;
            saveArg = i;
            break;
          case 'g':
            forceGen = true;
            break;
          case 'w':
            forceWindow = true;
            break;
          case 't':
            if(argv[i][2] == 0 || argv[i][2] == 'd'){
              testArg = true;
              testDir = (argv[i][2] != 0);
              break;
            }
          default:
            printf("Unrecognized option \"%s\"\n", argv[i]);
            return 0;
        }
      }
    }
  }
  
  initArchiveData();
  
  if(openArg > 0){
    setOpenFilename(argv[openArg]);
  }
  
  if(testArg){
    if(testDir){
      testMaps(argv[openArg]);
      setOpenFilename(argv[openArg]);
    }else{
      compareGen(argv[openArg], stdout);
    }
  }
  
  if(saveArg > 0){
    if(openArg == 0 || (testArg && getCHK(NULL) == NULL)){
      puts("Nothing to save.");
      setOpenFilename(""); // nothing to open either
    }else{
      if(testArg == false){
        if(loadMap(argv[openArg]) == false){
          puts("Could not load map.");
          setOpenFilename("");
          saveArg = 1;
        }else{
          if(!forceGen && hasISOMData() && initISOMData()){
            puts("Source ISOM is valid.");
            // not an error
          }else{
            if(forceGen){
              clearMapISOM();
              initISOMData();
            }
            if(generateISOMData() == false){
              puts("ISOM generation failed.");
              saveArg = 1;
            }
          }
        }
      }
      if(saveArg > 1){
        if(writeMap(argv[saveArg])){
          puts("File saved successfully!");
          setOpenFilename(argv[saveArg]);
        }
      }
    }
  }
  
  if((!testArg && saveArg == 0) || forceWindow){
    makeWindow();
  }
  
  closeArchiveData();
  unloadCHK();
  unloadTileset();
  
  return 0;
}


// scans files in path and compares default ISOM data to generated ISOM data
void testMaps(const char* dirpath){
  FILE* log = fopen("isom test.log", "w");
  if(log == NULL){
    puts("Could not open log file.");
    return;
  }
  
  DIR* dir = opendir(dirpath);
  if(dir == NULL){
    puts("Could not open maps folder.");
    fclose(log);
    return;
  }
  
  struct dirent* file;
  u32 count = 0;
  u32 pass = 0;
  char path[520];
  bool useSlash = (dirpath[strlen(dirpath)-1] != '\\');
  
  while ((file = readdir(dir)) != NULL){
    if(file->d_name[0] == '.') continue;
    
    count++;
    fprintf(log, "%-32s-- ", file->d_name);
    if(useSlash){
      sprintf(path, "%s\\%s", dirpath, file->d_name);
    }else{
      sprintf(path, "%s%s", dirpath, file->d_name);
    }
    if(compareGen(path, log)) pass++;
  }
  closedir(dir);
  
  setOpenFilename(path);
  
  sprintf(path, "\n%d of %d passed.\n", pass, count);
  fputs(path, log);
  puts(path);
  fclose(log);
}

// saves default ISOM data then re-generates it and compares the two
bool compareGen(const char* file, FILE* log){
  if(loadMap(file) == false){
    fputs("Could not load map.\n", log);
    return false;
  }
  
  if(hasISOMData() == false || initISOMData() == false){
    fputs("Source ISOM is invalid.\n", log);
    return false;
  }
  
  getMapISOM(mapIsom);
  
  // force ISOM generation
  clearMapISOM();
  initISOMData();
  if(generateISOMData() == false){
    fputs("ISOM generation failed.\n", log);
    return false;
  }
  
  getMapISOM(genIsom);
  
  u32 w,h,isomSize;
  getMapDim(&w, &h);
  isomSize = (w/2+1)*(h+1)*sizeof(ISOMRect);
  u32 x,y,i;
  bool match = true;
  u32 defaultTerrain = 0;
  
  for(i = 0; i < (w/2+1)*(h+1); i++){
    if(mapIsom[i].left.dir == 0){
      if(defaultTerrain == 0){
        defaultTerrain = mapIsom[i].left.type;
      }else if(defaultTerrain != mapIsom[i].left.type){
        printf("inconsistent default terrain type ? l %d != %d\n", defaultTerrain, mapIsom[i].left.type);
        defaultTerrain = 0;
        break;
      }
    }
    if(mapIsom[i].up.dir == 0){
      if(defaultTerrain == 0){
        defaultTerrain = mapIsom[i].up.type;
      }else if(defaultTerrain != mapIsom[i].up.type){
        printf("inconsistent default terrain type ? u %d != %d\n", defaultTerrain, mapIsom[i].up.type);
        defaultTerrain = 0;
        break;
      }
    }
    if(mapIsom[i].down.dir == 0){
      if(defaultTerrain == 0){
        defaultTerrain = mapIsom[i].down.type;
      }else if(defaultTerrain != mapIsom[i].down.type){
        printf("inconsistent default terrain type ? d %d != %d\n", defaultTerrain, mapIsom[i].down.type);
        defaultTerrain = 0;
        break;
      }
    }
  }
  
  i = 0;
  for(y = 0; match && y <= h; y++){
    for(x = 0; match && x <= w/2; x++){
      if(memcmp(&(mapIsom[i]), &(genIsom[i]), sizeof(ISOMRect)) != 0){
        // lazy checking by copying the unused entries directly from the source
        if(x == w/2){
          genIsom[i].right.type = mapIsom[i].right.type;
          if(y&1){
            genIsom[i].up.type = mapIsom[i].up.type;
          }else{
            genIsom[i].down.type = mapIsom[i].down.type;
          }
        }
        if(y == h){
          genIsom[i].down.type = mapIsom[i].down.type;
          if((x&1) == (y&1)){
            genIsom[i].right.type = mapIsom[i].right.type;
          }else{
            genIsom[i].left.type = mapIsom[i].left.type;
          }
        }
        
        if(mapIsom[i].left.dir == 0 && mapIsom[i].left.type == defaultTerrain && genIsom[i].left.type != defaultTerrain) match = false;
        if(mapIsom[i].up.dir == 0 && mapIsom[i].up.type == defaultTerrain && genIsom[i].up.type != defaultTerrain) match = false;
        if(mapIsom[i].right.dir == 0 && mapIsom[i].right.type == defaultTerrain && genIsom[i].right.type != defaultTerrain) match = false;
        if(mapIsom[i].down.dir == 0 && mapIsom[i].down.type == defaultTerrain && genIsom[i].down.type != defaultTerrain) match = false;
        if(x == 0){
          if(isISOMPartialEdgeSimple(genIsom[i].left.type, mapIsom[i].left.type, RIGHT) == false) match = false;
        }else if(x >= (w/2-1)){
          if(isISOMPartialEdgeSimple(genIsom[i].right.type, mapIsom[i].right.type, LEFT) == false) match = false;
        }
        if(y == 0){
          if(isISOMPartialEdgeSimple(genIsom[i].up.type, mapIsom[i].up.type, DOWN) == false) match = false;
        }else if(y >= (h-1)){
          if(isISOMPartialEdgeSimple(genIsom[i].right.type, mapIsom[i].right.type, UP) == false) match = false;
        }
      }
      i++;
    }
  }
  
  if(match){
    fputs("Generated ISOM matches.\n", log);
  }else{
    fputs("Generatied ISOM does not match.\n", log);
  }
  
  return match;
}
