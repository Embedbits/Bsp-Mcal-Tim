# 🕒 TIM module

This repository provides the **MCAL (Microcontroller Abstraction Layer)** driver for **timer peripherals** (`TIMx`) used in **STM32 microcontrollers**.  
It offers a unified and portable interface for working with all general-purpose, basic, and advanced timers across different STM32 families.

Each STM32 family is supported in a **dedicated branch** of this repository:
- `STM32G4`
- `STM32U5`
- `STM32L4`
- `STM32H5`
- and others as needed.

---

## 📘 Overview

The **TIM MCAL driver** abstracts the STM32 timer peripherals into a consistent, hardware-independent interface.  
It enables initialization, configuration, and runtime control of all timer channels and modes, such as:
- Input Capture / Output Compare  
- PWM Generation  
- One-pulse and Time-base operation  
- Encoder mode  
- DMA and interrupt-based operation  

All hardware-specific configurations (RCC setup, GPIO alternate functions, interrupt handling, etc.) are handled **internally** by this module.

> ✅ The user **does not need to include or use** any additional modules such as RCC, GPIO, or NVIC drivers.  
> Everything required for the timers to function is already included and automatically initialized by the TIM module.

---

## 🧩 Architecture

The architecture follows the standard MCAL layering used across all STM32 MCAL repositories:

```
┌────────────────────────────┐
│        Application         │
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│           HAL              │
│(Hardware Abstraction Layer)│
└────────────┬───────────────┘
┌────────────▼───────────────┐
│       MCAL - Tim           │
│       ├── Tim_Port.h       │  
│       ├── Tim_Types.h      │  
│       └── Tim.c/.h         │  
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│           RAL              │
│(Register Abstraction Layer)│
└────────────────────────────┘
```

---

## 🧠 Usage Guidelines

The user shall **only** interact with the following two public headers:

| File | Purpose |
|------|----------|
| `Tim_Port.h` | Contains all public API functions to control timers (init, start, stop, set duty cycle, read counter, etc.) |
| `Tim_Types.h` | Contains type definitions and enumerations used in the API (timer IDs, channel numbers, configuration structs, etc.) |

Everything else — configuration files, static tables, helper functions — is **internal** and must not be accessed directly.

---

## 🔧 STM32L4 / STM32L4+ specifics

- Public interface of the STM32H5 module (Dev/STM32H5), STM32G4 implementation as base.
- Timers: TIM1 / TIM8 (advanced, 6 channels, complementary outputs CH1N - CH3N, break and break 2),
  TIM2 / TIM5 (32-bit), TIM3 / TIM4, TIM6 / TIM7 (basic), TIM15 (2 channels, CH1N), TIM16 / TIM17
  (1 channel, CH1N). TIM3 / TIM4 / TIM5 / TIM7 / TIM8 / TIM17 only on devices with the peripheral
  (pin and peripheral enumerations are guarded). LPTIM1 / LPTIM2 are not handled by this module.
- Pin enumerations (`TIM_<n>_CH<k>_P<x><y>`, `..._CH<k>N_...`, `..._BKIN_...`, `..._BKIN2_...`,
  `..._ETR_...`) are generated from the STM32CubeMX GPIO modes database - every pin item is active
  on exactly the CMSIS device lines whose package or die has the pin (guards by the device line).
  The STM32L4+ database lists the break inputs only with the comparator alternate functions - the
  plain break input alternate functions are taken from the STM32L49x subgroup (same pins) and stay
  active on the STM32L4+ lines as before. Comparator break variants (`TIMx_BKIN_COMPy`) are not
  generated.
- Trigger inputs (`tim_ExtClkSource_t`: `Tim_Set_SlaveMode()`, `Tim_Set_ClockSource()` with external clock mode 1,
  `tim_PeriphConfig_t.ExtClockSource` / `SlaveTriggerInput`) and ETR sources (`tim_EtrSource_t`,
  `Tim_Set_EtrSource()`) are lists of the valid items of every timer, so a connection that the hardware does
  not have cannot be selected: `TIM_TRIGGER_INPUT_<slave>_ITR<n>_<master timer>_<signal>` (e.g.
  `TIM_TRIGGER_INPUT_TIM2_ITR0_TIM1_TRGO`), `TIM_TRIGGER_INPUT_<timer>_TI1F_ED` / `_TI1FP1` / `_TI2FP2` / `_ETRF` and
  `TIM_ETR_SOURCE_<timer>_PIN` / `_COMP1_OUT` / `_COMP2_OUT`; `TIM_TRIGGER_INPUT_UNUSED` is the value of an unused
  trigger input. An item of another timer is refused. The connections come from the tables "TIMx internal trigger
  connection" and the TIMx_OR2 descriptions of RM0351 (STM32L47x / L48x / L49x / L4Ax), RM0394 (STM32L41x - L46x)
  and RM0432 (STM32L4+); an item is active exactly on the device lines where the manual has the connection and the
  master exists (`TIM_DEVICES_RM0394`, `TIM_DEVICES_RM0351` are defined in Tim_Types.h for the devices of the
  manual). Not part of the lists: the connections selected by a remap bit of the option register TIMx_OR1 that the
  module does not support (TIM2 ITR1 = USB SOF on STM32L41x - L46x and OTG FS SOF on the other devices, TIM2 ETR
  from LSE, ETR from the analog watchdogs of ADC1 / ADC2 / ADC3 of TIM1 / TIM8) and the connections of TIM3 of
  STM32L451 / L452 / L462 (RM0394 gives neither an internal trigger table nor TIM3_OR2).
- Shared NVIC lines: TIM1 break / update lines are shared with TIM15 / TIM16 global interrupts
  (`TIM1_BRK_TIM15`, `TIM1_UP_TIM16`), TIM1 trigger / commutation with TIM17 on devices with TIM17
  (`TIM1_TRG_COM_TIM17`) - the line handler processes the interrupts of both timers. TIM6 shares
  its line with DAC1 underrun on devices with DAC1 (DAC interrupts are not processed by this
  module). `Tim_Deinit()` does not disable shared NVIC lines.
- Option registers: break input enable / polarity and ETR source selection in TIMx_OR2 (break 2 in
  TIMx_OR3) instead of AF1 / AF2 of STM32G4 / STM32H5. ETR source (`Tim_Set_EtrSource()`) is the field
  ETRSEL[2:0]. DMA burst base registers `TIM_DMA_BURST_REG_AF1` / `_AF2` / `_OR1` select
  TIMx_OR2 / OR3 / OR1, burst length up to 18 transfers.
- Features of the STM32H5 / STM32G4 interface not available on STM32L4 / STM32L4+ (functions return
  error, interface kept):
  - encoder index (`Tim_Set_EncoderIndex()`), index / direction change / encoder error interrupts
    (`TIM_IRQ_INDEX`, `TIM_IRQ_DIRECTION`, `TIM_IRQ_ERROR`),
  - encoder modes other than x2 TI1 / x2 TI2 / x4 TI12 (x1, clock plus direction, directional clock),
  - dithering (`Tim_Set_DitheringActive()` / `Inactive()`),
  - asymmetric dead-time (`Tim_Set_DeadTime()` with different rising and falling dead-time),
  - bidirectional break / break 2 (`TIM_BREAK_MODE_BIDIRECTIONAL`),
  - input selection TISEL (`Tim_Set_InputSource()` / `Tim_Get_InputSource()`, inputs are remapped
    by TIMx_OR1 on STM32L4 - not supported),
  - output compare modes pulse on compare / direction output, trigger output encoder clock, slave
    mode combined gated + reset, complementary output CH4N,
  - DMA burst source selection (DBSS) and base registers DTR2 / ECR / TISEL.
- DMA requests of the timers: STM32L4+ routes them by DMAMUX1, STM32L4 has the fixed request
  mapping of DMA1 / DMA2 channels (Dma module), register addresses for DMA transfers are provided
  by `Tim_Get_DmaRegAddr()`.
- Every interrupt service routine ends with DSB (Cortex-M4 r0p1 erratum 838869). STM32L4 errata
  sheets are not reviewed yet.

---

## ⚙️ Typical Usage Example

```c
#include "Tim_Port.h"

int main(void)
{
    // Initialize timer TIM2 for PWM mode on channel 1
    tim_PeriphConfig_t pwmConfig = 
    {
        .PeriphId               = TIM_PERIPH_1;
        .ClockSource            = TIM_CLOCKSOURCE_INT_CLK;
        .SlaveMode              = TIM_SLAVE_MODE_DISABLE;
        .TimerFrequency         = 10000000u;
        .AutoreloadPreloadState = TIM_FUNCTION_INACTIVE;
        .UpdateEventState       = TIM_FUNCTION_ACTIVE;
        .CounterDirection       = TIM_COUNTER_DIR_UP;
        ...
    };

    Tim_Init(&pwmConfig);
    Tim_Start(TIM_PERIPH_1);

    while (1)
    {
        // Change duty cycle dynamically
        Tim_Set_PwmMode_DutyCycle(TIM_PERIPH_1, TIM_CHANNEL_1, 75U);
    }
}
```

---

## 🧾 Branching Strategy

Each STM32 family has its own branch:

| Branch | Description |
|--------|--------------|
| `STM32G4` | MCAL driver for STM32G4 family |
| `STM32U5` | MCAL driver for STM32U5 family |
| `STM32L4` | MCAL driver for STM32L4 family |
| `STM32H5` | MCAL driver for STM32H5 family |

These branches contain family-specific register definitions, channel mapping, and RCC/GPIO bindings while maintaining a common interface.

---

## 🧩 Dependencies

- **Nvic_Lib** – For core definitions and interrupt handling  
- **Rcc_Lib** – For clock definitions
- **Gpio_Lib** – For GPIO definitions
- **RAL (Register Abstraction Layer)** – Used internally to access low-level registers  

All mandatory RCC and GPIO configurations are handled internally.

---

## 🧱 Example Directory Structure

```
Tim/
├── Tim_Port.h
├── Tim_Types.h
├── Tim.c
├── Tim.h
├── CMakeLists.txt
└── README.md
```

---

## 🛠 CMake Integration

1. Include `Tim_Lib` in your CMake library.
2. Include `Tim_Port.h` in your project.
3. Link against the Tim module implementation files.
4. Configure the module as needed for your hardware.

---

## License

This project is licensed under the **Creative Commons Attribution–NonCommercial 4.0 International (CC BY-NC 4.0)**.

You are free to use, modify, and share this work for **non-commercial purposes**, provided appropriate credit is given.

See [LICENSE.md](LICENSE.md) for full terms or visit [creativecommons.org/licenses/by-nc/4.0](https://creativecommons.org/licenses/by-nc/4.0/).

---

## Authors

- **Mr.Nobody** — [embedbits.com](https://embedbits.com)

Contributions are welcome! Please open a pull request.

---

## 🌐 Useful Links

- [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html)
- [Azure DevOps](https://azure.microsoft.com/en-us/services/devops/)
- [Embedbits Github](https://github.com/Embedbits)
- [CC BY-NC 4.0 License](https://creativecommons.org/licenses/by-nc/4.0/)