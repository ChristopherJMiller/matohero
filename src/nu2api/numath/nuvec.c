#include "nuvec.h"

#include "nufloat.h"
#include "numtx.h"
#include "nutrig.h"
#include "nutypes.h"

/* 3D vectors. Based on opensagadev/saga src/nu2api/numath/nuvec.cpp (GPL-3.0); names
 * and signatures from the PS2 prototype. In PC order; the PC exe has 24 of the PS2's
 * 46 nuvec functions (the rest were unused and dropped by the linker). */

// GLOBAL: BIONICLE 0x617530
NUVEC nuvec_one = {1.0f, 1.0f, 1.0f};

// GLOBAL: BIONICLE 0x61753c
NUVEC nuvec_minus_one = {-1.0f, -1.0f, -1.0f};

// FUNCTION: BIONICLE 0x570240
float NuVecMtxTransformH(NUVEC *v, NUVEC *v0, NUMTX *m0) {
    float w = NuFdiv(1.0f, m0->_03 * v0->x + m0->_13 * v0->y + m0->_23 * v0->z + m0->_33);
    float y = (m0->_01 * v0->x + m0->_11 * v0->y + m0->_21 * v0->z + m0->_31) * w;
    float z = (m0->_02 * v0->x + m0->_12 * v0->y + m0->_22 * v0->z + m0->_32) * w;

    v->x = (m0->_00 * v0->x + m0->_10 * v0->y + m0->_20 * v0->z + m0->_30) * w;
    v->y = y;
    v->z = z;
    return w;
}

// FUNCTION: BIONICLE 0x5702e0
void NuVecMtxRotate(NUVEC *v, NUVEC *v0, NUMTX *m0) {
    float y = m0->_01 * v0->x + m0->_11 * v0->y + m0->_21 * v0->z;
    float z = m0->_02 * v0->x + m0->_12 * v0->y + m0->_22 * v0->z;

    v->x = m0->_00 * v0->x + m0->_10 * v0->y + m0->_20 * v0->z;
    v->y = y;
    v->z = z;
}

// FUNCTION: BIONICLE 0x570340
void NuVecInvMtxTransform(NUVEC *v, NUVEC *v0, NUMTX *m0) {
    float x = v0->x - m0->_30;
    float y = v0->y - m0->_31;
    float z = v0->z - m0->_32;

    v->x = x * m0->_00 + y * m0->_01 + z * m0->_02;
    v->y = x * m0->_10 + y * m0->_11 + z * m0->_12;
    v->z = x * m0->_20 + y * m0->_21 + z * m0->_22;
}

// FUNCTION: BIONICLE 0x5703a0
void NuVecInvMtxRotate(NUVEC *v, NUVEC *v0, NUMTX *m0) {
    float y = m0->_10 * v0->x + m0->_11 * v0->y + v0->z * m0->_12;
    float z = m0->_20 * v0->x + m0->_21 * v0->y + v0->z * m0->_22;

    v->x = m0->_00 * v0->x + m0->_01 * v0->y + v0->z * m0->_02;
    v->y = y;
    v->z = z;
}

// FUNCTION: BIONICLE 0x570400
void NuVecRotateX(NUVEC *v, NUVEC *v0, NUANG a) {
    float c = NU_COS_LUT(a);
    float s = NU_SIN_LUT(a);
    float y = v0->y;

    v->x = v0->x;
    v->y = y * c - v0->z * s;
    v->z = y * s + v0->z * c;
}

// FUNCTION: BIONICLE 0x570460
void NuVecRotateY(NUVEC *v, NUVEC *v0, NUANG a) {
    float c = NU_COS_LUT(a);
    float s = NU_SIN_LUT(a);
    float x = v0->x;

    v->x = x * c + v0->z * s;
    v->y = v0->y;
    v->z = v0->z * c - x * s;
}

// FUNCTION: BIONICLE 0x5704c0
void NuVecRotateZ(NUVEC *v, NUVEC *v0, NUANG a) {
    float c = NU_COS_LUT(a);
    float s = NU_SIN_LUT(a);
    float x = v0->x;

    v->x = x * c - v0->y * s;
    v->y = x * s + v0->y * c;
    v->z = v0->z;
}

// FUNCTION: BIONICLE 0x570520
void NuVecAdd(NUVEC *v, NUVEC *v0, NUVEC *v1) {
    v->x = v0->x + v1->x;
    v->y = v0->y + v1->y;
    v->z = v0->z + v1->z;
}

// FUNCTION: BIONICLE 0x570550
__inline void NuVecSub(NUVEC *v, NUVEC *v0, NUVEC *v1) {
    v->x = v0->x - v1->x;
    v->y = v0->y - v1->y;
    v->z = v0->z - v1->z;
}

// FUNCTION: BIONICLE 0x570580
void NuVecScale(NUVEC *v, NUVEC *v0, float k) {
    v->x = v0->x * k;
    v->y = v0->y * k;
    v->z = v0->z * k;
}

// FUNCTION: BIONICLE 0x5705b0
void NuVecAddScale(NUVEC *v, NUVEC *v0, NUVEC *v1, float k) {
    v->x = v0->x + v1->x * k;
    v->y = v0->y + v1->y * k;
    v->z = v0->z + v1->z * k;
}

// FUNCTION: BIONICLE 0x5705f0
__inline void NuVecCross(NUVEC *v, NUVEC *v0, NUVEC *v1) {
    float y = v0->z * v1->x - v0->x * v1->z;
    float z = v0->x * v1->y - v0->y * v1->x;

    v->x = v0->y * v1->z - v0->z * v1->y;
    v->y = y;
    v->z = z;
}

// FUNCTION: BIONICLE 0x570630
float NuVecDot(NUVEC *v0, NUVEC *v1) {
    return v0->x * v1->x + v0->y * v1->y + v0->z * v1->z;
}

// FUNCTION: BIONICLE 0x570650
void NuVecMax(NUVEC *v0, NUVEC *v1, NUVEC *v2) {
    if (v1->x > v2->x) {
        v0->x = v1->x;
    } else {
        v0->x = v2->x;
    }
    if (v1->y > v2->y) {
        v0->y = v1->y;
    } else {
        v0->y = v2->y;
    }
    if (v1->z > v2->z) {
        v0->z = v1->z;
    } else {
        v0->z = v2->z;
    }
}

// FUNCTION: BIONICLE 0x5706b0
void NuVecMin(NUVEC *v0, NUVEC *v1, NUVEC *v2) {
    if (v1->x < v2->x) {
        v0->x = v1->x;
    } else {
        v0->x = v2->x;
    }
    if (v1->y < v2->y) {
        v0->y = v1->y;
    } else {
        v0->y = v2->y;
    }
    if (v1->z < v2->z) {
        v0->z = v1->z;
    } else {
        v0->z = v2->z;
    }
}

// FUNCTION: BIONICLE 0x570710
__inline float NuVecMagSqr(NUVEC *v0) {
    return v0->x * v0->x + v0->y * v0->y + v0->z * v0->z;
}

// FUNCTION: BIONICLE 0x570740
float NuVecNorm(NUVEC *v, NUVEC *v0) {
    float len = NuFsqrt(NuVecMagSqr(v0));
    float leni;

    if (len > 0.0f) {
        leni = 1.0f / len;
    } else {
        leni = 0.0f;
    }

    v->x = leni * v0->x;
    v->y = leni * v0->y;
    v->z = leni * v0->z;
    return len;
}

// FUNCTION: BIONICLE 0x5707b0
float NuVecDistSqr(NUVEC *v0, NUVEC *v1, NUVEC *d) {
    NUVEC dist;

    if (d != NULL) {
        NuVecSub(d, v0, v1);
        return NuVecMagSqr(d);
    }
    NuVecSub(&dist, v0, v1);
    return NuVecMagSqr(&dist);
}

// FUNCTION: BIONICLE 0x570830
float NuVecXZDistSqr(NUVEC *v0, NUVEC *v1, NUVEC *d) {
    NUVEC dist;

    if (d != NULL) {
        d->x = v0->x - v1->x;
        d->y = 0.0f;
        d->z = v0->z - v1->z;
        return d->x * d->x + d->z * d->z;
    }
    dist.x = v0->x - v1->x;
    dist.z = v0->z - v1->z;
    return dist.x * dist.x + dist.z * dist.z;
}

// FUNCTION: BIONICLE 0x570890
void NuVecLerp(NUVEC *vt, NUVEC *v1, NUVEC *v0, float t) {
    float it = 1.0f - t;

    vt->x = t * v1->x + it * v0->x;
    vt->y = it * v0->y + t * v1->y;
    vt->z = it * v0->z + t * v1->z;
}

// FUNCTION: BIONICLE 0x5708e0
__inline float NuVecMag(NUVEC *v0) {
    return NuFsqrt(NuVecMagSqr(v0));
}

// FUNCTION: BIONICLE 0x570920
float NuVecDist(NUVEC *v0, NUVEC *v1, NUVEC *d) {
    NUVEC dist;

    if (d != NULL) {
        NuVecSub(d, v0, v1);
        return NuVecMag(d);
    } else {
        NuVecSub(&dist, v0, v1);
        return NuVecMag(&dist);
    }
}

// FUNCTION: BIONICLE 0x5709b0
float NuVecXZDist(NUVEC *v0, NUVEC *v1, NUVEC *d) {
    NUVEC dist;

    if (d != NULL) {
        d->x = v0->x - v1->x;
        d->y = 0.0f;
        d->z = v0->z - v1->z;
        return NuVecMag(d);
    } else {
        dist.x = v0->x - v1->x;
        dist.y = 0.0f;
        dist.z = v0->z - v1->z;
        return NuVecMag(&dist);
    }
}

// FUNCTION: BIONICLE 0x570a30
int NuLineLineIntersect(NUVEC *p0, NUVEC *v0, NUVEC *p1, NUVEC *v1, float *s, float *t) {
    NUVEC cross;
    float len;
    float inv_len;
    NUMTX m;

    NuVecCross(&cross, v0, v1);
    len = NuVecMag(&cross);
    if (len == 0.0f) {
        inv_len = 0.0f;
    } else {
        inv_len = 1.0f / len;
    }
    inv_len *= inv_len;

    m._00 = p1->x - p0->x;
    m._01 = p1->y - p0->y;
    m._02 = p1->z - p0->z;
    m._10 = v1->x;
    m._11 = v1->y;
    m._12 = v1->z;
    m._20 = cross.x;
    m._21 = cross.y;
    m._22 = cross.z;
    *s = NuMtxDet3(&m) * inv_len;

    m._10 = v0->x;
    m._11 = v0->y;
    m._12 = v0->z;
    *t = NuMtxDet3(&m) * inv_len;

    if (inv_len != 0.0f) {
        return 1;
    }
    return 0;
}
