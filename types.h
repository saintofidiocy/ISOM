#ifndef H_TYPES
#define H_TYPES
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef uint8_t  u8;
typedef  int8_t  s8;
typedef uint16_t u16;
typedef  int16_t s16;
typedef uint32_t u32;
typedef  int32_t s32;
typedef uint64_t u64;
typedef  int64_t s64;
typedef enum{false, true} bool;

typedef struct {
  s16 left;
  s16 up;
  s16 right;
  s16 down;
} Rect16;

typedef union {
  struct {
    u8 r,g,b,a;
  };
  u32 raw;
} RGBA;

// color definition macros
#define COLOR(r,g,b,a) (((u32)(a)<<24)|((u32)(b)<<16)|((u32)((g)<<8))|((u32)(r)))
#define COLOR_R(c)     ((u32)(c)&0xFFUL)
#define COLOR_G(c)     (((u32)(c)&0xFF00UL)>>8)
#define COLOR_B(c)     (((u32)(c)&0xFF0000UL)>>16)
#define COLOR_A(c)     (((u32)(c)&0xFF000000UL)>>24)
#define BLEND_A(c1,c2) (COLOR_A(c1)*(255-COLOR_A(c2))+COLOR_A(c2)*255)
#define BLEND(c1,c2)   COLOR( (COLOR_R(c1)*COLOR_A(c1)*(255-COLOR_A(c2))+COLOR_R(c2)*COLOR_A(c2)*255+127)/BLEND_A(c1,c2), \
                              (COLOR_G(c1)*COLOR_A(c1)*(255-COLOR_A(c2))+COLOR_G(c2)*COLOR_A(c2)*255+127)/BLEND_A(c1,c2), \
                              (COLOR_B(c1)*COLOR_A(c1)*(255-COLOR_A(c2))+COLOR_B(c2)*COLOR_A(c2)*255+127)/BLEND_A(c1,c2), \
                              (BLEND_A(c1,c2)+127)/255 )

#endif
