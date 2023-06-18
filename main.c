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
    fputc('\n', log);
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
    fputs("Could not load map.", log);
    return false;
  }
  
  if(hasISOMData() == false || initISOMData() == false){
    fputs("Source ISOM is invalid.", log);
    return false;
  }
  
  getMapISOM(mapIsom);
  
  // force ISOM generation
  clearMapISOM();
  initISOMData();
  if(generateISOMData() == false){
    fputs("ISOM generation failed.", log);
    return false;
  }
  
  getMapISOM(genIsom);
  
  u32 w,h,isomSize;
  getMapDim(&w, &h);
  isomSize = (w/2+1)*(h+1)*sizeof(ISOMRect);
  u32 x,y,i;
  bool match = true;
  
  i = 0;
  for(y = 0; match && y <= h; y++){
    for(x = 0; match && x <= w; x++){
      if(memcmp(&(mapIsom[i]), &(genIsom[i]), sizeof(ISOMRect)) != 0){
        if(mapIsom[i].left.dir == 0 && mapIsom[i].left.type != genIsom[i].left.type) match = false;
        if(mapIsom[i].up.dir == 0 && mapIsom[i].up.type != genIsom[i].up.type) match = false;
        if(mapIsom[i].right.dir == 0 && mapIsom[i].right.type != genIsom[i].right.type) match = false;
        if(mapIsom[i].down.dir == 0 && mapIsom[i].down.type != genIsom[i].down.type) match = false;
        if(x == 0 || y == 0 || x >= (w-1) || y >= (h-1)){
          if(isISOMPartialEdgeSimple(genIsom[i].left.type, mapIsom[i].left.type) == false ||
             isISOMPartialEdgeSimple(genIsom[i].up.type, mapIsom[i].up.type) == false ||
             isISOMPartialEdgeSimple(genIsom[i].right.type, mapIsom[i].right.type) == false ||
             isISOMPartialEdgeSimple(genIsom[i].down.type, mapIsom[i].down.type) == false){
            match = false;
          }
        }
      }
    }
  }
  
  if(match){
    fputs("Generated ISOM matches.", log);
  }else{
    fputs("Generatied ISOM does not match.", log);
  }
  
  return match;
}
