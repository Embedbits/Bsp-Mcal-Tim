# 🕒 TIM module

This repository provides the **MCAL (Microcontroller Abstraction Layer)** driver for **timer peripherals** (`TIMx`) used in **STM32 microcontrollers**.  
It offers a unified and portable interface for working with all general-purpose, basic, and advanced timers across different STM32 families.

Each STM32 family is supported in a **dedicated branch** of this repository:
- `STM32G4`
- `STM32U5`
- `STM32L4`
- `STM32H5`
- `STM32H7` (this branch - public interface of STM32H5, see [STM32H7 specifics](#-stm32h7-specifics))
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

## 🔧 STM32H7 specifics

Public interface of the STM32H5 module (`Dev/STM32H5`). Functionality not present on STM32H7 timers is kept in the
interface and refused with `TIM_REQUEST_ERROR` (or reported as not available).

| Feature                                  | STM32H7 behavior                                                                                     |
|------------------------------------------|------------------------------------------------------------------------------------------------------|
| Timers                                   | TIM1 / TIM8 (advanced, 16-bit), TIM2 / TIM5 (32-bit), TIM3 / TIM4 (16-bit), TIM6 / TIM7 (basic), TIM12 - TIM17; TIM23 / TIM24 (32-bit) on STM32H72x / H73x |
| Channels                                 | TIM1 / TIM8: channels 1 - 4 with complementary outputs on channels 1 - 3 (no CH4N), internal channels 5 / 6 |
| Break inputs                             | Break and break 2 (TIM1 / TIM8), break (TIM15 - TIM17); bidirectional break (BKBID / BK2BID) only on STM32H72x / H73x / H7A3 / H7B0 / H7B3 - `Tim_Set_BreakConfig()` refuses bidirectional mode on other devices; system break interrupt (SBIF) |
| Input selection / ETR                    | TISEL input remapping, ETR source selection (AF1.ETRSEL), break input sources (AF1 / AF2)            |
| Trigger inputs                           | ITR0 - ITR13 (STM32H7) / ITR0 - ITR14 (STM32H7R / H7S): list of the valid items of every timer (`TIM_TRIGGER_INPUT_*`) |
| Not available                            | dithering, encoder index / direction / error interrupts and index configuration, X1 / clock plus direction / directional clock encoder modes, pulse on compare / direction output channel modes, encoder clock master trigger, combined gated + reset slave mode, asymmetric dead-time (no DTR2), DMA burst source selection - refused |
| Interrupt lines                          | TIM1 / TIM8 split (break, update, trigger + commutation, capture / compare); TIM12 / TIM13 / TIM14 share `TIM8_BRK_TIM12` / `TIM8_UP_TIM13` / `TIM8_TRG_COM_TIM14` with TIM8, TIM6 shares `TIM6_DAC` with DAC1 - one handler processes all timers of the line, `Tim_Deinit()` keeps shared lines enabled |
| Pins                                     | Alternate function pin enumerations generated from ST open pin data per device line (pins not present on the device line are not defined) |

**STM32H7R3 / H7R7 / H7S3 / H7S7** (Ral family STM32H7RS, macro `STM32H7RS`): the timers are the STM32H5 timer IP -
the STM32H5 implementation is compiled for everything listed as not available above (dithering, encoder index /
direction / error interrupts, all encoder modes, pulse on compare / direction output, encoder clock master trigger,
combined gated + reset slave mode, asymmetric dead-time, DMA burst source selection) and TIM1 has CH4N. Timers: TIM1,
TIM2 - TIM7, TIM9 (2 channels), TIM12 - TIM17 (no TIM8, TIM23, TIM24); every timer has own NVIC lines (no shared line);
no TIMx_OR1 register (DMA burst base `TIM_DMA_BURST_REG_OR1` refused). Pin enumerations from the STM32H7RS open pin data.

Not yet tested on hardware (integration tests prepared for STM32H7 Nucleo boards).

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

## 🔗 Trigger inputs and ETR sources

The trigger input of the slave mode controller / external clock mode 1 (`tim_ExtClkSource_t`, used by
`Tim_Set_SlaveMode()`, `Tim_Set_ClockSource()` and the members `ExtClockSource` / `SlaveTriggerInput` of
`tim_PeriphConfig_t`) is a list of the valid items of every timer, so a connection that the hardware does
not have cannot be selected:

- `TIM_TRIGGER_INPUT_<slave>_ITR<n>_<master timer>_<signal>` - internal trigger input named by the slave timer,
  the input and the master timer signal behind it (e.g. `TIM_TRIGGER_INPUT_TIM3_ITR1_TIM2_TRGO`),
- `TIM_TRIGGER_INPUT_<timer>_TI1F_ED` / `_TI1FP1` / `_TI2FP2` / `_ETRF` - inputs of the timer itself (`_ETRF` only
  on the timers with the ETR input),
- `TIM_TRIGGER_INPUT_UNUSED` - unused trigger input; an item of another timer is refused by the functions.

The connections come from the tables "TIMx internal trigger connection" of the reference manual (RM0433 (= RM0399), RM0455, RM0468, RM0477): an item is
active exactly on the device lines where the manual has the connection and the master exists (e.g. TIM2 ITR4 = ETH PPS only with ETH, TIM1 ITR12 / ITR13 = TIM23 / TIM24 TRGO only on STM32H72x / H73x, ITR4 - ITR14 of STM32H7R / H7S from TIM5, TIM12 - TIM17 and the USB OTG SOF; TIM12 / TIM15 have no ETRF).

The source of the external trigger input (`tim_EtrSource_t`, `Tim_Set_EtrSource()`) is a list in the same way:
`TIM_ETR_SOURCE_<timer>_PIN` (the ETR pin) and `TIM_ETR_SOURCE_<timer>_<signal>` (e.g. `TIM_ETR_SOURCE_TIM1_ADC1_AWD1`) from the ETRSEL descriptions of TIMx_AF1 (STM32H7) and the tables "Interconnect to the tim_etr input multiplexer" (STM32H7R / H7S), group macros `TIM_DEVICES_RM0433` / `TIM_DEVICES_RM0455` / `TIM_DEVICES_RM0468` / `TIM_DEVICES_RM0477`
of RM0433 (= RM0399), RM0455, RM0468, RM0477; the signals of a peripheral only with the peripheral; TIM4 of the STM32H7 devices has no ETR source selection (the ETR pin is selected after reset).

The source of a timer channel input (`tim_InputSource_t`, `Tim_Set_InputSource()` / `Tim_Get_InputSource()`) is a list
in the same way: `TIM_INPUT_SOURCE_<timer>_CH<n>_PIN` (the channel input pin) and
`TIM_INPUT_SOURCE_<timer>_CH<n>_<signal>` (e.g. `TIM_INPUT_SOURCE_TIM1_CH1_COMP1_OUT`) from the tables "Interconnect to the tim_tiX input
multiplexer" of RM0433 (= RM0399), RM0455, RM0468, RM0477; the signals of a peripheral only with the peripheral; the STM32H7R / H7S lines have TIM9, TIM12 - TIM17 with input selection, the STM32H7 lines TIM12 - TIM17 (TIM23 / TIM24 on STM32H72x / H73x). The functions refuse an item of another timer or channel.

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
| `STM32H7` | MCAL driver for STM32H7 family |

These branches contain family-specific register definitions, channel mapping, and RCC/GPIO bindings while maintaining a common interface.

---

## 🧩 Dependencies

- **Nvic_Lib** – For core definitions and interrupt handling  
- **Rcc_Lib** – For clock definitions
- **Gpio_Lib** – For GPIO definitions
- **Unity / CMock / RegMem** – Unit tests (`Tests/UnitTests/Test_Tim.c`, host build with emulated timer registers)
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