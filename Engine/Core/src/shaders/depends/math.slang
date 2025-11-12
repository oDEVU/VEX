public float3x3 inverse33(float3x3 m)
{
    float a00 = m[0][0], a01 = m[0][1], a02 = m[0][2];
    float a10 = m[1][0], a11 = m[1][1], a12 = m[1][2];
    float a20 = m[2][0], a21 = m[2][1], a22 = m[2][2];

    float det = a00*(a11*a22 - a12*a21)
              - a01*(a10*a22 - a12*a20)
              + a02*(a10*a21 - a11*a20);

    float invDet = 1.0 / det;

    return float3x3(
        (a11*a22 - a12*a21) * invDet,
        (a02*a21 - a01*a22) * invDet,
        (a01*a12 - a02*a11) * invDet,

        (a12*a20 - a10*a22) * invDet,
        (a00*a22 - a02*a20) * invDet,
        (a02*a10 - a00*a12) * invDet,

        (a10*a21 - a11*a20) * invDet,
        (a01*a20 - a00*a21) * invDet,
        (a00*a11 - a01*a10) * invDet
    );
}
