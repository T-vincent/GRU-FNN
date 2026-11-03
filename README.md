# 台达杯 · 基于 GRU-FNN 的三相电流型整流器控制固件

面向 **TI C2000 F28P550SJ9（LAUNCHXL-F28P55X）** 的三相电流型整流器（CSR）控制固件，开关频率 20 kHz。
在片内 **硬件 NPU** 上跑 16 维 GRU 做电网电压估计，并用 **CLA 异步稀疏 FNN** 做调制波生成，形成"神经估计 + 神经调制"的可切换对照实验平台。

> 当前版本为 **HIL（Typhoon 半实物）验证版**，并非常规量产固件。模拟量比例、保护阈值、门极时序均按 HIL 台架标定。

## 亮点

- **20 kHz 单周期闭环**：Timer0 每 50 µs 采样并完成估计 → 前馈 → 调制 → 门极调度，整帧预算 7425 计数（49.5 µs）。
- **两路神经加速**：GRU 走 NPU（TVM 编译产物 `mod.a`），调制网络走 CLA 协处理器，与 CPU SVPWM 可在线切换。
- **估计器三选一**：原始电压 / DSOGI / GRU16，切换后自动预热 50 ms，保证对比实验条件一致。
- **门极事件离散化**：Timer1 按事件表输出六路门极，双矢量预算 43 µs、单矢量 39 µs，故障路径独立于主流程。

## 硬件与接线

| 信号 | HIL 侧 | 控制器侧 |
|---|---|---|
| 三相电压 | AO2 / AO3 / AO4 | ADCINA0 / ADCINA4 / ADCINA5 |
| 电流 | AO5 | ADCINA9 |
| 门极 G1–G6 | DI1–DI6 | GPIO0 / GPIO1 / GPIO2 / GPIO3 / GPIO10 / GPIO11 |

模拟量比例（`IMC_ANALOG_PROFILE=1`）：电压 0.002 V/V，电流 0.05 V/A，AO 偏置 1.65 V 由 HIL 设置，ADC 零点 2048 码在主程序扣除。

## 仓库结构

```
code/                                  CCS (Theia) 工程
  ex_adc_poll_4channels_main.c         入口：外设初始化、50 µs 控制中断、门极事件服务
  source/                              控制算法
    imc_gru.c                          GRU16 推理（NPU 提交 + 取回）
    imc_estimator.c                    DSOGI 估计与幅值前馈
    imc_cla_fnn.c                      CLA 稀疏 FNN 调制
    imc_gate_scheduler.c               门极事件表与预算调度
    imc_dma_gates.c                    DMA 门极通路（保留，未做实物验证）
    imc_npu_fast_backend.c             NPU 快速后端
    imc_gru_weights.c / _sigmoid_tables.c   GRU 权重与 16×256 的逐单元 sigmoid 查找表
  include/                             配置与接口（imc_user_config.h 为集中配置入口）
  npu_model/                           TVM 编译产物 mod.a + tvmgen_default.h
  device/driverlib/                    C2000Ware DriverLib
  syscfg/                              SysConfig 生成的 board / c2000ware_libraries
  28p55x_generic_flash_lnk.cmd         FLASH 链接脚本（另有 RAM 版）
  targetConfigs/                       TMS320F28P550SJ9 调试配置 (.ccxml)
Typhoon半实物仿真平台文件/               HIL 模型与场景
  harmonic(1).tse                      谐波电网场景
  sag(1).tse                           电压跌落场景
  show.cus                             监控画面配置
中期报告.pdf / 决赛报告终版.pdf          设计文档
策划书ver.2.docx / 比赛答辩2.pptx        方案与答辩材料
比赛海报.png                            展板
未竟的pcb.zip                           功率板 / 驱动板 PCB 工程（约 41 MB）
```

## 编译与烧写

1. 安装 **CCS 20.x（Theia 版）**、**C2000Ware**（提供 DriverLib 与 SysConfig 组件）。
2. `File → Import → CCS Projects`，选择 `code/` 目录导入工程 `imc_gru_fnn_dsogi_svpwm`。
3. 选择构建配置 `CPU1_LAUNCHXL_FLASH`，编译。构建输出全部落在该目录下，已被 `.gitignore` 排除。
4. 用 `targetConfigs/TMS320F28P550SJ9_LaunchPad.ccxml` 连接 LaunchPad，烧写 `imc_gru_fnn_dsogi_svpwm.out`。

## 可切换配置

| 开关 | 取值 | 含义 |
|---|---|---|
| `g_estimatorMode` | 0 / 1 / 2 | 原始电压 / DSOGI / GRU16 |
| `g_modulatorMode` | 0 / 1 | CPU SVPWM / CLA 异步稀疏 FNN |
| `g_pwmEnable` | 0 / 1 | 独立 PWM 使能（仍需控制使能 + 预热完成 + 无故障） |

已验证组合为 `(估计, 调制) = (1, 0)` 与 `(2, 1)`，两项选择相互独立。

`include/imc_user_config.h` 还集中了以下参数：模拟量比例 `IMC_ANALOG_PROFILE`、调制比上升步长、门极预算（`IMC_GATE_DUAL_BUDGET_US` / `IMC_GATE_SINGLE_BUDGET_US`）、电流采样窗口（30 计数）、帧截止（`IMC_FRAME_DEADLINE_COUNTS = 7425`）与流水线相位外推 `IMC_PREDICTION_LEAD`。

## 时序资源

| 定时器 | 用途 |
|---|---|
| Timer0 | 50 µs 控制周期采样中断 |
| Timer1 | 门极事件输出（150 MHz 计数，1 计数 = 1/150 µs） |
| Timer2 | 150 MHz 递减计时，用于整帧保护 |

单帧保护 7425 计数 = 49.5 µs，已包含 ADC 等待与中断开销。

## 说明

- 门极默认走 CPU Timer1 调度（`IMC_USE_CPU_GATES=1`）；DMA 到 GPIO 的路径代码保留，但**未通过实物验证**。
- HIL 模型中必须保留直流续流二极管，否则全关断故障工况无法复现。
- `code/` 下的 DriverLib 为 C2000Ware 副本，便于工程自包含；升级 C2000Ware 后建议同步替换。
