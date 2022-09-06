#include "gui.h"
#include "terrain.h"
#include "chk.h"
#include "isom.h"
#include "files.h"
#include <windows.h>
#include <commctrl.h>

typedef struct {
  BITMAPINFOHEADER bmiHeader;
  RGBA bmiColors[256];
} BMI;

const char szClassName[] = "isomedit";
const char szClassImage[] = "staticmemes";

HFONT hFont;
HBRUSH hbrbg;

HWND mainwnd;
HWND hMini;
HWND hMap;
HWND hOptions;
HWND hStatus;

HMENU hMenu;
HMENU hFileMenu;
HACCEL accel;

HDC hdcMini;
HDC hdcMap;
BMI bmiMini = {{sizeof(BITMAPINFOHEADER), 128, -128, 1,  8, BI_RGB, 0}, 0};
BMI bmiMap  = {{sizeof(BITMAPINFOHEADER),   0,    0, 1, 24, BI_RGB, 0}, 0};

s32 mapTileWidth = 0;
s32 mapTileHeight = 0;
s32 mapPixelWidth = 0;
s32 mapPixelHeight = 0;

u8 minibuf[128*128] = {0};
u32 miniScale = MINIMAP_256;

u8* mapbuf = NULL;
u32 mapBufSize = 0;
u32 mapBufWidth = 0;

s32 scrollX = 0;
s32 scrollY = 0;
s32 screenWidth = 0;
s32 screenHeight = 0;

char loadedExtension[4] = "";

void redraw(bool scroll);
void resizeWindow(u32 width, u32 height);
void setScrollbars();

bool makeWindow(u32 w, u32 h);
HWND CreateControl(DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND mainwnd, int id);
LRESULT CALLBACK WinProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ImgProc(HWND, UINT, WPARAM, LPARAM);



bool initWindow(){
  if(makeWindow(800,600) == false){
    printf("Error making window");
    return false;
  }
  
  MSG msg;
  while(GetMessage(&msg, NULL, 0, 0)){
    if(!TranslateAccelerator(msg.hwnd, accel, &msg)){
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  
  DestroyAcceleratorTable(accel);
  return true;
}


void setStatusText(const char* message){
  if(hStatus != NULL) SetWindowText(hStatus, message);
}


void openMap(){
  char path[260];
  char title[260];
  OPENFILENAME ofn = {sizeof(ofn), mainwnd, NULL, "StarCraft Scenario File (*.scm,*.scx,*.chk)\0*.scm;*.scx;*.chk\0All Files\0*.*\0",
                      NULL, 0, 1, path, sizeof(path), NULL, 0, NULL, NULL, OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR,
                      0,0, NULL, 0, NULL, NULL};
  path[0] = 0;
  size_t result;
  if(!GetOpenFileName(&ofn)){
    if(CommDlgExtendedError() != 0){
      MessageBox(0, "Error opening file.", "Error", 0);
    }
    return;
  }
  
  u32 size = 0;
  u8* chk = (u8*)readFile(path, &size, FILE_MAP_FILE);
  
  if(chk == NULL){
    MessageBox(0, "Error opening file.", "Error", 0);
    return;
  }
  if(parseCHK(chk, size) == false){
    MessageBox(0, "Error parsing CHK.", "Error", 0);
    free(chk);
    return;
  }
  if(loadTileset(getMapEra()) == false){
    MessageBox(0, "Error loading tileset.", "Error", 0);
    unloadCHK();
    return;
  }
  copyPal(bmiMini.bmiColors);
  
  initISOMData();
  
  getMapDim(&mapTileWidth, &mapTileHeight);
  mapPixelWidth = mapTileWidth*32;
  mapPixelHeight = mapTileHeight*32;
  
  miniScale = (mapTileWidth > mapTileHeight) ? mapTileWidth : mapTileHeight;
  if(miniScale <= 64){
    miniScale = MINIMAP_64;
  }else if(miniScale <= 128){
    miniScale = MINIMAP_128;
  }else{
    miniScale = MINIMAP_256;
  }
  
  scrollX = 0;
  scrollY = 0;
  setScrollbars();
  
  strncpy(loadedExtension, path + ofn.nFileExtension, 4);
  if(loadedExtension[3] != 0) loadedExtension[0] = 0;
  
  sprintf(title, "ISOM - [ %s ]", path + ofn.nFileOffset);
  SetWindowText(mainwnd, title);
}


void saveMap(){
  char path[260];
  char title[260];
  OPENFILENAME ofn = {sizeof(ofn), mainwnd, NULL, "StarCraft Scenario File (*.scm,*.scx,*.chk)\0*.scm;*.scx;*.chk\0All Files\0*.*\0",
                      NULL, 0, 1, path, sizeof(path), NULL, 0, NULL, NULL, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_NOCHANGEDIR,
                      0,0, NULL, 0, NULL, NULL};
  path[0] = 0;
  size_t result;
  
  u32 saveMode;
  u32 size;
  u8* chk = getCHK(&size);
  if(chk == NULL) return;
  
  if(generateISOMData() == false){
    MessageBox(0, "Error generating ISOM data.", "Error", 0);
    return;
  }
  
  if(!GetSaveFileName(&ofn)){
    if(CommDlgExtendedError() != 0){
      MessageBox(0, "Error opening file.", "Error", 0);
    }
    return;
  }
  
  if(stricmp(path + ofn.nFileExtension, "scm") == 0 || stricmp(path + ofn.nFileExtension, "scx") == 0 || stricmp(path + ofn.nFileExtension, "mpq") == 0){
    puts("mpq file");
    saveMode = FILE_MAP_FILE;
  }else{
    puts("chk file");
    saveMode = FILE_DISK;
  }
  
  if(ofn.nFileExtension == 0 && loadedExtension[0] != 0){
    ofn.nFileExtension = strlen(path);
    path[ofn.nFileExtension++] = '.';
    strcpy(path + ofn.nFileExtension, loadedExtension);
  }
  
  if(writeFile(path, chk, size, saveMode) == false){
    MessageBox(0, "Error writing map.", "Error", 0);
    return;
  }
  
  sprintf(title, "ISOM - [ %s ]", path + ofn.nFileOffset);
  SetWindowText(mainwnd, title);
  setStatusText("File saved successfully!");
}







void redraw(bool scroll){
  if(mapbuf == NULL || screenWidth == 0) return;
  
  if(scroll){
    s32 x1,x2,y1,y2;
    s32 xOffs, yOffs;
    s32 x,y;
    u32 tile;
    RGBA shading = {0};
    
    x1 = scrollX / 32;
    y1 = scrollY / 32;
    x2 = (scrollX + screenWidth + 31) / 32;
    y2 = (scrollY + screenHeight + 31) / 32;
    xOffs = -(scrollX & 31);
    yOffs = -(scrollY & 31);
    if(x2 > mapTileWidth) x2 = mapTileWidth;
    if(y2 > mapTileHeight) y2 = mapTileHeight;
    
    // map tiles
    for(y = y1; y < y2; y++){
      for(x = x1; x < x2; x++){
        tile = getTileAt(x, y, &shading);
        drawTile(mapbuf, mapBufWidth, screenHeight, (x-x1)*32 + xOffs, (y-y1)*32 + yOffs, tile, shading);
      }
    }
    
    // minimap tiles
    memset(minibuf, 0, sizeof(minibuf));
    switch(miniScale){
      case MINIMAP_64:
        xOffs = (128 - mapTileWidth*2)/2;
        yOffs = (128 - mapTileHeight*2)/2;
        x1 = scrollX / 16;
        y1 = scrollY / 16;
        x2 = x1 + screenWidth / 16;
        y2 = y1 + screenHeight / 16;
        if(x2 >= mapTileWidth*2) x2 = mapTileWidth*2-1;
        if(y2 >= mapTileHeight*2) y2 = mapTileHeight*2-1;
        break;
      case MINIMAP_128:
        xOffs = (128 - mapTileWidth)/2;
        yOffs = (128 - mapTileHeight)/2;
        x2 = x1 + screenWidth / 32;
        y2 = y1 + screenHeight / 32;
        if(x2 >= mapTileWidth) x2 = mapTileWidth-1;
        if(y2 >= mapTileHeight) y2 = mapTileHeight-1;
        break;
      case MINIMAP_256:
        xOffs = (128 - mapTileWidth/2)/2;
        yOffs = (128 - mapTileHeight/2)/2;
        x1 /= 2;
        y1 /= 2;
        x2 = x1 + screenWidth / 64;
        y2 = y1 + screenHeight / 64;
        if(x2 >= mapTileWidth/2) x2 = mapTileWidth/2-1;
        if(y2 >= mapTileHeight/2) y2 = mapTileHeight/2-1;
        break;
    }
    drawMinimap(minibuf + yOffs*128+xOffs, 128, mapTileHeight, mapTileWidth, mapTileHeight, miniScale);
    
    // minimap screen position
    for(x = x1; x <= x2; x++){
      minibuf[(yOffs+y1)*128+x+xOffs] = 255;
      minibuf[(yOffs+y2)*128+x+xOffs] = 255;
    }
    for(y = y1; y < y2; y++){
      minibuf[(y+yOffs)*128+x1+xOffs] = 255;
      minibuf[(y+yOffs)*128+x2+xOffs] = 255;
    }
    
    // minimap border
    switch(miniScale){
      case MINIMAP_64:
        x1 = (128 - mapTileWidth*2)/2;
        y1 = (128 - mapTileHeight*2)/2;
        x2 = x1 + mapTileWidth*2;
        y2 = y1 + mapTileHeight*2;
        break;
      case MINIMAP_128:
        x1 = (128 - mapTileWidth)/2;
        y1 = (128 - mapTileHeight)/2;
        x2 = x1 + mapTileWidth;
        y2 = y1 + mapTileHeight;
        break;
      case MINIMAP_256:
        x1 = (128 - mapTileWidth/2)/2;
        y1 = (128 - mapTileHeight/2)/2;
        x2 = x1 + mapTileWidth/2;
        y2 = y1 + mapTileHeight/2;
        break;
    }
    x1--;
    y1--;
    if(y1 >= 0 || y2 < 128){
      for(x = x1; x <= x2 && x < 128; x++){
        if(x < 0) x = 0;
        if(y1 >= 0) minibuf[y1*128+x] = 255;
        if(y2 < 128) minibuf[y2*128+x] = 83;
      }
    }
    if(x1 >= 0 || x2 < 128){
      for(y = y1+1; y <= y2 && y < 128; y++){
        if(y < 0) y = 0;
        if(x1 >= 0) minibuf[y*128+x1] = 255;
        if(x2 < 128) minibuf[y*128+x2] = 83;
      }
    }
  }
  SetDIBitsToDevice(hdcMini, 0, 0, 128, 128, 0, 0, 0, 128, minibuf, (BITMAPINFO*)&bmiMini, DIB_RGB_COLORS);
  SetDIBitsToDevice(hdcMap, 0, 0, screenWidth, screenHeight, 0, 0, 0, screenHeight, mapbuf, (BITMAPINFO*)&bmiMap, DIB_RGB_COLORS);
  RedrawWindow(hMap, NULL, NULL, RDW_VALIDATE);
}

void resizeWindow(u32 width, u32 height){
  s32 newWidth;
  s32 newHeight;
  u32 newSize;
  RECT rect;
  
  // subtract off status bar height
  GetWindowRect(hStatus, &rect);
  newWidth = width - 132;
  newHeight = height - (rect.bottom - rect.top);
  
  // for some reason it has a really big height upon startup ?
  if(newHeight > 1000000) return;
  
  // update map area & listbox sizes
  SetWindowPos(hOptions, NULL, 0, 132, 132, newHeight - 132, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);
  SetWindowPos(hMap, NULL, 132, 0, newWidth, newHeight, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);
  
  // update screen position & scrollbars
  GetClientRect(hMap, &rect);
  screenWidth = rect.right;
  screenHeight = rect.bottom;
  
  mapBufWidth = screenWidth*3;
  mapBufWidth += ((-mapBufWidth)&3);
  //mapBufWidth = screenWidth + ((-screenWidth)&3);
  newSize = screenHeight * mapBufWidth;
  if(newSize > mapBufSize){
    free(mapbuf);
    mapbuf = calloc(newSize/4, 4);
    if(mapbuf != NULL){
      mapBufSize = newSize;
    }else{
      mapBufSize = 0;
      puts("wat");
    }
  }
  
  bmiMap.bmiHeader.biWidth = screenWidth;
  bmiMap.bmiHeader.biHeight = screenHeight;
  
  setScrollbars();
}

void setScrollbars(){
  SCROLLINFO si;
  
  // horizontal scroll
  if(mapPixelWidth > screenWidth){
    si.nMax = mapPixelWidth;
    if(scrollX + screenWidth >= mapPixelWidth){
      scrollX = mapPixelWidth - screenWidth;
    }
  }else{
    si.nMax = 0;
    scrollX = 0;
  }
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_RANGE|SIF_PAGE|SIF_POS|SIF_DISABLENOSCROLL;
  si.nMin = 0;
  si.nPage = screenWidth;
  si.nPos = scrollX;
  si.nTrackPos = 0;
  SetScrollInfo(hMap, SB_HORZ, &si, true); 
  
  // vertical scroll
  if(mapPixelHeight > screenHeight){
    si.nMax = mapPixelHeight;
    if(scrollY + screenHeight >= mapPixelHeight){
      scrollY = mapPixelHeight - screenHeight;
    }
  }else{
    si.nMax = 0;
    scrollY = 0;
  }
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_RANGE|SIF_PAGE|SIF_POS|SIF_DISABLENOSCROLL;
  si.nMin = 0;
  si.nPage = screenHeight;
  si.nPos = scrollY;
  si.nTrackPos = 0;
  SetScrollInfo(hMap, SB_VERT, &si, true); 
  
  redraw(true);
}



bool makeWindow(u32 w, u32 h){
  int i,j;
  hbrbg = CreateSolidBrush(0);
  RECT border = {0, 0, w, h};
  WNDCLASSEX wincl = {sizeof(WNDCLASSEX), CS_DBLCLKS, WinProc, 0, 0, GetModuleHandle(NULL), LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, IDC_ARROW), (HBRUSH)COLOR_BACKGROUND+1, NULL, szClassName, LoadIcon(NULL, IDI_APPLICATION)};
  WNDCLASSEX imgcl = {sizeof(WNDCLASSEX), CS_DBLCLKS, ImgProc, 0, 0, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), hbrbg, NULL, szClassImage, NULL};
  MENUITEMINFO mii;
  ACCEL akeys[5];
  char path[260];
  
  InitCommonControls();
  
  if(!RegisterClassEx(&wincl)) return false;
  if(!RegisterClassEx(&imgcl)) return false;
  
  AdjustWindowRect(&border, WS_OVERLAPPEDWINDOW, 1);
  mainwnd = CreateWindowEx(0, szClassName, "ISOM", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, border.right - border.left, border.bottom - border.top, HWND_DESKTOP, NULL, GetModuleHandle(NULL), NULL);
 
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.cbSize = sizeof(MENUITEMINFO);
  mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
  mii.fType = MFT_STRING;
 
  // File menu
  hFileMenu = CreatePopupMenu();
  mii.wID = MENU_F_OPEN;
  mii.fState = 0;
  mii.dwTypeData = "&Open Map\tCtrl+O";
  InsertMenuItem(hFileMenu, 0, 1, &mii);
  
  mii.wID = MENU_F_SAVE;
  mii.dwTypeData = "Save Map";
  InsertMenuItem(hFileMenu, 1, 1, &mii);
  
  mii.wID = MENU_F_SAVE_AS;
  mii.dwTypeData = "&Save Map As...\tCtrl+S";
  InsertMenuItem(hFileMenu, 2, 1, &mii);
  
  mii.wID = MENU_F__SEP;
  mii.fType = MFT_SEPARATOR;
  InsertMenuItem(hFileMenu, 3, 1, &mii);
  
  mii.wID = MENU_F_EXIT;
  mii.fType = MFT_STRING;
  mii.dwTypeData = "E&xit";
  InsertMenuItem(hFileMenu, 4, 1, &mii);
  
  // Menu Bar
  hMenu = CreateMenu();
  mii.wID = MENU_FILE;
  mii.fMask |= MIIM_SUBMENU;
  mii.dwTypeData = "&File";
  mii.hSubMenu = hFileMenu;
  InsertMenuItem(hMenu, 0, 1, &mii);
  
  SetMenu(mainwnd, hMenu);
  
  // Menu Accelerators
  
  akeys[0].fVirt = FCONTROL | FVIRTKEY;
  akeys[0].key = 'O';
  akeys[0].cmd = MENU_F_OPEN;
  akeys[1].fVirt = FCONTROL | FVIRTKEY;
  akeys[1].key = 'S';
  akeys[1].cmd = MENU_F_SAVE_AS;
  accel = CreateAcceleratorTable(akeys, 2);
  if(accel == NULL) MessageBox(0, ":C", ":C", 0);
  
  hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
  
  hMini = CreateControl(WS_EX_CLIENTEDGE, szClassImage, "", WS_CHILD | WS_VISIBLE, 0, 0, 132, 132, mainwnd, WIND_MINIMAP);
  hMap = CreateControl(WS_EX_CLIENTEDGE, szClassImage, "", WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL, 132, 0, 300, 300, mainwnd, WIND_MAP);
  hOptions = CreateControl(WS_EX_CLIENTEDGE, "Listbox", "", WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | LBS_NOINTEGRALHEIGHT, 0, 132, 132, 300, mainwnd, WIND_OPTIONS);
  hStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, "", mainwnd, WIND_STATUS);
  
  hdcMini = GetDC(hMini);
  hdcMap = GetDC(hMap);
  
  ShowWindow(mainwnd, SW_SHOWDEFAULT);
  return true;
}

HWND CreateControl(DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND mainwnd, int id){
  HWND hwnd;
  hwnd = CreateWindowEx(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, mainwnd, (HMENU)id, 0, NULL);
  SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
  return hwnd;
}


LRESULT CALLBACK WinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
  int i;
  switch(message){
    case WM_DESTROY:
      free(mapbuf);
      mapbuf = NULL;
      mapBufSize = 0;
      PostQuitMessage(0);
      break;
    //case WM_HSCROLL:
    //case WM_VSCROLL:
    //  printf("scroll %08X %08X %08X %08X\n", hwnd, message, wParam, lParam);
    //  break;
    //case WM_PAINT:
      //redraw(false);
      //break;
    case WM_SIZE:
      if(wParam != SIZE_MINIMIZED){
        SendMessage(hStatus, WM_SIZE, wParam, lParam);
        resizeWindow(LOWORD(lParam), HIWORD(lParam));
      }
      break;
    case WM_COMMAND:
      switch(LOWORD(wParam)){
       case MENU_F_OPEN:
        openMap();
        return 0;
       case MENU_F_SAVE:
       case MENU_F_SAVE_AS:
        saveMap();
        return 0;
       case MENU_F_EXIT:
        DestroyWindow(mainwnd);
        return 0;
      }
      return DefWindowProc(hwnd, message, wParam, lParam);
    case WM_CTLCOLORSTATIC:
      if((HWND)lParam == hMini || (HWND)lParam == hMap){
        SetBkColor((HDC)wParam, 0);
        return (INT_PTR)hbrbg;
      }
      break;
  }
  return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK ImgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
  int i;
  switch(message){
    case WM_ERASEBKGND:
     return 0;
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      BeginPaint(hwnd, &ps);
      if(hwnd == hMini){
        SetDIBitsToDevice(hdcMini, 0, 0, 128, 128, 0, 0, 0, 128, minibuf, (BITMAPINFO*)&bmiMini, DIB_RGB_COLORS);
      }
      if(hwnd == hMap){
        SetDIBitsToDevice(hdcMap, 0, 0, screenWidth, screenHeight, 0, 0, 0, screenHeight, mapbuf, (BITMAPINFO*)&bmiMap, DIB_RGB_COLORS);
      }
      EndPaint(hwnd, &ps);
      return 0;
    }
    case WM_LBUTTONDOWN:
    case WM_MOUSEMOVE:
    {
      if(hwnd != hMini) break;
      if(wParam & MK_LBUTTON){
        s32 xNewPos = LOWORD(lParam);
        s32 yNewPos = HIWORD(lParam);
        switch(miniScale){
          case MINIMAP_64:
            xNewPos -= (128 - mapTileWidth*2)/2;
            yNewPos -= (128 - mapTileHeight*2)/2;
            xNewPos *= 16;
            yNewPos *= 16;
            break;
          case MINIMAP_128:
            xNewPos -= (128 - mapTileWidth)/2;
            yNewPos -= (128 - mapTileHeight)/2;
            xNewPos *= 32;
            yNewPos *= 32;
            break;
          case MINIMAP_256:
            xNewPos -= (128 - mapTileWidth/2)/2;
            yNewPos -= (128 - mapTileHeight/2)/2;
            xNewPos *= 64;
            yNewPos *= 64;
            break;
        }
        xNewPos -= screenWidth/2;
        yNewPos -= screenHeight/2;
        if(xNewPos + screenWidth > mapPixelWidth) xNewPos = mapPixelWidth - screenWidth;
        if(xNewPos < 0) xNewPos = 0;
        if(yNewPos + screenHeight > mapPixelHeight) yNewPos = mapPixelHeight - screenHeight;
        if(yNewPos < 0) yNewPos = 0;
        if(xNewPos != scrollX || yNewPos != scrollY){
          scrollX = xNewPos;
          scrollY = yNewPos;
          setScrollbars();
          return 0;
        }
      }
      break;
    }
    case WM_HSCROLL:
    {
      if(hwnd == hMini) break;
      SCROLLINFO si;
      int xDelta;
      int xNewPos;
      int yDelta = 0;
      switch(LOWORD(wParam)){
        case SB_PAGEUP:
          xNewPos = scrollX - screenWidth;
          break;
        case SB_PAGEDOWN:
          xNewPos = scrollX + screenWidth;
          break;
        case SB_LINEUP:
          xNewPos = scrollX - 32;
          break;
        case SB_LINEDOWN:
          xNewPos = scrollX + 32;
          break;
        case SB_THUMBPOSITION:
          xNewPos = HIWORD(wParam);
          break;
        case SB_THUMBTRACK:
          xNewPos = HIWORD(wParam);
          break;
        default:
          xNewPos = scrollX;
      }
      if(xNewPos + screenWidth >= mapPixelWidth) xNewPos = mapPixelWidth - screenWidth;
      if(xNewPos < 0) xNewPos = 0;
      if(xNewPos == scrollX) break;
      xDelta = xNewPos - scrollX;
      scrollX = xNewPos;
      ScrollWindowEx(hMap, -xDelta, -yDelta, (CONST RECT*)NULL, (CONST RECT*)NULL, (HRGN)NULL, (PRECT)NULL, SW_INVALIDATE);
      setScrollbars();
      break;
    }
    case WM_VSCROLL:
    {
      if(hwnd == hMini) break;
      SCROLLINFO si;
      int xDelta = 0;
      int yDelta;
      int yNewPos;
      switch(LOWORD(wParam)){
        case SB_PAGEUP:
          yNewPos = scrollY - screenHeight;
          break;
        case SB_PAGEDOWN:
          yNewPos = scrollY + screenHeight;
          break;
        case SB_LINEUP:
          yNewPos = scrollY - 32;
          break;
        case SB_LINEDOWN:
          yNewPos = scrollY + 32;
          break;
        case SB_THUMBPOSITION:
          yNewPos = HIWORD(wParam);
          break;
        case SB_THUMBTRACK:
          yNewPos = HIWORD(wParam);
          break;
        default:
          yNewPos = scrollY;
      }
      if(yNewPos + screenHeight >= mapPixelHeight) yNewPos = mapPixelHeight - screenHeight;
      if(yNewPos < 0) yNewPos = 0;
      if(yNewPos == scrollY) break;
      yDelta = yNewPos - scrollY;
      scrollY = yNewPos;
      ScrollWindowEx(hMap, -xDelta, -yDelta, (CONST RECT*)NULL, (CONST RECT*)NULL, (HRGN)NULL, (PRECT)NULL, SW_INVALIDATE);
      setScrollbars();
      break;
    }
  }
  return DefWindowProc(hwnd, message, wParam, lParam);
}
