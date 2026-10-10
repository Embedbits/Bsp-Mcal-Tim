# 🕒 TIM module

This repository provides the **MCAL (Microcontroller Abstraction Layer)** driver for **timer peripherals** (`TIMx`) used in **STM32 microcontrollers**.  
It offers a unified and portable interface for working with all general-purpose, basic, and advanced timers across different STM32 families.

Each STM32 family is supported in a **dedicated branch** of this repository:
- `STM32G4`
- `STM32U5`
- `STM32L4`
- `STM32H5`
- `STM32F4`
- `STM32F7` (this branch - TIM1 / TIM8 with internal channels 5 / 6, break 2 input and trigger output 2, shared NVIC lines of TIM9 - TIM14)
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

### STM32F7 device errata

Timer limitations of the device errata sheet (ES0334) without a workaround in the module:

- **One-pulse mode trigger not detected in master-slave reset + trigger configuration** - cascaded
  timers in one-pulse mode with the master in combined reset + trigger mode and MSM = 1: a trigger at
  counter = ARR generates no pulse. Keep the master / slave mode (MSM) inactive unless cycle-accurate
  synchronization is required.
- **Consecutive compare event missed in specific conditions** - an abrupt compare value change
  creating a single timer clock cycle wide pulse in toggle mode can be missed. Other output compare
  modes are not affected; no workaround.
- **Output compare clear not working with external counter reset** - with the slave modes reset,
  combined reset + trigger or combined gated + reset, the PWM output stays inactive one extra
  period after an output compare clear followed by a counter reset. Use the break input with the
  automatic output enable instead of the output compare clear.

The automatic output enable is refused while the clock security system is enabled (RCC CSSON) -
behavior of the STM32F4 implementation kept for the same advanced-control timer.

---

## 🔗 Trigger inputs

The trigger input of the slave mode controller / external clock mode 1 (`tim_ExtClkSource_t`, used by
`Tim_Set_SlaveMode()`, `Tim_Set_ClockSource()` and the members `ExtClockSource` / `SlaveTriggerInput` of
`tim_PeriphConfig_t`) is a list of the valid items of every timer, so a connection that the hardware does
not have cannot be selected:

- `TIM_TRIGGER_INPUT_<slave>_ITR<n>_<master timer>_<signal>` - internal trigger input named by the slave timer,
  the input and the master timer signal behind it (e.g. `TIM_TRIGGER_INPUT_TIM3_ITR1_TIM2_TRGO`),
- `TIM_TRIGGER_INPUT_<timer>_TI1F_ED` / `_TI1FP1` / `_TI2FP2` / `_ETRF` - inputs of the timer itself (`_ETRF` only
  on the timers with the ETR input),
- `TIM_TRIGGER_INPUT_UNUSED` - unused trigger input; an item of another timer is refused by the functions.

The connections come from the tables "TIMx internal trigger connection" of the reference manuals (RM0385, RM0410, RM0431): an item is
active exactly on the device lines where the manual has the connection and the master exists (e.g. TIM2 ITR1 = TIM8 TRGO only on the devices with TIM8; TIM9 / TIM12 have no ETRF); the connections selected by a remap bit (TIM2 ITR1 from ETH PTP / OTG FS SOF / OTG HS SOF) are not part of the list.

The source of a timer channel input (`tim_InputSource_t`, `Tim_Set_InputSource()` / `Tim_Get_InputSource()`) is a list
in the same way: `TIM_INPUT_SOURCE_<timer>_CH<n>_PIN` (the channel input pin) and
`TIM_INPUT_SOURCE_<timer>_CH<n>_<signal>` exist only for the channels with the input remap of TIMx_OR - TIM5 channel 4
(`_LSI`, `_LSE`, `_RTC_WKUP`) and TIM11 channel 1 (`_HSE_RTC`, `_SPDIFRX_FRAME_SYNC` on the devices with SPDIFRX, `_MCO1`) -
the other channels have no input selection. The functions refuse an item of another timer or channel.

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
        .UpdateEventState       = TIM_FUNCTION_INACTIVE;
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
| `STM32F4` | MCAL driver for STM32F4 family |
| `STM32F7` | MCAL driver for STM32F7 family |

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