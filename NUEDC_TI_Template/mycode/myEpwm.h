#ifndef MYEPWM_H
#define MYEPWM_H

#include "device.h"
#include "driverlib.h"
#include "global.h"
#include "math.h"

// 宏定义：方便修改映射关系
#define EPWM_A_BASE EPWM1_BASE
#define EPWM_B_BASE EPWM2_BASE
#define EPWM_C_BASE EPWM3_BASE
#define EPWM_D_BASE EPWM4_BASE
#define EPWM_E_BASE EPWM5_BASE
#define EPWM_F_BASE EPWM6_BASE

// 函数声明
void EPWM_Start(void);
void EPWM_Start_DEF(void);
void EPWM_Stop_DEF(void);
void EPWM_Stop(void);


RAMFUNC void CB_SVPWM_3Ph(float Ua_pu, float Ub_pu, float Uc_pu);
RAMFUNC void CB_SVPWM_3Ph_DEF(float Ua_pu, float Ub_pu, float Uc_pu, float ZERO);
void SPWM(float D);

#endif
