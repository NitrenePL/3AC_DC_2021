#include "park_trans.h"

/**
 * Park 变换 (Alpha-Beta 到 d-q)
 * 采用用户原有的旋转逻辑
 */
void ab_to_dq(float32_t Alpha, float32_t Beta, float32_t wt, float32_t *d, float32_t *q)
{
    float32_t s, c;
    // TI C2000 在开启 TMU 后，sinf/__cos 是极速的硬件指令
    s = __sin(wt);
    c = __cos(wt);

    *d = Beta * s + Alpha * c;
    *q = Beta * c - Alpha * s;
}

/**
 * 逆 Park 变换 (d-q 到 Alpha-Beta)
 */
void dq_to_AlphaBeta(float32_t *Alpha, float32_t *Beta, float32_t wt, float32_t d, float32_t q)
{
    float32_t s, c;
    s = __sin(wt);
    c = __cos(wt);

    *Alpha = d * c - q * s;
    *Beta = d * s + q * c;
}

/**
 * abc to Alpha-Beta-0 transform (amplitude invariant)
 */
void abc_to_abz(float32_t a, float32_t b, float32_t c, float32_t *Alpha, float32_t *Beta, float32_t *z)
{
    *Alpha = (2.0f / 3.0f) * (a - 0.5f * b - 0.5f * c);
    *Beta = 0.5773502691896258f * (b - c);
    *z = (1.0f / 3.0f) * (a + b + c);
}

/**
 * Inverse Alpha-Beta-0 to abc transform
 */
void abz_to_abc(float32_t *a, float32_t *b, float32_t *c, float32_t Alpha, float32_t Beta, float32_t z)
{
    *a = Alpha + z;
    *b = -0.5f * Alpha + 0.8660254037844386f * Beta + z;
    *c = -0.5f * Alpha - 0.8660254037844386f * Beta + z;
}

/**
 * abc to dq0 transform (amplitude invariant)
 */
void abc_to_dqz(float32_t a, float32_t b, float32_t c, float32_t wt, float32_t *d, float32_t *q, float32_t *z)
{
    float32_t theta[3];
    float32_t c0, c1, c2, s0, s1, s2;

    theta[0] = wt;
    theta[1] = wt + (4.0f * PI / 3.0f);
    theta[2] = wt + (2.0f * PI / 3.0f);

    // 角度归一化到 [0, 2PI]
    int i;
    for (i = 1; i < 3; i++)
    {
        if (theta[i] >= 2.0f * PI)
            theta[i] -= 2.0f * PI;
    }

    c0 = __cos(theta[0]);
    c1 = __cos(theta[1]);
    c2 = __cos(theta[2]);
    s0 = __sin(theta[0]);
    s1 = __sin(theta[1]);
    s2 = __sin(theta[2]);

    *d = (2.0f / 3.0f) * (a * c0 + b * c1 + c * c2);
    *q = (-2.0f / 3.0f) * (a * s0 + b * s1 + c * s2);
    *z = (1.0f / 3.0f) * (a + b + c);
}

/**
 * 逆 dq0 到 abc 变换
 */
void dqz_to_abc(float32_t *a, float32_t *b, float32_t *c, float32_t wt, float32_t d, float32_t q, float32_t z)
{
    float32_t theta[3];
    theta[0] = wt;
    theta[1] = wt + (4.0f * PI / 3.0f);
    theta[2] = wt + (2.0f * PI / 3.0f);

    int i;
    for (i = 1; i < 3; i++)
    {
        if (theta[i] >= 2.0f * PI)
            theta[i] -= 2.0f * PI;
    }

    *a = d * __cos(theta[0]) - q * __sin(theta[0]) + z;
    *b = d * __cos(theta[1]) - q * __sin(theta[1]) + z;
    *c = d * __cos(theta[2]) - q * __sin(theta[2]) + z;
}
