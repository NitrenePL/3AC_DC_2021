#ifndef QPR_H_
#define QPR_H_

#include "DCLF32.h"
#include <math.h>

/*
 * Computes PR/QPR controller coefficients for TI DCL_DF22.
 *
 * Continuous controller:
 *
 *                  2 * Kr * wc * s
 *     Gc(s) = Kp + ---------------------
 *                  s^2 + 2 * wc * s + wo^2
 *
 * Discretization uses Tustin transform with resonance-frequency prewarping:
 *
 *     wo_adjusted = 2 * fs * tan(wo / (2 * fs))
 *
 * Parameters:
 *     v   - target DCL_DF22 controller
 *     kp  - proportional gain
 *     kr  - resonant gain
 *     wo  - resonant angular frequency, rad/s
 *     fs  - sampling frequency, Hz
 *     wrc - resonant bandwidth angular frequency, rad/s
 */
static inline void computeDF22_PRcontrollerCoeff(DCL_DF22 *v,
                                                 float32_t kp,
                                                 float32_t kr,
                                                 float32_t wo,
                                                 float32_t fs,
                                                 float32_t wrc)
{
    float32_t temp1;
    float32_t temp2;
    float32_t wo_adjusted;

    wo_adjusted = 2.0f * fs * tanf(wo / (2.0f * fs));
    temp1 = 4.0f * fs * fs + wo_adjusted * wo_adjusted + 4.0f * fs * wrc;
    temp2 = 4.0f * kr * wrc * fs / temp1;

    v->b0 = temp2;
    v->b1 = 0.0f;
    v->b2 = -temp2;
    v->a1 = (-8.0f * fs * fs + 2.0f * wo_adjusted * wo_adjusted) / temp1;
    v->a2 = (temp1 - 8.0f * fs * wrc) / temp1;

    if (kp != 0.0f)
    {
        v->b0 += kp;
        v->b1 += kp * v->a1;
        v->b2 += kp * v->a2;
    }

    DCL_resetDF22(v);
}

#endif /* QPR_H_ */
