# F280049C_Template

这是一个面向 TI C2000 DSP F280049C 的 CCS 20 模板工程，适合作为电力电子控制、三相调制、数字电源实验和 NUEDC 类项目的起始工程。

## 开发环境

- CCS 20
- TI C2000Ware 26.01.00.00
- SysConfig 1.27.0+
- TI C2000 code generation tools `ti-cgt-c2000_22.6.3.LTS`
- 目标芯片：TMS320F280049C

工程中 VS Code 配置主要用于代码阅读、IntelliSense 和调用 CCS 编译链；实际工程属性仍以 CCS 项目配置为准。

## 工程结构

```text
NUEDC_TI_Template/
  c2000.syscfg                     SysConfig 外设与 DCL 对象配置
  28004x_generic_flash_lnk.cmd     Flash 构建链接脚本
  28004x_generic_ram_lnk.cmd       RAM 构建链接脚本
  CPU1_FLASH/                      CCS Flash 构建输出目录
  device/                          F28004x device support 与 driverlib
  mycode/
    Control.c / Control.h          主控入口、慢速任务、ADC 主中断
    global.h                       全局变量 extern、RAMFUNC、常用宏
    myEpwm.c / myEpwm.h            EPWM、SPWM、CB-SVPWM 等调制代码
    QPR.h                          DCL_DF22 PR/QPR 控制器系数计算
    RMS.c / RMS.h                  有效值计算
    OLED_Display.c / .h            OLED 显示应用层
    oled.c / oled.h                OLED 硬件 I2C 底层
    KeyBoard.c / .h                键盘扫描与按键状态
    SPLL/                          软件锁相环及其依赖
    power_meas_sine_analyzer.h     TI 数字电源功率分析仪头文件
TI库参考文档/                       DCL 相关参考文档
```

## 默认控制架构

当前模板采用“EPWM 触发 ADC，ADC 中断执行实时控制”的结构：

1. EPWM 上下计数，`TBPRD = 2500`，默认对应 20 kHz PWM。
2. EPWM 在计数最低点触发 ADC SOC。
3. ADC 完成配置的 SOC 转换后触发 `ADCA1` 中断。
4. `ADC_SamplingISR()` 作为主控中断，放置所有 time critical 控制代码。

如果后续增加多个 ADC SOC，需要把 `ADC INT1` 的触发源设置为最后一个 SOC/EOC。当前 ADC 优先级配置为按 SOC 编号排序，因此转换顺序通常为 `SOC0 -> SOC1 -> ... -> SOCn`。

## 慢速任务

`CPUTIMER0` 用作 1 ms 慢速任务节拍：

- LED 心跳任务
- OLED 刷新请求
- 其他非实时任务调度

OLED 刷新采用标志位方式：Timer 中断只置位，实际 I2C 刷新在 `Loop()` 中由 `OLED_Display_Task()` 执行，避免在中断里阻塞。

## DCL 与控制器

模板建议使用 TI DCL 库组织控制器对象，并通过 SysConfig 统一生成对象定义。

常用对象和函数：

- `DCL_runPI_C3()`
- `DCL_runDF11_C1()` / `DCL_runDF11_C2()`
- `DCL_runDF22_C1()` / `DCL_runDF22_C2()`
- `DCL_runRefgen()`

`QPR.h` 提供 PR/QPR 控制器到 `DCL_DF22` 的系数计算。后续若添加一阶滤波器、虚拟阻抗或对象模型，可参考同类系数计算文件映射到 `DCL_DF11`。

三角函数建议使用 TI intrinsic，例如 `__sin()`、`__cos()`。项目已打开 TMU 支持，适合在实时控制中使用。

## RAMFUNC 与内存分配

实时路径中的函数应尽量放入 RAM 中运行，可使用：

```c
RAMFUNC void foo(void)
{
}
```

链接脚本中主要分区含义：

- `RAMLS_CODE`：RAM 中运行的代码，例如 `.TI.ramfunc`
- `RAMLS_DATA`：高速 RAMLS 数据区，容量较小
- `RAMGS_DATA`：RAMGS 数据区，容量较大，适合 RMS 结构体、日志、状态缓存等
- `FLASH_APP`：合并后的应用 Flash 区域

注意：某个 ISR 放入 RAM，并不代表它调用的所有函数也自动在 RAM 中。被调用函数若也在实时路径上，需要单独使用 `RAMFUNC` 或确认其所在库段已经被链接到 RAM。

## OLED 与键盘

- OLED 使用硬件 I2C。
- 默认引脚：`GPIO26` 为 SDA，`GPIO27` 为 SCL。
- OLED 应用层在 `OLED_Display.c`，底层驱动在 `oled.c`。
- 键盘 GPIO 由 SysConfig 配置，扫描定时器使用 `CPUTIMER1`。
- OLED 支持分页显示，默认翻页按键为 `K15` 和 `K16`。

## 移植检查清单

迁移到新工程或新硬件时，建议依次检查：

1. CCS Project Properties 中的 C2000Ware、CGT、SysConfig 路径。
2. `c2000.syscfg` 中 ADC、EPWM、GPIO、I2C、CPU Timer 配置。
3. 实际板卡的 PWM、ADC、OLED、键盘、LED 引脚。
4. `28004x_generic_flash_lnk.cmd` 中 RAM/Flash 分区是否满足当前代码规模。
5. ADC SOC 数量变化后，`ADC INT1` 是否仍由最后一个 EOC 触发。
6. 实时路径函数是否已经使用 `RAMFUNC` 或来自已放入 RAM 的库段。

## 构建与内存占用

默认 Flash 构建配置为 `CPU1_FLASH`。构建后可重点查看：

- `.map` 文件：最直接的段分布和符号占用
- `*_linkInfo.xml`：链接器生成的结构化内存占用信息
- CCS Memory Allocation 视图：图形化查看 Flash/RAM 使用情况

一般关注：

- `.text`、`.cinit`、`.const` 等 Flash 占用
- `.TI.ramfunc`、`dclfuncs` 等 RAM 运行代码段
- `controlVariables`、`logVariables` 等自定义数据段

## 参考文档

本仓库保留了部分 TI DCL 参考文档，位于 `TI库参考文档/`。更完整的外设说明请参考 F28004x Technical Reference Manual、C2000Ware driverlib 文档和 Digital Power SDK 示例。
