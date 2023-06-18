#include <dirent.h>
#include "types.h"
#include "gui.h"
#include "chk.h"
#include "terrain.h"
#include "isom.h"


void testMaps();
bool compareGen(const char* file, FILE* log);

int main(int argc, char *argv[]){
  initArchiveData();
  
  //testMaps();
  makeWindow();
  
  closeArchiveData();
  unloadCHK();
  unloadTileset();
  
  return 0;
}


// scans files in "maps/" and compares default ISOM data to generated ISOM data
void testMaps(){
  FILE* log = fopen("isom test.log", "w");
  if(log == NULL){
    puts("Could not open log file.");
    system("pause");
    return;
  }
  
  DIR* dir = opendir("maps");
  if(dir == NULL){
    puts("Could not open maps folder.");
    fclose(log);
    system("pause");
    return;
  }
  
  struct dirent* file;
  u32 count = 0;
  u32 pass = 0;
  char path[260];
  
  while ((file = readdir(dir)) != NULL){
    if(file->d_name[0] == '.') continue;
    
    count++;
    fprintf(log, "%-32s-- ", file->d_name);
    sprintf(path, "title %d -- %s\n", count, file->d_name);
    system(path);
    sprintf(path, "maps\\%s", file->d_name);
    if(compareGen(path, log)) pass++;
    fputc('\n', log);
  }
  closedir(dir);
  
  sprintf(path, "\n%d of %d passed.\n", pass, count);
  fputs(path, log);
  puts(path);
  fclose(log);
  
  system("pause");
}

ISOMRect mapIsom[MAX_ISOM_WIDTH*MAX_ISOM_HEIGHT] = {0};
ISOMRect genIsom[MAX_ISOM_WIDTH*MAX_ISOM_HEIGHT] = {0};

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
