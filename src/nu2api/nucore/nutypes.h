#ifndef NUTYPES_H
#define NUTYPES_H

typedef signed char i8;
typedef unsigned char u8;
typedef signed short i16;
typedef unsigned short u16;
typedef signed int i32;
typedef unsigned int u32;

/* Pointer into a buffer being carved up (PS2 names; the PS2 union also has members for
 * engine types and PS2 hardware registers). */
typedef union variptr_u {
    void *voidptr;
    unsigned char *u8;
    char *s8;
    unsigned short *u16;
    short *s16;
    unsigned int *u32;
    int *s32;
    float *f32;
    unsigned int intaddr;
} VARIPTR;

#ifndef NULL
#define NULL 0
#endif

#endif
