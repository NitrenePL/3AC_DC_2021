#include "PLL.h"

// -------------------------------------------------------------------------
// 变量定义
// -------------------------------------------------------------------------
#pragma SET_DATA_SECTION("logVariables")
PIDStructure pid_PLL;
float theta = 0.0f;
float w = 100.0f * PI; // 默认 50Hz 基频
uint16_t PLL_Locked = 0;
uint32_t lockHold = 0;

PIDStructure pid_PLL_myself;
float theta_myself = 0.0f;
float w_myself = 100.0f * PI;
uint16_t PLL_Locked_myself = 0;
uint32_t lockHold_myself = 0;

// IIR 滤波器实例与系数
IIRStructure iirInputSec1Struct, iirInputSec2Struct, iirPLLStruct;
IIRStructure iirInputSec1Struct_myself, iirInputSec2Struct_myself, iirPLLStruct_myself;

// 系数保持不变 (Matlab 计算出的二阶 Section 系数)
const float iirInputSec1Num[3] = {0.005808126f, 0.011616252f, 0.005808126f};
const float iirInputSec1Den[2] = {-1.863800492f, 0.887032999f};
const float iirInputSec2Num[3] = {0.005378494f, 0.010756988f, 0.005378494f};
const float iirInputSec2Den[2] = {-1.725933395f, 0.747447371f};
const float iirPLLNum[3] = {0.000000616f, 0.000001232f, 0.000000616f};
const float iirPLLDen[2] = {-1.997778559f, 0.997781024f};
#pragma SET_DATA_SECTION()

// -------------------------------------------------------------------------
// PLL 初始化
// -------------------------------------------------------------------------
void PLL_Init(void)
{
    // 初始化 PID: Kp=8, Ki=40, Ts=1/20000, OutMin, OutMax, IntMax
    PID_Init(&pid_PLL, 8.0f, 40.0f, 1.0f / PLL_SAMPLING_FREQ, -10.0f * PI, 10.0f * PI, 10.0f * PI);
    pid_PLL.ref = 0.0f;

    // 初始化 IIR 滤波器
    IIR2nd_Init(&iirPLLStruct, iirPLLNum, iirPLLDen);
    IIR2nd_Init(&iirInputSec1Struct, iirInputSec1Num, iirInputSec1Den);
    IIR2nd_Init(&iirInputSec2Struct, iirInputSec2Num, iirInputSec2Den);

}

/**
 * @brief 20kHz(默认)软件锁相环计算。
 * @param input 输入电压瞬时值。
 * @note 输出到全局变量theta、w、PLL_Locked；约650 CPU clks @100MHz。
 */
RAMFUNC void PLL_Calc(float input)
{
    static float phaseError = 0, iirInput, IIRtheta = 0;
    static int16_t phaseErrorNoFiltered = 0;
    static uint16_t inSgn, inSgnBkp = 0, outSgn, outSgnBkp = 0;
    static uint32_t inPosEdgeHold = 0;

    // 1. 输入信号两级低通滤波
    float temp = IIR2nd_Calc(&iirInputSec1Struct, input);
    iirInput = IIR2nd_Calc(&iirInputSec2Struct, temp);

    // 2. 软件鉴相 (零交点检测逻辑)
    if (iirInput > 0.0f)
        inSgn = 1;
    else
        inSgn = 0;

    if (inSgnBkp == 0 && inSgn == 1 && inPosEdgeHold == 0)
    {
        phaseErrorNoFiltered += 1;
        inPosEdgeHold = (uint32_t)(0.7f * PLL_SAMPLING_FREQ / (w / (2.0f * PI)));
    }
    if (inPosEdgeHold != 0)
        inPosEdgeHold--;
    inSgnBkp = inSgn;

    if ((IIRtheta < PI / 2.0f) || (IIRtheta > PI * 1.5f))
        outSgn = 1;
    else
        outSgn = 0;

    if (outSgnBkp == 0 && outSgn == 1)
        phaseErrorNoFiltered -= 1;
    outSgnBkp = outSgn;

    // 限幅
    if (phaseErrorNoFiltered < -5)
        phaseErrorNoFiltered = -5;
    else if (phaseErrorNoFiltered > 5)
        phaseErrorNoFiltered = 5;

    // 3. 环路滤波与角频率更新
    phaseError = 2.0f * PI * IIR2nd_Calc(&iirPLLStruct, (float)phaseErrorNoFiltered);

    // PID 输出调整基频 (100*PI = 50Hz)
    w = PID_Calc(&pid_PLL, -1.0f * phaseError) + 100.0f * PI;

    // 4. 相位积分
    IIRtheta += w / PLL_SAMPLING_FREQ;
    if (IIRtheta > 2.0f * PI)
        IIRtheta -= 2.0f * PI;

    // 相位补偿 (由于计算和滤波延迟)
    theta = IIRtheta + 8.357e-4f * w;
    if (theta > 2.0f * PI)
        theta -= 2.0f * PI;

    // 5. 锁相判断逻辑
    if (phaseError < 0.1f && phaseError > -0.1f)
    {
        if (lockHold < 20000)
            lockHold++;
    }
    else if (lockHold > 0)
        lockHold--;

    if (!PLL_Locked && lockHold > 19800)
        PLL_Locked = 1;
    else if (PLL_Locked && lockHold < 15000)
    {
        PLL_Locked = 0;
        lockHold = 0;
    }

}

// -------------------------------------------------------------------------
// Myself 版本的初始化与计算 (逻辑一致，仅变量不同)
// -------------------------------------------------------------------------
void PLL_Init_myself(void)
{
    PID_Init(&pid_PLL_myself, 8.0f, 40.0f, 1.0f / PLL_SAMPLING_FREQ, -10.0f * PI, 10.0f * PI, 10.0f * PI);
    pid_PLL_myself.ref = 0.0f;
    IIR2nd_Init(&iirPLLStruct_myself, iirPLLNum, iirPLLDen);
    IIR2nd_Init(&iirInputSec1Struct_myself, iirInputSec1Num, iirInputSec1Den);
    IIR2nd_Init(&iirInputSec2Struct_myself, iirInputSec2Num, iirInputSec2Den);
}

/**
 * @brief 20kHz软件锁相环备用计算通道。
 * @param input 输入电压瞬时值。
 * @note 输出到全局变量theta_myself、w_myself、PLL_Locked_myself。
 */
RAMFUNC void PLL_Calc_myself(float input)
{
    static float phaseError = 0, iirInput, IIRtheta = 0;
    static int16_t phaseErrorNoFiltered = 0;
    static uint16_t inSgn, inSgnBkp = 0, outSgn, outSgnBkp = 0;
    static uint32_t inPosEdgeHold = 0;

    float temp = IIR2nd_Calc(&iirInputSec1Struct_myself, input);
    iirInput = IIR2nd_Calc(&iirInputSec2Struct_myself, temp);

    if (iirInput > 0.0f)
        inSgn = 1;
    else
        inSgn = 0;

    if (inSgnBkp == 0 && inSgn == 1 && inPosEdgeHold == 0)
    {
        phaseErrorNoFiltered += 1;
        inPosEdgeHold = (uint32_t)(0.7f * PLL_SAMPLING_FREQ / (w_myself / (2.0f * PI)));
    }
    if (inPosEdgeHold != 0)
        inPosEdgeHold--;
    inSgnBkp = inSgn;

    if ((IIRtheta < PI / 2.0f) || (IIRtheta > PI * 1.5f))
        outSgn = 1;
    else
        outSgn = 0;

    if (outSgnBkp == 0 && outSgn == 1)
        phaseErrorNoFiltered -= 1;
    outSgnBkp = outSgn;

    if (phaseErrorNoFiltered < -5)
        phaseErrorNoFiltered = -5;
    else if (phaseErrorNoFiltered > 5)
        phaseErrorNoFiltered = 5;

    phaseError = 2.0f * PI * IIR2nd_Calc(&iirPLLStruct_myself, (float)phaseErrorNoFiltered);
    w_myself = PID_Calc(&pid_PLL_myself, -1.0f * phaseError) + 100.0f * PI;

    IIRtheta += w_myself / PLL_SAMPLING_FREQ;
    if (IIRtheta > 2.0f * PI)
        IIRtheta -= 2.0f * PI;

    theta_myself = IIRtheta + 8.357e-4f * w_myself;
    if (theta_myself > 2.0f * PI)
        theta_myself -= 2.0f * PI;

    if (phaseError < 0.1f && phaseError > -0.1f)
    {
        if (lockHold_myself < 20000)
            lockHold_myself++;
    }
    else if (lockHold_myself > 0)
        lockHold_myself--;

    if (!PLL_Locked_myself && lockHold_myself > 19800)
        PLL_Locked_myself = 1;
    else if (PLL_Locked_myself && lockHold_myself < 15000)
    {
        PLL_Locked_myself = 0;
        lockHold_myself = 0;
    }

}
