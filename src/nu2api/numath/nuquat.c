#include "nuquat.h"

#include "nufloat.h"
#include "nutrig.h"

/* Quaternions. NuQuatToMtx/Normalise are based on opensagadev/saga
 * src/nu2api/numath/nuquat.cpp (GPL-3.0). NuMtxToQuat (the classic Shoemake conversion)
 * and NuQuatSlerp (older than saga's) are written from the PC code, with the PS2
 * prototype's locals. Names from the
 * PS2 prototype; in PC order (the PC exe keeps 4 of the 15 nuquat functions). */

/* Element (r, c) of a matrix as a 4x4 array. */
#define MTX_RC(m, r, c) (((float *)(m))[(r) * 4 + (c)])

// FUNCTION: BIONICLE 0x56fd60
void NuMtxToQuat(NUMTX *m, NUQUAT *q) {
    int nxt[3] = {1, 2, 0};
    float tr;
    float s;
    float qx[4];
    int i;
    int j;
    int k;

    tr = m->_00 + m->_11 + m->_22;
    if (tr > 0.0f) {
        s = NuFsqrt(tr + 1.0f);
        q->w = s * 0.5f;
        s = 0.5f / s;
        q->x = (m->_12 - m->_21) * s;
        q->y = (m->_20 - m->_02) * s;
        q->z = (m->_01 - m->_10) * s;
    } else {
        i = 0;
        if (m->_11 > m->_00) {
            i = 1;
        }
        if (m->_22 > MTX_RC(m, i, i)) {
            i = 2;
        }
        j = nxt[i];
        k = nxt[j];
        s = NuFsqrt(MTX_RC(m, i, i) - (MTX_RC(m, j, j) + MTX_RC(m, k, k)) + 1.0f);
        qx[i] = s * 0.5f;
        if (s != 0.0f) {
            s = 0.5f / s;
        }
        qx[3] = (MTX_RC(m, j, k) - MTX_RC(m, k, j)) * s;
        qx[j] = (MTX_RC(m, i, j) + MTX_RC(m, j, i)) * s;
        qx[k] = (MTX_RC(m, i, k) + MTX_RC(m, k, i)) * s;
        q->x = qx[0];
        q->y = qx[1];
        q->z = qx[2];
        q->w = qx[3];
    }
}

// FUNCTION: BIONICLE 0x56fec0
void NuQuatToMtx(NUQUAT *q, NUMTX *m) {
    float w_sq;
    float x_sq;
    float y_sq;
    float z_sq;
    float xy_times_2;
    float xz_times_2;
    float xw_times_2;
    float yz_times_2;
    float yw_times_2;
    float zw_times_2;

    w_sq = q->w * q->w;
    x_sq = q->x * q->x;
    y_sq = q->y * q->y;
    z_sq = q->z * q->z;

    xy_times_2 = (q->x * q->y) + (q->x * q->y);
    xz_times_2 = (q->x * q->z) + (q->x * q->z);
    xw_times_2 = (q->x * q->w) + (q->x * q->w);
    yz_times_2 = (q->y * q->z) + (q->y * q->z);
    yw_times_2 = (q->y * q->w) + (q->y * q->w);
    zw_times_2 = (q->z * q->w) + (q->z * q->w);

    m->_00 = w_sq + x_sq - y_sq - z_sq;
    m->_10 = xy_times_2 - zw_times_2;
    m->_20 = xz_times_2 + yw_times_2;
    m->_30 = 0.0f;

    m->_01 = xy_times_2 + zw_times_2;
    m->_11 = w_sq - x_sq + y_sq - z_sq;
    m->_21 = yz_times_2 - xw_times_2;
    m->_31 = 0.0f;

    m->_02 = xz_times_2 - yw_times_2;
    m->_12 = yz_times_2 + xw_times_2;
    m->_22 = w_sq - x_sq - y_sq + z_sq;
    m->_32 = 0.0f;

    m->_03 = 0.0f;
    m->_13 = 0.0f;
    m->_23 = 0.0f;
    m->_33 = 1.0f;
}

// FUNCTION: BIONICLE 0x56ffb0
void NuQuatNormalise(NUQUAT *q, NUQUAT *q0) {
    float magnitude_squared = q0->w * q0->w + q0->x * q0->x + q0->y * q0->y + q0->z * q0->z;
    if (magnitude_squared > 0.0f) {
        float inverse_magnitude = NuFdiv(1.0f, NuFsqrt(magnitude_squared));
        q->w = q0->w * inverse_magnitude;
        q->x = q0->x * inverse_magnitude;
        q->y = q0->y * inverse_magnitude;
        q->z = q0->z * inverse_magnitude;
    } else {
        *q = *q0;
    }
}

// FUNCTION: BIONICLE 0x570060
void NuQuatSlerp(NUQUAT *q, NUQUAT *from, NUQUAT *to, float t) {
    float to1[4];
    float omega;
    float cosom;
    float sinom;
    float scale0;
    float scale1;

    cosom = from->x * to->x + from->y * to->y + from->z * to->z + from->w * to->w;
    if (cosom < 0.0f) {
        cosom = -cosom;
        to1[0] = -to->x;
        to1[1] = -to->y;
        to1[2] = -to->z;
        to1[3] = -to->w;
    } else {
        to1[0] = to->x;
        to1[1] = to->y;
        to1[2] = to->z;
        to1[3] = to->w;
    }
    if (1.0f - cosom > 0.0f) {
        omega = NuACos(cosom);
        sinom = NU_SIN_LUT((int)omega);
        scale0 = NuFdiv(NU_SIN_LUT((int)((1.0f - t) * omega)), sinom);
        scale1 = NuFdiv(NU_SIN_LUT((int)(t * omega)), sinom);
    } else {
        scale0 = 1.0f - t;
        scale1 = t;
    }
    q->x = scale0 * from->x + scale1 * to1[0];
    q->y = scale0 * from->y + scale1 * to1[1];
    q->z = scale0 * from->z + scale1 * to1[2];
    q->w = scale0 * from->w + scale1 * to1[3];
}
