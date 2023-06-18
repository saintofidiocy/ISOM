#ifndef H_GUI
#define H_GUI
#include "types.h"

bool makeWindow();
void setStatusText(const char* message);
void dispError(const char* error);
void setOpenFilename(const char* filename);


#define MENU_FILE        0x0000
#define MENU_F_OPEN      0x0010
#define MENU_F_SAVE      0x0011
#define MENU_F_SAVE_AS   0x0012
#define MENU_F__SEP      0x0013
#define MENU_F_EXIT      0x0014

#define WIND_MINIMAP     0x0001
#define WIND_MAP         0x0002
#define WIND_OPTIONS     0x0003
#define WIND_STATUS      0x0004

#endif
