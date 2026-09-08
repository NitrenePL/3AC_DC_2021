#ifndef IIR_H_
#define IIR_H_

#include <stdint.h>
#include "global.h"

typedef struct
{
    float inBuf[2];
    float outBuf[2];
    float Num[3];
    float Den[2];
} IIRStructure;

static inline void IIR2nd_Clear(IIRStructure *S)
{
    S->inBuf[0] = 0.0f;
    S->inBuf[1] = 0.0f;
    S->outBuf[0] = 0.0f;
    S->outBuf[1] = 0.0f;
}

static inline void IIR2nd_Init(IIRStructure *S, const float *pNum, const float *pDen)
{
    S->Num[0] = pNum[0];
    S->Num[1] = pNum[1];
    S->Num[2] = pNum[2];
    S->Den[0] = pDen[0];
    S->Den[1] = pDen[1];
    IIR2nd_Clear(S);
}

/**
 * @brief 二阶IIR滤波计算。
 * @param S IIR滤波器状态与系数。
 * @param input 当前输入采样值。
 * @return 当前滤波输出。
 */
static inline RAMFUNC float IIR2nd_Calc(IIRStructure *S, float input)
{
    float out;

    out = (S->Num[0] * input) + (S->Num[1] * S->inBuf[0]) + (S->Num[2] * S->inBuf[1]) -
          (S->Den[0] * S->outBuf[0]) - (S->Den[1] * S->outBuf[1]);

    S->inBuf[1] = S->inBuf[0];
    S->inBuf[0] = input;
    S->outBuf[1] = S->outBuf[0];
    S->outBuf[0] = out;

    return out;
}

#endif /* IIR_H_ */
