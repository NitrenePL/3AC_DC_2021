#ifndef PID_H_
#define PID_H_

#include <stdint.h>
#include "global.h"

typedef struct
{
    float ref; // 设定值 (Reference)
    float fdb; // 反馈值 (Feedback)
    float kp;  // 比例系数
    float ki;  // 积分系数
    float Ts;  // 采样周期 (单位：秒)

    float pOut; // 比例输出
    float iOut; // 积分累加值
    float out;  // PID最终输出

    float outMax; // 输出最大限制 (例如占空比 0.95)
    float outMin; // 输出最小限制 (例如占空比 0.05)
    float intMax; // 积分限幅 (抗饱和)
} PIDStructure;

/**
 * PID 初始化
 */
static inline void PID_Init(PIDStructure *sptr, const float kp, const float ki, const float Ts, const float outMin,
                            const float outMax, const float intMax)
{
    sptr->kp = kp;
    sptr->ki = ki;
    sptr->Ts = Ts;
    sptr->outMax = outMax;
    sptr->outMin = outMin;
    sptr->intMax = intMax;

    sptr->iOut = 0.0f;
    sptr->pOut = 0.0f;
    sptr->out = 0.0f;
    sptr->ref = 0.0f;
    sptr->fdb = 0.0f;
}

/**
 * PID 计算
 * 对于 Buck 变换器：fdb 是采集的输出电压，ref 是目标电压，返回的是占空比
 */
/**
 * @brief PI/PID控制器计算。
 * @param sptr PID控制器状态与参数。
 * @param fdb 当前反馈值。
 * @return 限幅后的控制器输出。
 */
static inline RAMFUNC float PID_Calc(PIDStructure *sptr, const float fdb)
{
    float error;
    sptr->fdb = fdb;
    error = sptr->ref - fdb;

    // 1. 计算积分项 (后向矩形积分)
    sptr->iOut += sptr->ki * error * sptr->Ts;

    // 2. 积分限幅 (Anti-windup 抗饱和)
    if (sptr->iOut > sptr->intMax)
        sptr->iOut = sptr->intMax;
    else if (sptr->iOut < -sptr->intMax)
        sptr->iOut = -sptr->intMax;

    // 3. 计算比例项
    sptr->pOut = sptr->kp * error;

    // 4. 总输出计算
    sptr->out = sptr->pOut + sptr->iOut;

    // 5. 输出限幅 (例如占空比不能超过 1.0 且不能小于 0)
    if (sptr->out > sptr->outMax)
        sptr->out = sptr->outMax;
    if (sptr->out < sptr->outMin)
        sptr->out = sptr->outMin;

    return (sptr->out);
}

/**
 * 清除积分项 (通常在关机或发生保护时调用)
 */
static inline void PID_Clear(PIDStructure *sptr)
{
    sptr->iOut = 0.0f;
    sptr->out = 0.0f;
}

#endif /* PID_H_ */
