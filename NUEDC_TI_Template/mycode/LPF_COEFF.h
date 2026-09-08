#ifndef LPF_COEFF_H_
#define LPF_COEFF_H_

#include "DCLF32.h"


/*
 * Computes RL plant coefficients for TI DCL_DF11.
 *
 * Continuous transfer function:
 *
 *                    1
 *     G(s) = ----------------
 *                L * s + R
 *
 *              b0 + b1 * z^(-1)
 *     G(z) = ------------------------
 *              1 + a1 * z^(-1)
 *
 * Coefficients:
 *
 *               Ts
 *     b0 = b1 = -----------
 *              2L + R*Ts
 *
 *              R*Ts - 2L
 *     a1 = ----------------
 *              R*Ts + 2L
 *
 * DCL_DF11 difference equation:
 *
 *     y[k] = b0*x[k] + b1*x[k-1] - a1*y[k-1]
 *
 * Parameters:
 *     v   - target DCL_DF11 object
 *     L   - inductance, H
 *     R   - resistance, ohm
 *     fs  - sampling frequency, Hz
 */
static inline void computeDF11_LPFCoeff(DCL_DF11 *v, float32_t L, float32_t R, float32_t fs)
{
    float32_t denominator;

    /*
     * Equivalent to:
     *
     * denominator = 2.0f * L + R / fs;
     * b0 = (1.0f / fs) / denominator;
     *
     * Multiplying numerator and denominator by fs gives the
     * numerically simpler expression below.
     */
    denominator = 2.0f * L * fs + R;

    v->b0 = 1.0f / denominator;
    v->b1 = v->b0;
    v->a1 = (R - 2.0f * L * fs) / denominator;

    DCL_resetDF11(v);
}

#endif /* LPF_COEFF_H_ */
