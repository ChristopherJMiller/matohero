#ifndef NUMATH_H
#define NUMATH_H

/* Core math types (original names and layouts from the PS2 prototype). */

typedef int NUANG; /* 65536 units per turn */

typedef struct nuvec_s {
    float x;
    float y;
    float z;
} NUVEC;

typedef struct numtx_s {
    float _00, _01, _02, _03;
    float _10, _11, _12, _13;
    float _20, _21, _22, _23;
    float _30, _31, _32, _33;
} NUMTX;

typedef struct nuplane_s {
    float a;
    float b;
    float c;
    float d;
} NUPLANE;

#endif
