#ifndef PLL_H_
#define PLL_H_

#include "global.h"
#include "IIR.h"
#include "pid.h"

#ifndef PI
#define PI 3.14159265358979f
#endif

#define PLL_SAMPLING_FREQ 20000.0f

extern float theta, w;
extern uint16_t PLL_Locked;

extern float theta_myself, w_myself;
extern uint16_t PLL_Locked_myself;

void PLL_Init(void);
RAMFUNC void PLL_Calc(float input);

void PLL_Init_myself(void);
RAMFUNC void PLL_Calc_myself(float input);

#endif /* PLL_H_ */
