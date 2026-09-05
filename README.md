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