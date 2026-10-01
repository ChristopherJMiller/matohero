#include "nuvec4.h"

/* Four-component vectors. Names and locals from the PS2 prototype; only these three
 * are in the PC exe. */

// FUNCTION: BIONICLE 0x5714f0
void NuVec4Scale(NUVEC4 *v, NUVEC4 *v0, float k) {
    v->x = v0->x * k;
    v->y = v0->y * k;
    v->z = v0->z * k;
    v->w = v0->w * k;
}

// FUNCTION: BIONICLE 0x571520
void NuVec4MtxTransform(NUVEC4 *v, NUVEC *v0, NUMTX *m0) {
    float y = m0->_01 * v0->x + m0->_11 * v0->y + m0->_21 * v0->z + m0->_31;
    float z = m0->_02 * v0->x + m0->_12 * v0->y + m0->_22 * v0->z + m0->_32;
    float w = m0->_03 * v0->x + m0->_13 * v0->y + m0->_23 * v0->z + m0->_33;

    v->x = m0->_00 * v0->x + m0->_10 * v0->y + m0->_20 * v0->z + m0->_30;
    v->y = y;
    v->z = z;
    v->w = w;
}

// FUNCTION: BIONICLE 0x5715b0
void NuVec4MtxTransformH(NUVEC4 *v, NUVEC4 *v0, NUMTX *m0) {
    float y;
    float z;
    float w;

    y = m0->_01 * v0->x + m0->_11 * v0->y + (m0->_21 * v0->z + m0->_31 * v0->w);
    z = m0->_02 * v0->x + m0->_12 * v0->y + (m0->_22 * v0->z + m0->_32 * v0->w);
    w = m0->_03 * v0->x + m0->_13 * v0->y + (m0->_23 * v0->z + m0->_33 * v0->w);
    v->x = m0->_00 * v0->x + m0->_10 * v0->y + (m0->_20 * v0->z + m0->_30 * v0->w);
    v->y = y;
    v->z = z;
    v->w = w;
}
