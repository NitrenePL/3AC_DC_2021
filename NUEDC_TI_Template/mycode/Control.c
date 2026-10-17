#include "Control.h"
#include "DCLF32.h"
#include "KeyBoard.h"
#include "LPF_COEFF.h"
#include "OLED_Display.h"
#include "PLL.h"
#include "QPR.h"
#include "RMS.h"
#include "global.h"
#include "myEpwm.h"
#include "park_trans.h"

// OLED屏幕刷新率, 当前15Hz
#define OLED_REFRESH_RATE_HZ 15U
#define SLOW_TASK_RATE_HZ    1000U

#pragma SET_DATA_SECTION("controlVariables")
// 高速RAM变量放在这里, 例如电流环、滤波器等
float32_t theta_ref = 0.f; // 参考相位
float32_t M = 0.05f;
float32_t Ud, Uq;
float32_t Id, Iq;
float32_t Ualpha, Ubeta;
float32_t Ialpha, Ibeta;
float32_t I_SET;
float32_t Udc;
float32_t Ua_inst,Ub_inst,Uc_inst;
float32_t Ia_inst,Ib_inst,Ic_inst;
#pragma SET_DATA_SECTION()

#pragma SET_DATA_SECTION("logVariables")
// 低速RAM变量放在这里, 例如RMS数组、状态数据等

uint16_t PWM_PRD; // 假设默认周期值
uint8_t Open = 0;     // 默认关闭
uint8_t MODE = 0;     // 默认模式0
uint8_t RECT_ENABLE = 0;//开启整流
uint8_t LAST_MODE = 0;

float32_t U_data[400];
float32_t I_data[400];

RMS_Obj Uan_RMS;
float32_t Uan_rms;
#pragma SET_DATA_SECTION()

// Timer0 中断服务函数 慢速任务 1kHz
__interrupt void INT_myCPUTIMER0_ISR(void)
{
    static uint16_t ledTaskCnt = 0;
    static uint16_t oledRefreshAcc = 0;

    oledRefreshAcc = (uint16_t)(oledRefreshAcc + OLED_REFRESH_RATE_HZ);
    if (oledRefreshAcc >= SLOW_TASK_RATE_HZ)
    {
        oledRefreshAcc = (uint16_t)(oledRefreshAcc - SLOW_TASK_RATE_HZ);
        OLED_Display_RequestRefresh();
    }

    if (++ledTaskCnt >= 400U)
    {
        ledTaskCnt = 0U;
        LED_TOGGLE1();
    }

    CPUTimer_clearOverflowFlag(CPUTIMER0_BASE);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

// ADC_INT 中断服务函数 主控中断 20kHz
RAMFUNC __interrupt void ADC_SamplingISR(void)
{
    static int i;
    /*
    这里准备拟合三相线电流测量，三相需要分别拟合，思路与拟合电压类似，同样借助dq变换。
    首先如果需要拟合a相，那么我们就要锁A相电流的相位，然后让A相电流用这个相角去变换。
    变换出的Id对应电流的峰值，Iq应该为0.
    b相与c相同理，拟合b相就锁b相电压
    这里选择节省ADC引脚，3个电流测量不采用差分的形式，分别只取测量模块的OUT_+
    A相电流接A1,B相接A2,C相接A3。

    由于是差分测量当单端测量使用，那么需要先确认测量偏置，不带载将Ia_inst三个变量打印到屏幕即为偏置值,将其校准为0左右即可。
    */
    float32_t temp = (float32_t)ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER0);
    Udc = 0.017101307f*temp - 0.192351315f;
    float32_t Uab_inst,Ubc_inst,Uca_inst;
    float32_t inst_H = (float32_t)ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER0);
    float32_t inst_L = (float32_t)ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER1);
    Uab_inst = 0.01619203f*(inst_H - inst_L) + 0.00994615f;
    inst_H = (float32_t)ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER2);
    inst_L = (float32_t)ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER3);
    Ubc_inst = 0.01621422f*(inst_H - inst_L) + 0.03109598f;
    Uca_inst = 0.f - Uab_inst - Ubc_inst;

    Ia_inst = (float32_t)ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER1) - 2055.f;
    Ib_inst = (float32_t)ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER2) - 2047.f;
    Ic_inst = (float32_t)ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER3) - 2046.f;

    Ua_inst = (2.f * Uab_inst + Ubc_inst) / 3.f;
    Ub_inst = (2.f * Ubc_inst + Uca_inst) / 3.f;
    Uc_inst = (2.f * Uca_inst + Uab_inst) / 3.f;

    PLL_Calc(Ua_inst);
    Ualpha = Ua_inst;
    U_data[i] = Ualpha;
    //获取beta轴
    if (i >= 100)
    {
        Ubeta = U_data[i - 100];
    }
    else
    {
        Ubeta = U_data[i + 300];
    }
    //变换
    ab_to_dq(Ualpha, Ubeta, theta_ref, &Ud, &Uq);//注意是theta_ref
    //注意是theta
    static float32_t filter,last_filter;
    filter = 0.01f*Ud + 0.99f*last_filter;
    last_filter = filter;
    Ud = filter;
    //进行滤波方便拟合测量，闭环时应去掉。
    if (MODE == 1 )
    {

        theta_ref += CONST_2PI_32 * 0.0025f;
        if (theta_ref >= 2.0f * CONST_2PI_32)
        {
            theta_ref -= 2.0f * CONST_2PI_32;
        }
        float32_t Ua_pu = M * __cos(theta_ref);
        float32_t Ub_pu = M * __cos(theta_ref - CONST_2PI_32 / 3.f);
        float32_t Uc_pu = M * __cos(theta_ref + CONST_2PI_32 / 3.f);
        CB_SVPWM_3Ph(Ua_pu, Ub_pu, Uc_pu);
        EPWM_Start();
    }
    else if (MODE == 2) 
    {

    }
    else if (MODE == 0)
    {
        EPWM_Stop();
    }
    i++;
    if (i >= 400)
    {
        i = 0;
    }
    ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

void Loop(void)
{
    OLED_Display_Task(); // 10ms刷新周期 由CPUTIMER0调度
}
void Setup(void)
{
    PWM_PRD = EPWM_getTimeBasePeriod(EPWM_A_BASE);
    EPWM_Stop();

    // 控制器参数计算&初始化

    // 锁相环初始化
    PLL_Init();


    Keyboard_Init();       // 初始化键盘GPIO
    KEYBOARD_TIMER_Init(); // 初始化键盘CPUTIMER1中断

    Interrupt_enable(INT_ADCA1); // 使能主控中断

    OLED_Display_Init();

}

void MyProtect(void)
{
}
