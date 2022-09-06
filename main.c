#include "types.h"
#include "gui.h"
#include "chk.h"
#include "terrain.h"

int main(int argc, char *argv[]){
  initArchiveData();
  
  initWindow();
  
  closeArchiveData();
  unloadCHK();
  unloadTileset();
  
  return 0;
}
