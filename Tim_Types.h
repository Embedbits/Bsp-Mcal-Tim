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
#include "Gpio_Types.h"                     /* GPIO types definitions         */
#include "Stm32_tim.h"                      /* Timer registry definitions     */
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


typedef enum
{
    TIM_COUNTER_DIR_UP = 0u,
    TIM_COUNTER_DIR_DOWN,
    TIM_COUNTER_DIR_CENTER_UP,
    TIM_COUNTER_DIR_CENTER_DOWN,
    TIM_COUNTER_DIR_CENTER_UP_DOWN,
    TIM_COUNTER_DIR_CNT
}   tim_CounterDir_t;


/** Type signaling count of channels in timer peripheral */
typedef uint8_t tim_ChannelCnt_t;


typedef enum
{
    TIM_ERROR_INACTIVE   = 0b00, /**< No active error detected           */
    TIM_ERROR_INDEX      = 0b01, /**< Index error has been detected      */
    TIM_ERROR_TRANSITION = 0b10, /**< Transition error has been detected */
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
    TIM_CHANNEL_MODE_INPUT_CAPTURE = 0u,
    TIM_CHANNEL_MODE_OUTPUT_FORCED_ACTIVE,           /**< OCyREF is forced high */
    TIM_CHANNEL_MODE_OUTPUT_FORCED_INACTIVE,         /**< OCyREF is forced low  */
    TIM_CHANNEL_MODE_OUTPUT_COMPARE_PULSE,           /**< Pulse on Compare mode */
    TIM_CHANNEL_MODE_OUTPUT_COMPARE_FORCED_ACTIVE,   /**< OCyREF is forced high on compare match */
    TIM_CHANNEL_MODE_OUTPUT_COMPARE_FORCED_INACTIVE, /**< OCyREF is forced low on compare match  */
    TIM_CHANNEL_MODE_OUTPUT_COMPARE_TOGGLE,          /**< OCyREF toggles on compare match        */
    TIM_CHANNEL_MODE_OUTPUT_PWM,                     /**< In upcounting, channel y is active as long as TIMx_CNT<TIMx_CCRy else inactive.  In downcounting, channel y is inactive as long as TIMx_CNT>TIMx_CCRy else active. */
    TIM_CHANNEL_MODE_OUTPUT_PWM_INV,                 /**< In upcounting, channel y is inactive as long as TIMx_CNT<TIMx_CCRy else active.  In downcounting, channel y is active as long as TIMx_CNT>TIMx_CCRy else inactive. */
    TIM_CHANNEL_MODE_OUTPUT_COMBINED_PWM,            /**< Combined PWM mode 1   */
    TIM_CHANNEL_MODE_OUTPUT_COMBINED_PWM_INV,        /**< Combined PWM mode 2   */
    TIM_CHANNEL_MODE_OUTPUT_ASSYMETRIC_PWM,          /**< Asymmetric PWM mode 1 */
    TIM_CHANNEL_MODE_OUTPUT_ASSYMETRIC_PWM_INV,      /**< Asymmetric PWM mode 2 */
    TIM_CHANNEL_MODE_OUTPUT_DIRECTION,               /**< Direction output mode */
    TIM_CHANNEL_MODE_CNT
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
    TIM_INPUT_FILTER_INACTIVE       ,/**< No filter, sampling is done at fDTS */
//    TIM_INPUT_FILTER_               ,/**< fSAMPLING = internal clock, N=2     */
//    TIM_INPUT_FILTER_               ,/**< fSAMPLING = internal clock, N=4     */
//    TIM_INPUT_FILTER_               ,/**< fSAMPLING = internal clock, N=8     */
//    TIM_INPUT_FILTER_               ,/**< fSAMPLING = fDTS/2, N=6               */
//    TIM_INPUT_FILTER_               ,/**< fSAMPLING = fDTS/2, N=8               */
//    TIM_INPUT_FILTER_               ,/**< fSAMPLING = fDTS/4, N=6               */
//    TIM_INPUT_FILTER_               ,/**< fSAMPLING = fDTS/4, N=8               */
//    TIM_INPUT_FILTER_               ,/**< fSAMPLING = fDTS/8, N=6               */
//    TIM_INPUT_FILTER_               ,/**< fSAMPLING = fDTS/8, N=8               */
//    TIM_INPUT_FILTER_               ,/**< fSAMPLING = fDTS/16, N=5              */
//    TIM_INPUT_FILTER_               ,/**< fSAMPLING = fDTS/16, N=6              */
//    TIM_INPUT_FILTER_               ,/**< fSAMPLING = fDTS/16, N=8              */
//    TIM_INPUT_FILTER_               ,/**< fSAMPLING = fDTS/32, N=5              */
//    TIM_INPUT_FILTER_               ,/**< fSAMPLING = fDTS/32, N=6              */
//    TIM_INPUT_FILTER_               ,/**< fSAMPLING = fDTS/32, N=8              */
}   tim_InputFilter_t;

typedef enum
{
    TIM_RESOLUTION_16BIT = 0xFFFFu,    /**< Timer resolution 16bits */
    TIM_RESOLUTION_32BIT = 0xFFFFFFFFu /**< Timer resolution 32bits */
}   tim_Resolution_t;


typedef enum
{
    /** Slave mode disabled */
    TIM_SLAVE_MODE_DISABLE = 0u,

    /** The external clock mode 2 can be used in addition to another slave mode
     * (except external clock mode 1 and encoder mode). In this case, the
     * tim_etr_in signal is used as external clock input, and another input can
     * be selected as trigger input (in reset mode, gated mode or trigger mode).
     * It is recommended not to select tim_etr_in as tim_trgi through the TS bits
     * of TIMx_SMCR register. */
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


typedef enum
{
    TIM_DMA_TRIGER_EVENT_UPDATE = 0u,
    TIM_DMA_TRIGER_EVENT_CAPTURE_COMPARE,
    TIM_DMA_TRIGER_EVENT_COMMUTATION,
    TIM_DMA_TRIGER_EVENT_TRIGGER
}   tim_DmaTriggerEvent_t;


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
    TIM_EXT_CLK_SOURCE_ITR9 = LL_TIM_TS_ITR9,

    /** Internal trigger input bus. These inputs can be used for the slave mode
     * controller or as a input clock (below 1/4 of the tim_ker_ck clock). */
    TIM_EXT_CLK_SOURCE_ITR10 = LL_TIM_TS_ITR10,

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


typedef enum
{
    /** The timer is clocked by the internal clock provided from the RCC */
    TIM_CLOCKSOURCE_INT_CLK = LL_TIM_CLOCKSOURCE_INTERNAL,

    /** Counter counts at each rising or falling edge on a selected input */
    TIM_CLOCKSOURCE_EXTERNAL_CH_IN = LL_TIM_CLOCKSOURCE_EXT_MODE1,

    /** Counter counts at each rising or falling edge on the external trigger input ETR */
    TIM_CLOCKSOURCE_EXTERNAL_ETR = LL_TIM_CLOCKSOURCE_EXT_MODE2,
}   tim_ClockSource_t;


/** \brief Enumeration storing possible interrupt requests for Timer peripheral */
typedef enum
{
    TIM_IRQ_UPDATE              = 0x0001, /**< Update interrupt bit mask                      */
    TIM_IRQ_CAPTURE_COMPARE_CH1 = 0x0002, /**< Capture/Compare channel 1 interrupt bit mask   */
    TIM_IRQ_CAPTURE_COMPARE_CH2 = 0x0004, /**< Capture/Compare channel 2 interrupt bit mask   */
    TIM_IRQ_CAPTURE_COMPARE_CH3 = 0x0008, /**< Capture/Compare channel 3 interrupt bit mask   */
    TIM_IRQ_CAPTURE_COMPARE_CH4 = 0x0010, /**< Capture/Compare channel 4 interrupt bit mask   */
    TIM_IRQ_COMMUTATION         = 0x0020, /**< Commutation error interrupt bit mask           */
    TIM_IRQ_TRIGGER             = 0x0040, /**< Trigger interrupt bit mask                     */
    TIM_IRQ_INDEX               = 0x0080, /**< Index interrupt bit mask                       */
    TIM_IRQ_DIRECTION           = 0x0100, /**< Direction interrupt bit mask                   */
    TIM_IRQ_BREAK               = 0x0200, /**< Break interrupt bit mask                       */
    TIM_IRQ_BREAK2              = 0x0400, /**< Break 2 interrupt bit mask                     */
    TIM_IRQ_SYSTEM_BREAK        = 0x0800, /**< System break interrupt bit mask                */
    TIM_IRQ_ERROR               = 0x1000, /**< Error interrupt bit mask                       */
}   tim_IrqId_t;


/** \brief Enumeration of all possible Channel inputs/outputs (Ch) */
typedef enum
{
    TIM_1_CH1_PA8     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_1   , GPIO_PORT_C   , GPIO_PIN_ID_0   , GPIO_ALT_FUNC_2   ),
    TIM_1_CH1_PC0     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_1   , GPIO_PORT_E   , GPIO_PIN_ID_9   , GPIO_ALT_FUNC_2   ),
    TIM_1_CH1_PE9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_1   , GPIO_PORT_A   , GPIO_PIN_ID_8   , GPIO_ALT_FUNC_6   ),
    TIM_1_CH2_PA9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_1   , GPIO_PORT_A   , GPIO_PIN_ID_9   , GPIO_ALT_FUNC_6   ),
    TIM_1_CH2_PC1     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_1   , GPIO_PORT_C   , GPIO_PIN_ID_1   , GPIO_ALT_FUNC_2   ),
    TIM_1_CH2_PE11    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_1   , GPIO_PORT_E   , GPIO_PIN_ID_11  , GPIO_ALT_FUNC_2   ),
    TIM_1_CH3_PC2     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_1   , GPIO_PORT_C   , GPIO_PIN_ID_2   , GPIO_ALT_FUNC_2   ),
    TIM_1_CH3_PE13    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_1   , GPIO_PORT_E   , GPIO_PIN_ID_13  , GPIO_ALT_FUNC_2   ),
    TIM_1_CH3_PA10    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_1   , GPIO_PORT_A   , GPIO_PIN_ID_10  , GPIO_ALT_FUNC_6   ),
    TIM_1_CH4_PE14    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_1   , GPIO_PORT_E   , GPIO_PIN_ID_14  , GPIO_ALT_FUNC_2   ),
    TIM_1_CH4_PC3     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_1   , GPIO_PORT_C   , GPIO_PIN_ID_3   , GPIO_ALT_FUNC_2   ),
    TIM_1_CH4_PA111   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_1   , GPIO_PORT_A   , GPIO_PIN_ID_11  , GPIO_ALT_FUNC_11  ),
    TIM_2_CH1_PA0     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_2   , GPIO_PORT_A   , GPIO_PIN_ID_0   , GPIO_ALT_FUNC_1   ),
    TIM_2_CH1_PA5     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_2   , GPIO_PORT_A   , GPIO_PIN_ID_5   , GPIO_ALT_FUNC_1   ),
    TIM_2_CH1_PA15    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_2   , GPIO_PORT_A   , GPIO_PIN_ID_15  , GPIO_ALT_FUNC_1   ),
    TIM_2_CH1_PD3     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_2   , GPIO_PORT_D   , GPIO_PIN_ID_3   , GPIO_ALT_FUNC_2   ),
    TIM_2_CH2_PA1     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_2   , GPIO_PORT_A   , GPIO_PIN_ID_1   , GPIO_ALT_FUNC_1   ),
    TIM_2_CH2_PB3     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_2   , GPIO_PORT_B   , GPIO_PIN_ID_3   , GPIO_ALT_FUNC_1   ),
    TIM_2_CH2_PD4     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_2   , GPIO_PORT_D   , GPIO_PIN_ID_4   , GPIO_ALT_FUNC_2   ),
    TIM_2_CH3_PA2     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_2   , GPIO_PORT_A   , GPIO_PIN_ID_2   , GPIO_ALT_FUNC_1   ),
    TIM_2_CH3_PA90    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_2   , GPIO_PORT_A   , GPIO_PIN_ID_9   , GPIO_ALT_FUNC_10  ),
    TIM_2_CH3_PB10    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_2   , GPIO_PORT_B   , GPIO_PIN_ID_10  , GPIO_ALT_FUNC_1   ),
    TIM_2_CH3_PD7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_2   , GPIO_PORT_D   , GPIO_PIN_ID_7   , GPIO_ALT_FUNC_2   ),
    TIM_2_CH4_PA4     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_2   , GPIO_PORT_A   , GPIO_PIN_ID_4   , GPIO_ALT_FUNC_1   ),
    TIM_2_CH4_PA100   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_2   , GPIO_PORT_A   , GPIO_PIN_ID_10  , GPIO_ALT_FUNC_10  ),
    TIM_2_CH4_PB11    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_2   , GPIO_PORT_B   , GPIO_PIN_ID_11  , GPIO_ALT_FUNC_1   ),
    TIM_2_CH4_PD6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_2   , GPIO_PORT_D   , GPIO_PIN_ID_6   , GPIO_ALT_FUNC_2   ),
    TIM_3_CH1_PA6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_3   , GPIO_PORT_A   , GPIO_PIN_ID_6   , GPIO_ALT_FUNC_2   ),
    TIM_3_CH1_PB4     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_3   , GPIO_PORT_B   , GPIO_PIN_ID_4   , GPIO_ALT_FUNC_2   ),
    TIM_3_CH1_PC6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_3   , GPIO_PORT_C   , GPIO_PIN_ID_6   , GPIO_ALT_FUNC_2   ),
    TIM_3_CH1_PE2     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_3   , GPIO_PORT_E   , GPIO_PIN_ID_2   , GPIO_ALT_FUNC_2   ),
    TIM_3_CH2_PA4     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_3   , GPIO_PORT_A   , GPIO_PIN_ID_4   , GPIO_ALT_FUNC_2   ),
    TIM_3_CH2_PA7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_3   , GPIO_PORT_A   , GPIO_PIN_ID_7   , GPIO_ALT_FUNC_2   ),
    TIM_3_CH2_PB5     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_3   , GPIO_PORT_B   , GPIO_PIN_ID_5   , GPIO_ALT_FUNC_2   ),
    TIM_3_CH2_PC7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_3   , GPIO_PORT_C   , GPIO_PIN_ID_7   , GPIO_ALT_FUNC_2   ),
    TIM_3_CH2_PE3     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_3   , GPIO_PORT_E   , GPIO_PIN_ID_3   , GPIO_ALT_FUNC_2   ),
    TIM_3_CH3_PB0     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_3   , GPIO_PORT_B   , GPIO_PIN_ID_0   , GPIO_ALT_FUNC_2   ),
    TIM_3_CH3_PC8     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_3   , GPIO_PORT_C   , GPIO_PIN_ID_8   , GPIO_ALT_FUNC_2   ),
    TIM_3_CH3_PE4     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_3   , GPIO_PORT_E   , GPIO_PIN_ID_4   , GPIO_ALT_FUNC_2   ),
    TIM_3_CH4_PB1     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_3   , GPIO_PORT_B   , GPIO_PIN_ID_1   , GPIO_ALT_FUNC_2   ),
    TIM_3_CH4_PB70    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_3   , GPIO_PORT_B   , GPIO_PIN_ID_7   , GPIO_ALT_FUNC_10  ),
    TIM_3_CH4_PC9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_3   , GPIO_PORT_C   , GPIO_PIN_ID_9   , GPIO_ALT_FUNC_2   ),
    TIM_3_CH4_PE5     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_3   , GPIO_PORT_E   , GPIO_PIN_ID_5   , GPIO_ALT_FUNC_2   ),
    TIM_4_CH1_PA110   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_4   , GPIO_PORT_A   , GPIO_PIN_ID_11  , GPIO_ALT_FUNC_10  ),
    TIM_4_CH1_PB6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_4   , GPIO_PORT_B   , GPIO_PIN_ID_6   , GPIO_ALT_FUNC_2   ),
    TIM_4_CH1_PD12    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_4   , GPIO_PORT_D   , GPIO_PIN_ID_12  , GPIO_ALT_FUNC_2   ),
    TIM_4_CH2_PA120   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_4   , GPIO_PORT_A   , GPIO_PIN_ID_12  , GPIO_ALT_FUNC_10  ),
    TIM_4_CH2_PB7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_4   , GPIO_PORT_B   , GPIO_PIN_ID_7   , GPIO_ALT_FUNC_2   ),
    TIM_4_CH2_PD13    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_4   , GPIO_PORT_D   , GPIO_PIN_ID_13  , GPIO_ALT_FUNC_2   ),
    TIM_4_CH3_PA130   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_4   , GPIO_PORT_A   , GPIO_PIN_ID_13  , GPIO_ALT_FUNC_10  ),
    TIM_4_CH3_PB8     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_4   , GPIO_PORT_B   , GPIO_PIN_ID_8   , GPIO_ALT_FUNC_2   ),
    TIM_4_CH3_PD14    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_4   , GPIO_PORT_D   , GPIO_PIN_ID_14  , GPIO_ALT_FUNC_2   ),
    TIM_4_CH4_PD15    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_4   , GPIO_PORT_D   , GPIO_PIN_ID_15  , GPIO_ALT_FUNC_2   ),
    TIM_4_CH4_PB9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_4   , GPIO_PORT_B   , GPIO_PIN_ID_9   , GPIO_ALT_FUNC_2   ),
    TIM_4_CH4_PF6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_4   , GPIO_PORT_F   , GPIO_PIN_ID_6   , GPIO_ALT_FUNC_2   ),
    TIM_5_CH1_PF6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_5   , GPIO_PORT_F   , GPIO_PIN_ID_6   , GPIO_ALT_FUNC_6   ),
    TIM_5_CH2_PF7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_5   , GPIO_PORT_F   , GPIO_PIN_ID_7   , GPIO_ALT_FUNC_6   ),
    TIM_5_CH3_PF8     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_5   , GPIO_PORT_F   , GPIO_PIN_ID_8   , GPIO_ALT_FUNC_6   ),
    TIM_5_CH4_PF9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_5   , GPIO_PORT_F   , GPIO_PIN_ID_9   , GPIO_ALT_FUNC_6   ),
    TIM_5_CH1_PA0     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_5   , GPIO_PORT_A   , GPIO_PIN_ID_0   , GPIO_ALT_FUNC_2   ),
    TIM_5_CH2_PA1     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_5   , GPIO_PORT_A   , GPIO_PIN_ID_1   , GPIO_ALT_FUNC_2   ),
    TIM_5_CH3_PA2     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_5   , GPIO_PORT_A   , GPIO_PIN_ID_2   , GPIO_ALT_FUNC_2   ),
    TIM_5_CH4_PA3     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_5   , GPIO_PORT_A   , GPIO_PIN_ID_3   , GPIO_ALT_FUNC_2   ),
    TIM_5_CH1_PB2     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_5   , GPIO_PORT_B   , GPIO_PIN_ID_2   , GPIO_ALT_FUNC_2   ),
    TIM_5_CH2_PC12    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_5   , GPIO_PORT_C   , GPIO_PIN_ID_12  , GPIO_ALT_FUNC_1   ),
    TIM_5_CH3_PE8     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_5   , GPIO_PORT_E   , GPIO_PIN_ID_8   , GPIO_ALT_FUNC_1   ),
    TIM_5_CH4_PE9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_5   , GPIO_PORT_E   , GPIO_PIN_ID_9   , GPIO_ALT_FUNC_1   ),
    TIM_8_CH1_PA15    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_8   , GPIO_PORT_A   , GPIO_PIN_ID_15  , GPIO_ALT_FUNC_2   ),
    TIM_8_CH1_PB6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_8   , GPIO_PORT_B   , GPIO_PIN_ID_6   , GPIO_ALT_FUNC_5   ),
    TIM_8_CH1_PC6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_8   , GPIO_PORT_C   , GPIO_PIN_ID_6   , GPIO_ALT_FUNC_4   ),
    TIM_8_CH2_PA14    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_8   , GPIO_PORT_A   , GPIO_PIN_ID_14  , GPIO_ALT_FUNC_5   ),
    TIM_8_CH2_PB8     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_8   , GPIO_PORT_B   , GPIO_PIN_ID_8   , GPIO_ALT_FUNC_10  ),
    TIM_8_CH2_PC7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_8   , GPIO_PORT_C   , GPIO_PIN_ID_7   , GPIO_ALT_FUNC_4   ),
    TIM_8_CH3_PB9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_8   , GPIO_PORT_B   , GPIO_PIN_ID_9   , GPIO_ALT_FUNC_10  ),
    TIM_8_CH3_PC8     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_8   , GPIO_PORT_C   , GPIO_PIN_ID_8   , GPIO_ALT_FUNC_4   ),
    TIM_8_CH4_PC9     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_8   , GPIO_PORT_C   , GPIO_PIN_ID_9   , GPIO_ALT_FUNC_4   ),
    TIM_8_CH4_PD1     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_8   , GPIO_PORT_D   , GPIO_PIN_ID_1   , GPIO_ALT_FUNC_4   ),
    TIM_15_CH1_PA2    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_15  , GPIO_PORT_A   , GPIO_PIN_ID_2   , GPIO_ALT_FUNC_9   ),
    TIM_15_CH1_PB14   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_15  , GPIO_PORT_B   , GPIO_PIN_ID_14  , GPIO_ALT_FUNC_1   ),
    TIM_15_CH1_PF9    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_15  , GPIO_PORT_F   , GPIO_PIN_ID_9   , GPIO_ALT_FUNC_3   ),
    TIM_15_CH2_PA3    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_15  , GPIO_PORT_A   , GPIO_PIN_ID_3   , GPIO_ALT_FUNC_9   ),
    TIM_15_CH2_PB15   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_15  , GPIO_PORT_B   , GPIO_PIN_ID_15  , GPIO_ALT_FUNC_1   ),
    TIM_15_CH2_PF10   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_15  , GPIO_PORT_F   , GPIO_PIN_ID_10  , GPIO_ALT_FUNC_3   ),
    TIM_16_CH1_PA6    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_16  , GPIO_PORT_A   , GPIO_PIN_ID_6   , GPIO_ALT_FUNC_1   ),
    TIM_16_CH1_PA12   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_16  , GPIO_PORT_A   , GPIO_PIN_ID_12  , GPIO_ALT_FUNC_1   ),
    TIM_16_CH1_PB4    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_16  , GPIO_PORT_B   , GPIO_PIN_ID_4   , GPIO_ALT_FUNC_1   ),
    TIM_16_CH1_PB8    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_16  , GPIO_PORT_B   , GPIO_PIN_ID_8   , GPIO_ALT_FUNC_1   ),
    TIM_16_CH1_PE0    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_16  , GPIO_PORT_E   , GPIO_PIN_ID_0   , GPIO_ALT_FUNC_4   ),
    TIM_17_CH1_PA7    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_17  , GPIO_PORT_A   , GPIO_PIN_ID_7   , GPIO_ALT_FUNC_1   ),
    TIM_17_CH1_PB50   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_17  , GPIO_PORT_B   , GPIO_PIN_ID_5   , GPIO_ALT_FUNC_10  ),
    TIM_17_CH1_PB9    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_17  , GPIO_PORT_B   , GPIO_PIN_ID_9   , GPIO_ALT_FUNC_1   ),
    TIM_17_CH1_PE1    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_17  , GPIO_PORT_E   , GPIO_PIN_ID_1   , GPIO_ALT_FUNC_4   ),
    TIM_20_CH1_PB2    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_20  , GPIO_PORT_B   , GPIO_PIN_ID_2   , GPIO_ALT_FUNC_3   ),
    TIM_20_CH1_PE2    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_20  , GPIO_PORT_E   , GPIO_PIN_ID_2   , GPIO_ALT_FUNC_6   ),
    TIM_20_CH1_PF12   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_20  , GPIO_PORT_F   , GPIO_PIN_ID_12  , GPIO_ALT_FUNC_2   ),
    TIM_20_CH2_PC2    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_20  , GPIO_PORT_C   , GPIO_PIN_ID_2   , GPIO_ALT_FUNC_6   ),
    TIM_20_CH2_PE3    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_20  , GPIO_PORT_E   , GPIO_PIN_ID_3   , GPIO_ALT_FUNC_6   ),
    TIM_20_CH2_PF13   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_20  , GPIO_PORT_F   , GPIO_PIN_ID_13  , GPIO_ALT_FUNC_2   ),
    TIM_20_CH3_PC8    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_20  , GPIO_PORT_C   , GPIO_PIN_ID_8   , GPIO_ALT_FUNC_6   ),
    TIM_20_CH3_PF2    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_20  , GPIO_PORT_F   , GPIO_PIN_ID_2   , GPIO_ALT_FUNC_2   ),
    TIM_20_CH3_PF14   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_20  , GPIO_PORT_F   , GPIO_PIN_ID_14  , GPIO_ALT_FUNC_2   ),
    TIM_20_CH4_PE1    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_20  , GPIO_PORT_E   , GPIO_PIN_ID_1   , GPIO_ALT_FUNC_6   ),
    TIM_20_CH4_PF15   = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_20  , GPIO_PORT_F   , GPIO_PIN_ID_15  , GPIO_ALT_FUNC_2   ),
    TIM_20_CH4_PF3    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_20  , GPIO_PORT_F   , GPIO_PIN_ID_3   , GPIO_ALT_FUNC_2   ),
    TIM_CH_PIN_UNUSED = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_CNT , TIM_PERIPH_CNT , GPIO_PORT_CNT , GPIO_PIN_ID_CNT , GPIO_ALT_FUNC_CNT )
}   tim_IoPin_t;


/** \brief Enumeration of all possible Negative Channel inputs/outputs (ChN) */
typedef enum
{
    TIM_1_CH1N_PA7      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_1   , GPIO_PORT_A   , GPIO_PIN_ID_7   , GPIO_ALT_FUNC_6   ),
    TIM_1_CH1N_PA11     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_1   , GPIO_PORT_A   , GPIO_PIN_ID_11  , GPIO_ALT_FUNC_6   ),
    TIM_1_CH1N_PB13     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_1   , GPIO_PORT_B   , GPIO_PIN_ID_13  , GPIO_ALT_FUNC_6   ),
    TIM_1_CH1N_PE8      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_1   , GPIO_PORT_E   , GPIO_PIN_ID_8   , GPIO_ALT_FUNC_2   ),
    TIM_1_CH1N_PC13     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_1   , GPIO_PORT_C   , GPIO_PIN_ID_13  , GPIO_ALT_FUNC_4   ),
    TIM_1_CH2N_PE10     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_1   , GPIO_PORT_E   , GPIO_PIN_ID_10  , GPIO_ALT_FUNC_2   ),
    TIM_1_CH2N_PA12     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_1   , GPIO_PORT_A   , GPIO_PIN_ID_12  , GPIO_ALT_FUNC_6   ),
    TIM_1_CH2N_PB0      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_1   , GPIO_PORT_B   , GPIO_PIN_ID_0   , GPIO_ALT_FUNC_6   ),
    TIM_1_CH2N_PB14     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_1   , GPIO_PORT_B   , GPIO_PIN_ID_14  , GPIO_ALT_FUNC_6   ),
    TIM_1_CH3N_PB1      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_1   , GPIO_PORT_B   , GPIO_PIN_ID_1   , GPIO_ALT_FUNC_6   ),
    TIM_1_CH3N_PB15     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_1   , GPIO_PORT_B   , GPIO_PIN_ID_15  , GPIO_ALT_FUNC_4   ),
    TIM_1_CH3N_PE12     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_1   , GPIO_PORT_E   , GPIO_PIN_ID_12  , GPIO_ALT_FUNC_2   ),
    TIM_1_CH3N_PF0      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_1   , GPIO_PORT_F   , GPIO_PIN_ID_0   , GPIO_ALT_FUNC_6   ),
    TIM_1_CH3N_PB92     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_1   , GPIO_PORT_B   , GPIO_PIN_ID_9   , GPIO_ALT_FUNC_12  ),
    TIM_1_CH4N_PC5      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_1   , GPIO_PORT_C   , GPIO_PIN_ID_5   , GPIO_ALT_FUNC_6   ),
    TIM_1_CH4N_PE15     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_1   , GPIO_PORT_E   , GPIO_PIN_ID_15  , GPIO_ALT_FUNC_6   ),
    TIM_8_CH1N_PA7      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_8   , GPIO_PORT_A   , GPIO_PIN_ID_7   , GPIO_ALT_FUNC_4   ),
    TIM_8_CH1N_PB3      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_8   , GPIO_PORT_B   , GPIO_PIN_ID_3   , GPIO_ALT_FUNC_4   ),
    TIM_8_CH1N_PC10     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_8   , GPIO_PORT_C   , GPIO_PIN_ID_10  , GPIO_ALT_FUNC_4   ),
    TIM_8_CH2N_PB0      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_8   , GPIO_PORT_B   , GPIO_PIN_ID_0   , GPIO_ALT_FUNC_4   ),
    TIM_8_CH2N_PB4      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_8   , GPIO_PORT_B   , GPIO_PIN_ID_4   , GPIO_ALT_FUNC_4   ),
    TIM_8_CH2N_PC11     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_8   , GPIO_PORT_C   , GPIO_PIN_ID_11  , GPIO_ALT_FUNC_4   ),
    TIM_8_CH3N_PB1      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_8   , GPIO_PORT_B   , GPIO_PIN_ID_1   , GPIO_ALT_FUNC_4   ),
    TIM_8_CH3N_PB5      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_8   , GPIO_PORT_B   , GPIO_PIN_ID_5   , GPIO_ALT_FUNC_3   ),
    TIM_8_CH3N_PC12     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_8   , GPIO_PORT_C   , GPIO_PIN_ID_12  , GPIO_ALT_FUNC_4   ),
    TIM_8_CH4N_PC13     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_8   , GPIO_PORT_C   , GPIO_PIN_ID_13  , GPIO_ALT_FUNC_6   ),
    TIM_8_CH4N_PD0      = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_8   , GPIO_PORT_D   , GPIO_PIN_ID_0   , GPIO_ALT_FUNC_6   ),
    TIM_15_CH1N_PA1     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_15  , GPIO_PORT_A   , GPIO_PIN_ID_1   , GPIO_ALT_FUNC_9   ),
    TIM_15_CH1N_PB15    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_15  , GPIO_PORT_B   , GPIO_PIN_ID_15  , GPIO_ALT_FUNC_2   ),
    TIM_15_CH1N_PG94    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_15  , GPIO_PORT_G   , GPIO_PIN_ID_9   , GPIO_ALT_FUNC_14  ),
    TIM_16_CH1N_PB6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_16  , GPIO_PORT_B   , GPIO_PIN_ID_6   , GPIO_ALT_FUNC_1   ),
    TIM_16_CH1N_PA13    = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_16  , GPIO_PORT_A   , GPIO_PIN_ID_13  , GPIO_ALT_FUNC_1   ),
    TIM_17_CH1N_PB7     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_17  , GPIO_PORT_B   , GPIO_PIN_ID_7   , GPIO_ALT_FUNC_1   ),
    TIM_20_CH1N_PE4     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_20  , GPIO_PORT_E   , GPIO_PIN_ID_4   , GPIO_ALT_FUNC_6   ),
    TIM_20_CH1N_PF4     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_20  , GPIO_PORT_F   , GPIO_PIN_ID_4   , GPIO_ALT_FUNC_3   ),
    TIM_20_CH1N_PG0     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_1   , TIM_PERIPH_20  , GPIO_PORT_G   , GPIO_PIN_ID_0   , GPIO_ALT_FUNC_2   ),
    TIM_20_CH2N_PE5     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_20  , GPIO_PORT_E   , GPIO_PIN_ID_5   , GPIO_ALT_FUNC_6   ),
    TIM_20_CH2N_PG1     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_20  , GPIO_PORT_G   , GPIO_PIN_ID_1   , GPIO_ALT_FUNC_2   ),
    TIM_20_CH2N_PF5     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_2   , TIM_PERIPH_20  , GPIO_PORT_F   , GPIO_PIN_ID_5   , GPIO_ALT_FUNC_2   ),
    TIM_20_CH3N_PE6     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_20  , GPIO_PORT_E   , GPIO_PIN_ID_6   , GPIO_ALT_FUNC_6   ),
    TIM_20_CH3N_PG2     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_3   , TIM_PERIPH_20  , GPIO_PORT_G   , GPIO_PIN_ID_2   , GPIO_ALT_FUNC_2   ),
    TIM_20_CH4N_PE0     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_20  , GPIO_PORT_E   , GPIO_PIN_ID_0   , GPIO_ALT_FUNC_3   ),
    TIM_20_CH4N_PG3     = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_4   , TIM_PERIPH_20  , GPIO_PORT_G   , GPIO_PIN_ID_3   , GPIO_ALT_FUNC_6   ),
    TIM_CH_N_PIN_UNUSED = TIM_CHANNEL_PIN_BIT_MASK_ENCODE( TIM_CHANNEL_CNT , TIM_PERIPH_CNT , GPIO_PORT_CNT , GPIO_PIN_ID_CNT , GPIO_ALT_FUNC_CNT )
}   tim_IOComplPin_t;


/** \brief Enumeration of all possible Break Input (BKIN) timers inputs */
typedef enum
{
    TIM_1_BKIN_PA6      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1   , GPIO_PORT_A   , GPIO_PIN_ID_6   , GPIO_ALT_FUNC_6   ),
    TIM_1_BKIN_PA14     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1   , GPIO_PORT_A   , GPIO_PIN_ID_14  , GPIO_ALT_FUNC_6   ),
    TIM_1_BKIN_PA15     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1   , GPIO_PORT_A   , GPIO_PIN_ID_15  , GPIO_ALT_FUNC_9   ),
    TIM_1_BKIN_PB8      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1   , GPIO_PORT_B   , GPIO_PIN_ID_8   , GPIO_ALT_FUNC_12  ),
    TIM_1_BKIN_PB10     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1   , GPIO_PORT_B   , GPIO_PIN_ID_10  , GPIO_ALT_FUNC_12  ),
    TIM_1_BKIN_PB12     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1   , GPIO_PORT_B   , GPIO_PIN_ID_12  , GPIO_ALT_FUNC_6   ),
    TIM_1_BKIN_PC13     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1   , GPIO_PORT_C   , GPIO_PIN_ID_13  , GPIO_ALT_FUNC_2   ),
    TIM_1_BKIN_PE15     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1   , GPIO_PORT_E   , GPIO_PIN_ID_15  , GPIO_ALT_FUNC_2   ),
    TIM_8_BKIN_PA0      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8   , GPIO_PORT_A   , GPIO_PIN_ID_0   , GPIO_ALT_FUNC_9   ),
    TIM_8_BKIN_PA6      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8   , GPIO_PORT_A   , GPIO_PIN_ID_6   , GPIO_ALT_FUNC_4   ),
    TIM_8_BKIN_PA10     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8   , GPIO_PORT_A   , GPIO_PIN_ID_10  , GPIO_ALT_FUNC_11  ),
    TIM_8_BKIN_PB7      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8   , GPIO_PORT_B   , GPIO_PIN_ID_7   , GPIO_ALT_FUNC_5   ),
    TIM_8_BKIN_PD2      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8   , GPIO_PORT_D   , GPIO_PIN_ID_2   , GPIO_ALT_FUNC_4   ),
    TIM_15_BKIN_PA9     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_15  , GPIO_PORT_A   , GPIO_PIN_ID_9   , GPIO_ALT_FUNC_9   ),
    TIM_15_BKIN_PC5     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_15  , GPIO_PORT_C   , GPIO_PIN_ID_5   , GPIO_ALT_FUNC_2   ),
    TIM_16_BKIN_PB5     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_16  , GPIO_PORT_B   , GPIO_PIN_ID_5   , GPIO_ALT_FUNC_1   ),
    TIM_17_BKIN_PA10    = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_17  , GPIO_PORT_A   , GPIO_PIN_ID_10  , GPIO_ALT_FUNC_1   ),
    TIM_17_BKIN_PB4     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_17  , GPIO_PORT_B   , GPIO_PIN_ID_4   , GPIO_ALT_FUNC_10  ),
    TIM_20_BKIN_PF7     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_20  , GPIO_PORT_F   , GPIO_PIN_ID_7   , GPIO_ALT_FUNC_2   ),
    TIM_20_BKIN_PF9     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_20  , GPIO_PORT_F   , GPIO_PIN_ID_9   , GPIO_ALT_FUNC_2   ),
    TIM_20_BKIN_PG3     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_20  , GPIO_PORT_G   , GPIO_PIN_ID_3   , GPIO_ALT_FUNC_2   ),
    TIM_20_BKIN_PG6     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_20  , GPIO_PORT_G   , GPIO_PIN_ID_6   , GPIO_ALT_FUNC_2   ),
    TIM_BKIN_PIN_UNUSED = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_CNT , GPIO_PORT_CNT , GPIO_PIN_ID_CNT , GPIO_ALT_FUNC_CNT )
}   tim_BkinPin_t;


/** \brief Enumeration of all possible Break Input 2 (BKIN2) timers inputs */
typedef enum
{
    TIM_1_BKIN2_PA11     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1   , GPIO_PORT_A   , GPIO_PIN_ID_11  , GPIO_ALT_FUNC_12  ), /**< Pin PA11 used as Break Input 2 for timer 1  */
    TIM_1_BKIN2_PC3      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1   , GPIO_PORT_C   , GPIO_PIN_ID_3   , GPIO_ALT_FUNC_6   ), /**< Pin PC3 used as Break Input 2 for timer 1   */
    TIM_1_BKIN2_PE14     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1   , GPIO_PORT_E   , GPIO_PIN_ID_14  , GPIO_ALT_FUNC_6   ), /**< Pin PE14 used as Break Input 2 for timer 1  */
    TIM_8_BKIN2_PB6      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8   , GPIO_PORT_B   , GPIO_PIN_ID_6   , GPIO_ALT_FUNC_10  ), /**< Pin PB6 used as Break Input 2 for timer 8   */
    TIM_8_BKIN2_PC9      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8   , GPIO_PORT_C   , GPIO_PIN_ID_9   , GPIO_ALT_FUNC_6   ), /**< Pin PC9 used as Break Input 2 for timer 8   */
    TIM_8_BKIN2_PD1      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8   , GPIO_PORT_D   , GPIO_PIN_ID_1   , GPIO_ALT_FUNC_6   ), /**< Pin PD1 used as Break Input 2 for timer 8   */
    TIM_20_BKIN2_PF8     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_20  , GPIO_PORT_F   , GPIO_PIN_ID_8   , GPIO_ALT_FUNC_2   ), /**< Pin PF8 used as Break Input 2 for timer 20  */
    TIM_20_BKIN2_PF10    = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_20  , GPIO_PORT_F   , GPIO_PIN_ID_10  , GPIO_ALT_FUNC_2   ), /**< Pin PF10 used as Break Input 2 for timer 20 */
    TIM_20_BKIN2_PG4     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_20  , GPIO_PORT_G   , GPIO_PIN_ID_4   , GPIO_ALT_FUNC_2   ), /**< Pin PG4 used as Break Input 2 for timer 20  */
    TIM_BKIN2_PIN_UNUSED = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_CNT , GPIO_PORT_CNT , GPIO_PIN_ID_CNT , GPIO_ALT_FUNC_CNT )  /**< Count of pins available for Break Input 2   */
}   tim_Bkin2Pin_t;


/** \brief Enumeration of all possible External Trigger (ETR) timers inputs */
typedef enum
{
    TIM_1_ETR_PA12     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1   , GPIO_PORT_A   , GPIO_PIN_ID_12  , GPIO_ALT_FUNC_11  ), /**< Pin PA12 used as External Trigger input for timer 1  */
    TIM_1_ETR_PC4      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1   , GPIO_PORT_C   , GPIO_PIN_ID_4   , GPIO_ALT_FUNC_2   ), /**< Pin PC4 used as External Trigger input for timer 1   */
    TIM_1_ETR_PE7      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_1   , GPIO_PORT_E   , GPIO_PIN_ID_7   , GPIO_ALT_FUNC_2   ), /**< Pin PE7 used as External Trigger input for timer 1   */
    TIM_2_ETR_PA0      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_2   , GPIO_PORT_A   , GPIO_PIN_ID_0   , GPIO_ALT_FUNC_14  ), /**< Pin PA0 used as External Trigger input for timer 2   */
    TIM_2_ETR_PA5      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_2   , GPIO_PORT_A   , GPIO_PIN_ID_5   , GPIO_ALT_FUNC_2   ), /**< Pin PA5 used as External Trigger input for timer 2   */
    TIM_2_ETR_PA15     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_2   , GPIO_PORT_A   , GPIO_PIN_ID_15  , GPIO_ALT_FUNC_14  ), /**< Pin PA15 used as External Trigger input for timer 2  */
    TIM_2_ETR_PD3      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_2   , GPIO_PORT_D   , GPIO_PIN_ID_3   , GPIO_ALT_FUNC_2   ), /**< Pin PD3 used as External Trigger input for timer 2   */
    TIM_3_ETR_PB3      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_3   , GPIO_PORT_B   , GPIO_PIN_ID_3   , GPIO_ALT_FUNC_10  ), /**< Pin PB3 used as External Trigger input for timer 3   */
    TIM_3_ETR_PD2      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_3   , GPIO_PORT_D   , GPIO_PIN_ID_2   , GPIO_ALT_FUNC_2   ), /**< Pin PD2 used as External Trigger input for timer 3   */
    TIM_4_ETR_PA8      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_4   , GPIO_PORT_A   , GPIO_PIN_ID_8   , GPIO_ALT_FUNC_10  ), /**< Pin PA8 used as External Trigger input for timer 4   */
    TIM_4_ETR_PB3      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_4   , GPIO_PORT_B   , GPIO_PIN_ID_3   , GPIO_ALT_FUNC_2   ), /**< Pin PB3 used as External Trigger input for timer 4   */
    TIM_4_ETR_PE0      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_4   , GPIO_PORT_E   , GPIO_PIN_ID_0   , GPIO_ALT_FUNC_2   ), /**< Pin PE0 used as External Trigger input for timer 4   */
    TIM_5_ETR_PB12     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_5   , GPIO_PORT_B   , GPIO_PIN_ID_12  , GPIO_ALT_FUNC_2   ), /**< Pin PB12 used as External Trigger input for timer 5  */
    TIM_5_ETR_PD11     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_5   , GPIO_PORT_D   , GPIO_PIN_ID_11  , GPIO_ALT_FUNC_1   ), /**< Pin PD11 used as External Trigger input for timer 5  */
    TIM_5_ETR_PF6      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_5   , GPIO_PORT_F   , GPIO_PIN_ID_6   , GPIO_ALT_FUNC_1   ), /**< Pin PF6 used as External Trigger input for timer 5   */
    TIM_8_ETR_PA0      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8   , GPIO_PORT_A   , GPIO_PIN_ID_0   , GPIO_ALT_FUNC_10  ), /**< Pin PA0 used as External Trigger input for timer 8   */
    TIM_8_ETR_PB6      = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_8   , GPIO_PORT_B   , GPIO_PIN_ID_6   , GPIO_ALT_FUNC_6   ), /**< Pin PB6 used as External Trigger input for timer 8   */
    TIM_20_ETR_PE0     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_20  , GPIO_PORT_E   , GPIO_PIN_ID_0   , GPIO_ALT_FUNC_6   ), /**< Pin PE0 used as External Trigger input for timer 20  */
    TIM_20_ETR_PF11    = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_20  , GPIO_PORT_F   , GPIO_PIN_ID_11  , GPIO_ALT_FUNC_2   ), /**< Pin PF11 used as External Trigger input for timer 20 */
    TIM_20_ETR_PG5     = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_20  , GPIO_PORT_G   , GPIO_PIN_ID_5   , GPIO_ALT_FUNC_2   ), /**< Pin PG5 used as External Trigger input for timer 20  */
    TIM_ETR_PIN_UNUSED = TIM_TIMER_PIN_BIT_MASK_ENCODE( TIM_PERIPH_CNT , GPIO_PORT_CNT , GPIO_PIN_ID_CNT , GPIO_ALT_FUNC_CNT )  /**< Count of External Trigger input pins for timers      */
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

}   tim_ChannelConfig_t;


typedef struct
{
    tim_PeriphId_t      PeriphId;
    tim_ClockSource_t   ClockSource;
    tim_SlaveMode_t     SlaveMode;                            /**< Timers synchronization mode */
    tim_FreqHz_t        TimerFrequency;                       /**< Required timer frequency in Hz */
    tim_FunctionState_t AutoreloadPreloadState;               /**< Activation/de-activation state of auto-reload pre-load functionality */
    tim_FunctionState_t UpdateEventState;                     /**< Update event generation state */
    tim_CounterDir_t    CounterDirection;                     /**< Direction of counter (up/down counting) */
    tim_ChannelConfig_t ChannelConfig[ TIM_CHANNEL_CNT ];

    tim_FreqHz_t        RefreshFrequency;
    tim_FunctionState_t ArrPreloadState;

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
