/*
 * $Creator: Nikita Smith
 * $Date: Oct 25 2019
 */

#ifndef XLIB_CORE_MATH_INCLUDE_H
#define XLIB_CORE_MATH_INCLUDE_H

typedef struct x_v2 {
    F32 x;
    F32 y;
} x_v2;

typedef struct x_v3 {
    F32 x;
    F32 y;
    F32 z;
} x_v3;

typedef struct x_mat4 {
    F32 row0[4];
    F32 row1[4];
    F32 row2[4];
    F32 row3[4];
} x_mat4;

x_api x_v2
x_V2(F32 x, F32 y);

x_api x_v3
x_V3(F32 x, F32 y, F32 z);

x_api x_mat4
x_MAT4(F32 values[]);

x_api x_mat4
x_project_ortho(F32 left, F32 right, F32 top, F32 bottom, F32 near, F32 far);

#endif /* XLIB_CORE_MATH_INCLUDE_H */

#ifdef XLIB_CORE_MATH_IMPLEMENTATION

x_api x_v2
x_V2(F32 x, F32 y)
{
    x_v2 result;
    result.x = x;
    result.y = y;
    return result;
}

x_api x_v3
x_V3(F32 x, F32 y, F32 z)
{
    x_v3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

x_api x_mat4
x_MAT4(F32 values[])
{
    x_mat4 result;
    result.row0[0] = values[0];
    result.row0[1] = values[1];
    result.row0[2] = values[2];
    result.row0[3] = values[3];

    result.row1[0] = values[4];
    result.row1[1] = values[5];
    result.row1[2] = values[6];
    result.row1[3] = values[7];

    result.row2[0] = values[8];
    result.row2[1] = values[9];
    result.row2[2] = values[10];
    result.row2[3] = values[11];

    result.row3[0] = values[12];
    result.row3[1] = values[13];
    result.row3[2] = values[14];
    result.row3[3] = values[15];

    return result;
}

x_api x_mat4
x_project_ortho(F32 l, F32 r, F32 t, F32 b, F32 n, F32 f)
{
    x_mat4 result;

    result.row0[0] = 2.0f/(r - l);
    result.row0[1] = 0.0f;
    result.row0[2] = 0.0f;
    result.row0[3] = 0.0f;

    result.row1[0] = 0.0f;
    result.row1[1] = 2.0f/(t - b);
    result.row1[2] = 0.0f;
    result.row1[3] = 0.0f;

    result.row2[0] = 0.0f;
    result.row2[1] = 0.0f;
    result.row2[2] = -2.0f/(f - n);
    result.row2[3] = 0.0f;

    result.row3[0] = -(r + l)/(r - l);
    result.row3[1] = -(t + b)/(t - b);
    result.row3[2] = -(f + n)/(f - n);
    result.row3[3] = 1.0f;

    return result;
}

#endif /* XLIB_CORE_MATH_IMPLEMENTATION */
