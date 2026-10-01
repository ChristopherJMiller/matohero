#ifndef NUTYPES_H
#define NUTYPES_H

typedef signed char i8;
typedef unsigned char u8;
typedef signed short i16;
typedef unsigned short u16;
typedef signed int i32;
typedef unsigned int u32;

/* Pointer into a buffer being carved up; the original union has many more members. */
typedef union variptr_u {
    void *void_ptr;
    char *char_ptr;
    u8 *u8_ptr;
    u32 addr;
} VARIPTR;

#ifndef NULL
#define NULL 0
#endif

#endif
