/**
 * \author Mr.Nobody
 * \file Tim_Types.h
 * \ingroup Tim
 * \brief Tim module global types definition
 *
 * This file contains the types definitions used across the module and are 
 * available for other modules through Port file.
 *
 */

#ifndef TIM_TIM_TYPES_H
#define TIM_TIM_TYPES_H
/* ============================== INCLUDES ================================== */
#include "stdint.h"                         /* Module types definition        */
#include "Stm32_tim.h"                      /* TIM utilities functionality    */
#include "Gpio_Port.h"                      /* GPIO Port module definitions   */
/* ========================== SYMBOLIC CONSTANTS ============================ */

/** Null pointer definition */
#define TIM_NULL_PTR                        ( ( void* ) 0u )

/** Channel identification bit offset in encoded value */
#define TIM_BIT_MASK_CHANNEL_BIT_OFFSET     ( 20u )

/** Peripheral identification bit offset in encoded value */
#define TIM_BIT_MASK_PERIPH_BIT_OFFSET      ( 15u )

/** Port identification bit offset in encoded value */
#define TIM_BIT_MASK_PORT_BIT_OFFSET        ( 10u )

/** Pin identification bit offset in encoded value */
#define TIM_BIT_MASK_PIN_BIT_OFFSET         ( 5u )

/** Alternative function identification bit offset in encoded value */
#define TIM_BIT_MASK_AF_BIT_OFFSET          ( 0u )

/* ========================== EXPORTED MACROS =============================== */

/** Encode channel pin configuration into single 16bit bit-mask */
#define TIM_CHANNEL_PIN_BIT_MASK_ENCODE(CHANNEL_ID,PERIPH_ID,PORT_ID,PIN_ID,AF_ID)  ( ( CHANNEL_ID << TIM_BIT_MASK_CHANNEL_BIT_OFFSET ) | \
                                                                                      ( PERIPH_ID  << TIM_BIT_MASK_PERIPH_BIT_OFFSET  ) | \
                                                                                      ( PORT_ID    << TIM_BIT_MASK_PORT_BIT_OFFSET    ) | \
                                                                                      ( PIN_ID     << TIM_BIT_MASK_PIN_BIT_OFFSET     ) | \
                                                                                      ( AF_ID      << TIM_BIT_MASK_AF_BIT_OFFSET      )   )

/** Encode timer pin configuration into single 16bit bit-mask */
#define TIM_TIMER_PIN_BIT_MASK_ENCODE(PERIPH_ID,PORT_ID,PIN_ID,AF_ID)               ( ( PERIPH_ID << TIM_BIT_MASK_PERIPH_BIT_OFFSET ) | \
                                                                                      ( PORT_ID   << TIM_BIT_MASK_PORT_BIT_OFFSET   ) | \
                                                                                      ( PIN_ID    << TIM_BIT_MASK_PIN_BIT_OFFSET    ) | \
                                                                                      ( AF_ID     << TIM_BIT_MASK_AF_BIT_OFFSET     )   )

/** Extract peripheral ID from encoded value */
#define TIM_BIT_MASK_DECODE_PERIPH(CODED_VAL)               ( ( CODED_VAL >> TIM_BIT_MASK_PERIPH_BIT_OFFSET ) & 0x1F )

/** Extract channel ID from encoded value */
#define TIM_BIT_MASK_DECODE_CHANNEL(CODED_VAL)              ( ( CODED_VAL >> TIM_BIT_MASK_CHANNEL_BIT_OFFSET ) & 0x1F )

/** Extract port ID from encoded value */
#define TIM_BIT_MASK_DECODE_PORT(CODED_VAL)                 ( ( CODED_VAL >> TIM_BIT_MASK_PORT_BIT_OFFSET ) & 0x1F )

/** Extract pin ID from encoded value */
#define TIM_BIT_MASK_DECODE_PIN(CODED_VAL)                  ( ( CODED_VAL >> TIM_BIT_MASK_PIN_BIT_OFFSET ) & 0x1F )

/** Extract alternative function ID from encoded value */
#define TIM_BIT_MASK_DECODE_AF(CODED_VAL)                   ( ( CODED_VAL >> TIM_BIT_MASK_AF_BIT_OFFSET ) & 0x1F )


/* ============================== TYPEDEFS ================================== */

/** \brief Type signaling major version of SW module */
typedef uint8_t tim_MajorVersion_t;


/** \brief Type signaling minor version of SW module */
typedef uint8_t tim_MinorVersion_t;


/** \brief Type signaling patch version of SW module */
typedef uint8_t tim_PatchVersion_t;


/** \brief Type signaling actual version of SW module */
typedef struct
{
    tim_MajorVersion_t Major; /**< Major version */
    tim_MinorVersion_t Minor; /**< Minor version */
    tim_PatchVersion_t Patch; /**< Patch version */
}   tim_ModuleVersion_t;


/** Function status enumeration */
typedef enum
{
    TIM_FUNCTION_INACTIVE = 0u, /**< Function status is inactive */
    TIM_FUNCTION_ACTIVE         /**< Function status is active   */
}   tim_FunctionState_t;


/** Enumeration used to signal request processing state */
typedef enum
{
    TIM_REQUEST_ERROR = 0u, /**< Processing request failed  */
    TIM_REQUEST_OK          /**< Processing request succeed */
}   tim_RequestState_t;


/** Flag states enumeration */
typedef enum
{
    TIM_FLAG_INACTIVE = 0u, /**< Inactive flag state */
    TIM_FLAG_ACTIVE         /**< Active flag state   */
}   tim_FlagState_t;

/**
 * \brief Type used to signal time values
 *
 * Use this datatype for time values, where the following characteristics
 * are sufficent:
 *  - Range of values : 0 ns to 4.294 s in nano-seconds [0.000 000 001 s]
 *  - Offset          : 0 ns
 *  - Step size       : 1 ns
 */
typedef uint32_t tim_Time_ns_t;


/** Frequency values type represented in Hz */
typedef uint32_t tim_FreqHz_t;


/**
 * \brief Type used to signal percent values
 *
 * Use this datatype for centi-percent values, where the following characteristics
 * are sufficient:
 *  - Range of values: 0 % to 100,00 % in [c%]
 *  - Resolution: 0.01 %
 */
typedef uint16_t tim_CentiPercent_t;


/** Enumeration used to signal signal polarity (normal/inverted) */
typedef enum
{
    TIM_POLARITY_HIGH = 0u, /**< Required state is high */
    TIM_POLARITY_LOW        /**< Required state is low  */
}   tim_Polarity_t;


/** Enumeration used to signal signal polarity (normal/inverted) */
typedef enum
{
    /** Non-inverted/rising edge. The circuit is sensitive to TIxFP1 rising edge
     * (capture or trigger operations in reset, external clock or trigger mode),
     * TIxFP1 is not inverted (trigger operation in gated mode or encoder mode). */
    TIM_INPUT_POLARITY_NORMAL = LL_TIM_IC_POLARITY_RISING,

    /** Inverted/falling edge. The circuit is sensitive to TIxFP1 falling edge
     * (capture or trigger operations in reset, external clock or trigger mode),
     * TIxFP1 is inverted (trigger operation in gated mode or encoder mode). */
    TIM_INPUT_POLARITY_INVERTED = LL_TIM_IC_POLARITY_FALLING,

    /** Non-inverted/both edges/ The circuit is sensitive to both TIxFP1 rising
     * and falling edges (capture or trigger operations in reset, external clock
     * or trigger mode), TIxFP1is not inverted (trigger operation in gated
     * mode). This configuration must not be used in encoder mode. */
    TIM_INPUT_POLARITY_BOTH_EDGES = LL_TIM_IC_POLARITY_BOTHEDGE

}   tim_InputPolarity_t;


/**
 * \brief Counter direction / counting mode enumeration
 *
 * Down and center-aligned modes are available only on TIM1 / TIM2 / TIM3 /
 * TIM4 / TIM5 / TIM8, other timers support up-counting only.
 */
typedef enum
{
    TIM_COUNTER_DIR_UP = 0u,        /**< Counter counts up from 0 to auto-reload value                           */
    TIM_COUNTER_DIR_DOWN,           /**< Counter counts down from auto-reload value to 0                         */
    TIM_COUNTER_DIR_CENTER_UP,      /**< Center-aligned, compare flags set only when counting up                 */
    TIM_COUNTER_DIR_CENTER_DOWN,    /**< Center-aligned, compare flags set only when counting down               */
    TIM_COUNTER_DIR_CENTER_UP_DOWN, /**< Center-aligned, compare flags set when counting up and down             */
    TIM_COUNTER_DIR_CNT             /**< Count of counter directions                                             */
}   tim_CounterDir_t;


/**
 * \brief Timer counter register value type (counter, auto-reload, compare)
 *
 * Full range is used by 32-bit timers (TIM2 / TIM5), other timers are limited
 * to 16-bit values.
 */
typedef uint32_t tim_Counter_t;


/**
 * \brief Repetition counter value type
 *
 * Update event is generated after (value + 1) counter overflows / underflows.
 * Range 0 - 65535 on TIM1 / TIM8, 0 - 255 on TIM15 / TIM16 / TIM17.
 */
typedef uint16_t tim_RepCnt_t;


/** \brief Update event (UEV) request source enumeration */
typedef enum
{
    TIM_UPDATE_SOURCE_ANY = 0u, /**< Counter overflow / underflow, UG bit or slave mode controller reset */
    TIM_UPDATE_SOURCE_COUNTER,  /**< Counter overflow / underflow only                                  */
    TIM_UPDATE_SOURCE_CNT       /**< Count of update sources                                            */
}   tim_UpdateSource_t;


/** \brief Software generated timer events enumeration */
typedef enum
{
    TIM_EVENT_UPDATE = 0u,  /**< Update event (re-initializes counter, updates shadow registers) */
    TIM_EVENT_CC1,          /**< Capture / compare event of channel 1                           */
    TIM_EVENT_CC2,          /**< Capture / compare event of channel 2                           */
    TIM_EVENT_CC3,          /**< Capture / compare event of channel 3                           */
    TIM_EVENT_CC4,          /**< Capture / compare event of channel 4                           */
    TIM_EVENT_COMMUTATION,  /**< Capture / compare control update (commutation) event           */
    TIM_EVENT_TRIGGER,      /**< Trigger event                                                  */
    TIM_EVENT_BREAK,        /**< Break event                                                    */
    TIM_EVENT_BREAK2,       /**< Break 2 event                                                  */
    TIM_EVENT_CNT           /**< Count of software generated events                             */
}   tim_EventId_t;


/** Type signaling count of channels in timer peripheral */
typedef uint8_t tim_ChannelCnt_t;


typedef enum
{
    TIM_ERROR_INACTIVE   = 0x00u, /**< No active error detected           */
    TIM_ERROR_INDEX      = 0x01u, /**< Index error has been detected      */
    TIM_ERROR_TRANSITION = 0x02u, /**< Transition error has been detected */
}   tim_ErrorMask_t;


/**
 * \brief Enumeration signaling over-capture state
 *
 * If a capture occurs while the CCxIF flag was already high, then the
 * over-capture flag CCxOF (TIMx_SR register) is set.
 */
typedef enum
{
    TIM_OVERCAPTURE_INACTIVE = 0u, /**< Over-capture was not detected */
    TIM_OVERCAPTURE_ACTIVE,        /**< Over-capture was detected     */
}   tim_OvercaptureFlag_t;


/** \brief Error ISR routine type definition */
typedef void ( tim_ErrIsrCallback_t )( tim_ErrorMask_t errorMask );


/** \brief Update event ISR routine type definition */
typedef void ( tim_UpdateIsrCallback_t )( void );


/** \brief Capture/Compare event ISR routine type definition */
typedef void ( tim_CaptureComapreIsrCallback_t )( tim_OvercaptureFlag_t overcaptureFlag );


/** \brief Trigger event ISR routine type definition */
typedef void ( tim_TriggerIsrCallback_t )( void );


/** \brief Commutation event ISR routine type definition */
typedef void ( tim_CommutationIsrCallback_t )( void );


/** \brief Break events ISR routine type definition */
typedef void ( tim_BreakIsrCallback_t )( void );


/** \brief Break events ISR routine type definition */
typedef void ( tim_Break2IsrCallback_t )( void );


/** \brief System break event ISR routine type definition */
typedef void ( tim_SystemBreakIsrCallback_t )( void );


/** \brief Direction change event ISR routine type definition */
typedef void ( tim_DirectionIsrCallback_t )( void );


/** \brief Index event ISR routine type definition */
typedef void ( tim_IndexIsrCallback_t )( void );


/** \brief Timer peripheral enumeration */
typedef enum
{
#ifdef TIM1
    TIM_PERIPH_1 = 0u,  /**< Timer1 peripheral  */
#endif
#ifdef TIM2
    TIM_PERIPH_2 ,      /**< Timer2 peripheral  */
#endif
#ifdef TIM3
    TIM_PERIPH_3 ,      /**< Timer3 peripheral  */
#endif
#ifdef TIM4
    TIM_PERIPH_4 ,      /**< Timer4 peripheral  */
#endif
#ifdef TIM5
    TIM_PERIPH_5 ,      /**< Timer5 peripheral  */
#endif
#ifdef TIM6
    TIM_PERIPH_6 ,      /**< Timer6 peripheral  */
#endif
#ifdef TIM7
    TIM_PERIPH_7 ,      /**< Timer7 peripheral  */
#endif
#ifdef TIM8
    TIM_PERIPH_8 ,      /**< Timer8 peripheral  */
#endif
#ifdef TIM9
    TIM_PERIPH_9 ,      /**< Timer9 peripheral  */
#endif
#ifdef TIM10
    TIM_PERIPH_10,      /**< Timer10 peripheral */
#endif
#ifdef TIM11
    TIM_PERIPH_11,      /**< Timer11 peripheral */
#endif
#ifdef TIM12
    TIM_PERIPH_12,      /**< Timer12 peripheral */
#endif
#ifdef TIM13
    TIM_PERIPH_13,      /**< Timer13 peripheral */
#endif
#ifdef TIM14
    TIM_PERIPH_14,      /**< Timer14 peripheral */
#endif
#ifdef TIM15
    TIM_PERIPH_15,      /**< Timer15 peripheral */
#endif
#ifdef TIM16
    TIM_PERIPH_16,      /**< Timer16 peripheral */
#endif
#ifdef TIM17
    TIM_PERIPH_17,      /**< Timer17 peripheral */
#endif
#ifdef TIM18
    TIM_PERIPH_18,      /**< Timer18 peripheral */
#endif
#ifdef TIM19
    TIM_PERIPH_19,      /**< Timer19 peripheral */
#endif
#ifdef TIM20
    TIM_PERIPH_20,      /**< Timer20 peripheral */
#endif
    TIM_PERIPH_CNT      /**< Count of available peripherals */
}   tim_PeriphId_t;


typedef enum
{
    TIM_CHANNEL_1 = 0u, /**< Timer channel 1 (if available) */
    TIM_CHANNEL_2,      /**< Timer channel 2 (if available) */
    TIM_CHANNEL_3,      /**< Timer channel 3 (if available) */
    TIM_CHANNEL_4,      /**< Timer channel 4 (if available) */
    TIM_CHANNEL_5,      /**< Timer channel 5 (if available) */
    TIM_CHANNEL_6,      /**< Timer channel 6 (if available) */
    TIM_CHANNEL_CNT     /**< Count of timer channels        */
}   tim_ChannelId_t;


typedef enum
{
    TIM_OUTPUT_1 = 0u, /**< Timer channel 1 output(if available)                */
    TIM_OUTPUT_1_N,    /**< Timer channel 1 complementary output (if available) */
    TIM_OUTPUT_2,      /**< Timer channel 2 output (if available)               */
    TIM_OUTPUT_2_N,    /**< Timer channel 2 complementary output (if available) */
    TIM_OUTPUT_3,      /**< Timer channel 3 output (if available)               */
    TIM_OUTPUT_3_N,    /**< Timer channel 3 complementary output (if available) */
    TIM_OUTPUT_4,      /**< Timer channel 4 output (if available)               */
    TIM_OUTPUT_4_N,    /**< Timer channel 4 complementary output (if available) */
    TIM_OUTPUT_5,      /**< Timer channel 5 output (if available)               */
    TIM_OUTPUT_6,      /**< Timer channel 6 output (if available)               */
    TIM_OUTPUT_CNT     /**< Count of timer channels outputs                     */
}   tim_OutputId_t;


/** Channel modes enumerations */
typedef enum
{
    TIM_CHANNEL_MODE_INPUT_CAPTURE = 0u,             /**< Channel configured as input (capture)  */
    TIM_CHANNEL_MODE_OUTPUT_FORCED_ACTIVE,           /**< OCyREF is forced high */
    TIM_CHANNEL_MODE_OUTPUT_FORCED_INACTIVE,         /**< OCyREF is forced low  */
    TIM_CHANNEL_MODE_OUTPUT_COMPARE_PULSE,           /**< Pulse on Compare mode (channels 3 / 4 only) */
    TIM_CHANNEL_MODE_OUTPUT_COMPARE_FORCED_ACTIVE,   /**< OCyREF is forced high on compare match */
    TIM_CHANNEL_MODE_OUTPUT_COMPARE_FORCED_INACTIVE, /**< OCyREF is forced low on compare match  */
    TIM_CHANNEL_MODE_OUTPUT_COMPARE_TOGGLE,          /**< OCyREF toggles on compare match        */
    TIM_CHANNEL_MODE_OUTPUT_PWM,                     /**< In upcounting, channel y is active as long as TIMx_CNT<TIMx_CCRy else inactive.  In downcounting, channel y is inactive as long as TIMx_CNT>TIMx_CCRy else active. */
    TIM_CHANNEL_MODE_OUTPUT_PWM_INV,                 /**< In upcounting, channel y is inactive as long as TIMx_CNT<TIMx_CCRy else active.  In downcounting, channel y is active as long as TIMx_CNT>TIMx_CCRy else inactive. */
    TIM_CHANNEL_MODE_OUTPUT_COMBINED_PWM,            /**< Combined PWM mode 1   */
    TIM_CHANNEL_MODE_OUTPUT_COMBINED_PWM_INV,        /**< Combined PWM mode 2   */
    TIM_CHANNEL_MODE_OUTPUT_ASSYMETRIC_PWM,          /**< Asymmetric PWM mode 1 */
    TIM_CHANNEL_MODE_OUTPUT_ASSYMETRIC_PWM_INV,      /**< Asymmetric PWM mode 2 */
    TIM_CHANNEL_MODE_OUTPUT_DIRECTION,               /**< Direction output mode (channels 3 / 4 only) */
    TIM_CHANNEL_MODE_OUTPUT_FROZEN,                  /**< Compare has no effect on the output (timing only) */
    TIM_CHANNEL_MODE_CNT                             /**< Count of channel modes */
}   tim_ChannelMode_t;


/** Channel modes enumerations */
typedef enum
{
    /** To select Encoder Interface mode write SMS=‘0001’ in the TIMx_SMCR register
     * if the counter is counting on tim_ti1 edges only, SMS=’0010’ if it is
     * counting on tim_ti2 edges only and SMS=’0011’ if it is counting on both
     * tim_ti1 and tim_ti2 edges. Select the tim_ti1 and tim_ti2 polarity by
     * programming the CC1P and CC2P bits in the TIMx_CCER register. When needed,
     * the input filter can be programmed as well. CC1NP and CC2NP must be kept low.
     * The two inputs tim_ti1 and tim_ti2 are used to interface to an quadrature
     * encoder. The counter is clocked by each valid transition on tim_ti1fp1 or
     * tim_ti2fp2 (tim_ti1 and tim_ti2 after input filter and polarity selection,
     * tim_ti1fp1=tim_ti1 if not filtered and not inverted, tim_ti2fp2=tim_ti2
     * if not filtered and not inverted) assuming that it is enabled (CEN bit in
     * TIMx_CR1 register written to ‘1’). The sequence of transitions of the two
     * inputs is evaluated and generates count pulses as well as the direction
     * signal. Depending on the sequence the counter counts up or down, the DIR
     * bit in the TIMx_CR1 register is modified by hardware accordingly. The DIR
     * bit is calculated at each transition on any input (tim_ti1 or tim_ti2),
     * whatever the counter is counting on tim_ti1 only, tim_ti2 only or both
     * tim_ti1 and tim_ti2.
     * Encoder interface mode acts simply as an external clock with direction
     * selection. This means that the counter just counts continuously between 0
     * and the auto-reload value in the TIMx_ARR register (0 to ARR or ARR down
     * to 0 depending on the direction). So the TIMx_ARR must be configured
     * before starting. In the same way, the capture, compare, prescaler,
     * repetition counter, trigger output features continue to work as normal.
     * Encoder mode and External clock mode 2 are not compatible and must not be
     * selected together. In this mode, the counter is modified automatically
     * following the speed and the direction of the quadrature encoder and its
     * content, therefore, always represents the encoder’s position. The count
     * direction correspond to the rotation direction of the connected sensor.
     * The table summarizes the possible combinations, assuming tim_ti1 and
     * tim_ti2 do not switch at the same time.
     */
    TIM_TIMER_MODE_INPUT_ENCODER = 0u,

    TIM_TIMER_MODE_ONE_PULSE,

    TIM_TIMER_MODE_RETRIG_ONE_PULSE,

//#define LL_TIM_OCMODE_RETRIG_OPM1       /*!<Retrigerrable One-Pulse mode 1*/
//#define LL_TIM_OCMODE_RETRIG_OPM2       /*!<Retrigerrable One-Pulse mode 2*/

    TIM_TIMER_MODE_XOR,


    TIM_MODE_INPUT_HAL_SENSOR,

    /** This mode allows to measure both the period and the duty cycle of a PWM
     * signal connected to single tim_tix input:
     * - The TIMx_CCR1 register holds the period value (interval between two
     *   consecutive rising edges)
     * - The TIM_CCR2 register holds the pulse-width (interval between two
     *   consecutive rising and falling edges
     * This mode is a particular case of input capture mode. The set-up procedure
     * is similar with the following differences:
     * - Two ICx signals are mapped on the same tim_tixfp1 input.
     * - These 2 ICx signals are active on edges with opposite polarity.
     * - One of the two tim_tixfp signals is selected as trigger input and the
     *   slave mode controller is configured in reset mode.
     * The period and the pulse-width of a PWM signal applied on tim_ti1 can be
     * measured using the following procedure:
     * - Select the active input for TIMx_CCR1: write the CC1S bits to 01 in the
     *   TIMx_CCMR1 register (tim_ti1 selected).
     * - Select the active polarity for tim_ti1fp1 (used both for capture in
     *   TIMx_CCR1 and counter clear): write the CC1P and CC1NP bits to ‘0’
     *   (active on rising edge).
     * - Select the active input for TIMx_CCR2: write the CC2S bits to 10 in the
     *   TIMx_CCMR1 register (tim_ti1 selected).
     * - Select the active polarity for tim_ti1fp2 (used for capture in TIMx_CCR2):
     *   write the CC2P and CC2NP bits to CC2P/CC2NP=’10’ (active on falling edge).
     * - Select the valid trigger input: write the TS bits to 00101 in the
     *   TIMx_SMCR register (tim_ti1fp1 selected).
     * - Configure the slave mode controller in reset mode: write the SMS bits
     *   to 0100 in the TIMx_SMCR register.
     * - Enable the captures: write the CC1E and CC2E bits to ‘1’ in the TIMx_CCER register.
     */
    TIM_TIMER_MODE_INPUT_PWM,
    TIM_TIMER_MODE_CNT
}   tim_TimerMode_t;


/**
 * \brief Enumeration of possible input filter configurations
 *
 * This configuration defines the frequency used to sample tim_ti1 input and the
 * length of the digital filter applied to tim_tix. The digital filter is made
 * of an event counter in which N consecutive events are needed to validate a
 * transition on the output.
 * fDTS -> Timer Dead Time and Sampling clock (Configured by TIMx_CR1)
 *
 */
typedef enum
{
    TIM_INPUT_FILTER_INACTIVE = 0u, /**< No filter, sampling is done at fDTS       */
    TIM_INPUT_FILTER_FDIV1_N2,      /**< fSAMPLING = internal clock, N = 2         */
    TIM_INPUT_FILTER_FDIV1_N4,      /**< fSAMPLING = internal clock, N = 4         */
    TIM_INPUT_FILTER_FDIV1_N8,      /**< fSAMPLING = internal clock, N = 8         */
    TIM_INPUT_FILTER_FDIV2_N6,      /**< fSAMPLING = fDTS / 2, N = 6               */
    TIM_INPUT_FILTER_FDIV2_N8,      /**< fSAMPLING = fDTS / 2, N = 8               */
    TIM_INPUT_FILTER_FDIV4_N6,      /**< fSAMPLING = fDTS / 4, N = 6               */
    TIM_INPUT_FILTER_FDIV4_N8,      /**< fSAMPLING = fDTS / 4, N = 8               */
    TIM_INPUT_FILTER_FDIV8_N6,      /**< fSAMPLING = fDTS / 8, N = 6               */
    TIM_INPUT_FILTER_FDIV8_N8,      /**< fSAMPLING = fDTS / 8, N = 8               */
    TIM_INPUT_FILTER_FDIV16_N5,     /**< fSAMPLING = fDTS / 16, N = 5              */
    TIM_INPUT_FILTER_FDIV16_N6,     /**< fSAMPLING = fDTS / 16, N = 6              */
    TIM_INPUT_FILTER_FDIV16_N8,     /**< fSAMPLING = fDTS / 16, N = 8              */
    TIM_INPUT_FILTER_FDIV32_N5,     /**< fSAMPLING = fDTS / 32, N = 5              */
    TIM_INPUT_FILTER_FDIV32_N6,     /**< fSAMPLING = fDTS / 32, N = 6              */
    TIM_INPUT_FILTER_FDIV32_N8,     /**< fSAMPLING = fDTS / 32, N = 8              */
    TIM_INPUT_FILTER_CNT            /**< Count of input filter configurations      */
}   tim_InputFilter_t;


/** \brief Input capture prescaler (capture done once every N events) */
typedef enum
{
    TIM_INPUT_PRESCALER_DIV1 = 0u,  /**< Capture on every event        */
    TIM_INPUT_PRESCALER_DIV2,       /**< Capture once every 2 events   */
    TIM_INPUT_PRESCALER_DIV4,       /**< Capture once every 4 events   */
    TIM_INPUT_PRESCALER_DIV8,       /**< Capture once every 8 events   */
    TIM_INPUT_PRESCALER_CNT         /**< Count of prescaler values     */
}   tim_InputPrescaler_t;


/** \brief Input capture active input selection */
typedef enum
{
    TIM_ACTIVE_INPUT_DIRECT = 0u,   /**< ICx is mapped on TIx (own channel input)            */
    TIM_ACTIVE_INPUT_INDIRECT,      /**< ICx is mapped on TIy (paired channel 1-2 / 3-4)      */
    TIM_ACTIVE_INPUT_TRC,           /**< ICx is mapped on TRC (trigger input)                 */
    TIM_ACTIVE_INPUT_CNT            /**< Count of active input selections                     */
}   tim_ActiveInput_t;


/**
 * \brief Timer input (TIx) source selection
 *
 * \ref TIM_INPUT_SOURCE_PIN selects the GPIO input, other values select internal
 * signals specific for each timer and input (e.g. LSE / LSI / HSE / MCO / RTC
 * wake-up / comparator outputs), see TIMx_TISEL register description in the
 * reference manual.
 */
typedef enum
{
    TIM_INPUT_SOURCE_PIN = 0u,      /**< TIx input pin (GPIO)       */
    TIM_INPUT_SOURCE_1,             /**< Timer specific source 1    */
    TIM_INPUT_SOURCE_2,             /**< Timer specific source 2    */
    TIM_INPUT_SOURCE_3,             /**< Timer specific source 3    */
    TIM_INPUT_SOURCE_4,             /**< Timer specific source 4    */
    TIM_INPUT_SOURCE_5,             /**< Timer specific source 5    */
    TIM_INPUT_SOURCE_6,             /**< Timer specific source 6    */
    TIM_INPUT_SOURCE_7,             /**< Timer specific source 7    */
    TIM_INPUT_SOURCE_8,             /**< Timer specific source 8    */
    TIM_INPUT_SOURCE_9,             /**< Timer specific source 9    */
    TIM_INPUT_SOURCE_10,            /**< Timer specific source 10   */
    TIM_INPUT_SOURCE_11,            /**< Timer specific source 11   */
    TIM_INPUT_SOURCE_12,            /**< Timer specific source 12   */
    TIM_INPUT_SOURCE_13,            /**< Timer specific source 13   */
    TIM_INPUT_SOURCE_14,            /**< Timer specific source 14   */
    TIM_INPUT_SOURCE_15,            /**< Timer specific source 15   */
    TIM_INPUT_SOURCE_CNT            /**< Count of input source selections */
}   tim_InputSource_t;


/** \brief Dead-time and sampling clock (fDTS) division of timer kernel clock */
typedef enum
{
    TIM_CLOCK_DIV_1 = 0u,           /**< fDTS = timer kernel clock      */
    TIM_CLOCK_DIV_2,                /**< fDTS = timer kernel clock / 2  */
    TIM_CLOCK_DIV_4,                /**< fDTS = timer kernel clock / 4  */
    TIM_CLOCK_DIV_CNT               /**< Count of clock divisions       */
}   tim_ClockDiv_t;


/** \brief Encoder interface counting mode */
typedef enum
{
    TIM_ENCODER_MODE_X2_TI1 = 0u,               /**< Quadrature, counts on TI1 edges (x2)                 */
    TIM_ENCODER_MODE_X2_TI2,                    /**< Quadrature, counts on TI2 edges (x2)                 */
    TIM_ENCODER_MODE_X4_TI12,                   /**< Quadrature, counts on TI1 and TI2 edges (x4)         */
    TIM_ENCODER_MODE_X1_TI1,                    /**< Quadrature, counts on TI1 single edge (x1)           */
    TIM_ENCODER_MODE_X1_TI2,                    /**< Quadrature, counts on TI2 single edge (x1)           */
    TIM_ENCODER_MODE_CLOCK_PLUS_DIRECTION_X2,   /**< Clock on TI2 (both edges), direction on TI1          */
    TIM_ENCODER_MODE_CLOCK_PLUS_DIRECTION_X1,   /**< Clock on TI2 (single edge), direction on TI1         */
    TIM_ENCODER_MODE_DIRECTIONAL_CLOCK_X2,      /**< Up clock on TI1, down clock on TI2 (both edges)      */
    TIM_ENCODER_MODE_DIRECTIONAL_CLOCK_X1,      /**< Up clock on TI1, down clock on TI2 (single edge)     */
    TIM_ENCODER_MODE_CNT                        /**< Count of encoder modes                               */
}   tim_EncoderMode_t;


/** \brief Encoder interface configuration */
typedef struct
{
    tim_EncoderMode_t   EncoderMode;    /**< Counting mode                                                  */
    tim_InputPolarity_t Ti1Polarity;    /**< TI1 polarity (\ref TIM_INPUT_POLARITY_NORMAL / INVERTED only)  */
    tim_InputPolarity_t Ti2Polarity;    /**< TI2 polarity (\ref TIM_INPUT_POLARITY_NORMAL / INVERTED only)  */
    tim_InputFilter_t   InputFilter;    /**< Digital filter of both inputs                                  */
    tim_Counter_t       Period;         /**< Auto-reload value (encoder counts per revolution - 1)          */
}   tim_EncoderConfig_t;


/** \brief Encoder index direction (counting direction in which index resets the counter) */
typedef enum
{
    TIM_INDEX_DIR_UP_DOWN = 0u,     /**< Index active in both counting directions */
    TIM_INDEX_DIR_UP,               /**< Index active when up-counting only        */
    TIM_INDEX_DIR_DOWN,             /**< Index active when down-counting only      */
    TIM_INDEX_DIR_CNT               /**< Count of index directions                 */
}   tim_IndexDir_t;


/**
 * \brief Encoder index position (quadrature state of TI1 / TI2 in which index resets the counter)
 *
 * In x1 / x2 encoder modes only the first position bit is used (DOWN_x = index
 * at TI1 or TI2 low, UP_x = index at high).
 */
typedef enum
{
    TIM_INDEX_POS_DOWN_DOWN = 0u,   /**< Index resets counter when AB = 00 */
    TIM_INDEX_POS_DOWN_UP,          /**< Index resets counter when AB = 01 */
    TIM_INDEX_POS_UP_DOWN,          /**< Index resets counter when AB = 10 */
    TIM_INDEX_POS_UP_UP,            /**< Index resets counter when AB = 11 */
    TIM_INDEX_POS_CNT               /**< Count of index positions          */
}   tim_IndexPos_t;


/** \brief Encoder index blanking */
typedef enum
{
    TIM_INDEX_BLANK_ALWAYS = 0u,    /**< Index always active                      */
    TIM_INDEX_BLANK_TI3,            /**< Index disabled while TI3 input is active */
    TIM_INDEX_BLANK_TI4,            /**< Index disabled while TI4 input is active */
    TIM_INDEX_BLANK_CNT             /**< Count of index blanking selections       */
}   tim_IndexBlank_t;


/** \brief Encoder index configuration (index signal on ETR input) */
typedef struct
{
    tim_FunctionState_t IndexState;     /**< Index function activation state                */
    tim_IndexDir_t      Direction;      /**< Counting direction(s) with active index        */
    tim_IndexPos_t      Position;       /**< Quadrature position of index                   */
    tim_IndexBlank_t    Blanking;       /**< Index blanking                                 */
    tim_FunctionState_t FirstIndexOnly; /**< Only first index resets the counter            */
}   tim_EncoderIndexConfig_t;


/** \brief Hall sensor interface configuration (sensors on TI1 / TI2 / TI3) */
typedef struct
{
    tim_InputPolarity_t  InputPolarity;     /**< Polarity of XOR-ed hall inputs                          */
    tim_InputFilter_t    InputFilter;       /**< Digital filter of hall inputs                           */
    tim_InputPrescaler_t InputPrescaler;    /**< Capture prescaler                                       */
    tim_Counter_t        CommutationDelay;  /**< Delay of commutation pulse (OC2REF) after hall edge [steps] */
}   tim_HallSensorConfig_t;


/** \brief Break input mode */
typedef enum
{
    TIM_BREAK_MODE_INPUT = 0u,      /**< Break input only                                            */
    TIM_BREAK_MODE_BIDIRECTIONAL,   /**< Break input / output (open drain, signals active break)     */
    TIM_BREAK_MODE_CNT              /**< Count of break modes                                        */
}   tim_BreakMode_t;


/** \brief Break (break 2) function configuration */
typedef struct
{
    tim_FunctionState_t BreakState;     /**< Break function activation state                 */
    tim_Polarity_t      BreakPolarity;  /**< Active level of break signal                    */
    tim_InputFilter_t   BreakFilter;    /**< Digital filter of break input                   */
    tim_BreakMode_t     BreakMode;      /**< Break input mode                                */
}   tim_BreakConfig_t;


/** \brief Register lock level (write protection until next reset) */
typedef enum
{
    TIM_LOCK_LEVEL_OFF = 0u,        /**< No write protection                                        */
    TIM_LOCK_LEVEL_1,               /**< Dead-time, break, idle states and off-state locked          */
    TIM_LOCK_LEVEL_2,               /**< Level 1 + channel polarity and off-state selection locked   */
    TIM_LOCK_LEVEL_3,               /**< Level 2 + output compare mode and preload locked            */
    TIM_LOCK_LEVEL_CNT              /**< Count of lock levels                                        */
}   tim_LockLevel_t;


/** \brief Update source of preloaded capture / compare control bits (commutation) */
typedef enum
{
    TIM_COMMUTATION_UPDATE_COMG = 0u,   /**< Update by COMG bit only (software commutation event)   */
    TIM_COMMUTATION_UPDATE_COMG_TRGI,   /**< Update by COMG bit or rising edge on trigger input     */
    TIM_COMMUTATION_UPDATE_CNT          /**< Count of commutation update sources                    */
}   tim_CommutationUpdate_t;

typedef enum
{
    TIM_RESOLUTION_16BIT = 0xFFFFu,    /**< Timer resolution 16bits */
    TIM_RESOLUTION_32BIT = 0xFFFFFFFFu /**< Timer resolution 32bits */
}   tim_Resolution_t;


typedef enum
{
    /** Slave mode disabled */
    TIM_SLAVE_MODE_DISABLE = 0u,

    /** External clock mode 1 - counter is clocked by rising edges of the
     * selected trigger input. This mode is configured as clock source
     * (\ref TIM_CLOCKSOURCE_EXTERNAL_CH_IN, \ref Tim_Set_ClockSource), it is only
     * reported by \ref Tim_Get_SlaveMode. External clock mode 2 (ETR) can be
     * combined with other slave modes. */
    TIM_SLAVE_MODE_EXTERNAL_CLOCK,

    /** The counter and its prescaler can be reinitialized in response to an event
     * on a trigger input. Moreover, if the URS bit from the TIMx_CR1 register
     * is low, an update event UEV is generated. Then all the preloaded registers
     * (TIMx_ARR, TIMx_CCRx) are updated. */
    TIM_SLAVE_MODE_RESET,

    /** The counter can be enabled depending on the level of a selected input. */
    TIM_SLAVE_MODE_GATED,

    /** The counter can start in response to an event on a selected input. */
    TIM_SLAVE_MODE_TRIGGER,

    /** In this case, a rising edge of the selected trigger input (tim_trgi)
     * reinitializes the counter, generates an update of the registers, and
     * starts the counter. This mode is used for One-pulse mode. */
    TIM_SLAVE_MODE_RESET_TRIGGER,

    /**< The counter clock is enabled when the trigger input (tim_trgi) is high.
     * The counter stops and is reset) as soon as the trigger becomes low. Both
     * start and stop of the counter are controlled. This mode allows to detect
     * out-of-range PWM signal (duty cycle exceeding a maximum expected value). */
    TIM_SLAVE_MODE_GATED_RESET,

    /** Count of slave modes */
    TIM_SLAVE_MODE_CNT
}   tim_SlaveMode_t;


/** \brief Master mode trigger output (TRGO) selection */
typedef enum
{
    TIM_MASTER_TRIGGER_RESET = 0u,  /**< UG bit / slave mode reset is used as trigger output   */
    TIM_MASTER_TRIGGER_ENABLE,      /**< Counter enable signal is used as trigger output      */
    TIM_MASTER_TRIGGER_UPDATE,      /**< Update event is used as trigger output               */
    TIM_MASTER_TRIGGER_CC1,         /**< Capture / compare 1 event (CC1IF set) as trigger     */
    TIM_MASTER_TRIGGER_OC1REF,      /**< OC1REF signal is used as trigger output              */
    TIM_MASTER_TRIGGER_OC2REF,      /**< OC2REF signal is used as trigger output              */
    TIM_MASTER_TRIGGER_OC3REF,      /**< OC3REF signal is used as trigger output              */
    TIM_MASTER_TRIGGER_OC4REF,      /**< OC4REF signal is used as trigger output              */
    TIM_MASTER_TRIGGER_ENCODER_CLK, /**< Encoder clock is used as trigger output              */
    TIM_MASTER_TRIGGER_CNT          /**< Count of trigger output selections                   */
}   tim_MasterTrigger_t;


/** \brief Master mode trigger output 2 (TRGO2, TIM1 / TIM8 only, e.g. ADC trigger) selection */
typedef enum
{
    TIM_MASTER_TRIGGER2_RESET = 0u,                 /**< UG bit / slave mode reset is used as trigger output 2 */
    TIM_MASTER_TRIGGER2_ENABLE,                     /**< Counter enable signal                                 */
    TIM_MASTER_TRIGGER2_UPDATE,                     /**< Update event                                          */
    TIM_MASTER_TRIGGER2_CC1,                        /**< Capture / compare 1 event                             */
    TIM_MASTER_TRIGGER2_OC1REF,                     /**< OC1REF signal                                         */
    TIM_MASTER_TRIGGER2_OC2REF,                     /**< OC2REF signal                                         */
    TIM_MASTER_TRIGGER2_OC3REF,                     /**< OC3REF signal                                         */
    TIM_MASTER_TRIGGER2_OC4REF,                     /**< OC4REF signal                                         */
    TIM_MASTER_TRIGGER2_OC5REF,                     /**< OC5REF signal                                         */
    TIM_MASTER_TRIGGER2_OC6REF,                     /**< OC6REF signal                                         */
    TIM_MASTER_TRIGGER2_OC4REF_RISING_FALLING,      /**< OC4REF rising or falling edges                        */
    TIM_MASTER_TRIGGER2_OC6REF_RISING_FALLING,      /**< OC6REF rising or falling edges                        */
    TIM_MASTER_TRIGGER2_OC4REF_RISING_OC6REF_RISING,  /**< OC4REF or OC6REF rising edges                       */
    TIM_MASTER_TRIGGER2_OC4REF_RISING_OC6REF_FALLING, /**< OC4REF rising or OC6REF falling edges               */
    TIM_MASTER_TRIGGER2_OC5REF_RISING_OC6REF_RISING,  /**< OC5REF or OC6REF rising edges                       */
    TIM_MASTER_TRIGGER2_OC5REF_RISING_OC6REF_FALLING, /**< OC5REF rising or OC6REF falling edges               */
    TIM_MASTER_TRIGGER2_CNT                         /**< Count of trigger output 2 selections                  */
}   tim_MasterTrigger2_t;


/** \brief Timer DMA request identification (also DMA burst source) */
typedef enum
{
    TIM_DMA_REQUEST_UPDATE = 0u,    /**< DMA request on update event                        */
    TIM_DMA_REQUEST_CC1,            /**< DMA request on capture / compare 1 event           */
    TIM_DMA_REQUEST_CC2,            /**< DMA request on capture / compare 2 event           */
    TIM_DMA_REQUEST_CC3,            /**< DMA request on capture / compare 3 event           */
    TIM_DMA_REQUEST_CC4,            /**< DMA request on capture / compare 4 event           */
    TIM_DMA_REQUEST_COMMUTATION,    /**< DMA request on commutation event                   */
    TIM_DMA_REQUEST_TRIGGER,        /**< DMA request on trigger event                       */
    TIM_DMA_REQUEST_CNT             /**< Count of DMA requests                              */
}   tim_DmaRequest_t;


/** \brief First register of DMA burst transfer (DMA burst base address) */
typedef enum
{
    TIM_DMA_BURST_REG_CR1 = 0u,     /**< TIMx_CR1   */
    TIM_DMA_BURST_REG_CR2,          /**< TIMx_CR2   */
    TIM_DMA_BURST_REG_SMCR,         /**< TIMx_SMCR  */
    TIM_DMA_BURST_REG_DIER,         /**< TIMx_DIER  */
    TIM_DMA_BURST_REG_SR,           /**< TIMx_SR    */
    TIM_DMA_BURST_REG_EGR,          /**< TIMx_EGR   */
    TIM_DMA_BURST_REG_CCMR1,        /**< TIMx_CCMR1 */
    TIM_DMA_BURST_REG_CCMR2,        /**< TIMx_CCMR2 */
    TIM_DMA_BURST_REG_CCER,         /**< TIMx_CCER  */
    TIM_DMA_BURST_REG_CNT_REG,      /**< TIMx_CNT   */
    TIM_DMA_BURST_REG_PSC,          /**< TIMx_PSC   */
    TIM_DMA_BURST_REG_ARR,          /**< TIMx_ARR   */
    TIM_DMA_BURST_REG_RCR,          /**< TIMx_RCR   */
    TIM_DMA_BURST_REG_CCR1,         /**< TIMx_CCR1  */
    TIM_DMA_BURST_REG_CCR2,         /**< TIMx_CCR2  */
    TIM_DMA_BURST_REG_CCR3,         /**< TIMx_CCR3  */
    TIM_DMA_BURST_REG_CCR4,         /**< TIMx_CCR4  */
    TIM_DMA_BURST_REG_BDTR,         /**< TIMx_BDTR  */
    TIM_DMA_BURST_REG_CCR5,         /**< TIMx_CCR5  */
    TIM_DMA_BURST_REG_CCR6,         /**< TIMx_CCR6  */
    TIM_DMA_BURST_REG_CCMR3,        /**< TIMx_CCMR3 */
    TIM_DMA_BURST_REG_DTR2,         /**< TIMx_DTR2  */
    TIM_DMA_BURST_REG_ECR,          /**< TIMx_ECR   */
    TIM_DMA_BURST_REG_TISEL,        /**< TIMx_TISEL */
    TIM_DMA_BURST_REG_AF1,          /**< TIMx_AF1   */
    TIM_DMA_BURST_REG_AF2,          /**< TIMx_AF2   */
    TIM_DMA_BURST_REG_OR1,          /**< TIMx_OR1   */
    TIM_DMA_BURST_REG_CNT           /**< Count of DMA burst base registers */
}   tim_DmaBurstReg_t;


/** \brief Count of registers transferred in one DMA burst (1 - 26) */
typedef uint8_t tim_DmaBurstLen_t;


/** \brief Timer register used as DMA transfer destination / source */
typedef enum
{
    TIM_DMA_REG_CCR1 = 0u,          /**< Capture / compare register 1                      */
    TIM_DMA_REG_CCR2,               /**< Capture / compare register 2                      */
    TIM_DMA_REG_CCR3,               /**< Capture / compare register 3                      */
    TIM_DMA_REG_CCR4,               /**< Capture / compare register 4                      */
    TIM_DMA_REG_ARR,                /**< Auto-reload register                              */
    TIM_DMA_REG_DMAR,               /**< DMA burst access register                         */
    TIM_DMA_REG_CNT                 /**< Count of DMA registers                            */
}   tim_DmaReg_t;


/** \brief Peripheral register address type (DMA transfer address) */
typedef uint32_t tim_RegAddr_t;


/** \brief External trigger (ETR) prescaler */
typedef enum
{
    TIM_ETR_PRESCALER_DIV1 = 0u,    /**< ETR prescaler off                 */
    TIM_ETR_PRESCALER_DIV2,         /**< ETR frequency divided by 2        */
    TIM_ETR_PRESCALER_DIV4,         /**< ETR frequency divided by 4        */
    TIM_ETR_PRESCALER_DIV8,         /**< ETR frequency divided by 8        */
    TIM_ETR_PRESCALER_CNT           /**< Count of ETR prescaler values     */
}   tim_EtrPrescaler_t;


/**
 * \brief External trigger (ETR) source selection
 *
 * \ref TIM_ETR_SOURCE_PIN selects the ETR pin, other values select internal
 * signals specific for each timer (e.g. comparator outputs, ADC analog
 * watchdogs, LSE), see TIMx_AF1 ETRSEL description in the reference manual.
 */
typedef enum
{
    TIM_ETR_SOURCE_PIN = 0u,        /**< ETR input pin (GPIO)       */
    TIM_ETR_SOURCE_1,               /**< Timer specific source 1    */
    TIM_ETR_SOURCE_2,               /**< Timer specific source 2    */
    TIM_ETR_SOURCE_3,               /**< Timer specific source 3    */
    TIM_ETR_SOURCE_4,               /**< Timer specific source 4    */
    TIM_ETR_SOURCE_5,               /**< Timer specific source 5    */
    TIM_ETR_SOURCE_6,               /**< Timer specific source 6    */
    TIM_ETR_SOURCE_7,               /**< Timer specific source 7    */
    TIM_ETR_SOURCE_8,               /**< Timer specific source 8    */
    TIM_ETR_SOURCE_9,               /**< Timer specific source 9    */
    TIM_ETR_SOURCE_10,              /**< Timer specific source 10   */
    TIM_ETR_SOURCE_11,              /**< Timer specific source 11   */
    TIM_ETR_SOURCE_12,              /**< Timer specific source 12   */
    TIM_ETR_SOURCE_13,              /**< Timer specific source 13   */
    TIM_ETR_SOURCE_14,              /**< Timer specific source 14   */
    TIM_ETR_SOURCE_15,              /**< Timer specific source 15   */
    TIM_ETR_SOURCE_CNT              /**< Count of ETR source selections */
}   tim_EtrSource_t;


typedef enum
{
    /** Internal trigger input bus. These inputs can be used for the slave mode
     * controller or as a input clock (below 1/4 of the tim_ker_ck clock). */
    TIM_EXT_CLK_SOURCE_ITR0 = LL_TIM_TS_ITR0,

    /** Internal trigger input bus. These inputs can be used for the slave mode
     * controller or as a input clock (below 1/4 of the tim_ker_ck clock). */
    TIM_EXT_CLK_SOURCE_ITR1 = LL_TIM_TS_ITR1,

    /** Internal trigger input bus. These inputs can be used for the slave mode
     * controller or as a input clock (below 1/4 of the tim_ker_ck clock). */
    TIM_EXT_CLK_SOURCE_ITR2 = LL_TIM_TS_ITR2,

    /** Internal trigger input bus. These inputs can be used for the slave mode
     * controller or as a input clock (below 1/4 of the tim_ker_ck clock). */
    TIM_EXT_CLK_SOURCE_ITR3 = LL_TIM_TS_ITR3,

    /** Internal trigger input bus. These inputs can be used for the slave mode
     * controller or as a input clock (below 1/4 of the tim_ker_ck clock). */
    TIM_EXT_CLK_SOURCE_ITR4 = LL_TIM_TS_ITR4,

    /** Internal trigger input bus. These inputs can be used for the slave mode
     * controller or as a input clock (below 1/4 of the tim_ker_ck clock). */
    TIM_EXT_CLK_SOURCE_ITR5 = LL_TIM_TS_ITR5,

    /** Internal trigger input bus. These inputs can be used for the slave mode
     * controller or as a input clock (below 1/4 of the tim_ker_ck clock). */
    TIM_EXT_CLK_SOURCE_ITR6 = LL_TIM_TS_ITR6,

    /** Internal trigger input bus. These inputs can be used for the slave mode
     * controller or as a input clock (below 1/4 of the tim_ker_ck clock). */
    TIM_EXT_CLK_SOURCE_ITR7 = LL_TIM_TS_ITR7,

    /** Internal trigger input bus. These inputs can be used for the slave mode
     * controller or as a input clock (below 1/4 of the tim_ker_ck clock). */
    TIM_EXT_CLK_SOURCE_ITR8 = LL_TIM_TS_ITR8,

    /** Internal trigger input bus. These inputs can be used for the slave mode
     * controller or as a input clock (below 1/4 of the tim_ker_ck clock). */
    TIM_EXT_CLK_SOURCE_ITR11 = LL_TIM_TS_ITR11,

    /** External trigger internal input bus. These inputs can be used as
     * trigger, external clock or for hardware cycle-by-cycle pulsewidth control.
     * These inputs can receive clock with a frequency higher than the
     * tim_ker_ck if the tim_etr_in prescaler is used. */
    TIM_EXT_CLK_SOURCE_ETR1 = LL_TIM_TS_ETRF,

    /** Filtered external Trigger (ETRF) is used as trigger input */
    TIM_EXT_CLK_SOURCE_TI1_ED = LL_TIM_TS_TI1F_ED,

    /** Filtered Timer Input 1 (TI1FP1) is used as trigger input */
    TIM_EXT_CLK_SOURCE_TI1FP1 = LL_TIM_TS_TI1FP1,

    /*!< Filtered Timer Input 2 (TI12P2) is used as trigger input */
    TIM_EXT_CLK_SOURCE_TI2FP2 = LL_TIM_TS_TI2FP2,

}   tim_ExtClkSource_t;


/** \brief Trigger input (TRGI) selection of slave mode controller, values from \ref tim_ExtClkSource_t */
typedef tim_ExtClkSource_t tim_TriggerInput_t;


typedef enum
{
    /** The timer is clocked by the internal clock provided from the RCC */
    TIM_CLOCKSOURCE_INT_CLK = LL_TIM_CLOCKSOURCE_INTERNAL,

    /** Counter counts at each rising or falling edge on a selected input */
    TIM_CLOCKSOURCE_EXTERNAL_CH_IN = LL_TIM_CLOCKSOURCE_EXT_MODE1,

    /** Counter counts at each rising or falling edge on the external trigger input ETR */
    TIM_CLOCKSOURCE_EXTERNAL_ETR = LL_TIM_CLOCKSOURCE_EXT_MODE2,
}   tim_ClockSource_t;


/**
 * \brief Timer interrupt request (and status flag) identification
 *
 * Availability depends on the timer: capture / compare channels, commutation
 * and break interrupts only on timers with the feature, index / direction /
 * error interrupts only on timers with encoder interface.
 */
typedef enum
{
    TIM_IRQ_UPDATE = 0u,            /**< Update interrupt                                            */
    TIM_IRQ_CAPTURE_COMPARE_CH1,    /**< Capture / compare channel 1 interrupt                       */
    TIM_IRQ_CAPTURE_COMPARE_CH2,    /**< Capture / compare channel 2 interrupt                       */
    TIM_IRQ_CAPTURE_COMPARE_CH3,    /**< Capture / compare channel 3 interrupt                       */
    TIM_IRQ_CAPTURE_COMPARE_CH4,    /**< Capture / compare channel 4 interrupt                       */
    TIM_IRQ_COMMUTATION,            /**< Commutation interrupt                                       */
    TIM_IRQ_TRIGGER,                /**< Trigger interrupt                                           */
    TIM_IRQ_INDEX,                  /**< Encoder index interrupt                                     */
    TIM_IRQ_DIRECTION,              /**< Encoder direction change interrupt                          */
    TIM_IRQ_BREAK,                  /**< Break interrupt (shares enable with break 2 / system break) */
    TIM_IRQ_BREAK2,                 /**< Break 2 interrupt                                           */
    TIM_IRQ_SYSTEM_BREAK,           /**< System break interrupt                                      */
    TIM_IRQ_ERROR,                  /**< Encoder index / transition error interrupt                  */
    TIM_IRQ_CNT                     /**< Count of timer interrupt requests                           */
}   tim_IrqId_t;


/**
 * \brief Interrupt priority type
 *
 * Range 0 (highest) to 2^NVIC_PRIO_BITS - 1 (lowest), validated by NVIC module.
 */
typedef uint32_t tim_IrqPrio_t;



/** \brief Enumeration of all possible Channel inputs/outputs (Ch) */
typedef enum
{
    /*----------------------------- Timer 1 pins -----------------------------*/
#ifdef TIM1
    TIM_1_CH1_PA8     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_1  , GPIO_PORT_A  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_1   ),
#if defined (GPIOE)
    TIM_1_CH1_PE9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_1  , GPIO_PORT_E  , GPIO_PIN_ID_9  , GPIO_ALT_FUNC_1   ),
#endif
#if !defined( STM32H503xx ) && !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_1_CH1_PH11    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_1  , GPIO_PORT_H  , GPIO_PIN_ID_11 , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_1_CH1_PI4     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_1  , GPIO_PORT_I  , GPIO_PIN_ID_4  , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H503xx )
    TIM_1_CH1_PA13    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_1  , GPIO_PORT_A  , GPIO_PIN_ID_13 , GPIO_ALT_FUNC_1   ),
    TIM_1_CH1_PB1     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_1  , GPIO_ALT_FUNC_14  ),
    TIM_1_CH1_PB7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_14  ),
    TIM_1_CH1_PC6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_1  , GPIO_PORT_C  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_1   ),
#endif

    TIM_1_CH2_PA9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_1  , GPIO_PORT_A  , GPIO_PIN_ID_9  , GPIO_ALT_FUNC_1   ),
#if defined (GPIOE)
    TIM_1_CH2_PE11    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_1  , GPIO_PORT_E  , GPIO_PIN_ID_11 , GPIO_ALT_FUNC_1   ),
#endif
#if !defined( STM32H503xx ) && !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_1_CH2_PH9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_1  , GPIO_PORT_H  , GPIO_PIN_ID_9  , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_1_CH2_PI6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_1  , GPIO_PORT_I  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H503xx )
    TIM_1_CH2_PA14    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_1  , GPIO_PORT_A  , GPIO_PIN_ID_14 , GPIO_ALT_FUNC_1   ),
    TIM_1_CH2_PB4     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_4  , GPIO_ALT_FUNC_14  ),
    TIM_1_CH2_PB6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_14  ),
    TIM_1_CH2_PC7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_1  , GPIO_PORT_C  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_1   ),
#endif

    TIM_1_CH3_PA10    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_1  , GPIO_PORT_A  , GPIO_PIN_ID_10 , GPIO_ALT_FUNC_1   ),
#if defined (GPIOE)
    TIM_1_CH3_PE13    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_1  , GPIO_PORT_E  , GPIO_PIN_ID_13 , GPIO_ALT_FUNC_1   ),
#endif
#if !defined( STM32H503xx ) && !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_1_CH3_PH7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_1  , GPIO_PORT_H  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_1_CH3_PI9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_1  , GPIO_PORT_I  , GPIO_PIN_ID_9  , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H503xx )
    TIM_1_CH3_PA1     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_1  , GPIO_PORT_A  , GPIO_PIN_ID_1  , GPIO_ALT_FUNC_14  ),
    TIM_1_CH3_PB5     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_5  , GPIO_ALT_FUNC_1   ),
    TIM_1_CH3_PC8     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_1  , GPIO_PORT_C  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_1   ),
#endif

    TIM_1_CH4_PA11    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_1  , GPIO_PORT_A  , GPIO_PIN_ID_11 , GPIO_ALT_FUNC_1   ),
#if defined (GPIOE)
    TIM_1_CH4_PE14    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_1  , GPIO_PORT_E  , GPIO_PIN_ID_14 , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_1_CH4_PI11    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_1  , GPIO_PORT_I  , GPIO_PIN_ID_11 , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H503xx )
    TIM_1_CH4_PA2     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_1  , GPIO_PORT_A  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_14  ),
    TIM_1_CH4_PC9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_1  , GPIO_PORT_C  , GPIO_PIN_ID_9  , GPIO_ALT_FUNC_1   ),
    TIM_1_CH4_PC12    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_1  , GPIO_PORT_C  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_14  ),
#endif
#endif
    /*----------------------------- Timer 2 pins -----------------------------*/
#ifdef TIM2
    TIM_2_CH1_PA0     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_2  , GPIO_PORT_A  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_1   ),
    TIM_2_CH1_PA5     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_2  , GPIO_PORT_A  , GPIO_PIN_ID_5  , GPIO_ALT_FUNC_1   ),
    TIM_2_CH1_PA15    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_2  , GPIO_PORT_A  , GPIO_PIN_ID_15 , GPIO_ALT_FUNC_1   ),
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_2_CH1_PF0     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_2  , GPIO_PORT_F  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H503xx )
    TIM_2_CH1_PB2     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_2  , GPIO_PORT_B  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_14  ),
#endif

    TIM_2_CH2_PA1     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_2  , GPIO_PORT_A  , GPIO_PIN_ID_1  , GPIO_ALT_FUNC_1   ),
    TIM_2_CH2_PB3     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_2  , GPIO_PORT_B  , GPIO_PIN_ID_3  , GPIO_ALT_FUNC_1   ),
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_2_CH2_PF1     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_2  , GPIO_PORT_F  , GPIO_PIN_ID_1  , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H503xx )
    TIM_2_CH2_PC11    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_2  , GPIO_PORT_C  , GPIO_PIN_ID_11 , GPIO_ALT_FUNC_1   ),
#endif

    TIM_2_CH3_PA2     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_2  , GPIO_PORT_A  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_1   ),
    TIM_2_CH3_PB10    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_2  , GPIO_PORT_B  , GPIO_PIN_ID_10 , GPIO_ALT_FUNC_1   ),
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_2_CH3_PF3     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_2  , GPIO_PORT_F  , GPIO_PIN_ID_3  , GPIO_ALT_FUNC_1   ),
    TIM_2_CH3_PF11    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_2  , GPIO_PORT_F  , GPIO_PIN_ID_11 , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H503xx )
    TIM_2_CH3_PA7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_2  , GPIO_PORT_A  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_14  ),
    TIM_2_CH3_PD2     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_2  , GPIO_PORT_D  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_1   ),
#endif

    TIM_2_CH4_PA3     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_2  , GPIO_PORT_A  , GPIO_PIN_ID_3  , GPIO_ALT_FUNC_1   ),
    TIM_2_CH4_PC4     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_2  , GPIO_PORT_C  , GPIO_PIN_ID_4  , GPIO_ALT_FUNC_1   ),
#if !defined( STM32H503xx ) && !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_2_CH4_PB11    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_2  , GPIO_PORT_B  , GPIO_PIN_ID_11 , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_2_CH4_PF4     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_2  , GPIO_PORT_F  , GPIO_PIN_ID_4  , GPIO_ALT_FUNC_1   ),
    TIM_2_CH4_PF12    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_2  , GPIO_PORT_F  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H503xx )
    TIM_2_CH4_PA12    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_2  , GPIO_PORT_A  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_14  ),
    TIM_2_CH4_PC12    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_2  , GPIO_PORT_C  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_1   ),
#endif
#endif
    /*----------------------------- Timer 3 pins -----------------------------*/
#ifdef TIM3
    TIM_3_CH1_PA6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_3  , GPIO_PORT_A  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_2   ),
    TIM_3_CH1_PB4     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_3  , GPIO_PORT_B  , GPIO_PIN_ID_4  , GPIO_ALT_FUNC_2   ),
    TIM_3_CH1_PC6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_3  , GPIO_PORT_C  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_2   ),
#if defined (GPIOK)
    TIM_3_CH1_PK11    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_3  , GPIO_PORT_K  , GPIO_PIN_ID_11 , GPIO_ALT_FUNC_2   ),
#endif
#if defined( STM32H503xx )
    TIM_3_CH1_PA0     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_3  , GPIO_PORT_A  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_2   ),
    TIM_3_CH1_PA14    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_3  , GPIO_PORT_A  , GPIO_PIN_ID_14 , GPIO_ALT_FUNC_2   ),
#endif

    TIM_3_CH2_PA7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_3  , GPIO_PORT_A  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_2   ),
    TIM_3_CH2_PB5     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_3  , GPIO_PORT_B  , GPIO_PIN_ID_5  , GPIO_ALT_FUNC_2   ),
    TIM_3_CH2_PC7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_3  , GPIO_PORT_C  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_2   ),
#if defined (GPIOK)
    TIM_3_CH2_PK12    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_3  , GPIO_PORT_K  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_2   ),
#endif
#if defined( STM32H503xx )
    TIM_3_CH2_PA11    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_3  , GPIO_PORT_A  , GPIO_PIN_ID_11 , GPIO_ALT_FUNC_2   ),
#endif

    TIM_3_CH3_PB0     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_3  , GPIO_PORT_B  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_2   ),
    TIM_3_CH3_PC8     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_3  , GPIO_PORT_C  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_2   ),
#if defined (GPIOK)
    TIM_3_CH3_PK13    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_3  , GPIO_PORT_K  , GPIO_PIN_ID_13 , GPIO_ALT_FUNC_2   ),
#endif
#if defined( STM32H503xx )
    TIM_3_CH3_PA8     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_3  , GPIO_PORT_A  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_2   ),
    TIM_3_CH3_PB6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_3  , GPIO_PORT_B  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_2   ),
#endif

    TIM_3_CH4_PB1     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_3  , GPIO_PORT_B  , GPIO_PIN_ID_1  , GPIO_ALT_FUNC_2   ),
    TIM_3_CH4_PC9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_3  , GPIO_PORT_C  , GPIO_PIN_ID_9  , GPIO_ALT_FUNC_2   ),
#if defined (GPIOK)
    TIM_3_CH4_PK14    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_3  , GPIO_PORT_K  , GPIO_PIN_ID_14 , GPIO_ALT_FUNC_2   ),
#endif
#if defined( STM32H503xx )
    TIM_3_CH4_PA12    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_3  , GPIO_PORT_A  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_2   ),
    TIM_3_CH4_PB15    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_3  , GPIO_PORT_B  , GPIO_PIN_ID_15 , GPIO_ALT_FUNC_14  ),
#endif
#endif
    /*----------------------------- Timer 4 pins -----------------------------*/
#ifdef TIM4
    TIM_4_CH1_PB6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_4  , GPIO_PORT_B  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_2   ),
    TIM_4_CH1_PD12    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_4  , GPIO_PORT_D  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_2   ),
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_4_CH1_PG0     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_4  , GPIO_PORT_G  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_2   ),
#endif
#if defined (GPIOJ)
    TIM_4_CH1_PJ3     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_4  , GPIO_PORT_J  , GPIO_PIN_ID_3  , GPIO_ALT_FUNC_2   ),
#endif

    TIM_4_CH2_PB7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_4  , GPIO_PORT_B  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_2   ),
    TIM_4_CH2_PD13    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_4  , GPIO_PORT_D  , GPIO_PIN_ID_13 , GPIO_ALT_FUNC_2   ),
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_4_CH2_PG1     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_4  , GPIO_PORT_G  , GPIO_PIN_ID_1  , GPIO_ALT_FUNC_2   ),
#endif
#if defined (GPIOJ)
    TIM_4_CH2_PJ4     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_4  , GPIO_PORT_J  , GPIO_PIN_ID_4  , GPIO_ALT_FUNC_2   ),
#endif

    TIM_4_CH3_PB8     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_4  , GPIO_PORT_B  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_2   ),
    TIM_4_CH3_PD14    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_4  , GPIO_PORT_D  , GPIO_PIN_ID_14 , GPIO_ALT_FUNC_2   ),
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_4_CH3_PG2     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_4  , GPIO_PORT_G  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_2   ),
#endif
#if defined (GPIOJ)
    TIM_4_CH3_PJ5     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_4  , GPIO_PORT_J  , GPIO_PIN_ID_5  , GPIO_ALT_FUNC_2   ),
#endif

    TIM_4_CH4_PB9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_4  , GPIO_PORT_B  , GPIO_PIN_ID_9  , GPIO_ALT_FUNC_2   ),
    TIM_4_CH4_PC2     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_4  , GPIO_PORT_C  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_2   ),
    TIM_4_CH4_PD15    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_4  , GPIO_PORT_D  , GPIO_PIN_ID_15 , GPIO_ALT_FUNC_2   ),
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_4_CH4_PG3     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_4  , GPIO_PORT_G  , GPIO_PIN_ID_3  , GPIO_ALT_FUNC_2   ),
#endif
#if defined (GPIOJ)
    TIM_4_CH4_PJ6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_4  , GPIO_PORT_J  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_2   ),
#endif
#endif
    /*----------------------------- Timer 5 pins -----------------------------*/
#ifdef TIM5
    TIM_5_CH1_PA0     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_5  , GPIO_PORT_A  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_2   ),
#if !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_5_CH1_PH10    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_5  , GPIO_PORT_H  , GPIO_PIN_ID_10 , GPIO_ALT_FUNC_2   ),
#endif
#if defined (GPIOJ)
    TIM_5_CH1_PJ7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_5  , GPIO_PORT_J  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_2   ),
#endif

    TIM_5_CH2_PA1     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_5  , GPIO_PORT_A  , GPIO_PIN_ID_1  , GPIO_ALT_FUNC_2   ),
#if !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_5_CH2_PH11    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_5  , GPIO_PORT_H  , GPIO_PIN_ID_11 , GPIO_ALT_FUNC_2   ),
#endif
#if defined (GPIOJ)
    TIM_5_CH2_PJ8     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_5  , GPIO_PORT_J  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_2   ),
#endif

    TIM_5_CH3_PA2     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_5  , GPIO_PORT_A  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_2   ),
#if !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_5_CH3_PH12    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_5  , GPIO_PORT_H  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_2   ),
#endif
#if defined (GPIOJ)
    TIM_5_CH3_PJ9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_5  , GPIO_PORT_J  , GPIO_PIN_ID_9  , GPIO_ALT_FUNC_2   ),
#endif

    TIM_5_CH4_PA3     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_5  , GPIO_PORT_A  , GPIO_PIN_ID_3  , GPIO_ALT_FUNC_2   ),
#if defined (GPIOI)
    TIM_5_CH4_PI0     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_5  , GPIO_PORT_I  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_2   ),
#endif
#if defined (GPIOJ)
    TIM_5_CH4_PJ10    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_5  , GPIO_PORT_J  , GPIO_PIN_ID_10 , GPIO_ALT_FUNC_2   ),
#endif
#endif
    /*----------------------------- Timer 8 pins -----------------------------*/
#ifdef TIM8
    TIM_8_CH1_PC6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_8  , GPIO_PORT_C  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_3   ),
#if !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_8_CH1_PH6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_8  , GPIO_PORT_H  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_3   ),
#endif
#if defined (GPIOI)
    TIM_8_CH1_PI5     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_8  , GPIO_PORT_I  , GPIO_PIN_ID_5  , GPIO_ALT_FUNC_3   ),
#endif
#if defined( STM32H523xx ) || defined( STM32H533xx ) || defined( STM32H543xx ) || defined( STM32H553xx )
    TIM_8_CH1_PB10    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_8  , GPIO_PORT_B  , GPIO_PIN_ID_10 , GPIO_ALT_FUNC_2   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_8_CH1_PC0     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_8  , GPIO_PORT_C  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_3   ),
#endif
#if defined (GPIOK)
    TIM_8_CH1_PK0     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_8  , GPIO_PORT_K  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_3   ),
#endif

    TIM_8_CH2_PC7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_8  , GPIO_PORT_C  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_3   ),
#if !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_8_CH2_PH8     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_8  , GPIO_PORT_H  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_3   ),
#endif
#if defined (GPIOI)
    TIM_8_CH2_PI6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_8  , GPIO_PORT_I  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_3   ),
#endif
#if defined( STM32H523xx ) || defined( STM32H533xx ) || defined( STM32H543xx ) || defined( STM32H553xx )
    TIM_8_CH2_PB13    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_8  , GPIO_PORT_B  , GPIO_PIN_ID_13 , GPIO_ALT_FUNC_2   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_8_CH2_PC1     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_8  , GPIO_PORT_C  , GPIO_PIN_ID_1  , GPIO_ALT_FUNC_3   ),
#endif
#if defined (GPIOK)
    TIM_8_CH2_PK2     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_8  , GPIO_PORT_K  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_3   ),
#endif

    TIM_8_CH3_PC8     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_8  , GPIO_PORT_C  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_3   ),
#if !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_8_CH3_PH10    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_8  , GPIO_PORT_H  , GPIO_PIN_ID_10 , GPIO_ALT_FUNC_3   ),
#endif
#if defined (GPIOI)
    TIM_8_CH3_PI7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_8  , GPIO_PORT_I  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_3   ),
#endif
#if defined( STM32H523xx ) || defined( STM32H533xx ) || defined( STM32H543xx ) || defined( STM32H553xx )
    TIM_8_CH3_PB12    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_8  , GPIO_PORT_B  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_2   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_8_CH3_PC2     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_8  , GPIO_PORT_C  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_3   ),
#endif
#if defined (GPIOK)
    TIM_8_CH3_PK4     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_8  , GPIO_PORT_K  , GPIO_PIN_ID_4  , GPIO_ALT_FUNC_3   ),
#endif

    TIM_8_CH4_PC9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_8  , GPIO_PORT_C  , GPIO_PIN_ID_9  , GPIO_ALT_FUNC_3   ),
#if defined (GPIOI)
    TIM_8_CH4_PI2     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_8  , GPIO_PORT_I  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_3   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_8_CH4_PD3     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_8  , GPIO_PORT_D  , GPIO_PIN_ID_3  , GPIO_ALT_FUNC_3   ),
#endif
#if defined (GPIOK)
    TIM_8_CH4_PK6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_8  , GPIO_PORT_K  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_3   ),
#endif
#endif
    /*---------------------------- Timer 12 pins -----------------------------*/
#ifdef TIM12
    TIM_12_CH1_PB14   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_12 , GPIO_PORT_B  , GPIO_PIN_ID_14 , GPIO_ALT_FUNC_2   ),
#if !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_12_CH1_PH6    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_12 , GPIO_PORT_H  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_2   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_12_CH1_PD7    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_12 , GPIO_PORT_D  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_2   ),
    TIM_12_CH1_PG9    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_12 , GPIO_PORT_G  , GPIO_PIN_ID_9  , GPIO_ALT_FUNC_2   ),
#endif
#if defined (GPIOK)
    TIM_12_CH1_PK9    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_12 , GPIO_PORT_K  , GPIO_PIN_ID_9  , GPIO_ALT_FUNC_2   ),
#endif

    TIM_12_CH2_PB15   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_12 , GPIO_PORT_B  , GPIO_PIN_ID_15 , GPIO_ALT_FUNC_2   ),
#if !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_12_CH2_PH9    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_12 , GPIO_PORT_H  , GPIO_PIN_ID_9  , GPIO_ALT_FUNC_2   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_12_CH2_PD8    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_12 , GPIO_PORT_D  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_2   ),
    TIM_12_CH2_PG10   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_12 , GPIO_PORT_G  , GPIO_PIN_ID_10 , GPIO_ALT_FUNC_2   ),
#endif
#if defined (GPIOK)
    TIM_12_CH2_PK10   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_12 , GPIO_PORT_K  , GPIO_PIN_ID_10 , GPIO_ALT_FUNC_2   ),
#endif
#endif
    /*---------------------------- Timer 13 pins -----------------------------*/
#ifdef TIM13
    TIM_13_CH1_PA6    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_13 , GPIO_PORT_A  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_9   ),
    TIM_13_CH1_PF8    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_13 , GPIO_PORT_F  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_9   ),
#if !defined( STM32H562xx ) && !defined( STM32H563xx ) && !defined( STM32H573xx )
    TIM_13_CH1_PD9    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_13 , GPIO_PORT_D  , GPIO_PIN_ID_9  , GPIO_ALT_FUNC_2   ),
    TIM_13_CH1_PI14   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_13 , GPIO_PORT_I  , GPIO_PIN_ID_14 , GPIO_ALT_FUNC_9   ),
#endif
#endif
    /*---------------------------- Timer 14 pins -----------------------------*/
#ifdef TIM14
    TIM_14_CH1_PA7    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_14 , GPIO_PORT_A  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_9   ),
    TIM_14_CH1_PF9    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_14 , GPIO_PORT_F  , GPIO_PIN_ID_9  , GPIO_ALT_FUNC_9   ),
#if !defined( STM32H562xx ) && !defined( STM32H563xx ) && !defined( STM32H573xx )
    TIM_14_CH1_PD10   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_14 , GPIO_PORT_D  , GPIO_PIN_ID_10 , GPIO_ALT_FUNC_2   ),
    TIM_14_CH1_PI15   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_14 , GPIO_PORT_I  , GPIO_PIN_ID_15 , GPIO_ALT_FUNC_9   ),
#endif
#endif
    /*---------------------------- Timer 15 pins -----------------------------*/
#ifdef TIM15
    TIM_15_CH1_PA2    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_15 , GPIO_PORT_A  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_4   ),
    TIM_15_CH1_PC12   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_15 , GPIO_PORT_C  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_2   ),
    TIM_15_CH1_PE5    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_15 , GPIO_PORT_E  , GPIO_PIN_ID_5  , GPIO_ALT_FUNC_4   ),
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_15_CH1_PI14   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_15 , GPIO_PORT_I  , GPIO_PIN_ID_14 , GPIO_ALT_FUNC_4   ),
#endif

    TIM_15_CH2_PA3    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_15 , GPIO_PORT_A  , GPIO_PIN_ID_3  , GPIO_ALT_FUNC_4   ),
    TIM_15_CH2_PE6    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_15 , GPIO_PORT_E  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_4   ),
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_15_CH2_PI15   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_15 , GPIO_PORT_I  , GPIO_PIN_ID_15 , GPIO_ALT_FUNC_4   ),
#endif
#endif
    /*---------------------------- Timer 16 pins -----------------------------*/
#ifdef TIM16
    TIM_16_CH1_PB8    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_16 , GPIO_PORT_B  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_1   ),
    TIM_16_CH1_PF6    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_16 , GPIO_PORT_F  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_1   ),
#if !defined( STM32H562xx ) && !defined( STM32H563xx ) && !defined( STM32H573xx )
    TIM_16_CH1_PG15   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_16 , GPIO_PORT_G  , GPIO_PIN_ID_15 , GPIO_ALT_FUNC_1   ),
#endif
#if defined (GPIOJ)
    TIM_16_CH1_PJ15   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_16 , GPIO_PORT_J  , GPIO_PIN_ID_15 , GPIO_ALT_FUNC_1   ),
#endif
#endif
    /*---------------------------- Timer 17 pins -----------------------------*/
#ifdef TIM17
    TIM_17_CH1_PB9    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_17 , GPIO_PORT_B  , GPIO_PIN_ID_9  , GPIO_ALT_FUNC_1   ),
    TIM_17_CH1_PC2    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_17 , GPIO_PORT_C  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_1   ),
    TIM_17_CH1_PF7    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_17 , GPIO_PORT_F  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_1   ),
#if defined (GPIOJ)
    TIM_17_CH1_PJ1    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_17 , GPIO_PORT_J  , GPIO_PIN_ID_1  , GPIO_ALT_FUNC_1   ),
#endif
#endif
    TIM_CH_PIN_UNUSED = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_CNT, TIM_PERIPH_CNT, GPIO_PORT_CNT, GPIO_PIN_ID_CNT, GPIO_ALT_FUNC_CNT )
}   tim_IoPin_t;


/** \brief Enumeration of all possible Negative Channel inputs/outputs (ChN) */
typedef enum
{
    /*----------------------------- Timer 1 pins -----------------------------*/
#ifdef TIM1
    TIM_1_CH1N_PA7      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_1  , GPIO_PORT_A  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_1   ),
    TIM_1_CH1N_PB13     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_13 , GPIO_ALT_FUNC_1   ),
#if defined (GPIOE)
    TIM_1_CH1N_PE8      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_1  , GPIO_PORT_E  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_1   ),
#endif
#if !defined( STM32H503xx ) && !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_1_CH1N_PH10     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_1  , GPIO_PORT_H  , GPIO_PIN_ID_10 , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_1_CH1N_PI3      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_1  , GPIO_PORT_I  , GPIO_PIN_ID_3  , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H503xx )
    TIM_1_CH1N_PA3      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_1  , GPIO_PORT_A  , GPIO_PIN_ID_3  , GPIO_ALT_FUNC_14  ),
#endif

    TIM_1_CH2N_PB0      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_1   ),
    TIM_1_CH2N_PB14     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_14 , GPIO_ALT_FUNC_1   ),
#if defined (GPIOE)
    TIM_1_CH2N_PE10     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_1  , GPIO_PORT_E  , GPIO_PIN_ID_10 , GPIO_ALT_FUNC_1   ),
#endif
#if !defined( STM32H503xx ) && !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_1_CH2N_PH8      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_1  , GPIO_PORT_H  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_1_CH2N_PI5      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_1  , GPIO_PORT_I  , GPIO_PIN_ID_5  , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H503xx )
    TIM_1_CH2N_PA4      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_1  , GPIO_PORT_A  , GPIO_PIN_ID_4  , GPIO_ALT_FUNC_1   ),
    TIM_1_CH2N_PB2      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_1   ),
    TIM_1_CH2N_PB7      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_1   ),
#endif

    TIM_1_CH3N_PB1      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_1  , GPIO_ALT_FUNC_1   ),
    TIM_1_CH3N_PB15     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_15 , GPIO_ALT_FUNC_1   ),
#if defined (GPIOE)
    TIM_1_CH3N_PE12     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_1  , GPIO_PORT_E  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_1   ),
#endif
#if !defined( STM32H503xx ) && !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_1_CH3N_PH6      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_1  , GPIO_PORT_H  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_1_CH3N_PI7      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_1  , GPIO_PORT_I  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H503xx )
    TIM_1_CH3N_PB6      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_1   ),
#endif

    TIM_1_CH4N_PC5      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_1  , GPIO_PORT_C  , GPIO_PIN_ID_5  , GPIO_ALT_FUNC_1   ),
#if !defined( STM32H503xx )
    TIM_1_CH4N_PD5      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_1  , GPIO_PORT_D  , GPIO_PIN_ID_5  , GPIO_ALT_FUNC_1   ),
#endif
#if defined (GPIOE)
    TIM_1_CH4N_PE15     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_1  , GPIO_PORT_E  , GPIO_PIN_ID_15 , GPIO_ALT_FUNC_3   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_1_CH4N_PI10     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_1  , GPIO_PORT_I  , GPIO_PIN_ID_10 , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H503xx )
    TIM_1_CH4N_PA8      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_1  , GPIO_PORT_A  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_14  ),
    TIM_1_CH4N_PA14     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_1  , GPIO_PORT_A  , GPIO_PIN_ID_14 , GPIO_ALT_FUNC_14  ),
    TIM_1_CH4N_PB4      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_4  , GPIO_ALT_FUNC_1   ),
#endif
#endif
    /*----------------------------- Timer 8 pins -----------------------------*/
#ifdef TIM8
    TIM_8_CH1N_PA5      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_8  , GPIO_PORT_A  , GPIO_PIN_ID_5  , GPIO_ALT_FUNC_3   ),
    TIM_8_CH1N_PA7      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_8  , GPIO_PORT_A  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_3   ),
#if !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_8_CH1N_PH7      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_8  , GPIO_PORT_H  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_3   ),
    TIM_8_CH1N_PH13     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_8  , GPIO_PORT_H  , GPIO_PIN_ID_13 , GPIO_ALT_FUNC_3   ),
#endif
#if defined (GPIOK)
    TIM_8_CH1N_PK1      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_8  , GPIO_PORT_K  , GPIO_PIN_ID_1  , GPIO_ALT_FUNC_3   ),
#endif

    TIM_8_CH2N_PB0      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_8  , GPIO_PORT_B  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_3   ),
    TIM_8_CH2N_PB14     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_8  , GPIO_PORT_B  , GPIO_PIN_ID_14 , GPIO_ALT_FUNC_3   ),
#if !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_8_CH2N_PH9      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_8  , GPIO_PORT_H  , GPIO_PIN_ID_9  , GPIO_ALT_FUNC_3   ),
    TIM_8_CH2N_PH14     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_8  , GPIO_PORT_H  , GPIO_PIN_ID_14 , GPIO_ALT_FUNC_3   ),
#endif
#if defined (GPIOK)
    TIM_8_CH2N_PK3      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2  , TIM_PERIPH_8  , GPIO_PORT_K  , GPIO_PIN_ID_3  , GPIO_ALT_FUNC_3   ),
#endif

    TIM_8_CH3N_PB1      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_8  , GPIO_PORT_B  , GPIO_PIN_ID_1  , GPIO_ALT_FUNC_3   ),
    TIM_8_CH3N_PB15     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_8  , GPIO_PORT_B  , GPIO_PIN_ID_15 , GPIO_ALT_FUNC_3   ),
#if !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_8_CH3N_PH11     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_8  , GPIO_PORT_H  , GPIO_PIN_ID_11 , GPIO_ALT_FUNC_3   ),
    TIM_8_CH3N_PH15     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_8  , GPIO_PORT_H  , GPIO_PIN_ID_15 , GPIO_ALT_FUNC_3   ),
#endif
#if defined (GPIOK)
    TIM_8_CH3N_PK5      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3  , TIM_PERIPH_8  , GPIO_PORT_K  , GPIO_PIN_ID_5  , GPIO_ALT_FUNC_3   ),
#endif

    TIM_8_CH4N_PB2      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_8  , GPIO_PORT_B  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_3   ),
    TIM_8_CH4N_PD0      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_8  , GPIO_PORT_D  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_3   ),
#if !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_8_CH4N_PH12     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_8  , GPIO_PORT_H  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_10  ),
#endif
#if defined (GPIOK)
    TIM_8_CH4N_PK7      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4  , TIM_PERIPH_8  , GPIO_PORT_K  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_3   ),
#endif
#endif
    /*---------------------------- Timer 15 pins -----------------------------*/
#ifdef TIM15
    TIM_15_CH1N_PA1     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_15 , GPIO_PORT_A  , GPIO_PIN_ID_1  , GPIO_ALT_FUNC_4   ),
    TIM_15_CH1N_PE4     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_15 , GPIO_PORT_E  , GPIO_PIN_ID_4  , GPIO_ALT_FUNC_4   ),
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_15_CH1N_PI13    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_15 , GPIO_PORT_I  , GPIO_PIN_ID_13 , GPIO_ALT_FUNC_4   ),
#endif
#endif
    /*---------------------------- Timer 16 pins -----------------------------*/
#ifdef TIM16
    TIM_16_CH1N_PB6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_16 , GPIO_PORT_B  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_1   ),
    TIM_16_CH1N_PF8     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_16 , GPIO_PORT_F  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_1   ),
#if !defined( STM32H562xx ) && !defined( STM32H563xx ) && !defined( STM32H573xx )
    TIM_16_CH1N_PG8     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_16 , GPIO_PORT_G  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_1   ),
#endif
#endif
    /*---------------------------- Timer 17 pins -----------------------------*/
#ifdef TIM17
    TIM_17_CH1N_PB7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_17 , GPIO_PORT_B  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_1   ),
    TIM_17_CH1N_PF9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_17 , GPIO_PORT_F  , GPIO_PIN_ID_9  , GPIO_ALT_FUNC_1   ),
#if defined (GPIOJ)
    TIM_17_CH1N_PJ13    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1  , TIM_PERIPH_17 , GPIO_PORT_J  , GPIO_PIN_ID_13 , GPIO_ALT_FUNC_1   ),
#endif
#endif
    TIM_CH_N_PIN_UNUSED = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_CNT, TIM_PERIPH_CNT, GPIO_PORT_CNT, GPIO_PIN_ID_CNT, GPIO_ALT_FUNC_CNT )
}   tim_IOComplPin_t;


/**
 * \brief Enumeration of all possible Break Input (BKIN) timers inputs
 */
typedef enum
{
    /*----------------------------- Timer 1 pins -----------------------------*/
#ifdef TIM1
    TIM_1_BKIN_PA6      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_A  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_1   ),
    TIM_1_BKIN_PB12     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_1   ),
#if defined (GPIOE)
    TIM_1_BKIN_PE15     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_E  , GPIO_PIN_ID_15 , GPIO_ALT_FUNC_1   ),
#endif
#if !defined( STM32H503xx ) && !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_1_BKIN_PH12     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_H  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_1_BKIN_PI12     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_I  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H503xx )
    TIM_1_BKIN_PA4      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_A  , GPIO_PIN_ID_4  , GPIO_ALT_FUNC_14  ),
    TIM_1_BKIN_PB3      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_3  , GPIO_ALT_FUNC_14  ),
#endif
#endif
    /*----------------------------- Timer 8 pins -----------------------------*/
#ifdef TIM8
    TIM_8_BKIN_PA6      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8  , GPIO_PORT_A  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_3   ),
    TIM_8_BKIN_PG2      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8  , GPIO_PORT_G  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_3   ),
#if !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_8_BKIN_PH12     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8  , GPIO_PORT_H  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_3   ),
#endif
#if defined (GPIOI)
    TIM_8_BKIN_PI4      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8  , GPIO_PORT_I  , GPIO_PIN_ID_4  , GPIO_ALT_FUNC_3   ),
#endif
#endif
    /*---------------------------- Timer 15 pins -----------------------------*/
#ifdef TIM15
    TIM_15_BKIN_PA0     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_15 , GPIO_PORT_A  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_4   ),
    TIM_15_BKIN_PD2     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_15 , GPIO_PORT_D  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_4   ),
    TIM_15_BKIN_PE3     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_15 , GPIO_PORT_E  , GPIO_PIN_ID_3  , GPIO_ALT_FUNC_4   ),
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_15_BKIN_PI12    = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_15 , GPIO_PORT_I  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_4   ),
#endif
#endif
    /*---------------------------- Timer 16 pins -----------------------------*/
#ifdef TIM16
    TIM_16_BKIN_PB4     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_16 , GPIO_PORT_B  , GPIO_PIN_ID_4  , GPIO_ALT_FUNC_1   ),
    TIM_16_BKIN_PC0     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_16 , GPIO_PORT_C  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_1   ),
    TIM_16_BKIN_PF10    = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_16 , GPIO_PORT_F  , GPIO_PIN_ID_10 , GPIO_ALT_FUNC_1   ),
#if defined (GPIOJ)
    TIM_16_BKIN_PJ14    = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_16 , GPIO_PORT_J  , GPIO_PIN_ID_14 , GPIO_ALT_FUNC_1   ),
#endif
#endif
    /*---------------------------- Timer 17 pins -----------------------------*/
#ifdef TIM17
    TIM_17_BKIN_PB5     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_17 , GPIO_PORT_B  , GPIO_PIN_ID_5  , GPIO_ALT_FUNC_1   ),
    TIM_17_BKIN_PG6     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_17 , GPIO_PORT_G  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_1   ),
#if defined (GPIOJ)
    TIM_17_BKIN_PJ0     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_17 , GPIO_PORT_J  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_1   ),
#endif
#endif
    TIM_BKIN_PIN_UNUSED = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_CNT, GPIO_PORT_CNT, GPIO_PIN_ID_CNT, GPIO_ALT_FUNC_CNT )
}   tim_BkinPin_t;


/**
 * \brief Enumeration of all possible Break Input 2 (BKIN2) timers inputs
 */
typedef enum
{
    /*----------------------------- Timer 1 pins -----------------------------*/
#ifdef TIM1
#if defined (GPIOE)
    TIM_1_BKIN2_PE6      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_E  , GPIO_PIN_ID_6  , GPIO_ALT_FUNC_1   ),
#endif
#if defined (GPIOG)
    TIM_1_BKIN2_PG4      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_G  , GPIO_PIN_ID_4  , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_1_BKIN2_PI1      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_I  , GPIO_PIN_ID_1  , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H503xx )
    TIM_1_BKIN2_PB8      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_1   ),
    TIM_1_BKIN2_PC10     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_C  , GPIO_PIN_ID_10 , GPIO_ALT_FUNC_1   ),
    TIM_1_BKIN2_PC11     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_C  , GPIO_PIN_ID_11 , GPIO_ALT_FUNC_14  ),
#endif
#endif
    /*----------------------------- Timer 8 pins -----------------------------*/
#ifdef TIM8
    TIM_8_BKIN2_PA8      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8  , GPIO_PORT_A  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_3   ),
    TIM_8_BKIN2_PG3      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8  , GPIO_PORT_G  , GPIO_PIN_ID_3  , GPIO_ALT_FUNC_3   ),
#if defined (GPIOI)
    TIM_8_BKIN2_PI1      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8  , GPIO_PORT_I  , GPIO_PIN_ID_1  , GPIO_ALT_FUNC_3   ),
#endif
#endif
    TIM_BKIN2_PIN_UNUSED = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_CNT, GPIO_PORT_CNT, GPIO_PIN_ID_CNT, GPIO_ALT_FUNC_CNT )
}   tim_Bkin2Pin_t;


/**
 * \brief Enumeration of all possible External Trigger (ETR) timers inputs
 */
typedef enum
{
    /*----------------------------- Timer 1 pins -----------------------------*/
#ifdef TIM1
    TIM_1_ETR_PA12     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_A  , GPIO_PIN_ID_12 , GPIO_ALT_FUNC_1   ),
#if defined (GPIOE)
    TIM_1_ETR_PE7      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_E  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_1   ),
#endif
#if defined (GPIOG)
    TIM_1_ETR_PG5      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_G  , GPIO_PIN_ID_5  , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_1_ETR_PI2      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_I  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_1   ),
#endif
#if defined( STM32H503xx )
    TIM_1_ETR_PA13     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_A  , GPIO_PIN_ID_13 , GPIO_ALT_FUNC_14  ),
    TIM_1_ETR_PB0      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_B  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_14  ),
    TIM_1_ETR_PC0      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1  , GPIO_PORT_C  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_1   ),
#endif
#endif
    /*----------------------------- Timer 2 pins -----------------------------*/
#ifdef TIM2
    TIM_2_ETR_PA0      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_2  , GPIO_PORT_A  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_14  ),
    TIM_2_ETR_PA5      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_2  , GPIO_PORT_A  , GPIO_PIN_ID_5  , GPIO_ALT_FUNC_14  ),
    TIM_2_ETR_PA15     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_2  , GPIO_PORT_A  , GPIO_PIN_ID_15 , GPIO_ALT_FUNC_14  ),
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_2_ETR_PF0      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_2  , GPIO_PORT_F  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_2   ),
#endif
#if defined( STM32H503xx )
    TIM_2_ETR_PD2      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_2  , GPIO_PORT_D  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_14  ),
#endif
#endif
    /*----------------------------- Timer 3 pins -----------------------------*/
#ifdef TIM3
    TIM_3_ETR_PD2      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_3  , GPIO_PORT_D  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_2   ),
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_3_ETR_PF14     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_3  , GPIO_PORT_F  , GPIO_PIN_ID_14 , GPIO_ALT_FUNC_2   ),
#endif
#if defined (GPIOK)
    TIM_3_ETR_PK15     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_3  , GPIO_PORT_K  , GPIO_PIN_ID_15 , GPIO_ALT_FUNC_2   ),
#endif
#if defined( STM32H503xx )
    TIM_3_ETR_PA2      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_3  , GPIO_PORT_A  , GPIO_PIN_ID_2  , GPIO_ALT_FUNC_2   ),
    TIM_3_ETR_PB7      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_3  , GPIO_PORT_B  , GPIO_PIN_ID_7  , GPIO_ALT_FUNC_2   ),
    TIM_3_ETR_PC1      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_3  , GPIO_PORT_C  , GPIO_PIN_ID_1  , GPIO_ALT_FUNC_2   ),
#endif
#endif
    /*----------------------------- Timer 4 pins -----------------------------*/
#ifdef TIM4
    TIM_4_ETR_PE0      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_4  , GPIO_PORT_E  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_2   ),
#if defined( STM32H5E4xx ) || defined( STM32H5E5xx ) || defined( STM32H5F4xx ) || defined( STM32H5F5xx )
    TIM_4_ETR_PF15     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_4  , GPIO_PORT_F  , GPIO_PIN_ID_15 , GPIO_ALT_FUNC_2   ),
#endif
#if defined (GPIOJ)
    TIM_4_ETR_PJ0      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_4  , GPIO_PORT_J  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_2   ),
#endif
#endif
    /*----------------------------- Timer 5 pins -----------------------------*/
#ifdef TIM5
    TIM_5_ETR_PA4      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_5  , GPIO_PORT_A  , GPIO_PIN_ID_4  , GPIO_ALT_FUNC_2   ),
#if !defined( STM32H523xx ) && !defined( STM32H533xx ) && !defined( STM32H543xx ) && !defined( STM32H553xx )
    TIM_5_ETR_PH8      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_5  , GPIO_PORT_H  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_2   ),
#endif
#if defined (GPIOJ)
    TIM_5_ETR_PJ11     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_5  , GPIO_PORT_J  , GPIO_PIN_ID_11 , GPIO_ALT_FUNC_2   ),
#endif
#endif
    /*----------------------------- Timer 8 pins -----------------------------*/
#ifdef TIM8
    TIM_8_ETR_PA0      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8  , GPIO_PORT_A  , GPIO_PIN_ID_0  , GPIO_ALT_FUNC_3   ),
    TIM_8_ETR_PG8      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8  , GPIO_PORT_G  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_3   ),
#if defined (GPIOI)
    TIM_8_ETR_PI3      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8  , GPIO_PORT_I  , GPIO_PIN_ID_3  , GPIO_ALT_FUNC_3   ),
#endif
#if defined (GPIOK)
    TIM_8_ETR_PK8      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8  , GPIO_PORT_K  , GPIO_PIN_ID_8  , GPIO_ALT_FUNC_3   ),
#endif
#endif
    TIM_ETR_PIN_UNUSED = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_CNT, GPIO_PORT_CNT, GPIO_PIN_ID_CNT, GPIO_ALT_FUNC_CNT )
}   tim_EtrPin_t;



typedef struct
{
    /** Channel ID */
    tim_ChannelId_t     ChannelId;

    /** Channel activation/de-activation state */
    tim_FunctionState_t ChannelState;

    /** Required channel operation mode */
    tim_ChannelMode_t   ChannelMode;

    /** Input/output pin configuration */
    tim_IoPin_t         IoPin;
    /** Complementary input/output pin configuration */
    tim_IOComplPin_t    IoComplPin;

    /* ----- PWM configuration ----- */
    tim_Polarity_t      OutputPolarity;
    tim_Polarity_t      IdleState;

    /* ----- Input capture configuration ----- */
    tim_InputPolarity_t  InputPolarity;  /**< Captured edge(s)                      */
    tim_InputFilter_t    InputFilter;    /**< Digital input filter                  */
    tim_InputPrescaler_t InputPrescaler; /**< Capture prescaler                     */
    tim_ActiveInput_t    ActiveInput;    /**< Input mapped to the capture channel   */

}   tim_ChannelConfig_t;


typedef struct
{
    tim_PeriphId_t      PeriphId;                             /**< Timer peripheral identification */
    tim_ClockSource_t   ClockSource;                          /**< Counter clock source */
    tim_ExtClkSource_t  ExtClockSource;                       /**< Trigger input used as clock (\ref TIM_CLOCKSOURCE_EXTERNAL_CH_IN only) */
    tim_SlaveMode_t     SlaveMode;                            /**< Timers synchronization mode */
    tim_TriggerInput_t  SlaveTriggerInput;                    /**< Trigger input of slave mode (SlaveMode other than disabled) */
    tim_MasterTrigger_t MasterTrigger;                        /**< Trigger output (TRGO) selection (timers with master mode) */
    tim_FreqHz_t        TimerFrequency;                       /**< Required timer frequency in Hz (internal clock source only) */
    tim_FunctionState_t AutoreloadPreloadState;               /**< Activation/de-activation state of auto-reload pre-load functionality */
    tim_FunctionState_t UpdateEventState;                     /**< Update event generation state (inactive: shadow registers are not updated) */
    tim_CounterDir_t    CounterDirection;                     /**< Direction of counter (up/down counting) */
    tim_ChannelConfig_t ChannelConfig[ TIM_CHANNEL_CNT ];     /**< Channels configuration */

    tim_FreqHz_t        RefreshFrequency;                     /**< Required counter period frequency in Hz (internal clock source only) */

    /* Timer GPIO configurations */
    tim_BkinPin_t  BreakInPin;
    tim_Polarity_t BreakInPinPolarity;
    tim_Bkin2Pin_t BreakIn2Pin;
    tim_Polarity_t BreakIn2PinPolarity;

    tim_EtrPin_t   TriggerEventPin;

}   tim_PeriphConfig_t;

/* ========================== EXPORTED VARIABLES ============================ */

/* ========================= EXPORTED FUNCTIONS ============================= */

#endif /* TIM_TIM_TYPES_H */
