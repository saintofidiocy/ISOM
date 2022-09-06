#include "files.h"
#include "sfmpq_static.h"
#include "CascLib.h"

// Why, CascLib, why
#undef bool
#undef sprintf

u8* readFileDisk(const char* path, u32* filesize);
bool readFileFixedDisk(const char* path, void* buffer, u32 filesize);
bool writeFileDisk(const char* path, u8* data, u32 filesize);
u8* readFileMPQ(const char* path, u32* filesize);
bool readFileFixedMPQ(const char* path, void* buffer, u32 filesize);
bool writeFileMPQ(const char* path, const char* mpqPath, u8* data, u32 filesize);
u8* readFileCASC(const char* path, u32* filesize);
bool readFileFixedCASC(const char* path, void* buffer, u32 filesize);
char* getInstallPathCASC();

bool mpqLoaded = false;
bool cascLoaded = false;
HANDLE casc = NULL;




void initArchiveData(){
  char* path = getInstallPathCASC();
  if(path != NULL){
    if(CascOpenStorage(path, 0, &casc) == true){
      puts("CASC data located.");
      cascLoaded = true;
    }
    free(path);
  }
  if(cascLoaded) return;
  
  // attempt to load MPQs
  /*
   SFileOpenArchive("\StarDat.mpq", 10, 0, hMPQ[0])
   SFileOpenArchive("\BrooDat.mpq", 50, 0, hMPQ[1])
   SFileOpenArchive("\patch_rt.mpq", 100, 0, hMPQ[2]) 
  */
}

void closeArchiveData(){
  if(cascLoaded){
    CascCloseStorage(casc);
    casc = NULL;
    cascLoaded = false;
  }
  if(mpqLoaded){
    // close MPQs
    mpqLoaded = false;
  }
}


u8* readFile(const char* path, u32* filesize, u32 source){
  MPQHANDLE hMPQ = NULL;
  u8* tmp = NULL;
  
  if(source == FILE_MAP_FILE){
    if(strcmpi(path + strlen(path) - 4, ".chk") == 0){
      source = FILE_DISK;
    }else{
      if(SFileOpenArchive(path, 120, 0, &hMPQ)){
        path = "staredit\\scenario.chk";
        source = FILE_MPQ;
      }else{
        source = FILE_DISK;
      }
    }
  }else if(source == FILE_ARCHIVE){
    if(cascLoaded){
      source = FILE_CASC;
    }else if(mpqLoaded){
      source = FILE_MPQ;
    }else{
      source = FILE_DISK;
    }
  }
  
  switch(source){
    case FILE_DISK:
      tmp = readFileDisk(path, filesize);
      break;
    case FILE_MPQ:
      tmp = readFileMPQ(path, filesize);
      break;
    case FILE_CASC:
      tmp = readFileCASC(path, filesize);
      break;
  }
  
  if(hMPQ != NULL){
    SFileCloseArchive(hMPQ);
  }
  
  return tmp;
}

bool readFileFixed(const char* path, void* buffer, u32 filesize, u32 source){
  if(source == FILE_MAP_FILE){
    return false;
  }else if(source == FILE_ARCHIVE){
    if(cascLoaded){
      source = FILE_CASC;
    }else if(mpqLoaded){
      source = FILE_MPQ;
    }else{
      source = FILE_DISK;
    }
  }
  
  switch(source){
    case FILE_DISK:
      return readFileFixedDisk(path, buffer, filesize);
    case FILE_MPQ:
      return readFileFixedMPQ(path, buffer, filesize);
    case FILE_CASC:
      return readFileFixedCASC(path, buffer, filesize);
  }
}


bool writeFile(const char* path, u8* data, u32 filesize, u32 destination){
  u8* tmp = NULL;
  
  if(destination == FILE_MAP_FILE){
    if(strcmpi(path + strlen(path) - 4, ".chk") == 0){
      destination = FILE_DISK;
    }else{
      destination = FILE_MPQ;
    }
  }
  
  switch(destination){
    case FILE_DISK:
      return writeFileDisk(path, data, filesize);
    case FILE_MPQ:
      return writeFileMPQ(path, "staredit\\scenario.chk", data, filesize);
    default:
      puts("ERROR: Unsupported write mode.");
      return false;
  }
}



u8* readFileDisk(const char* path, u32* filesize){
  u32 size;
  u8* buf;
  
  FILE* f = fopen(path, "rb");
  if(filesize != NULL) *filesize = 0;
  if(f == NULL){
    printf("ERR: Could not open \"%s\"\n", path);
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  size = ftell(f);
  rewind(f);
  if(size == 0 || size == 0xFFFFFFFF){
    printf("ERR: Could not seek \"%s\"\n", path);
    fclose(f);
    return NULL;
  }
  buf = malloc(size);
  if(buf == NULL){
    puts("ERR: Could not allocate memory\n");
    fclose(f);
    return NULL;
  }
  if(fread(buf, 1, size, f) != size){
    printf("ERR: Could not read \"%s\"\n", path);
    fclose(f);
    free(buf);
    return NULL;
  }
  fclose(f);
  if(filesize != NULL) *filesize = size;
  return buf;
}

bool readFileFixedDisk(const char* path, void* buffer, u32 filesize){
  FILE* f = fopen(path, "rb");
  if(f == NULL){
    printf("ERR: Could not open \"%s\"\n", path);
    return false;
  }
  if(buffer == NULL || fread(buffer, 1, filesize, f) != filesize){
    printf("ERR: Could not read \"%s\"\n", path);
    fclose(f);
    return false;
  }
  fclose(f);
  return true;
}

bool writeFileDisk(const char* path, u8* data, u32 filesize){
  FILE* f = fopen(path, "wb");
  if(f == NULL){
    printf("ERR: Could not open \"%s\"\n", path);
    return false;
  }
  if(filesize != 0 && fwrite(data, 1, filesize, f) != filesize){
    printf("ERR: Could not write \"%s\"\n", path);
    fclose(f);
    return false;
  }
  fclose(f);
  return true;
}


u8* readFileMPQ(const char* path, u32* filesize){
  MPQHANDLE hFile = NULL;
  u32 size;
  DWORD read = 0;
  u8* buf;
  
  if(filesize != NULL) *filesize = 0;
  
  if(SFileOpenFile(path, &hFile) == false){
    printf("ERR: Could not find file \"%s\" in archive\n", path);
    return NULL;
  }
  
  size = SFileGetFileSize(hFile, NULL);
  if(size == 0 || size == 0xFFFFFFFF){
    printf("ERR: Could not get filesize \"%s\"\n", path);
    SFileCloseFile(hFile);
    return NULL;
  }
  
  buf = malloc(size);
  if(buf == NULL){
    puts("ERR: Could not allocate memory\n");
    SFileCloseFile(hFile);
    return NULL;
  }
  
  SFileReadFile(hFile, buf, size, &read, NULL);
  if(read != size){
    printf("ERR: Could not read \"%s\"\n", path);
    SFileCloseFile(hFile);
    free(buf);
    return NULL;
  }
  
  SFileCloseFile(hFile);
  if(filesize != NULL) *filesize = size;
  return buf;
}

bool readFileFixedMPQ(const char* path, void* buffer, u32 filesize){
  MPQHANDLE hFile = NULL;
  DWORD read = 0;
  
  if(SFileOpenFile(path, &hFile) == false){
    printf("ERR: Could not find file \"%s\" in archive\n", path);
    return false;
  }
  
  if(buffer != NULL){
    SFileReadFile(hFile, buffer, filesize, &read, NULL);
  }
  SFileCloseFile(hFile);
  
  if(buffer == NULL || read != filesize){
    printf("ERR: Could not read \"%s\"\n", path);
    return false;
  }
  
  return true;
}


bool writeFileMPQ(const char* path, const char* mpqPath, u8* data, u32 filesize){
  MPQHANDLE hmpq = MpqOpenArchiveForUpdateEx(path, MOAU_CREATE_ALWAYS | MOAU_MAINTAIN_LISTFILE, 1024, DEFAULT_BLOCK_SIZE);
  if(hmpq == NULL){
    printf("ERR: Could not open \"%s\"\n", path);
    return false;
  }
  if(MpqAddFileFromBuffer(hmpq, data, filesize, mpqPath, MAFA_COMPRESS2) == false){
    printf("ERR: Could not write \"%s\"\n", path);
    MpqCloseUpdatedArchive(hmpq, 0);
    return false;
  }
  MpqCloseUpdatedArchive(hmpq, 0);
  return true;
}


u8* readFileCASC(const char* path, u32* filesize);
bool readFileFixedCASC(const char* path, void* buffer, u32 filesize);


u8* readFileCASC(const char* path, u32* filesize){
  HANDLE hFile = NULL;
  u32 size;
  DWORD read = 0;
  u8* buf;
  
  if(filesize != NULL) *filesize = 0;
  
  if(CascOpenFile(casc, path, 0, CASC_OPEN_BY_NAME, &hFile) == false){
    printf("ERR: Could not find file \"%s\" in archive\n", path);
    return NULL;
  }
  
  size = CascGetFileSize(hFile, NULL);
  if(size == 0 || size == CASC_INVALID_SIZE){
    printf("ERR: Could not get filesize \"%s\"\n", path);
    CascCloseFile(hFile);
    return NULL;
  }
  
  buf = malloc(size);
  if(buf == NULL){
    puts("ERR: Could not allocate memory\n");
    CascCloseFile(hFile);
    return NULL;
  }
  
  CascReadFile(hFile, buf, size, &read);
  if(read != size){
    printf("ERR: Could not read \"%s\"\n", path);
    CascCloseFile(hFile);
    free(buf);
    return NULL;
  }
  
  CascCloseFile(hFile);
  if(filesize != NULL) *filesize = size;
  return buf;
}

bool readFileFixedCASC(const char* path, void* buffer, u32 filesize){
  MPQHANDLE hFile = NULL;
  DWORD read = 0;
  
  if(CascOpenFile(casc, path, 0, CASC_OPEN_BY_NAME, &hFile) == false){
    printf("ERR: Could not find file \"%s\" in archive\n", path);
    return false;
  }
  
  if(buffer != NULL){
    CascReadFile(hFile, buffer, filesize, &read);
  }
  CascCloseFile(hFile);
  
  if(buffer == NULL || read != filesize){
    printf("ERR: Could not read \"%s\"\n", path);
    return false;
  }
  
  return true;
}



// --- CASC path garbage ---


// protobuf data
// https://developers.google.com/protocol-buffers/docs/encoding
#define WIRE_VARINT            0 // int32, int64, uint32, uint64, sint32, sint64, bool, enum
#define WIRE_64BIT             1 // fixed64, sfixed64, double
#define WIRE_LENGTH_DELIMITED  2 // string, bytes, embedded messages, packed repeated fields
#define WIRE_START_GROUP       3 // groups (deprecated)
#define WIRE_END_GROUP         4 // groups (deprecated)
#define WIRE_32BIT             5 // fixed32, sfixed32, float

// product.db format: https://gist.github.com/neivv/d4c822619c8c845d91ca35a52668d48e

// Arbitrary indexes for use in recursive function
#define MESSAGE_ROOT          0
#define MESSAGE_PRODUCT       1
#define MESSAGE_INSTALLATION  2

// used field ID's within the message types
#define ROOT_INSTALLEDPRODUCT 1
#define PRODUCT_VARIANT_UID   2
#define PRODUCT_INSTALL       3
#define INSTALLATION_PATH     1

const char sc_uid[] = "s1";

bool getInstallPathCASC_Proto(FILE* f, u32 start, u32 len, u32 message, char** path);
u32 varint(FILE* f, u32* decodeH);

// Attempts to find the starcraft install path from product.db
char* getInstallPathCASC(){
  char* path = NULL;
  
  char* programdata = getenv("PROGRAMDATA");
  if(programdata == NULL) return;
  
  char agentpath[0x400];
  sprintf(agentpath, "%s\\Battle.net\\Agent\\product.db", programdata);
  
  FILE* f = fopen(agentpath, "rb");
  if(f == NULL){
    puts("Error: Could not locate Battle.net Agent data.");
    return;
  }
  
  getInstallPathCASC_Proto(f, 0, 0, MESSAGE_ROOT, &path);
  
  fclose(f);
  return path;
}

// Decode protobuf structures
bool getInstallPathCASC_Proto(FILE* f, u32 start, u32 len, u32 message, char** path){
  u8 b;
  u8 wire;
  u8 field;
  u32 i;
  u32 decode;
  u32 decodeH;
  bool exit = false;
  u32 instoffs = 0;
  u32 instlen = 0;
  char uid[4] = {0,0,0,0};
  
  while(!exit && !feof(f) && (message == MESSAGE_ROOT || ftell(f)-start < len)){
    b = fgetc(f);
    wire = b&7;
    field = b>>3;
    switch(wire){
      case WIRE_VARINT:
        decode = varint(f, &decodeH);
        break;
      case WIRE_64BIT:
        fread(&decode, 1, 4, f);
        fread(&decodeH, 1, 4, f);
        break;
      case WIRE_LENGTH_DELIMITED:
      {
        // get length
        decode = varint(f, &decodeH);
        if(decodeH != 0) puts("Nonzero high value in message length !?");
        
        // Different behavior depending on message type
        switch(message){
          case MESSAGE_ROOT:
          {
            if(field == ROOT_INSTALLEDPRODUCT){
              exit = getInstallPathCASC_Proto(f, ftell(f), decode, MESSAGE_PRODUCT, path);
              decode = 0; // already processed, but not necessarily exiting
            }
            break;
          }
          case MESSAGE_PRODUCT:
          {
            switch(field){
              case PRODUCT_INSTALL:
                instoffs = ftell(f);
                instlen = decode;
                if(strcmp(uid, sc_uid) == 0){
                  // don't need to continue processing this message since correct sc_uid was already found
                  exit = true;
                }
                break;
              case PRODUCT_VARIANT_UID:
                if(decode == strlen(sc_uid) && decodeH == 0){
                  for(i=0; i<decode; i++){
                    uid[i] = fgetc(f);
                  }
                  if(instlen != 0 && strcmp(uid, sc_uid) == 0){
                    // don't need to continue processing since correct uid and install field already found
                    exit = true;
                  }
                  decode = 0; // already processed, but not necessarily exiting
                  break;
                }
                break;
              default:
                break;
            }
            break;
          }
          case MESSAGE_INSTALLATION:
          {
            if(field == INSTALLATION_PATH){
              *path = malloc(decode+1);
              if((*path) == NULL){
                puts("Error: Memory");
                return true;
              }
              fread(*path, 1, decode, f);
              (*path)[decode] = 0;
              puts(*path);
              return true;
            }
            break;
          }
        }
        
        // Eat bytes for any unread string/whatever
        if(!exit && decode != 0){
          fseek(f, decode, SEEK_CUR);
        }
        break;
      }
      case WIRE_START_GROUP:
        //puts("Start group (deprecated)");
        break;
      case WIRE_END_GROUP:
        //puts("End group (deprecated)");
        break;
      case WIRE_32BIT:
        fread(&decode, 1, 4, f);
        break;
      default:
        printf("Unknown wire type %d\n", wire);
        exit = true;
    }
  }
  
  // down here since fields 2 and 3 must both be found
  if(message == MESSAGE_PRODUCT && instlen != 0 && strcmp(uid, sc_uid) == 0){
    fseek(f, instoffs, SEEK_SET);
    exit = getInstallPathCASC_Proto(f, instoffs, instlen, MESSAGE_INSTALLATION, path);
  }
  return exit;
}

u32 varint(FILE* f, u32* decodeH){
  u8 b;
  u8 s = 0;
  u64 out = 0;
  do{
    b = fgetc(f);
    out |= (b&0x7F) << s;
    s += 7;
  } while(b&0x80);
  *decodeH = (u32)(out >> 32);
  return (u32)(out);
}


