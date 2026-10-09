/**
 * \author Mr.Nobody
 * \file Tim.c
 * \ingroup Tim
 * \brief Tim module common functionality (STM32F4 family)
 *
 * STM32F4 timer peripherals: TIM1 / TIM8 advanced control (4 channels, 3
 * complementary outputs, 8-bit repetition counter, break input, dead-time),
 * TIM2 / TIM5 32-bit and TIM3 / TIM4 16-bit general purpose (4 channels),
 * TIM9 / TIM12 general purpose with 2 channels and slave mode controller,
 * TIM10 / TIM11 / TIM13 / TIM14 general purpose with 1 channel and TIM6 /
 * TIM7 basic timers. Availability of timers depends on the device.
 *
 * Functions of features not existing on STM32F4 timers (break 2, system break,
 * encoder index / direction / error interrupts, dithering, UIF remapping,
 * trigger output 2, ETR source selection, combined / asymmetric PWM, combined
 * slave modes) are kept for compatibility of the module interface and return
 * \ref TIM_REQUEST_ERROR.
 *
 */
/* ============================== INCLUDES ================================== */
#include "Tim.h"                            /* Self include                   */
#include "Tim_Port.h"                       /* Own port file include          */
#include "Tim_Types.h"                      /* Module types definitions       */
#include "Nvic_Port.h"                      /* NVIC handler functionality     */
#include "Rcc_Port.h"                       /* RCC handler functionality      */
#include "Gpio_Port.h"                      /* GPIO handler functionality     */
#include "Stm32.h"                          /* MCU common functionality       */
#include "Stm32_tim.h"                      /* Timer module functionality     */
#include <stddef.h>                         /* Register offsets (offsetof)    */
/* ============================== TYPEDEFS ================================== */

/** \brief LL compare register write function type */
typedef void ( tim_LlSetCompare_t )( TIM_TypeDef *TIMx, uint32_t CompareValue );


/** \brief LL compare register read function type */
typedef uint32_t ( tim_LlGetCompare_t )( const TIM_TypeDef *TIMx );


/** \brief Timer channel register identification */
typedef struct
{
    tim_ChannelId_t     ChannelId;             /**< Channel identification                                      */
    uint32_t            ChannelReg;            /**< LL channel identification (capture / compare)               */
    uint32_t            ChannelOutputReg;      /**< LL channel output identification                            */
    uint32_t            ChannelComplOutputReg; /**< LL complementary output identification or \ref TIM_CHANNEL_NO_OUTPUT */
    tim_OutputId_t      OutputId;              /**< Channel output identification                               */
    tim_OutputId_t      ComplOutputId;         /**< Complementary output identification or \ref TIM_OUTPUT_CNT  */
    tim_LlSetCompare_t *SetCompare;            /**< LL compare register write function                          */
    tim_LlGetCompare_t *GetCompare;            /**< LL compare register read function                           */
}   tim_ChannelConfigStruct_t;


/** \brief Configuration structure of Interrupt Service Routines */
typedef struct
{
    nvic_IsrCallback_t   GeneralIsr;          /**< Combined all events interrupt service routine                       */
    nvic_PeriphIrqList_t GeneralIrqId;        /**< Combined all events interrupt request NVIC identification           */
    nvic_IsrCallback_t   BreakIsr;            /**< Break event interrupt service routine                               */
    nvic_PeriphIrqList_t BreakIrqId;          /**< Break event interrupt request NVIC identification                   */
    nvic_IsrCallback_t   UpdateIsr;           /**< Update event interrupt service routine                              */
    nvic_PeriphIrqList_t UpdateIrqId;         /**< Update event interrupt request NVIC identification                  */
    nvic_IsrCallback_t   TriggerCommIsr;      /**< Trigger and Commutation event interrupt service routine             */
    nvic_PeriphIrqList_t TriggerCommIrqId;    /**< Trigger and Commutation event interrupt request NVIC identification */
    nvic_IsrCallback_t   CaptureCompareIsr;   /**< Capture&Compare interrupt service routine                           */
    nvic_PeriphIrqList_t CaptureCompareIrqId; /**< Capture&Compare interrupt request NVIC identification               */
}   tim_IsrConfig_t;


/** \brief Available peripherals configuration structure. */
typedef struct
{
    TIM_TypeDef     *PeriphReg;         /**< Timer configuration register                                */
    rcc_PeriphId_t   PeriphRcc;         /**< Timer RCC configuration ID                                  */
    tim_Resolution_t Resolution;        /**< Timer resolution in bits                                    */
    tim_ChannelCnt_t ChannelCount;      /**< Count of available channels for timer                       */
    tim_ChannelCnt_t ComplChannelCount; /**< Count of channels with complementary output (channels 1..N) */
    tim_IsrConfig_t  IsrConfig;         /**< Configuration of module internal ISR's                      */
}   tim_ConfigStruct_t;


/** \brief User interrupt callbacks of timer peripheral (interrupts available on STM32F4) */
typedef struct
{
    tim_UpdateIsrCallback_t         *UpdateIsr;                             /**< Update interrupt callback             */
    tim_CaptureComapreIsrCallback_t *CaptureCompareIsr[ TIM_CHANNEL_CNT ];  /**< Capture / compare callbacks           */
    tim_CommutationIsrCallback_t    *CommutationIsr;                        /**< Commutation interrupt callback        */
    tim_TriggerIsrCallback_t        *TriggerIsr;                            /**< Trigger interrupt callback            */
    tim_BreakIsrCallback_t          *BreakIsr;                              /**< Break interrupt callback              */
}   tim_IsrCallback_t;


/** \brief Group of channel modes (configuration function of the mode) */
typedef enum
{
    TIM_MODE_GROUP_INPUT = 0u,  /**< Input capture                         */
    TIM_MODE_GROUP_FORCED,      /**< Forced output (\ref Tim_Set_Mode_ForcedOutput)  */
    TIM_MODE_GROUP_COMPARE,     /**< Output compare (\ref Tim_Set_Mode_OutputCompare) */
    TIM_MODE_GROUP_PWM          /**< PWM (\ref Tim_Set_Mode_Pwm)                      */
}   tim_ModeGroup_t;


/** \brief Channel mode configuration */
typedef struct
{
    tim_ChannelMode_t ChannelModeId; /**< Channel mode identification                                        */
    uint32_t          ModeRegVal;    /**< LL output compare mode register value or \ref TIM_REG_VAL_UNAVAILABLE */
    tim_ModeGroup_t   ModeGroup;     /**< Group of the channel mode                                          */
}   tim_ChannelModeConfig_t;


/** \brief Timer channel output identification */
typedef struct
{
    tim_OutputId_t  OutputId;     /**< Output identification                                        */
    tim_ChannelId_t ChannelId;    /**< Channel the output belongs to                                */
    tim_FlagState_t ComplOutput;  /**< \ref TIM_FLAG_ACTIVE for complementary output                */
    uint32_t        OutputReg;    /**< LL channel output identification (LL_TIM_CHANNEL_CHx / CHxN) */
}   tim_OutputConfig_t;


/** \brief Availability condition of timer feature (event, interrupt) */
typedef enum
{
    TIM_FEATURE_AVAIL_ALWAYS = 0u,  /**< Event available on all timers               */
    TIM_FEATURE_AVAIL_CHANNEL,      /**< Capture / compare channel must be available */
    TIM_FEATURE_AVAIL_COMMUTATION,  /**< Commutation feature must be available       */
    TIM_FEATURE_AVAIL_SLAVE,        /**< Slave mode controller must be available     */
    TIM_FEATURE_AVAIL_BREAK,        /**< Break feature must be available             */
    TIM_FEATURE_AVAIL_ENCODER,      /**< Encoder interface must be available         */
    TIM_FEATURE_AVAIL_MASTER,       /**< Master mode (TRGO) must be available        */
    TIM_FEATURE_AVAIL_CLOCK_DIV,    /**< Clock division (CKD) must be available      */
    TIM_FEATURE_AVAIL_HALL,         /**< Hall sensor interface must be available     */
    TIM_FEATURE_AVAIL_DMA,          /**< DMA requests must be available              */
    TIM_FEATURE_AVAIL_DMA_CC,       /**< Capture / compare DMA requests available    */
    TIM_FEATURE_AVAIL_DMA_BURST,    /**< DMA burst must be available                 */
    TIM_FEATURE_AVAIL_ETR,          /**< External trigger input must be available    */
    TIM_FEATURE_AVAIL_NONE          /**< Feature does not exist on STM32F4 timers    */
}   tim_FeatureAvail_t;


/** \brief LL action function type (event generation, interrupt enable, flag clear) */
typedef void ( tim_LlAction_t )( TIM_TypeDef *TIMx );


/** \brief LL state function type (interrupt enable state, flag state) */
typedef uint32_t ( tim_LlState_t )( const TIM_TypeDef *TIMx );


/** \brief Software generated event configuration */
typedef struct
{
    tim_EventId_t          EventId;       /**< Event identification                          */
    tim_FeatureAvail_t     Availability;  /**< Availability condition of the event           */
    tim_ChannelId_t        ChannelId;     /**< Channel (\ref TIM_FEATURE_AVAIL_CHANNEL only)   */
    tim_LlAction_t        *GenerateEvent; /**< LL event generation function (NULL if not available) */
}   tim_EventConfig_t;


/** \brief Timer interrupt groups (NVIC lines of timers with multiple interrupt lines) */
typedef enum
{
    TIM_IRQ_LINE_UPDATE = 0u,       /**< Update interrupt line                                  */
    TIM_IRQ_LINE_CAPTURE_COMPARE,   /**< Capture / compare interrupt line                       */
    TIM_IRQ_LINE_TRIGGER_COMM,      /**< Trigger and commutation line                           */
    TIM_IRQ_LINE_BREAK,             /**< Break line                                             */
    TIM_IRQ_LINE_CNT,               /**< Count of interrupt groups                              */
    TIM_IRQ_LINE_ALL = TIM_IRQ_LINE_CNT /**< All groups (global timer interrupt line)           */
}   tim_IrqLine_t;


/** \brief Timer interrupt request configuration */
typedef struct
{
    tim_IrqId_t         IrqId;        /**< Interrupt request identification                   */
    tim_FeatureAvail_t  Availability; /**< Availability condition of the interrupt            */
    tim_ChannelId_t     ChannelId;    /**< Channel (\ref TIM_FEATURE_AVAIL_CHANNEL only)      */
    tim_IrqLine_t       IrqLine;      /**< Interrupt group (NVIC line)                        */
    tim_LlAction_t     *EnableIt;     /**< LL interrupt enable function                       */
    tim_LlAction_t     *DisableIt;    /**< LL interrupt disable function                      */
    tim_LlState_t      *IsEnabledIt;  /**< LL interrupt enable state function                 */
    tim_LlState_t      *IsActiveFlag; /**< LL status flag state function                      */
    tim_LlAction_t     *ClearFlag;    /**< LL status flag clear function                      */
}   tim_IrqConfig_t;


/** \brief Capture / compare over-capture flag configuration */
typedef struct
{
    tim_ChannelId_t     ChannelId;    /**< Channel identification                             */
    tim_LlState_t      *IsActiveFlag; /**< LL over-capture flag state function                */
    tim_LlAction_t     *ClearFlag;    /**< LL over-capture flag clear function                */
}   tim_CcOvrConfig_t;


/** \brief DMA request configuration */
typedef struct
{
    tim_FeatureAvail_t  Availability; /**< Availability condition of the request              */
    tim_ChannelId_t     ChannelId;    /**< Channel of capture / compare request or TIM_CHANNEL_CNT */
    tim_LlAction_t     *EnableReq;    /**< LL DMA request enable function                     */
    tim_LlAction_t     *DisableReq;   /**< LL DMA request disable function                    */
    tim_LlState_t      *IsEnabledReq; /**< LL DMA request state function                      */
}   tim_DmaRequestConfig_t;


/** \brief Timer register for DMA transfer */
typedef struct
{
    tim_FeatureAvail_t  Availability; /**< Availability condition of the register             */
    tim_ChannelId_t     ChannelId;    /**< Channel of capture / compare register              */
    uint32_t            RegOffset;    /**< Register offset in timer register structure        */
}   tim_DmaRegConfig_t;

/* ========================== SYMBOLIC CONSTANTS ============================ */

/** Value of major version of SW module */
#define TIM_MAJOR_VERSION           ( 1u )

/** Value of minor version of SW module */
#define TIM_MINOR_VERSION           ( 0u )

/** Value of patch version of SW module */
#define TIM_PATCH_VERSION           ( 0u )


/** Maximum wait time for configuration request confirmation */
#define TIM_TIMEOUT_RAW             ( 0x84FCB )

/** Channel has no (complementary) output in \ref tim_ChannelConfig */
#define TIM_CHANNEL_NO_OUTPUT       ( 0u )

/** Register value of feature not available on STM32F4 (look-up tables) */
#define TIM_REG_VAL_UNAVAILABLE     ( 0xFFFFFFFFu )

/** Prescaler / auto-reload registers hold divider value decremented by 1 */
#define TIM_REG_DIV_OFFSET          ( 1u )

/** Duty cycle 100 % in hundredths of percent (\ref tim_CentiPercent_t) */
#define TIM_DUTY_CYCLE_MAX          ( 10000u )

/** Default timer counter frequency used by \ref Tim_Get_DefaultConfig */
#define TIM_DEFAULT_TIMER_FREQ_HZ   ( 10000000u )

/** Default refresh (period) frequency used by \ref Tim_Get_DefaultConfig */
#define TIM_DEFAULT_REFRESH_FREQ_HZ ( 1000000u )

/** Minimum value of timer prescaler register */
#define TIM_PRESCALER_MIN           ( 1u )

/** Maximum value of timer prescaler register */
#define TIM_PRESCALER_MAX           ( 65535u )

/** Counter register value after \ref Tim_Stop */
#define TIM_COUNTER_RESET_VAL       ( 0u )

/** Count of nanoseconds in one second */
#define TIM_NS_PER_S                ( 1000000000u )

/** Maximum value representable by \ref tim_Time_ns_t */
#define TIM_TIME_NS_MAX             ( 0xFFFFFFFFu )

/** Maximum repetition counter value (8-bit RCR of advanced timers TIM1 / TIM8) */
#define TIM_REP_CNT_MAX             ( 0xFFu )

/** Count of channels with capture / compare interrupt (channels 1 - 4) */
#define TIM_CC_IRQ_CHANNEL_CNT      ( 4u )

/** Count of channels with input stage (channels 1 - 4) */
#define TIM_INPUT_CHANNEL_CNT       ( 4u )

/** Dead-time generator range 2: (offset + DTG[5:0]) * step fDTS periods */
#define TIM_DTG_R2_OFFSET           ( 64u )

/** Dead-time generator range 2 step in fDTS periods */
#define TIM_DTG_R2_STEP             ( 2u )

/** Dead-time generator range 3: (offset + DTG[4:0]) * step fDTS periods */
#define TIM_DTG_R3_OFFSET           ( 32u )

/** Dead-time generator range 3 step in fDTS periods */
#define TIM_DTG_R3_STEP             ( 8u )

/** Dead-time generator range 4: (offset + DTG[4:0]) * step fDTS periods */
#define TIM_DTG_R4_OFFSET           ( 32u )

/** Dead-time generator range 4 step in fDTS periods */
#define TIM_DTG_R4_STEP             ( 16u )

/** Minimum count of registers transferred in one DMA burst */
#define TIM_DMA_BURST_LEN_MIN       ( 1u )

/** Maximum count of registers transferred in one DMA burst (STM32F4: 18) */
#define TIM_DMA_BURST_LEN_MAX       ( 18u )

/** Register bit(s) cleared (enable bit read-back value of disabled function) */
#define TIM_REG_BIT_CLEARED         ( 0u )

/** Count of selection codes of the input remap fields (field values 0 - 3) */
#define TIM_INPUT_REMAP_SOURCE_CNT  ( 4u )

/** NVIC line of TIM1 update interrupt (shared with TIM10 on devices with TIM10) */
#if defined( TIM10 )
#define TIM_TIM1_UP_IRQ             ( NVIC_PERIPH_IRQ_TIM1_UP_TIM10 )
#else
#define TIM_TIM1_UP_IRQ             ( NVIC_PERIPH_IRQ_TIM1_UP )
#endif

/** NVIC line of TIM6 interrupt (shared with DAC underrun on devices with DAC) */
#if defined( DAC )
#define TIM_TIM6_IRQ                ( NVIC_PERIPH_IRQ_TIM6_DAC )
#else
#define TIM_TIM6_IRQ                ( NVIC_PERIPH_IRQ_TIM6 )
#endif

/* ============================== TYPEDEFS ================================== */

/** \brief Input remap (TIMx_OR) of timer channel */
typedef struct
{
    TIM_TypeDef        *PeriphReg;                              /**< Timer with input remap                  */
    tim_ChannelId_t     ChannelId;                              /**< Remapped channel input                  */
    uint32_t            SelMask;                                /**< Remap field mask in TIMx_OR             */
    uint32_t            SelVal[ TIM_INPUT_REMAP_SOURCE_CNT ];   /**< Remap field value of input sources      */
}   tim_InputRemapConfig_t;

/* ======================== FORWARD DECLARATIONS ============================ */

static tim_RequestState_t Tim_Calc_StepTime( tim_PeriphId_t periphId, uint32_t clkDivider, tim_Time_ns_t * const stepTime );
static tim_RequestState_t Tim_Check_OutputId( tim_PeriphId_t periphId, tim_OutputId_t outputId );
static tim_RequestState_t Tim_Check_TriggerInput( tim_PeriphId_t periphId, tim_ExtClkSource_t triggerInput );
static tim_FunctionState_t Tim_Conv_RawToFunctionState( uint32_t rawState );
static tim_RequestState_t Tim_Config_CounterDirection( tim_PeriphId_t periphId, tim_CounterDir_t counterDir );
static tim_RequestState_t Tim_Config_UpdateEvent( tim_PeriphId_t periphId, tim_FunctionState_t eventState );
static tim_RequestState_t Tim_Config_ArrPreload( tim_PeriphId_t periphId, tim_FunctionState_t preloadState );
static tim_RequestState_t Tim_Config_OnePulseMode( tim_PeriphId_t periphId, tim_FunctionState_t modeState );
static tim_RequestState_t Tim_Check_EventId( tim_PeriphId_t periphId, tim_EventId_t eventId );
static tim_RequestState_t Tim_Check_Feature( tim_PeriphId_t periphId, tim_FeatureAvail_t availability, tim_ChannelId_t channelId );
static tim_RequestState_t Tim_Check_IrqId( tim_PeriphId_t periphId, tim_IrqId_t irqId );
static tim_RequestState_t Tim_Get_NvicLine( tim_PeriphId_t periphId, tim_IrqLine_t irqLine, nvic_PeriphIrqList_t * const nvicIrqId, nvic_IsrCallback_t * const nvicIsr );
static void               Tim_Process_Irq( tim_PeriphId_t periphId, tim_IrqLine_t irqLine );
static void               Tim_Handle_Irq( tim_PeriphId_t periphId, tim_IrqId_t irqId );
static tim_RequestState_t Tim_Check_ChannelId( tim_PeriphId_t periphId, tim_ChannelId_t channelId );
static tim_RequestState_t Tim_Check_ChannelConfig( tim_PeriphId_t periphId, const tim_ChannelConfig_t * const channelConfig, tim_ModeGroup_t modeGroup );
static tim_RequestState_t Tim_Config_OutputChannel( tim_PeriphId_t periphId, const tim_ChannelConfig_t * const channelConfig, tim_FunctionState_t preloadState );
static tim_RequestState_t Tim_Config_ComparePreload( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_FunctionState_t preloadState );
static tim_RequestState_t Tim_Config_MainOutput( tim_PeriphId_t periphId, tim_FunctionState_t outputState );
static tim_RequestState_t Tim_Config_MasterSlaveMode( tim_PeriphId_t periphId, tim_FunctionState_t modeState );
static tim_RequestState_t Tim_Config_SlaveModeReg( tim_PeriphId_t periphId, uint32_t slaveModeReg );
static tim_RequestState_t Tim_Check_InputChannel( tim_PeriphId_t periphId, tim_ChannelId_t channelId );
static tim_RequestState_t Tim_Check_InputPolarity( tim_InputPolarity_t inputPolarity );
static tim_RequestState_t Tim_Config_InputChannel( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_ActiveInput_t activeInput,
                                                   tim_InputPolarity_t inputPolarity, tim_InputFilter_t inputFilter,
                                                   tim_InputPrescaler_t inputPrescaler );
static const tim_InputRemapConfig_t * Tim_Get_InputRemap( tim_PeriphId_t periphId, tim_ChannelId_t channelId );
static tim_RequestState_t Tim_Check_InputSource( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_InputSource_t inputSource );
static tim_RequestState_t Tim_Calc_DeadTimeReg( tim_PeriphId_t periphId, tim_Time_ns_t deadTime, uint32_t * const dtgReg );
static tim_RequestState_t Tim_Check_BreakConfig( tim_PeriphId_t periphId, const tim_BreakConfig_t * const breakConfig );
static tim_RequestState_t Tim_Config_AutomaticOutput( tim_PeriphId_t periphId, tim_FunctionState_t outputState );
static tim_RequestState_t Tim_Config_BreakPinPolarity( tim_PeriphId_t periphId, tim_Polarity_t pinPolarity );
static tim_RequestState_t Tim_Check_DmaRequest( tim_PeriphId_t periphId, tim_DmaRequest_t dmaRequest );
static tim_RequestState_t Tim_Config_DmaRequest( tim_PeriphId_t periphId, tim_DmaRequest_t dmaRequest, tim_FunctionState_t requestState );

#ifdef TIM1
static void Tim_Tim1_Break_IsrHandler( void );
static void Tim_Tim1_Update_IsrHandler( void );
static void Tim_Tim1_Trigger_IsrHandler( void );
static void Tim_Tim1_CaptureCompare_IsrHandler( void );
#endif

#ifdef TIM2
static void Tim_Tim2_General_IsrHandler( void );
#endif

#ifdef TIM3
static void Tim_Tim3_General_IsrHandler( void );
#endif

#ifdef TIM4
static void Tim_Tim4_General_IsrHandler( void );
#endif

#ifdef TIM5
static void Tim_Tim5_General_IsrHandler( void );
#endif

#ifdef TIM6
static void Tim_Tim6_General_IsrHandler( void );
#endif

#ifdef TIM7
static void Tim_Tim7_General_IsrHandler( void );
#endif

#ifdef TIM8
static void Tim_Tim8_Break_IsrHandler( void );
static void Tim_Tim8_Update_IsrHandler( void );
static void Tim_Tim8_Trigger_IsrHandler( void );
static void Tim_Tim8_CaptureCompare_IsrHandler( void );
#endif

/* =============================== MACROS =================================== */

/** Trigger input item of the timer PERIPH_ID for the SMCR.TS code TS_CODE (inputs TI1F_ED, TI1FP1, TI2FP2 of the timer itself) */
#define TIM_TRIGGER_INPUT_OF(PERIPH_ID,TS_CODE)  ( (tim_TriggerInput_t)TIM_TRIGGER_INPUT_BIT_MASK_ENCODE( (uint32_t)( PERIPH_ID ), ( TS_CODE ) ) )

/** Input source item of the timer PERIPH_ID, channel CHANNEL_ID for the selection code SEL_CODE of its input remap field */
#define TIM_INPUT_SOURCE_OF(PERIPH_ID,CHANNEL_ID,SEL_CODE)  ( (tim_InputSource_t)TIM_INPUT_SOURCE_BIT_MASK_ENCODE( (uint32_t)( PERIPH_ID ), (uint32_t)( CHANNEL_ID ), ( SEL_CODE ) ) )

/* ========================== EXPORTED VARIABLES ============================ */

/* =========================== LOCAL VARIABLES ============================== */

/**
 * Timer peripherals configuration array.
 *
 * TIM9 - TIM11 share NVIC lines with TIM1 break / update / trigger interrupts,
 * TIM12 - TIM14 with TIM8 break / update / trigger interrupts. The shared line
 * handler processes interrupts of both timers.
 */
static tim_ConfigStruct_t const         tim_PeriphConf[ TIM_PERIPH_CNT ] =
{
#ifdef TIM1
  { .PeriphReg = TIM1 , .PeriphRcc = RCC_PERIPH_TIM1 , .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 4u, .ComplChannelCount = 3u,
    .IsrConfig = { .GeneralIsr        = TIM_NULL_PTR                        , .GeneralIrqId        = NVIC_PERIPH_IRQ_SIZE,
                   .BreakIsr          = Tim_Tim1_Break_IsrHandler           , .BreakIrqId          = NVIC_PERIPH_IRQ_TIM1_BRK_TIM9,
                   .UpdateIsr         = Tim_Tim1_Update_IsrHandler          , .UpdateIrqId         = TIM_TIM1_UP_IRQ,
                   .TriggerCommIsr    = Tim_Tim1_Trigger_IsrHandler         , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_TIM1_TRG_COM_TIM11,
                   .CaptureCompareIsr = Tim_Tim1_CaptureCompare_IsrHandler  , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_TIM1_CC  } },
#endif
#ifdef TIM2
  { .PeriphReg = TIM2 , .PeriphRcc = RCC_PERIPH_TIM2 , .Resolution = TIM_RESOLUTION_32BIT, .ChannelCount = 4u, .ComplChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim2_General_IsrHandler         , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM2,
                   .BreakIsr          = TIM_NULL_PTR                        , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = TIM_NULL_PTR                        , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = TIM_NULL_PTR                        , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = TIM_NULL_PTR                        , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM3
  { .PeriphReg = TIM3 , .PeriphRcc = RCC_PERIPH_TIM3 , .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 4u, .ComplChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim3_General_IsrHandler         , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM3,
                   .BreakIsr          = TIM_NULL_PTR                        , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = TIM_NULL_PTR                        , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = TIM_NULL_PTR                        , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = TIM_NULL_PTR                        , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM4
  { .PeriphReg = TIM4 , .PeriphRcc = RCC_PERIPH_TIM4 , .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 4u, .ComplChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim4_General_IsrHandler         , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM4,
                   .BreakIsr          = TIM_NULL_PTR                        , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = TIM_NULL_PTR                        , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = TIM_NULL_PTR                        , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = TIM_NULL_PTR                        , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM5
  { .PeriphReg = TIM5 , .PeriphRcc = RCC_PERIPH_TIM5 , .Resolution = TIM_RESOLUTION_32BIT, .ChannelCount = 4u, .ComplChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim5_General_IsrHandler         , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM5,
                   .BreakIsr          = TIM_NULL_PTR                        , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = TIM_NULL_PTR                        , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = TIM_NULL_PTR                        , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = TIM_NULL_PTR                        , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM6
  { .PeriphReg = TIM6 , .PeriphRcc = RCC_PERIPH_TIM6 , .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 0u, .ComplChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim6_General_IsrHandler         , .GeneralIrqId        = TIM_TIM6_IRQ,
                   .BreakIsr          = TIM_NULL_PTR                        , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = TIM_NULL_PTR                        , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = TIM_NULL_PTR                        , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = TIM_NULL_PTR                        , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM7
  { .PeriphReg = TIM7 , .PeriphRcc = RCC_PERIPH_TIM7 , .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 0u, .ComplChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim7_General_IsrHandler         , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM7,
                   .BreakIsr          = TIM_NULL_PTR                        , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = TIM_NULL_PTR                        , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = TIM_NULL_PTR                        , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = TIM_NULL_PTR                        , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM8
  { .PeriphReg = TIM8 , .PeriphRcc = RCC_PERIPH_TIM8 , .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 4u, .ComplChannelCount = 3u,
    .IsrConfig = { .GeneralIsr        = TIM_NULL_PTR                        , .GeneralIrqId        = NVIC_PERIPH_IRQ_SIZE,
                   .BreakIsr          = Tim_Tim8_Break_IsrHandler           , .BreakIrqId          = NVIC_PERIPH_IRQ_TIM8_BRK_TIM12,
                   .UpdateIsr         = Tim_Tim8_Update_IsrHandler          , .UpdateIrqId         = NVIC_PERIPH_IRQ_TIM8_UP_TIM13,
                   .TriggerCommIsr    = Tim_Tim8_Trigger_IsrHandler         , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_TIM8_TRG_COM_TIM14,
                   .CaptureCompareIsr = Tim_Tim8_CaptureCompare_IsrHandler  , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_TIM8_CC  } },
#endif
#ifdef TIM9
  { .PeriphReg = TIM9 , .PeriphRcc = RCC_PERIPH_TIM9 , .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 2u, .ComplChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim1_Break_IsrHandler           , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM1_BRK_TIM9,
                   .BreakIsr          = TIM_NULL_PTR                        , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = TIM_NULL_PTR                        , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = TIM_NULL_PTR                        , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = TIM_NULL_PTR                        , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM10
  { .PeriphReg = TIM10, .PeriphRcc = RCC_PERIPH_TIM10, .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 1u, .ComplChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim1_Update_IsrHandler          , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM1_UP_TIM10,
                   .BreakIsr          = TIM_NULL_PTR                        , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = TIM_NULL_PTR                        , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = TIM_NULL_PTR                        , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = TIM_NULL_PTR                        , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM11
  { .PeriphReg = TIM11, .PeriphRcc = RCC_PERIPH_TIM11, .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 1u, .ComplChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim1_Trigger_IsrHandler         , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM1_TRG_COM_TIM11,
                   .BreakIsr          = TIM_NULL_PTR                        , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = TIM_NULL_PTR                        , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = TIM_NULL_PTR                        , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = TIM_NULL_PTR                        , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM12
  { .PeriphReg = TIM12, .PeriphRcc = RCC_PERIPH_TIM12, .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 2u, .ComplChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim8_Break_IsrHandler           , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM8_BRK_TIM12,
                   .BreakIsr          = TIM_NULL_PTR                        , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = TIM_NULL_PTR                        , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = TIM_NULL_PTR                        , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = TIM_NULL_PTR                        , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM13
  { .PeriphReg = TIM13, .PeriphRcc = RCC_PERIPH_TIM13, .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 1u, .ComplChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim8_Update_IsrHandler          , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM8_UP_TIM13,
                   .BreakIsr          = TIM_NULL_PTR                        , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = TIM_NULL_PTR                        , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = TIM_NULL_PTR                        , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = TIM_NULL_PTR                        , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM14
  { .PeriphReg = TIM14, .PeriphRcc = RCC_PERIPH_TIM14, .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 1u, .ComplChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim8_Trigger_IsrHandler         , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM8_TRG_COM_TIM14,
                   .BreakIsr          = TIM_NULL_PTR                        , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = TIM_NULL_PTR                        , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = TIM_NULL_PTR                        , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = TIM_NULL_PTR                        , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
};


/** Channel modes, indexed by \ref tim_ChannelMode_t (modes with 4-bit OCxM are not available on STM32F4) */
static const tim_ChannelModeConfig_t    tim_ChannelModeConfig[ TIM_CHANNEL_MODE_CNT ] =
{
    { .ChannelModeId = TIM_CHANNEL_MODE_INPUT_CAPTURE                  , .ModeRegVal = 0u                             , .ModeGroup = TIM_MODE_GROUP_INPUT   },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_FORCED_ACTIVE           , .ModeRegVal = LL_TIM_OCMODE_FORCED_ACTIVE    , .ModeGroup = TIM_MODE_GROUP_FORCED  },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_FORCED_INACTIVE         , .ModeRegVal = LL_TIM_OCMODE_FORCED_INACTIVE  , .ModeGroup = TIM_MODE_GROUP_FORCED  },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_COMPARE_PULSE           , .ModeRegVal = TIM_REG_VAL_UNAVAILABLE        , .ModeGroup = TIM_MODE_GROUP_COMPARE },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_COMPARE_FORCED_ACTIVE   , .ModeRegVal = LL_TIM_OCMODE_ACTIVE           , .ModeGroup = TIM_MODE_GROUP_COMPARE },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_COMPARE_FORCED_INACTIVE , .ModeRegVal = LL_TIM_OCMODE_INACTIVE         , .ModeGroup = TIM_MODE_GROUP_COMPARE },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_COMPARE_TOGGLE          , .ModeRegVal = LL_TIM_OCMODE_TOGGLE           , .ModeGroup = TIM_MODE_GROUP_COMPARE },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_PWM                     , .ModeRegVal = LL_TIM_OCMODE_PWM1             , .ModeGroup = TIM_MODE_GROUP_PWM     },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_PWM_INV                 , .ModeRegVal = LL_TIM_OCMODE_PWM2             , .ModeGroup = TIM_MODE_GROUP_PWM     },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_COMBINED_PWM            , .ModeRegVal = TIM_REG_VAL_UNAVAILABLE        , .ModeGroup = TIM_MODE_GROUP_PWM     },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_COMBINED_PWM_INV        , .ModeRegVal = TIM_REG_VAL_UNAVAILABLE        , .ModeGroup = TIM_MODE_GROUP_PWM     },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_ASSYMETRIC_PWM          , .ModeRegVal = TIM_REG_VAL_UNAVAILABLE        , .ModeGroup = TIM_MODE_GROUP_PWM     },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_ASSYMETRIC_PWM_INV      , .ModeRegVal = TIM_REG_VAL_UNAVAILABLE        , .ModeGroup = TIM_MODE_GROUP_PWM     },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_DIRECTION               , .ModeRegVal = TIM_REG_VAL_UNAVAILABLE        , .ModeGroup = TIM_MODE_GROUP_COMPARE },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_FROZEN                  , .ModeRegVal = LL_TIM_OCMODE_FROZEN           , .ModeGroup = TIM_MODE_GROUP_COMPARE },
};


/**
 * Timer channels, indexed by \ref tim_ChannelId_t. STM32F4 timers have channels
 * 1 - 4 only (entries of channels 5 / 6 are empty, rejected by channel count),
 * channel 4 has no complementary output.
 */
static const tim_ChannelConfigStruct_t  tim_ChannelConfig[ TIM_CHANNEL_CNT ] =
{
    { .ChannelId = TIM_CHANNEL_1, .ChannelReg = LL_TIM_CHANNEL_CH1, .ChannelOutputReg = LL_TIM_CHANNEL_CH1, .ChannelComplOutputReg = LL_TIM_CHANNEL_CH1N,
      .OutputId  = TIM_OUTPUT_1 , .ComplOutputId = TIM_OUTPUT_1_N, .SetCompare = LL_TIM_OC_SetCompareCH1, .GetCompare = LL_TIM_OC_GetCompareCH1 },
    { .ChannelId = TIM_CHANNEL_2, .ChannelReg = LL_TIM_CHANNEL_CH2, .ChannelOutputReg = LL_TIM_CHANNEL_CH2, .ChannelComplOutputReg = LL_TIM_CHANNEL_CH2N,
      .OutputId  = TIM_OUTPUT_2 , .ComplOutputId = TIM_OUTPUT_2_N, .SetCompare = LL_TIM_OC_SetCompareCH2, .GetCompare = LL_TIM_OC_GetCompareCH2 },
    { .ChannelId = TIM_CHANNEL_3, .ChannelReg = LL_TIM_CHANNEL_CH3, .ChannelOutputReg = LL_TIM_CHANNEL_CH3, .ChannelComplOutputReg = LL_TIM_CHANNEL_CH3N,
      .OutputId  = TIM_OUTPUT_3 , .ComplOutputId = TIM_OUTPUT_3_N, .SetCompare = LL_TIM_OC_SetCompareCH3, .GetCompare = LL_TIM_OC_GetCompareCH3 },
    { .ChannelId = TIM_CHANNEL_4, .ChannelReg = LL_TIM_CHANNEL_CH4, .ChannelOutputReg = LL_TIM_CHANNEL_CH4, .ChannelComplOutputReg = TIM_CHANNEL_NO_OUTPUT,
      .OutputId  = TIM_OUTPUT_4 , .ComplOutputId = TIM_OUTPUT_CNT, .SetCompare = LL_TIM_OC_SetCompareCH4, .GetCompare = LL_TIM_OC_GetCompareCH4 },
};


/** Timer channel outputs, indexed by \ref tim_OutputId_t (CH4N / CH5 / CH6 do not exist on STM32F4) */
static const tim_OutputConfig_t         tim_OutputLut[ TIM_OUTPUT_CNT ] =
{
    { .OutputId = TIM_OUTPUT_1  , .ChannelId = TIM_CHANNEL_1, .ComplOutput = TIM_FLAG_INACTIVE, .OutputReg = LL_TIM_CHANNEL_CH1    },
    { .OutputId = TIM_OUTPUT_1_N, .ChannelId = TIM_CHANNEL_1, .ComplOutput = TIM_FLAG_ACTIVE  , .OutputReg = LL_TIM_CHANNEL_CH1N   },
    { .OutputId = TIM_OUTPUT_2  , .ChannelId = TIM_CHANNEL_2, .ComplOutput = TIM_FLAG_INACTIVE, .OutputReg = LL_TIM_CHANNEL_CH2    },
    { .OutputId = TIM_OUTPUT_2_N, .ChannelId = TIM_CHANNEL_2, .ComplOutput = TIM_FLAG_ACTIVE  , .OutputReg = LL_TIM_CHANNEL_CH2N   },
    { .OutputId = TIM_OUTPUT_3  , .ChannelId = TIM_CHANNEL_3, .ComplOutput = TIM_FLAG_INACTIVE, .OutputReg = LL_TIM_CHANNEL_CH3    },
    { .OutputId = TIM_OUTPUT_3_N, .ChannelId = TIM_CHANNEL_3, .ComplOutput = TIM_FLAG_ACTIVE  , .OutputReg = LL_TIM_CHANNEL_CH3N   },
    { .OutputId = TIM_OUTPUT_4  , .ChannelId = TIM_CHANNEL_4, .ComplOutput = TIM_FLAG_INACTIVE, .OutputReg = LL_TIM_CHANNEL_CH4    },
    { .OutputId = TIM_OUTPUT_4_N, .ChannelId = TIM_CHANNEL_4, .ComplOutput = TIM_FLAG_ACTIVE  , .OutputReg = TIM_CHANNEL_NO_OUTPUT },
    { .OutputId = TIM_OUTPUT_5  , .ChannelId = TIM_CHANNEL_5, .ComplOutput = TIM_FLAG_INACTIVE, .OutputReg = TIM_CHANNEL_NO_OUTPUT },
    { .OutputId = TIM_OUTPUT_6  , .ChannelId = TIM_CHANNEL_6, .ComplOutput = TIM_FLAG_INACTIVE, .OutputReg = TIM_CHANNEL_NO_OUTPUT },
};


/** SMCR.TS codes of the trigger inputs of the device (codes of the items of \ref tim_ExtClkSource_t) */
static const uint32_t                   tim_TriggerCodeLut[] =
{
    LL_TIM_TS_ITR0 , LL_TIM_TS_ITR1 , LL_TIM_TS_ITR2 , LL_TIM_TS_ITR3 ,
    LL_TIM_TS_TI1F_ED , LL_TIM_TS_TI1FP1 , LL_TIM_TS_TI2FP2 , LL_TIM_TS_ETRF ,
};

/** Count of items in \ref tim_TriggerCodeLut */
#define TIM_TRIGGER_CODE_CNT        ( sizeof( tim_TriggerCodeLut ) / sizeof( tim_TriggerCodeLut[ 0u ] ) )


/** LL counter mode register values, indexed by \ref tim_CounterDir_t */
static const uint32_t                   tim_CounterModeLut[ TIM_COUNTER_DIR_CNT ] =
{
    LL_TIM_COUNTERMODE_UP             , /* TIM_COUNTER_DIR_UP             */
    LL_TIM_COUNTERMODE_DOWN           , /* TIM_COUNTER_DIR_DOWN           */
    LL_TIM_COUNTERMODE_CENTER_UP      , /* TIM_COUNTER_DIR_CENTER_UP      */
    LL_TIM_COUNTERMODE_CENTER_DOWN    , /* TIM_COUNTER_DIR_CENTER_DOWN    */
    LL_TIM_COUNTERMODE_CENTER_UP_DOWN , /* TIM_COUNTER_DIR_CENTER_UP_DOWN */
};


/** LL update source register values, indexed by \ref tim_UpdateSource_t */
static const uint32_t                   tim_UpdateSourceLut[ TIM_UPDATE_SOURCE_CNT ] =
{
    LL_TIM_UPDATESOURCE_REGULAR, /* TIM_UPDATE_SOURCE_ANY     */
    LL_TIM_UPDATESOURCE_COUNTER, /* TIM_UPDATE_SOURCE_COUNTER */
};


/** LL trigger output (MMS) register values, indexed by \ref tim_MasterTrigger_t */
static const uint32_t                   tim_MasterTriggerLut[ TIM_MASTER_TRIGGER_CNT ] =
{
    LL_TIM_TRGO_RESET      , /* TIM_MASTER_TRIGGER_RESET       */
    LL_TIM_TRGO_ENABLE     , /* TIM_MASTER_TRIGGER_ENABLE      */
    LL_TIM_TRGO_UPDATE     , /* TIM_MASTER_TRIGGER_UPDATE      */
    LL_TIM_TRGO_CC1IF      , /* TIM_MASTER_TRIGGER_CC1         */
    LL_TIM_TRGO_OC1REF     , /* TIM_MASTER_TRIGGER_OC1REF      */
    LL_TIM_TRGO_OC2REF     , /* TIM_MASTER_TRIGGER_OC2REF      */
    LL_TIM_TRGO_OC3REF     , /* TIM_MASTER_TRIGGER_OC3REF      */
    LL_TIM_TRGO_OC4REF     , /* TIM_MASTER_TRIGGER_OC4REF      */
    TIM_REG_VAL_UNAVAILABLE, /* TIM_MASTER_TRIGGER_ENCODER_CLK */
};


/** LL slave mode (SMS) register values, indexed by \ref tim_SlaveMode_t */
static const uint32_t                   tim_SlaveModeLut[ TIM_SLAVE_MODE_CNT ] =
{
    LL_TIM_SLAVEMODE_DISABLED   , /* TIM_SLAVE_MODE_DISABLE        */
    LL_TIM_CLOCKSOURCE_EXT_MODE1, /* TIM_SLAVE_MODE_EXTERNAL_CLOCK */
    LL_TIM_SLAVEMODE_RESET      , /* TIM_SLAVE_MODE_RESET          */
    LL_TIM_SLAVEMODE_GATED      , /* TIM_SLAVE_MODE_GATED          */
    LL_TIM_SLAVEMODE_TRIGGER    , /* TIM_SLAVE_MODE_TRIGGER        */
    TIM_REG_VAL_UNAVAILABLE     , /* TIM_SLAVE_MODE_RESET_TRIGGER  */
    TIM_REG_VAL_UNAVAILABLE     , /* TIM_SLAVE_MODE_GATED_RESET    */
};


/** LL input filter register values, indexed by \ref tim_InputFilter_t */
static const uint32_t                   tim_InputFilterLut[ TIM_INPUT_FILTER_CNT ] =
{
    LL_TIM_IC_FILTER_FDIV1    , LL_TIM_IC_FILTER_FDIV1_N2 , LL_TIM_IC_FILTER_FDIV1_N4 , LL_TIM_IC_FILTER_FDIV1_N8 ,
    LL_TIM_IC_FILTER_FDIV2_N6 , LL_TIM_IC_FILTER_FDIV2_N8 , LL_TIM_IC_FILTER_FDIV4_N6 , LL_TIM_IC_FILTER_FDIV4_N8 ,
    LL_TIM_IC_FILTER_FDIV8_N6 , LL_TIM_IC_FILTER_FDIV8_N8 , LL_TIM_IC_FILTER_FDIV16_N5, LL_TIM_IC_FILTER_FDIV16_N6,
    LL_TIM_IC_FILTER_FDIV16_N8, LL_TIM_IC_FILTER_FDIV32_N5, LL_TIM_IC_FILTER_FDIV32_N6, LL_TIM_IC_FILTER_FDIV32_N8,
};


/** LL input capture prescaler register values, indexed by \ref tim_InputPrescaler_t */
static const uint32_t                   tim_InputPrescalerLut[ TIM_INPUT_PRESCALER_CNT ] =
{
    LL_TIM_ICPSC_DIV1, /* TIM_INPUT_PRESCALER_DIV1 */
    LL_TIM_ICPSC_DIV2, /* TIM_INPUT_PRESCALER_DIV2 */
    LL_TIM_ICPSC_DIV4, /* TIM_INPUT_PRESCALER_DIV4 */
    LL_TIM_ICPSC_DIV8, /* TIM_INPUT_PRESCALER_DIV8 */
};


/** LL active input register values, indexed by \ref tim_ActiveInput_t */
static const uint32_t                   tim_ActiveInputLut[ TIM_ACTIVE_INPUT_CNT ] =
{
    LL_TIM_ACTIVEINPUT_DIRECTTI  , /* TIM_ACTIVE_INPUT_DIRECT   */
    LL_TIM_ACTIVEINPUT_INDIRECTTI, /* TIM_ACTIVE_INPUT_INDIRECT */
    LL_TIM_ACTIVEINPUT_TRC       , /* TIM_ACTIVE_INPUT_TRC      */
};


/** LL clock division register values, indexed by \ref tim_ClockDiv_t */
static const uint32_t                   tim_ClockDivLut[ TIM_CLOCK_DIV_CNT ] =
{
    LL_TIM_CLOCKDIVISION_DIV1, /* TIM_CLOCK_DIV_1 */
    LL_TIM_CLOCKDIVISION_DIV2, /* TIM_CLOCK_DIV_2 */
    LL_TIM_CLOCKDIVISION_DIV4, /* TIM_CLOCK_DIV_4 */
};


/**
 * Input remap (TIMx_OR) of timer channels, register values indexed by the selection code of the item of
 * \ref tim_InputSource_t (value of the remap field 0 - 3).
 */
static const tim_InputRemapConfig_t     tim_InputRemapLut[] =
{
#if defined( TIM5 ) && defined( TIM_OR_TI4_RMP )
    /* TIM5 channel 4: GPIO, LSI, LSE, RTC wake-up interrupt */
    { .PeriphReg = TIM5 , .ChannelId = TIM_CHANNEL_4, .SelMask = TIM_OR_TI4_RMP,
      .SelVal = { 0u, TIM_OR_TI4_RMP_0, TIM_OR_TI4_RMP_1, TIM_OR_TI4_RMP } },
#endif
#if defined( TIM11 ) && defined( TIM_OR_TI1_RMP )
    /* TIM11 channel 1: GPIO, SPDIFRX frame synchronization (devices with SPDIFRX), HSE_RTC */
    { .PeriphReg = TIM11, .ChannelId = TIM_CHANNEL_1, .SelMask = TIM_OR_TI1_RMP,
#if defined( SPDIFRX )
      .SelVal = { 0u, TIM_OR_TI1_RMP_0, TIM_OR_TI1_RMP_1, TIM_REG_VAL_UNAVAILABLE } },
#else
      .SelVal = { 0u, TIM_REG_VAL_UNAVAILABLE, TIM_OR_TI1_RMP_1, TIM_REG_VAL_UNAVAILABLE } },
#endif
#endif
    /* Table end (no timer) */
    { .PeriphReg = TIM_NULL_PTR, .ChannelId = TIM_CHANNEL_CNT, .SelMask = 0u,
      .SelVal = { TIM_REG_VAL_UNAVAILABLE, TIM_REG_VAL_UNAVAILABLE, TIM_REG_VAL_UNAVAILABLE, TIM_REG_VAL_UNAVAILABLE } },
};

/** Count of items in \ref tim_InputRemapLut (including table end) */
#define TIM_INPUT_REMAP_CNT         ( sizeof( tim_InputRemapLut ) / sizeof( tim_InputRemapLut[ 0u ] ) )


/** LL encoder mode (SMS) register values, indexed by \ref tim_EncoderMode_t */
static const uint32_t                   tim_EncoderModeLut[ TIM_ENCODER_MODE_CNT ] =
{
    LL_TIM_ENCODERMODE_X2_TI1 , /* TIM_ENCODER_MODE_X2_TI1                  */
    LL_TIM_ENCODERMODE_X2_TI2 , /* TIM_ENCODER_MODE_X2_TI2                  */
    LL_TIM_ENCODERMODE_X4_TI12, /* TIM_ENCODER_MODE_X4_TI12                 */
    TIM_REG_VAL_UNAVAILABLE   , /* TIM_ENCODER_MODE_X1_TI1                  */
    TIM_REG_VAL_UNAVAILABLE   , /* TIM_ENCODER_MODE_X1_TI2                  */
    TIM_REG_VAL_UNAVAILABLE   , /* TIM_ENCODER_MODE_CLOCK_PLUS_DIRECTION_X2 */
    TIM_REG_VAL_UNAVAILABLE   , /* TIM_ENCODER_MODE_CLOCK_PLUS_DIRECTION_X1 */
    TIM_REG_VAL_UNAVAILABLE   , /* TIM_ENCODER_MODE_DIRECTIONAL_CLOCK_X2    */
    TIM_REG_VAL_UNAVAILABLE   , /* TIM_ENCODER_MODE_DIRECTIONAL_CLOCK_X1    */
};


/** Division factor of dead-time and sampling clock, indexed by \ref tim_ClockDiv_t */
static const uint32_t                   tim_ClockDivFactorLut[ TIM_CLOCK_DIV_CNT ] =
{
    1u, /* TIM_CLOCK_DIV_1 */
    2u, /* TIM_CLOCK_DIV_2 */
    4u, /* TIM_CLOCK_DIV_4 */
};


/** LL lock level register values, indexed by \ref tim_LockLevel_t */
static const uint32_t                   tim_LockLevelLut[ TIM_LOCK_LEVEL_CNT ] =
{
    LL_TIM_LOCKLEVEL_OFF, /* TIM_LOCK_LEVEL_OFF */
    LL_TIM_LOCKLEVEL_1  , /* TIM_LOCK_LEVEL_1   */
    LL_TIM_LOCKLEVEL_2  , /* TIM_LOCK_LEVEL_2   */
    LL_TIM_LOCKLEVEL_3  , /* TIM_LOCK_LEVEL_3   */
};


/** LL commutation update source register values, indexed by \ref tim_CommutationUpdate_t */
static const uint32_t                   tim_CommutationUpdateLut[ TIM_COMMUTATION_UPDATE_CNT ] =
{
    LL_TIM_CCUPDATESOURCE_COMG_ONLY    , /* TIM_COMMUTATION_UPDATE_COMG      */
    LL_TIM_CCUPDATESOURCE_COMG_AND_TRGI, /* TIM_COMMUTATION_UPDATE_COMG_TRGI */
};


/** DMA requests, indexed by \ref tim_DmaRequest_t */
static const tim_DmaRequestConfig_t     tim_DmaRequestLut[ TIM_DMA_REQUEST_CNT ] =
{
    { .Availability = TIM_FEATURE_AVAIL_DMA        , .ChannelId = TIM_CHANNEL_CNT, .EnableReq = LL_TIM_EnableDMAReq_UPDATE, .DisableReq = LL_TIM_DisableDMAReq_UPDATE, .IsEnabledReq = LL_TIM_IsEnabledDMAReq_UPDATE },
    { .Availability = TIM_FEATURE_AVAIL_DMA_CC     , .ChannelId = TIM_CHANNEL_1  , .EnableReq = LL_TIM_EnableDMAReq_CC1   , .DisableReq = LL_TIM_DisableDMAReq_CC1   , .IsEnabledReq = LL_TIM_IsEnabledDMAReq_CC1    },
    { .Availability = TIM_FEATURE_AVAIL_DMA_CC     , .ChannelId = TIM_CHANNEL_2  , .EnableReq = LL_TIM_EnableDMAReq_CC2   , .DisableReq = LL_TIM_DisableDMAReq_CC2   , .IsEnabledReq = LL_TIM_IsEnabledDMAReq_CC2    },
    { .Availability = TIM_FEATURE_AVAIL_DMA_CC     , .ChannelId = TIM_CHANNEL_3  , .EnableReq = LL_TIM_EnableDMAReq_CC3   , .DisableReq = LL_TIM_DisableDMAReq_CC3   , .IsEnabledReq = LL_TIM_IsEnabledDMAReq_CC3    },
    { .Availability = TIM_FEATURE_AVAIL_DMA_CC     , .ChannelId = TIM_CHANNEL_4  , .EnableReq = LL_TIM_EnableDMAReq_CC4   , .DisableReq = LL_TIM_DisableDMAReq_CC4   , .IsEnabledReq = LL_TIM_IsEnabledDMAReq_CC4    },
    { .Availability = TIM_FEATURE_AVAIL_COMMUTATION, .ChannelId = TIM_CHANNEL_CNT, .EnableReq = LL_TIM_EnableDMAReq_COM   , .DisableReq = LL_TIM_DisableDMAReq_COM   , .IsEnabledReq = LL_TIM_IsEnabledDMAReq_COM    },
    { .Availability = TIM_FEATURE_AVAIL_SLAVE      , .ChannelId = TIM_CHANNEL_CNT, .EnableReq = LL_TIM_EnableDMAReq_TRIG  , .DisableReq = LL_TIM_DisableDMAReq_TRIG  , .IsEnabledReq = LL_TIM_IsEnabledDMAReq_TRIG   },
};


/** LL DMA burst base address register values, indexed by \ref tim_DmaBurstReg_t (STM32F4: TIMx_CR1 - TIMx_BDTR) */
static const uint32_t                   tim_DmaBurstRegLut[ TIM_DMA_BURST_REG_CNT ] =
{
    LL_TIM_DMABURST_BASEADDR_CR1  , LL_TIM_DMABURST_BASEADDR_CR2  , LL_TIM_DMABURST_BASEADDR_SMCR , LL_TIM_DMABURST_BASEADDR_DIER ,
    LL_TIM_DMABURST_BASEADDR_SR   , LL_TIM_DMABURST_BASEADDR_EGR  , LL_TIM_DMABURST_BASEADDR_CCMR1, LL_TIM_DMABURST_BASEADDR_CCMR2,
    LL_TIM_DMABURST_BASEADDR_CCER , LL_TIM_DMABURST_BASEADDR_CNT  , LL_TIM_DMABURST_BASEADDR_PSC  , LL_TIM_DMABURST_BASEADDR_ARR  ,
    LL_TIM_DMABURST_BASEADDR_RCR  , LL_TIM_DMABURST_BASEADDR_CCR1 , LL_TIM_DMABURST_BASEADDR_CCR2 , LL_TIM_DMABURST_BASEADDR_CCR3 ,
    LL_TIM_DMABURST_BASEADDR_CCR4 , LL_TIM_DMABURST_BASEADDR_BDTR , TIM_REG_VAL_UNAVAILABLE       , TIM_REG_VAL_UNAVAILABLE       ,
    TIM_REG_VAL_UNAVAILABLE       , TIM_REG_VAL_UNAVAILABLE       , TIM_REG_VAL_UNAVAILABLE       , TIM_REG_VAL_UNAVAILABLE       ,
    TIM_REG_VAL_UNAVAILABLE       , TIM_REG_VAL_UNAVAILABLE       , TIM_REG_VAL_UNAVAILABLE       ,
};


/** Timer registers for DMA transfers, indexed by \ref tim_DmaReg_t */
static const tim_DmaRegConfig_t         tim_DmaRegLut[ TIM_DMA_REG_CNT ] =
{
    { .Availability = TIM_FEATURE_AVAIL_CHANNEL  , .ChannelId = TIM_CHANNEL_1  , .RegOffset = offsetof( TIM_TypeDef, CCR1 ) },
    { .Availability = TIM_FEATURE_AVAIL_CHANNEL  , .ChannelId = TIM_CHANNEL_2  , .RegOffset = offsetof( TIM_TypeDef, CCR2 ) },
    { .Availability = TIM_FEATURE_AVAIL_CHANNEL  , .ChannelId = TIM_CHANNEL_3  , .RegOffset = offsetof( TIM_TypeDef, CCR3 ) },
    { .Availability = TIM_FEATURE_AVAIL_CHANNEL  , .ChannelId = TIM_CHANNEL_4  , .RegOffset = offsetof( TIM_TypeDef, CCR4 ) },
    { .Availability = TIM_FEATURE_AVAIL_ALWAYS   , .ChannelId = TIM_CHANNEL_CNT, .RegOffset = offsetof( TIM_TypeDef, ARR  ) },
    { .Availability = TIM_FEATURE_AVAIL_DMA_BURST, .ChannelId = TIM_CHANNEL_CNT, .RegOffset = offsetof( TIM_TypeDef, DMAR ) },
};


/** LL ETR prescaler register values, indexed by \ref tim_EtrPrescaler_t */
static const uint32_t                   tim_EtrPrescalerLut[ TIM_ETR_PRESCALER_CNT ] =
{
    LL_TIM_ETR_PRESCALER_DIV1, /* TIM_ETR_PRESCALER_DIV1 */
    LL_TIM_ETR_PRESCALER_DIV2, /* TIM_ETR_PRESCALER_DIV2 */
    LL_TIM_ETR_PRESCALER_DIV4, /* TIM_ETR_PRESCALER_DIV4 */
    LL_TIM_ETR_PRESCALER_DIV8, /* TIM_ETR_PRESCALER_DIV8 */
};


/** LL ETR filter register values, indexed by \ref tim_InputFilter_t */
static const uint32_t                   tim_EtrFilterLut[ TIM_INPUT_FILTER_CNT ] =
{
    LL_TIM_ETR_FILTER_FDIV1    , LL_TIM_ETR_FILTER_FDIV1_N2 , LL_TIM_ETR_FILTER_FDIV1_N4 , LL_TIM_ETR_FILTER_FDIV1_N8 ,
    LL_TIM_ETR_FILTER_FDIV2_N6 , LL_TIM_ETR_FILTER_FDIV2_N8 , LL_TIM_ETR_FILTER_FDIV4_N6 , LL_TIM_ETR_FILTER_FDIV4_N8 ,
    LL_TIM_ETR_FILTER_FDIV8_N6 , LL_TIM_ETR_FILTER_FDIV8_N8 , LL_TIM_ETR_FILTER_FDIV16_N5, LL_TIM_ETR_FILTER_FDIV16_N6,
    LL_TIM_ETR_FILTER_FDIV16_N8, LL_TIM_ETR_FILTER_FDIV32_N5, LL_TIM_ETR_FILTER_FDIV32_N6, LL_TIM_ETR_FILTER_FDIV32_N8,
};


/** Software generated events, indexed by \ref tim_EventId_t */
static const tim_EventConfig_t          tim_EventLut[ TIM_EVENT_CNT ] =
{
    { .EventId = TIM_EVENT_UPDATE     , .Availability = TIM_FEATURE_AVAIL_ALWAYS     , .ChannelId = TIM_CHANNEL_CNT, .GenerateEvent = LL_TIM_GenerateEvent_UPDATE },
    { .EventId = TIM_EVENT_CC1        , .Availability = TIM_FEATURE_AVAIL_CHANNEL    , .ChannelId = TIM_CHANNEL_1  , .GenerateEvent = LL_TIM_GenerateEvent_CC1    },
    { .EventId = TIM_EVENT_CC2        , .Availability = TIM_FEATURE_AVAIL_CHANNEL    , .ChannelId = TIM_CHANNEL_2  , .GenerateEvent = LL_TIM_GenerateEvent_CC2    },
    { .EventId = TIM_EVENT_CC3        , .Availability = TIM_FEATURE_AVAIL_CHANNEL    , .ChannelId = TIM_CHANNEL_3  , .GenerateEvent = LL_TIM_GenerateEvent_CC3    },
    { .EventId = TIM_EVENT_CC4        , .Availability = TIM_FEATURE_AVAIL_CHANNEL    , .ChannelId = TIM_CHANNEL_4  , .GenerateEvent = LL_TIM_GenerateEvent_CC4    },
    { .EventId = TIM_EVENT_COMMUTATION, .Availability = TIM_FEATURE_AVAIL_COMMUTATION, .ChannelId = TIM_CHANNEL_CNT, .GenerateEvent = LL_TIM_GenerateEvent_COM    },
    { .EventId = TIM_EVENT_TRIGGER    , .Availability = TIM_FEATURE_AVAIL_SLAVE      , .ChannelId = TIM_CHANNEL_CNT, .GenerateEvent = LL_TIM_GenerateEvent_TRIG   },
    { .EventId = TIM_EVENT_BREAK      , .Availability = TIM_FEATURE_AVAIL_BREAK      , .ChannelId = TIM_CHANNEL_CNT, .GenerateEvent = LL_TIM_GenerateEvent_BRK    },
    { .EventId = TIM_EVENT_BREAK2     , .Availability = TIM_FEATURE_AVAIL_NONE       , .ChannelId = TIM_CHANNEL_CNT, .GenerateEvent = TIM_NULL_PTR                },
};


/** Timer interrupt requests, indexed by \ref tim_IrqId_t (interrupts not existing on STM32F4 have no LL functions) */
static const tim_IrqConfig_t            tim_IrqLut[ TIM_IRQ_CNT ] =
{
    { .IrqId = TIM_IRQ_UPDATE             , .Availability = TIM_FEATURE_AVAIL_ALWAYS     , .ChannelId = TIM_CHANNEL_CNT, .IrqLine = TIM_IRQ_LINE_UPDATE         ,
      .EnableIt = LL_TIM_EnableIT_UPDATE   , .DisableIt = LL_TIM_DisableIT_UPDATE   , .IsEnabledIt = LL_TIM_IsEnabledIT_UPDATE   , .IsActiveFlag = LL_TIM_IsActiveFlag_UPDATE   , .ClearFlag = LL_TIM_ClearFlag_UPDATE   },
    { .IrqId = TIM_IRQ_CAPTURE_COMPARE_CH1, .Availability = TIM_FEATURE_AVAIL_CHANNEL    , .ChannelId = TIM_CHANNEL_1  , .IrqLine = TIM_IRQ_LINE_CAPTURE_COMPARE,
      .EnableIt = LL_TIM_EnableIT_CC1      , .DisableIt = LL_TIM_DisableIT_CC1      , .IsEnabledIt = LL_TIM_IsEnabledIT_CC1      , .IsActiveFlag = LL_TIM_IsActiveFlag_CC1      , .ClearFlag = LL_TIM_ClearFlag_CC1      },
    { .IrqId = TIM_IRQ_CAPTURE_COMPARE_CH2, .Availability = TIM_FEATURE_AVAIL_CHANNEL    , .ChannelId = TIM_CHANNEL_2  , .IrqLine = TIM_IRQ_LINE_CAPTURE_COMPARE,
      .EnableIt = LL_TIM_EnableIT_CC2      , .DisableIt = LL_TIM_DisableIT_CC2      , .IsEnabledIt = LL_TIM_IsEnabledIT_CC2      , .IsActiveFlag = LL_TIM_IsActiveFlag_CC2      , .ClearFlag = LL_TIM_ClearFlag_CC2      },
    { .IrqId = TIM_IRQ_CAPTURE_COMPARE_CH3, .Availability = TIM_FEATURE_AVAIL_CHANNEL    , .ChannelId = TIM_CHANNEL_3  , .IrqLine = TIM_IRQ_LINE_CAPTURE_COMPARE,
      .EnableIt = LL_TIM_EnableIT_CC3      , .DisableIt = LL_TIM_DisableIT_CC3      , .IsEnabledIt = LL_TIM_IsEnabledIT_CC3      , .IsActiveFlag = LL_TIM_IsActiveFlag_CC3      , .ClearFlag = LL_TIM_ClearFlag_CC3      },
    { .IrqId = TIM_IRQ_CAPTURE_COMPARE_CH4, .Availability = TIM_FEATURE_AVAIL_CHANNEL    , .ChannelId = TIM_CHANNEL_4  , .IrqLine = TIM_IRQ_LINE_CAPTURE_COMPARE,
      .EnableIt = LL_TIM_EnableIT_CC4      , .DisableIt = LL_TIM_DisableIT_CC4      , .IsEnabledIt = LL_TIM_IsEnabledIT_CC4      , .IsActiveFlag = LL_TIM_IsActiveFlag_CC4      , .ClearFlag = LL_TIM_ClearFlag_CC4      },
    { .IrqId = TIM_IRQ_COMMUTATION        , .Availability = TIM_FEATURE_AVAIL_COMMUTATION, .ChannelId = TIM_CHANNEL_CNT, .IrqLine = TIM_IRQ_LINE_TRIGGER_COMM   ,
      .EnableIt = LL_TIM_EnableIT_COM      , .DisableIt = LL_TIM_DisableIT_COM      , .IsEnabledIt = LL_TIM_IsEnabledIT_COM      , .IsActiveFlag = LL_TIM_IsActiveFlag_COM      , .ClearFlag = LL_TIM_ClearFlag_COM      },
    { .IrqId = TIM_IRQ_TRIGGER            , .Availability = TIM_FEATURE_AVAIL_SLAVE      , .ChannelId = TIM_CHANNEL_CNT, .IrqLine = TIM_IRQ_LINE_TRIGGER_COMM   ,
      .EnableIt = LL_TIM_EnableIT_TRIG     , .DisableIt = LL_TIM_DisableIT_TRIG     , .IsEnabledIt = LL_TIM_IsEnabledIT_TRIG     , .IsActiveFlag = LL_TIM_IsActiveFlag_TRIG     , .ClearFlag = LL_TIM_ClearFlag_TRIG     },
    { .IrqId = TIM_IRQ_INDEX              , .Availability = TIM_FEATURE_AVAIL_NONE       , .ChannelId = TIM_CHANNEL_CNT, .IrqLine = TIM_IRQ_LINE_TRIGGER_COMM   ,
      .EnableIt = TIM_NULL_PTR             , .DisableIt = TIM_NULL_PTR              , .IsEnabledIt = TIM_NULL_PTR                , .IsActiveFlag = TIM_NULL_PTR                 , .ClearFlag = TIM_NULL_PTR              },
    { .IrqId = TIM_IRQ_DIRECTION          , .Availability = TIM_FEATURE_AVAIL_NONE       , .ChannelId = TIM_CHANNEL_CNT, .IrqLine = TIM_IRQ_LINE_TRIGGER_COMM   ,
      .EnableIt = TIM_NULL_PTR             , .DisableIt = TIM_NULL_PTR              , .IsEnabledIt = TIM_NULL_PTR                , .IsActiveFlag = TIM_NULL_PTR                 , .ClearFlag = TIM_NULL_PTR              },
    { .IrqId = TIM_IRQ_BREAK              , .Availability = TIM_FEATURE_AVAIL_BREAK      , .ChannelId = TIM_CHANNEL_CNT, .IrqLine = TIM_IRQ_LINE_BREAK          ,
      .EnableIt = LL_TIM_EnableIT_BRK      , .DisableIt = LL_TIM_DisableIT_BRK      , .IsEnabledIt = LL_TIM_IsEnabledIT_BRK      , .IsActiveFlag = LL_TIM_IsActiveFlag_BRK      , .ClearFlag = LL_TIM_ClearFlag_BRK      },
    { .IrqId = TIM_IRQ_BREAK2             , .Availability = TIM_FEATURE_AVAIL_NONE       , .ChannelId = TIM_CHANNEL_CNT, .IrqLine = TIM_IRQ_LINE_BREAK          ,
      .EnableIt = TIM_NULL_PTR             , .DisableIt = TIM_NULL_PTR              , .IsEnabledIt = TIM_NULL_PTR                , .IsActiveFlag = TIM_NULL_PTR                 , .ClearFlag = TIM_NULL_PTR              },
    { .IrqId = TIM_IRQ_SYSTEM_BREAK       , .Availability = TIM_FEATURE_AVAIL_NONE       , .ChannelId = TIM_CHANNEL_CNT, .IrqLine = TIM_IRQ_LINE_BREAK          ,
      .EnableIt = TIM_NULL_PTR             , .DisableIt = TIM_NULL_PTR              , .IsEnabledIt = TIM_NULL_PTR                , .IsActiveFlag = TIM_NULL_PTR                 , .ClearFlag = TIM_NULL_PTR              },
    { .IrqId = TIM_IRQ_ERROR              , .Availability = TIM_FEATURE_AVAIL_NONE       , .ChannelId = TIM_CHANNEL_CNT, .IrqLine = TIM_IRQ_LINE_TRIGGER_COMM   ,
      .EnableIt = TIM_NULL_PTR             , .DisableIt = TIM_NULL_PTR              , .IsEnabledIt = TIM_NULL_PTR                , .IsActiveFlag = TIM_NULL_PTR                 , .ClearFlag = TIM_NULL_PTR              },
};


/** Capture / compare over-capture flags, indexed by \ref tim_ChannelId_t (channels with interrupt only) */
static const tim_CcOvrConfig_t          tim_CcOvrLut[ TIM_CC_IRQ_CHANNEL_CNT ] =
{
    { .ChannelId = TIM_CHANNEL_1, .IsActiveFlag = LL_TIM_IsActiveFlag_CC1OVR, .ClearFlag = LL_TIM_ClearFlag_CC1OVR },
    { .ChannelId = TIM_CHANNEL_2, .IsActiveFlag = LL_TIM_IsActiveFlag_CC2OVR, .ClearFlag = LL_TIM_ClearFlag_CC2OVR },
    { .ChannelId = TIM_CHANNEL_3, .IsActiveFlag = LL_TIM_IsActiveFlag_CC3OVR, .ClearFlag = LL_TIM_ClearFlag_CC3OVR },
    { .ChannelId = TIM_CHANNEL_4, .IsActiveFlag = LL_TIM_IsActiveFlag_CC4OVR, .ClearFlag = LL_TIM_ClearFlag_CC4OVR },
};


/** User interrupt callbacks of timer peripherals (NULL = not registered) */
static tim_IsrCallback_t                tim_UserCallbacks[ TIM_PERIPH_CNT ];


/** User interrupt callbacks after de-initialization (none registered) */
static const tim_IsrCallback_t          tim_UserCallbacksDefault = { 0 };

/* ========================= EXPORTED FUNCTIONS ============================= */

/**
 * \brief Returns module SW version
 *
 * \return Module SW version
 */
tim_ModuleVersion_t Tim_Get_ModuleVersion( void )
{
    tim_ModuleVersion_t retVersion;

    retVersion.Major = TIM_MAJOR_VERSION;
    retVersion.Minor = TIM_MINOR_VERSION;
    retVersion.Patch = TIM_PATCH_VERSION;

    return (retVersion);
}


/**
 * \brief Timer peripheral initialization function.
 *
 * This function shall call every necessary sub-module initialization function 
 * and set up all the necessary resources for the module to work.
 *
 * \param timConfig [in]: Pointer to structure containing timer peripheral configuration. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Init( tim_PeriphConfig_t * const timConfig )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_NULL_PTR   != timConfig           ) &&
        ( TIM_PERIPH_CNT  > timConfig->PeriphId )    )
    {
        rcc_FunctionState_t rccActivationState = RCC_FUNCTION_INACTIVE;
        rcc_RequestState_t  rccRequestState    = RCC_REQUEST_ERROR;

        /*---------------- Peripheral clock activation section ---------------*/
        rccRequestState = Rcc_Get_PeriphState( tim_PeriphConf[ timConfig->PeriphId ].PeriphRcc, &rccActivationState );

        if( ( RCC_REQUEST_ERROR     != rccRequestState    ) &&
            ( RCC_FUNCTION_INACTIVE == rccActivationState )    )
        {
            rccRequestState = Rcc_Set_PeriphActive( tim_PeriphConf[ timConfig->PeriphId ].PeriphRcc );
        }
        else
        {
            /* Clock is already active or state request failed */
        }

        if( RCC_REQUEST_ERROR != rccRequestState )
        {
            tim_FunctionState_t timActivationState = TIM_FUNCTION_INACTIVE;

            retState = Tim_Get_PeriphState( timConfig->PeriphId, &timActivationState );

            if( ( TIM_REQUEST_OK      == retState           ) &&
                ( TIM_FUNCTION_ACTIVE == timActivationState )    )
            {
                /* Peripheral is already active, cannot proceed with
                 * configuration. User has to stop timer by himself to avoid
                 * possible problems in application execution. */
                retState = TIM_REQUEST_ERROR;
            }
            else
            {
                /* Peripheral is inactive, proceed with configuration */
            }
        }
        else
        {
            /* Peripheral clock activation failed */
            retState = TIM_REQUEST_ERROR;
        }

        /*---------------- Timer base initialization section -----------------*/

        if( TIM_REQUEST_OK == retState )
        {
            /* Initialize timer base functionality */
            retState = Tim_InitBase( timConfig );
        }
        else
        {
            /* Error during initialization process */
        }

        /*-------------- Timer channels initialization section ---------------*/

        /* Read count of available channels for selected timer */
        const tim_ChannelCnt_t timerChannelsCount = tim_PeriphConf[ timConfig->PeriphId ].ChannelCount;

        /* Loop through all used channels in configuration, stop at first error */
        for( tim_ChannelId_t timerChannelId = 0u; ( timerChannelsCount > timerChannelId ) && ( TIM_REQUEST_OK == retState ); timerChannelId ++ )
        {
            if( TIM_FUNCTION_INACTIVE != timConfig->ChannelConfig[ timerChannelId ].ChannelState )
            {
                retState = Tim_InitChannel( timConfig->PeriphId, &timConfig->ChannelConfig[ timerChannelId ] );
            }
            else
            {
                /* Channel is not used */
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief De-initializes timer peripheral.
 *
 * Counter is stopped, NVIC interrupt lines of the timer are de-activated, user
 * callbacks are de-registered and the peripheral is reset and its clock is
 * disabled through the RCC module (all timer registers return to reset values).
 *
 * \note  GPIO pins of the timer are not de-initialized (they may be shared
 *        with other functions of the application).
 *
 * \param timConfig [in]: Timer peripheral configuration (PeriphId used). Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR (all steps are
 *         executed also if one of them fails).
 */
tim_RequestState_t Tim_Deinit( tim_PeriphConfig_t * const timConfig )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_NULL_PTR   != timConfig           ) &&
        ( TIM_PERIPH_CNT  > timConfig->PeriphId )    )
    {
        const tim_PeriphId_t periphId = timConfig->PeriphId;
        tim_RequestState_t   nvicState = TIM_REQUEST_OK;

        /* -1- Counter */
        const tim_RequestState_t stopState = Tim_Stop( periphId );

        /* -2- Interrupt lines in NVIC */
        for( tim_IrqLine_t irqLine = TIM_IRQ_LINE_UPDATE; TIM_IRQ_LINE_CNT > irqLine; irqLine ++ )
        {
            nvic_PeriphIrqList_t     nvicIrqId = NVIC_PERIPH_IRQ_SIZE;
            nvic_IsrCallback_t       nvicIsr   = TIM_NULL_PTR;
            const tim_RequestState_t lineState = Tim_Get_NvicLine( periphId, irqLine, &nvicIrqId, &nvicIsr );

            if( TIM_REQUEST_OK == lineState )
            {
                const nvic_RequestState_t nvicLineState = Nvic_Set_PeriphIrq_Inactive( nvicIrqId );

                if( NVIC_REQUEST_OK != nvicLineState )
                {
                    nvicState = TIM_REQUEST_ERROR;
                }
                else
                {
                    /* NVIC line de-activated */
                }
            }
            else
            {
                /* Timer has no NVIC line for the interrupt group */
            }
        }

        /* -3- User callbacks */
        tim_UserCallbacks[ periphId ] = tim_UserCallbacksDefault;

        /* -4- Peripheral reset and clock */
        const rcc_RequestState_t rstActState   = Rcc_Set_ResetActive( tim_PeriphConf[ periphId ].PeriphRcc );
        const rcc_RequestState_t rstInactState = Rcc_Set_ResetInactive( tim_PeriphConf[ periphId ].PeriphRcc );
        const rcc_RequestState_t clkState      = Rcc_Set_PeriphInactive( tim_PeriphConf[ periphId ].PeriphRcc );

        if( ( TIM_REQUEST_OK == stopState     ) &&
            ( TIM_REQUEST_OK == nvicState     ) &&
            ( RCC_REQUEST_OK == rstActState   ) &&
            ( RCC_REQUEST_OK == rstInactState ) &&
            ( RCC_REQUEST_OK == clkState      )    )
        {
            retState = TIM_REQUEST_OK;
        }
        else
        {
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Main task of module Tim.
 *
 * \note  The module has no periodic processing (interrupt handling is done in
 *        interrupt service routines), the function is kept for the common
 *        module interface and does nothing.
 */
void Tim_Task( void )
{
    return;
}


/**
 * \brief Fills structure with default timer peripheral configuration values.
 *
 * \param timConfig [out]: Pointer to structure to be filled with default configuration values.
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
tim_RequestState_t Tim_Get_DefaultConfig( tim_PeriphConfig_t * const timConfig )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_NULL_PTR != timConfig )
    {
        timConfig->PeriphId                 = TIM_PERIPH_CNT;
        timConfig->ClockSource              = TIM_CLOCKSOURCE_INT_CLK;
        timConfig->ExtClockSource           = TIM_TRIGGER_INPUT_UNUSED;
        timConfig->SlaveMode                = TIM_SLAVE_MODE_DISABLE;
        timConfig->SlaveTriggerInput        = TIM_TRIGGER_INPUT_UNUSED;
        timConfig->MasterTrigger            = TIM_MASTER_TRIGGER_RESET;
        timConfig->TimerFrequency           = TIM_DEFAULT_TIMER_FREQ_HZ;
        timConfig->AutoreloadPreloadState   = TIM_FUNCTION_INACTIVE;
        /* Update event must be active, otherwise preloaded registers are never applied */
        timConfig->UpdateEventState         = TIM_FUNCTION_ACTIVE;
        timConfig->CounterDirection         = TIM_COUNTER_DIR_UP;

        timConfig->RefreshFrequency         = TIM_DEFAULT_REFRESH_FREQ_HZ;
        timConfig->ArrPreloadState          = TIM_FUNCTION_INACTIVE;

        /* Timer GPIO configurations */
        timConfig->BreakInPin               = TIM_BKIN_PIN_UNUSED;
        timConfig->BreakInPinPolarity       = TIM_POLARITY_HIGH;
        timConfig->BreakIn2Pin              = TIM_BKIN2_PIN_UNUSED;
        timConfig->BreakIn2PinPolarity      = TIM_POLARITY_HIGH;

        timConfig->TriggerEventPin          = TIM_ETR_PIN_UNUSED;

        for( tim_ChannelId_t channelId = 0u; TIM_CHANNEL_CNT > channelId; channelId++ )
        {
            timConfig->ChannelConfig[ channelId ].ChannelId      = channelId;
            timConfig->ChannelConfig[ channelId ].ChannelState   = TIM_FUNCTION_INACTIVE;
            timConfig->ChannelConfig[ channelId ].ChannelMode    = TIM_CHANNEL_MODE_CNT;
            timConfig->ChannelConfig[ channelId ].IoPin          = TIM_CH_PIN_UNUSED;
            timConfig->ChannelConfig[ channelId ].IoComplPin     = TIM_CH_N_PIN_UNUSED;
            timConfig->ChannelConfig[ channelId ].OutputPolarity = TIM_POLARITY_HIGH;
            timConfig->ChannelConfig[ channelId ].IdleState      = TIM_POLARITY_LOW;
            timConfig->ChannelConfig[ channelId ].InputPolarity  = TIM_INPUT_POLARITY_NORMAL;
            timConfig->ChannelConfig[ channelId ].InputFilter    = TIM_INPUT_FILTER_INACTIVE;
            timConfig->ChannelConfig[ channelId ].InputPrescaler = TIM_INPUT_PRESCALER_DIV1;
            timConfig->ChannelConfig[ channelId ].ActiveInput    = TIM_ACTIVE_INPUT_DIRECT;
        }

        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Starts (or resumes) timer counter.
 *
 * Counter continues from its actual value (after \ref Tim_Stop from
 * \ref TIM_COUNTER_RESET_VAL, after \ref Tim_Pause from the paused value).
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Start( tim_PeriphId_t periphId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_PERIPH_CNT > periphId )
    {
        retState = Tim_Set_PeriphActive( periphId );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Stops timer counter and resets counter value.
 *
 * Counter is disabled and counter register is set to \ref TIM_COUNTER_RESET_VAL.
 * Prescaler counter is not reset (it is reset by the next update event).
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Stop( tim_PeriphId_t periphId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_PERIPH_CNT > periphId )
    {
        retState = Tim_Set_PeriphInactive( periphId );

        if( TIM_REQUEST_OK == retState )
        {
            LL_TIM_SetCounter( tim_PeriphConf[ periphId ].PeriphReg, TIM_COUNTER_RESET_VAL );

            for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
            {
                const uint32_t counterValue = LL_TIM_GetCounter( tim_PeriphConf[ periphId ].PeriphReg );

                if( TIM_COUNTER_RESET_VAL == counterValue )
                {
                    retState = TIM_REQUEST_OK;
                    break;
                }
                else
                {
                    /* Counter value has not been written yet */
                    retState = TIM_REQUEST_ERROR;
                }
            }
        }
        else
        {
            /* Counter could not be disabled, counter value is not changed */
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Pauses timer counter (counter value is kept).
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Pause( tim_PeriphId_t periphId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_PERIPH_CNT > periphId )
    {
        retState = Tim_Set_PeriphInactive( periphId );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Configures timer clock with internal clock source
 *
 * Prescaler is calculated from the actual timer kernel clock (rounded to the
 * nearest divider). Prescaler register is verified by read-back; the new value
 * becomes effective with the next update event.
 *
 * \param periphId [in] : Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param reqFreq  [in] : Required timer counter frequency in Hz (1 Hz - timer kernel clock,
 *                        divider max. \ref TIM_PRESCALER_MAX + 1).
 * \param trueFreq [out]: Pointer to store real timer counter frequency in Hz. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_ClkInternal( tim_PeriphId_t periphId, tim_FreqHz_t reqFreq, tim_FreqHz_t * const trueFreq )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId ) &&
        ( 0u             < reqFreq  ) &&
        ( TIM_NULL_PTR  != trueFreq )    )
    {
        rcc_FreqHz_t             timerPeriphFreq = 0u;
        const rcc_RequestState_t rccRetState     = Rcc_Get_PeriphClk( tim_PeriphConf[ periphId ].PeriphRcc, &timerPeriphFreq );

        if( ( RCC_REQUEST_OK  == rccRetState     ) &&
            ( timerPeriphFreq >= reqFreq         )    )
        {
            /* Rounded divider decremented by 1 (register value) */
            const uint32_t prescalerValue = __LL_TIM_CALC_PSC( timerPeriphFreq, reqFreq );

            if( TIM_PRESCALER_MAX >= prescalerValue )
            {
                LL_TIM_SetPrescaler( tim_PeriphConf[ periphId ].PeriphReg, prescalerValue );

                for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
                {
                    const uint32_t regValue = LL_TIM_GetPrescaler( tim_PeriphConf[ periphId ].PeriphReg );

                    if( prescalerValue == regValue )
                    {
                        retState = TIM_REQUEST_OK;
                        break;
                    }
                    else
                    {
                        /* Register value has not been correctly configured yet */
                        retState = TIM_REQUEST_ERROR;
                    }
                }

                *trueFreq = timerPeriphFreq / ( prescalerValue + TIM_REG_DIV_OFFSET );
            }
            else
            {
                /* Required frequency is too low for 16-bit prescaler */
                retState = TIM_REQUEST_ERROR;
            }
        }
        else
        {
            /* Timer clock not available or lower than required frequency */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Returns actual value of timer frequency
 *
 * \note The returned value is only reliable if timer is clocked by internal
 *       clock.
 *
 * \param periphId [in] : Timer peripheral identification.
 * \param timFreq  [out]: Value of timer frequency in Hz.
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
tim_RequestState_t Tim_Get_ClkInternal( tim_PeriphId_t periphId, tim_FreqHz_t * const timFreq )
{
    tim_RequestState_t retState        = TIM_REQUEST_ERROR;
    rcc_FreqHz_t       timerPeriphFreq = 0u;

    if( ( TIM_PERIPH_CNT > periphId ) &&
        ( TIM_NULL_PTR  != timFreq  )    )
    {
        const rcc_RequestState_t rccRetState = Rcc_Get_PeriphClk( tim_PeriphConf[ periphId ].PeriphRcc, &timerPeriphFreq );

        if( RCC_REQUEST_OK == rccRetState )
        {
            const uint32_t prescaler = LL_TIM_GetPrescaler( tim_PeriphConf[ periphId ].PeriphReg );

            *timFreq = ( timerPeriphFreq / ( prescaler + TIM_REG_DIV_OFFSET ) );

            retState = TIM_REQUEST_OK;
        }
        else
        {
            /* Timer clock is not available */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/*------------------- Counter and time base functionality --------------------*/

/**
 * \brief Sets timer counter value.
 *
 * \note  Counter value is verified by read-back only while the counter is
 *        disabled (running counter changes its value immediately).
 *
 * \param periphId     [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param counterValue [in]: Required counter value (max. timer resolution, see \ref tim_Counter_t).
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_Counter( tim_PeriphId_t periphId, tim_Counter_t counterValue )
{
    tim_RequestState_t  retState    = TIM_REQUEST_ERROR;
    tim_FunctionState_t periphState = TIM_FUNCTION_ACTIVE;

    if( ( TIM_PERIPH_CNT                                         > periphId     ) &&
        ( (tim_Counter_t)tim_PeriphConf[ periphId ].Resolution >= counterValue )    )
    {
        TIM_TypeDef * const timReg = tim_PeriphConf[ periphId ].PeriphReg;

        LL_TIM_SetCounter( timReg, counterValue );

        retState = Tim_Get_PeriphState( periphId, &periphState );

        if( ( TIM_REQUEST_OK        == retState    ) &&
            ( TIM_FUNCTION_INACTIVE == periphState )    )
        {
            for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
            {
                const uint32_t regValue = LL_TIM_GetCounter( timReg );

                if( counterValue == regValue )
                {
                    retState = TIM_REQUEST_OK;
                    break;
                }
                else
                {
                    /* Register value has not been correctly configured yet */
                    retState = TIM_REQUEST_ERROR;
                }
            }
        }
        else
        {
            /* Counter is running, read-back is not possible */
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Returns actual timer counter value.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param counterValue [out]: Pointer to store counter value. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_Counter( tim_PeriphId_t periphId, tim_Counter_t * const counterValue )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId     ) &&
        ( TIM_NULL_PTR  != counterValue )    )
    {
        *counterValue = LL_TIM_GetCounter( tim_PeriphConf[ periphId ].PeriphReg );

        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Sets timer period (auto-reload register value).
 *
 * Counter period is (autoreloadValue + 1) counter steps. With auto-reload
 * preload active the new value is applied at the next update event.
 *
 * \param periphId        [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param autoreloadValue [in]: Auto-reload value (max. timer resolution, see \ref tim_Counter_t).
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_Period( tim_PeriphId_t periphId, tim_Counter_t autoreloadValue )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT                                         > periphId        ) &&
        ( (tim_Counter_t)tim_PeriphConf[ periphId ].Resolution >= autoreloadValue )    )
    {
        TIM_TypeDef * const timReg = tim_PeriphConf[ periphId ].PeriphReg;

        LL_TIM_SetAutoReload( timReg, autoreloadValue );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = LL_TIM_GetAutoReload( timReg );

            if( autoreloadValue == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Returns timer period (auto-reload register value).
 *
 * \param periphId         [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param autoreloadValue [out]: Pointer to store auto-reload value. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_Period( tim_PeriphId_t periphId, tim_Counter_t * const autoreloadValue )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId        ) &&
        ( TIM_NULL_PTR  != autoreloadValue )    )
    {
        *autoreloadValue = LL_TIM_GetAutoReload( tim_PeriphConf[ periphId ].PeriphReg );

        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Sets timer refresh (counter period) frequency.
 *
 * Auto-reload value is calculated from the actual timer counter frequency
 * (\ref Tim_Get_ClkInternal) and applied by \ref Tim_Set_Period.
 *
 * \pre   Timer must be clocked by internal clock (\ref TIM_CLOCKSOURCE_INT_CLK);
 *        otherwise \ref TIM_REQUEST_ERROR and no register is changed.
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param reqFreq   [in]: Required refresh frequency in Hz (timer frequency / (resolution + 1) - timer frequency).
 * \param trueFreq [out]: Pointer to store real refresh frequency in Hz. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_RefreshFreq( tim_PeriphId_t periphId, tim_FreqHz_t reqFreq, tim_FreqHz_t * const trueFreq )
{
    tim_RequestState_t retState       = TIM_REQUEST_ERROR;
    tim_ClockSource_t  clockSource    = TIM_CLOCKSOURCE_EXTERNAL_ETR;
    tim_FreqHz_t       timerFrequency = 0u;

    if( ( TIM_PERIPH_CNT > periphId ) &&
        ( 0u             < reqFreq  ) &&
        ( TIM_NULL_PTR  != trueFreq )    )
    {
        retState = Tim_Get_ClockSource( periphId, &clockSource );

        if( ( TIM_REQUEST_OK          == retState    ) &&
            ( TIM_CLOCKSOURCE_INT_CLK == clockSource )    )
        {
            retState = Tim_Get_ClkInternal( periphId, &timerFrequency );
        }
        else
        {
            /* Refresh frequency is defined only for internal clock source */
            retState = TIM_REQUEST_ERROR;
        }

        if( ( TIM_REQUEST_OK == retState       ) &&
            ( timerFrequency >= reqFreq        )    )
        {
            const tim_Counter_t autoreloadValue = ( timerFrequency / reqFreq ) - TIM_REG_DIV_OFFSET;

            retState = Tim_Set_Period( periphId, autoreloadValue );

            if( TIM_REQUEST_OK == retState )
            {
                *trueFreq = timerFrequency / ( autoreloadValue + TIM_REG_DIV_OFFSET );
            }
            else
            {
                /* Auto-reload value out of timer resolution or write failed */
            }
        }
        else
        {
            /* Timer frequency not available or lower than required frequency */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Returns timer refresh (counter period) frequency.
 *
 * \note  The returned value is only reliable if timer is clocked by internal clock.
 *
 * \param periphId     [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param refreshFreq [out]: Pointer to store refresh frequency in Hz. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_RefreshFreq( tim_PeriphId_t periphId, tim_FreqHz_t * const refreshFreq )
{
    tim_RequestState_t retState       = TIM_REQUEST_ERROR;
    tim_FreqHz_t       timerFrequency = 0u;

    if( ( TIM_PERIPH_CNT > periphId    ) &&
        ( TIM_NULL_PTR  != refreshFreq )    )
    {
        retState = Tim_Get_ClkInternal( periphId, &timerFrequency );

        if( TIM_REQUEST_OK == retState )
        {
            const uint64_t periodSteps = (uint64_t)LL_TIM_GetAutoReload( tim_PeriphConf[ periphId ].PeriphReg ) + TIM_REG_DIV_OFFSET;

            *refreshFreq = (tim_FreqHz_t)( (uint64_t)timerFrequency / periodSteps );
        }
        else
        {
            /* Timer frequency is not available */
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Sets counter direction (counting mode) of timer.
 *
 * \pre   Timer counter must be disabled; otherwise \ref TIM_REQUEST_ERROR and
 *        no register is changed. Timers other than TIM1 / TIM2 / TIM3 / TIM4 /
 *        TIM5 / TIM8 support only \ref TIM_COUNTER_DIR_UP.
 *
 * \param periphId   [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param counterDir [in]: Counter direction, value from \ref tim_CounterDir_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_CounterDirection( tim_PeriphId_t periphId, tim_CounterDir_t counterDir )
{
    tim_RequestState_t  retState    = TIM_REQUEST_ERROR;
    tim_FunctionState_t periphState = TIM_FUNCTION_ACTIVE;

    retState = Tim_Get_PeriphState( periphId, &periphState );

    if( ( TIM_REQUEST_OK        == retState    ) &&
        ( TIM_FUNCTION_INACTIVE == periphState )    )
    {
        retState = Tim_Config_CounterDirection( periphId, counterDir );
    }
    else
    {
        /* Invalid peripheral or counter is running */
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Returns counter direction (counting mode) of timer.
 *
 * \note  In edge-aligned mode the actual counting direction is returned (it can
 *        be changed by hardware in encoder mode).
 *
 * \param periphId    [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param counterDir [out]: Pointer to store counter direction. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_CounterDirection( tim_PeriphId_t periphId, tim_CounterDir_t * const counterDir )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId   ) &&
        ( TIM_NULL_PTR  != counterDir )    )
    {
        const uint32_t counterModeReg = LL_TIM_GetCounterMode( tim_PeriphConf[ periphId ].PeriphReg );

        for( tim_CounterDir_t dirIdx = TIM_COUNTER_DIR_UP; TIM_COUNTER_DIR_CNT > dirIdx; dirIdx ++ )
        {
            if( tim_CounterModeLut[ dirIdx ] == counterModeReg )
            {
                *counterDir = dirIdx;
                retState    = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Continue with next counter direction */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Sets repetition counter value.
 *
 * Update event is generated after (repetitionCnt + 1) counter overflows /
 * underflows. New value is applied at the next update event.
 *
 * \pre   Timer must have repetition counter (TIM1 / TIM8: 0 - 255); otherwise
 *        \ref TIM_REQUEST_ERROR and no register is changed.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param repetitionCnt [in]: Repetition counter value.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_RepetitionCounter( tim_PeriphId_t periphId, tim_RepCnt_t repetitionCnt )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_PERIPH_CNT > periphId )
    {
        TIM_TypeDef * const timReg              = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      repCntAvailability  = IS_TIM_REPETITION_COUNTER_INSTANCE( timReg );

        if( ( 0u              != repCntAvailability ) &&
            ( TIM_REP_CNT_MAX >= repetitionCnt      )    )
        {
            LL_TIM_SetRepetitionCounter( timReg, repetitionCnt );

            for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
            {
                const uint32_t regValue = LL_TIM_GetRepetitionCounter( timReg );

                if( repetitionCnt == regValue )
                {
                    retState = TIM_REQUEST_OK;
                    break;
                }
                else
                {
                    /* Register value has not been correctly configured yet */
                    retState = TIM_REQUEST_ERROR;
                }
            }
        }
        else
        {
            /* Repetition counter not available or value out of range */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Returns repetition counter value.
 *
 * \param periphId       [in]: Timer peripheral identification, value from \ref tim_PeriphId_t
 *                             (timer with repetition counter).
 * \param repetitionCnt [out]: Pointer to store repetition counter value. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_RepetitionCounter( tim_PeriphId_t periphId, tim_RepCnt_t * const repetitionCnt )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId      ) &&
        ( TIM_NULL_PTR  != repetitionCnt )    )
    {
        TIM_TypeDef * const timReg             = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      repCntAvailability = IS_TIM_REPETITION_COUNTER_INSTANCE( timReg );

        if( 0u != repCntAvailability )
        {
            *repetitionCnt = (tim_RepCnt_t)LL_TIM_GetRepetitionCounter( timReg );

            retState = TIM_REQUEST_OK;
        }
        else
        {
            /* Repetition counter is not available on this timer */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Activates update event (UEV) generation.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_UpdateEventActive( tim_PeriphId_t periphId )
{
    const tim_RequestState_t retState = Tim_Config_UpdateEvent( periphId, TIM_FUNCTION_ACTIVE );

    return ( retState );
}


/**
 * \brief De-activates update event (UEV) generation.
 *
 * \note  While update event is inactive, shadow registers (prescaler,
 *        auto-reload, compare) keep their values.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_UpdateEventInactive( tim_PeriphId_t periphId )
{
    const tim_RequestState_t retState = Tim_Config_UpdateEvent( periphId, TIM_FUNCTION_INACTIVE );

    return ( retState );
}


/**
 * \brief Returns update event (UEV) generation state.
 *
 * \param periphId    [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param eventState [out]: Pointer to store update event generation state. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_UpdateEventState( tim_PeriphId_t periphId, tim_FunctionState_t * const eventState )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId   ) &&
        ( TIM_NULL_PTR  != eventState )    )
    {
        const uint32_t updateEventRaw = LL_TIM_IsEnabledUpdateEvent( tim_PeriphConf[ periphId ].PeriphReg );

        *eventState = Tim_Conv_RawToFunctionState( updateEventRaw );

        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Sets update event (UEV) request source.
 *
 * \param periphId     [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param updateSource [in]: Update event source, value from \ref tim_UpdateSource_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_UpdateSource( tim_PeriphId_t periphId, tim_UpdateSource_t updateSource )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT        > periphId     ) &&
        ( TIM_UPDATE_SOURCE_CNT > updateSource )    )
    {
        TIM_TypeDef * const timReg          = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      updateSourceReg = tim_UpdateSourceLut[ updateSource ];

        LL_TIM_SetUpdateSource( timReg, updateSourceReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = LL_TIM_GetUpdateSource( timReg );

            if( updateSourceReg == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Returns update event (UEV) request source.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param updateSource [out]: Pointer to store update event source. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_UpdateSource( tim_PeriphId_t periphId, tim_UpdateSource_t * const updateSource )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId     ) &&
        ( TIM_NULL_PTR  != updateSource )    )
    {
        const uint32_t updateSourceReg = LL_TIM_GetUpdateSource( tim_PeriphConf[ periphId ].PeriphReg );

        for( tim_UpdateSource_t sourceIdx = TIM_UPDATE_SOURCE_ANY; TIM_UPDATE_SOURCE_CNT > sourceIdx; sourceIdx ++ )
        {
            if( tim_UpdateSourceLut[ sourceIdx ] == updateSourceReg )
            {
                *updateSource = sourceIdx;
                retState      = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Continue with next update source */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Generates timer event by software.
 *
 * \note  Event generation bits are cleared by hardware, therefore the request
 *        is not verified by read-back.
 *
 * \pre   Event must be available on the timer (capture / compare channel exists,
 *        commutation / break / trigger feature available, break 2 does not exist
 *        on STM32F4); otherwise
 *        \ref TIM_REQUEST_ERROR and no event is generated.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param eventId  [in]: Event identification, value from \ref tim_EventId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Generate_Event( tim_PeriphId_t periphId, tim_EventId_t eventId )
{
    tim_RequestState_t retState = Tim_Check_EventId( periphId, eventId );

    if( TIM_REQUEST_OK == retState )
    {
        tim_EventLut[ eventId ].GenerateEvent( tim_PeriphConf[ periphId ].PeriphReg );
    }
    else
    {
        /* Invalid peripheral or event not available on the timer */
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/*---------------------- Interrupt handling functionality --------------------*/

/**
 * \brief Activates timer interrupt request.
 *
 * Registers module interrupt handler of the corresponding NVIC line, activates
 * the NVIC line, clears pending status flag and enables the interrupt in the
 * timer. Registered user callback (\ref Tim_Set_UpdateCallback, ...) is called
 * from the interrupt handler.
 *
 * \pre   Interrupt must be available on the timer; otherwise \ref TIM_REQUEST_ERROR
 *        and no register is changed.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param irqId    [in]: Interrupt request identification, value from \ref tim_IrqId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_IrqActive( tim_PeriphId_t periphId, tim_IrqId_t irqId )
{
    tim_RequestState_t   retState  = Tim_Check_IrqId( periphId, irqId );
    nvic_PeriphIrqList_t nvicIrqId = NVIC_PERIPH_IRQ_SIZE;
    nvic_IsrCallback_t   nvicIsr   = TIM_NULL_PTR;

    /*--------------------------- NVIC configuration -------------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Get_NvicLine( periphId, tim_IrqLut[ irqId ].IrqLine, &nvicIrqId, &nvicIsr );
    }
    else
    {
        /* Invalid peripheral or interrupt not available on the timer */
    }

    if( TIM_REQUEST_OK == retState )
    {
        const nvic_RequestState_t handlerState = Nvic_Set_PeriphIrq_Handler( nvicIrqId, nvicIsr );
        const nvic_RequestState_t activeState  = Nvic_Set_PeriphIrq_Active( nvicIrqId );

        if( ( NVIC_REQUEST_OK == handlerState ) &&
            ( NVIC_REQUEST_OK == activeState  )    )
        {
            retState = TIM_REQUEST_OK;
        }
        else
        {
            /* NVIC line configuration failed */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        /* NVIC line of the interrupt is not available */
    }

    /*-------------------------- Timer configuration -------------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg = tim_PeriphConf[ periphId ].PeriphReg;

        /* Discard request pending from previous operation */
        tim_IrqLut[ irqId ].ClearFlag( timReg );
        tim_IrqLut[ irqId ].EnableIt( timReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t irqStateRaw = tim_IrqLut[ irqId ].IsEnabledIt( timReg );

            if( 0u != irqStateRaw )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Interrupt has not been enabled yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Previous step failed */
    }

    return ( retState );
}


/**
 * \brief De-activates timer interrupt request.
 *
 * \note  NVIC line stays active (it may be shared by other interrupts of the
 *        timer or by other timers).
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param irqId    [in]: Interrupt request identification, value from \ref tim_IrqId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_IrqInactive( tim_PeriphId_t periphId, tim_IrqId_t irqId )
{
    tim_RequestState_t retState = Tim_Check_IrqId( periphId, irqId );

    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg = tim_PeriphConf[ periphId ].PeriphReg;

        tim_IrqLut[ irqId ].DisableIt( timReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t irqStateRaw = tim_IrqLut[ irqId ].IsEnabledIt( timReg );

            if( 0u == irqStateRaw )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Interrupt has not been disabled yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid peripheral or interrupt not available on the timer */
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Returns activation state of timer interrupt request.
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param irqId     [in]: Interrupt request identification, value from \ref tim_IrqId_t.
 * \param irqState [out]: Pointer to store interrupt activation state. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_IrqState( tim_PeriphId_t periphId, tim_IrqId_t irqId, tim_FunctionState_t * const irqState )
{
    tim_RequestState_t retState = Tim_Check_IrqId( periphId, irqId );

    if( ( TIM_REQUEST_OK == retState ) &&
        ( TIM_NULL_PTR   != irqState )    )
    {
        const uint32_t irqStateRaw = tim_IrqLut[ irqId ].IsEnabledIt( tim_PeriphConf[ periphId ].PeriphReg );

        *irqState = Tim_Conv_RawToFunctionState( irqStateRaw );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Sets priority of all NVIC interrupt lines of the timer.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param irqPrio  [in]: Interrupt priority (see \ref tim_IrqPrio_t).
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_IrqPriority( tim_PeriphId_t periphId, tim_IrqPrio_t irqPrio )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_PERIPH_CNT > periphId )
    {
        for( tim_IrqLine_t irqLine = TIM_IRQ_LINE_UPDATE; TIM_IRQ_LINE_CNT > irqLine; irqLine ++ )
        {
            nvic_PeriphIrqList_t nvicIrqId = NVIC_PERIPH_IRQ_SIZE;
            nvic_IsrCallback_t   nvicIsr   = TIM_NULL_PTR;

            retState = Tim_Get_NvicLine( periphId, irqLine, &nvicIrqId, &nvicIsr );

            if( TIM_REQUEST_OK == retState )
            {
                const nvic_RequestState_t nvicState = Nvic_Set_PeriphIrq_Prio( nvicIrqId, (nvic_IrqPrio_t)irqPrio );

                if( NVIC_REQUEST_OK == nvicState )
                {
                    retState = TIM_REQUEST_OK;
                }
                else
                {
                    /* Priority out of range or NVIC configuration failed */
                    retState = TIM_REQUEST_ERROR;
                    break;
                }
            }
            else
            {
                /* Timer has no NVIC line */
                break;
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Returns priority of timer interrupts.
 *
 * \note  Priority of the update interrupt NVIC line is returned (all lines are
 *        configured to the same priority by \ref Tim_Set_IrqPriority).
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param irqPrio [out]: Pointer to store interrupt priority. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_IrqPriority( tim_PeriphId_t periphId, tim_IrqPrio_t * const irqPrio )
{
    tim_RequestState_t   retState  = TIM_REQUEST_ERROR;
    nvic_PeriphIrqList_t nvicIrqId = NVIC_PERIPH_IRQ_SIZE;
    nvic_IsrCallback_t   nvicIsr   = TIM_NULL_PTR;

    if( TIM_NULL_PTR != irqPrio )
    {
        retState = Tim_Get_NvicLine( periphId, TIM_IRQ_LINE_UPDATE, &nvicIrqId, &nvicIsr );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        nvic_IrqPrio_t            nvicPrio  = 0u;
        const nvic_RequestState_t nvicState = Nvic_Get_PeriphIrq_Prio( nvicIrqId, &nvicPrio );

        if( NVIC_REQUEST_OK == nvicState )
        {
            *irqPrio = (tim_IrqPrio_t)nvicPrio;
        }
        else
        {
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        /* Invalid parameters or timer has no NVIC line */
    }

    return ( retState );
}


/**
 * \brief Returns state of timer status flag.
 *
 * \param periphId   [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param flagId     [in]: Status flag identification, value from \ref tim_IrqId_t.
 * \param flagState [out]: Pointer to store flag state. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_Flag( tim_PeriphId_t periphId, tim_IrqId_t flagId, tim_FlagState_t * const flagState )
{
    tim_RequestState_t retState = Tim_Check_IrqId( periphId, flagId );

    if( ( TIM_REQUEST_OK == retState  ) &&
        ( TIM_NULL_PTR   != flagState )    )
    {
        const uint32_t flagStateRaw = tim_IrqLut[ flagId ].IsActiveFlag( tim_PeriphConf[ periphId ].PeriphReg );

        if( 0u != flagStateRaw )
        {
            *flagState = TIM_FLAG_ACTIVE;
        }
        else
        {
            *flagState = TIM_FLAG_INACTIVE;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Clears timer status flag.
 *
 * \note  Flag can be set again by hardware immediately, therefore the request
 *        is not verified by read-back.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param flagId   [in]: Status flag identification, value from \ref tim_IrqId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Clear_Flag( tim_PeriphId_t periphId, tim_IrqId_t flagId )
{
    tim_RequestState_t retState = Tim_Check_IrqId( periphId, flagId );

    if( TIM_REQUEST_OK == retState )
    {
        tim_IrqLut[ flagId ].ClearFlag( tim_PeriphConf[ periphId ].PeriphReg );
    }
    else
    {
        /* Invalid peripheral or flag not available on the timer */
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Registers user callback of update interrupt.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param callback [in]: User callback, NULL de-registers the callback.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_UpdateCallback( tim_PeriphId_t periphId, tim_UpdateIsrCallback_t * const callback )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_PERIPH_CNT > periphId )
    {
        tim_UserCallbacks[ periphId ].UpdateIsr = callback;

        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Registers user callback of capture / compare interrupt of timer channel.
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId [in]: Timer channel identification, value from \ref tim_ChannelId_t
 *                        (channels 1 - 4 available on the timer).
 * \param callback  [in]: User callback, NULL de-registers the callback.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_CaptureCompareCallback( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_CaptureComapreIsrCallback_t * const callback )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT                          > periphId  ) &&
        ( TIM_CC_IRQ_CHANNEL_CNT                  > channelId ) &&
        ( tim_PeriphConf[ periphId ].ChannelCount > channelId )    )
    {
        tim_UserCallbacks[ periphId ].CaptureCompareIsr[ channelId ] = callback;

        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Registers user callback of trigger interrupt.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param callback [in]: User callback, NULL de-registers the callback.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_TriggerCallback( tim_PeriphId_t periphId, tim_TriggerIsrCallback_t * const callback )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_PERIPH_CNT > periphId )
    {
        tim_UserCallbacks[ periphId ].TriggerIsr = callback;

        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Registers user callback of commutation interrupt.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param callback [in]: User callback, NULL de-registers the callback.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_CommutationCallback( tim_PeriphId_t periphId, tim_CommutationIsrCallback_t * const callback )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_PERIPH_CNT > periphId )
    {
        tim_UserCallbacks[ periphId ].CommutationIsr = callback;

        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Registers user callback of break interrupt.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param callback [in]: User callback, NULL de-registers the callback.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_BreakCallback( tim_PeriphId_t periphId, tim_BreakIsrCallback_t * const callback )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_PERIPH_CNT > periphId )
    {
        tim_UserCallbacks[ periphId ].BreakIsr = callback;

        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Registers user callback of break 2 interrupt.
 *
 * \note  Break 2 is not available on STM32F4 timers, the function is kept
 *        for compatibility of the module interface.
 *
 * \param periphId [in]: Timer peripheral identification (not used).
 * \param callback [in]: User callback (not used).
 *
 * \return Always \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_Break2Callback( tim_PeriphId_t periphId, tim_Break2IsrCallback_t * const callback )
{
    (void)periphId;
    (void)callback;

    return ( TIM_REQUEST_ERROR );
}


/**
 * \brief Registers user callback of system break interrupt.
 *
 * \note  System break is not available on STM32F4 timers, the function is kept
 *        for compatibility of the module interface.
 *
 * \param periphId [in]: Timer peripheral identification (not used).
 * \param callback [in]: User callback (not used).
 *
 * \return Always \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_SystemBreakCallback( tim_PeriphId_t periphId, tim_SystemBreakIsrCallback_t * const callback )
{
    (void)periphId;
    (void)callback;

    return ( TIM_REQUEST_ERROR );
}


/**
 * \brief Registers user callback of encoder direction change interrupt.
 *
 * \note  Encoder direction interrupt is not available on STM32F4 timers, the function is kept
 *        for compatibility of the module interface.
 *
 * \param periphId [in]: Timer peripheral identification (not used).
 * \param callback [in]: User callback (not used).
 *
 * \return Always \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_DirectionCallback( tim_PeriphId_t periphId, tim_DirectionIsrCallback_t * const callback )
{
    (void)periphId;
    (void)callback;

    return ( TIM_REQUEST_ERROR );
}


/**
 * \brief Registers user callback of encoder index interrupt.
 *
 * \note  Encoder index is not available on STM32F4 timers, the function is kept
 *        for compatibility of the module interface.
 *
 * \param periphId [in]: Timer peripheral identification (not used).
 * \param callback [in]: User callback (not used).
 *
 * \return Always \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_IndexCallback( tim_PeriphId_t periphId, tim_IndexIsrCallback_t * const callback )
{
    (void)periphId;
    (void)callback;

    return ( TIM_REQUEST_ERROR );
}


/**
 * \brief Registers user callback of encoder index / transition error interrupt.
 *
 * \note  Encoder error interrupt is not available on STM32F4 timers, the function is kept
 *        for compatibility of the module interface.
 *
 * \param periphId [in]: Timer peripheral identification (not used).
 * \param callback [in]: User callback (not used).
 *
 * \return Always \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_ErrorCallback( tim_PeriphId_t periphId, tim_ErrIsrCallback_t * const callback )
{
    (void)periphId;
    (void)callback;

    return ( TIM_REQUEST_ERROR );
}


/*----------------- Master / slave synchronization functionality -------------*/

/**
 * \brief Sets master mode trigger output (TRGO) of timer.
 *
 * Trigger output is used by other timers (internal trigger inputs ITRx) and
 * by other peripherals (e.g. ADC external trigger).
 *
 * \pre   Timer must have master mode (TIM1 / TIM2 / TIM3 / TIM4 / TIM5 / TIM6 /
 *        TIM7 / TIM8) and the trigger output must be available on STM32F4
 *        (\ref TIM_MASTER_TRIGGER_ENCODER_CLK is not); otherwise \ref TIM_REQUEST_ERROR
 *        and no register is changed.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param masterTrigger [in]: Trigger output selection, value from \ref tim_MasterTrigger_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_MasterTrigger( tim_PeriphId_t periphId, tim_MasterTrigger_t masterTrigger )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT                                   > periphId      ) &&
        ( TIM_MASTER_TRIGGER_CNT                           > masterTrigger ) &&
        ( TIM_REG_VAL_UNAVAILABLE != tim_MasterTriggerLut[ masterTrigger ] )    )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_MASTER, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg     = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      triggerReg = tim_MasterTriggerLut[ masterTrigger ];

        LL_TIM_SetTriggerOutput( timReg, triggerReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = READ_BIT( timReg->CR2, TIM_CR2_MMS );

            if( triggerReg == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid parameters or timer has no master mode */
    }

    return ( retState );
}


/**
 * \brief Returns master mode trigger output (TRGO) selection of timer.
 *
 * \param periphId       [in]: Timer peripheral identification, value from \ref tim_PeriphId_t
 *                             (timer with master mode).
 * \param masterTrigger [out]: Pointer to store trigger output selection. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_MasterTrigger( tim_PeriphId_t periphId, tim_MasterTrigger_t * const masterTrigger )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId      ) &&
        ( TIM_NULL_PTR  != masterTrigger )    )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_MASTER, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        const uint32_t triggerReg = READ_BIT( tim_PeriphConf[ periphId ].PeriphReg->CR2, TIM_CR2_MMS );

        retState = TIM_REQUEST_ERROR;

        for( tim_MasterTrigger_t triggerIdx = TIM_MASTER_TRIGGER_RESET; TIM_MASTER_TRIGGER_CNT > triggerIdx; triggerIdx ++ )
        {
            if( tim_MasterTriggerLut[ triggerIdx ] == triggerReg )
            {
                *masterTrigger = triggerIdx;
                retState       = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Continue with next trigger output selection */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid parameters or timer has no master mode */
    }

    return ( retState );
}


/**
 * \brief Sets master mode trigger output 2 (TRGO2) of timer.
 *
 * \note  Trigger output 2 is not available on STM32F4 timers, the function
 *        is kept for compatibility of the module interface.
 *
 * \param periphId       [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param masterTrigger2 [in]: Trigger output 2 selection, value from \ref tim_MasterTrigger2_t.
 *
 * \return Always \ref TIM_REQUEST_ERROR (no register is changed).
 */
tim_RequestState_t Tim_Set_MasterTrigger2( tim_PeriphId_t periphId, tim_MasterTrigger2_t masterTrigger2 )
{
    (void)periphId;
    (void)masterTrigger2;

    return ( TIM_REQUEST_ERROR );
}


/**
 * \brief Returns master mode trigger output 2 (TRGO2) selection of timer.
 *
 * \note  Trigger output 2 is not available on STM32F4 timers, the function
 *        is kept for compatibility of the module interface.
 *
 * \param periphId        [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param masterTrigger2 [out]: Pointer to store trigger output 2 selection (not written).
 *
 * \return Always \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_MasterTrigger2( tim_PeriphId_t periphId, tim_MasterTrigger2_t * const masterTrigger2 )
{
    (void)periphId;
    (void)masterTrigger2;

    return ( TIM_REQUEST_ERROR );
}


/**
 * \brief Activates master / slave mode (MSM).
 *
 * Effect of an event on the trigger input is delayed to allow perfect
 * synchronization between the timer and its slaves (through TRGO).
 *
 * \pre   Timer must have slave mode controller; otherwise \ref TIM_REQUEST_ERROR
 *        and no register is changed.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_MasterSlaveModeActive( tim_PeriphId_t periphId )
{
    const tim_RequestState_t retState = Tim_Config_MasterSlaveMode( periphId, TIM_FUNCTION_ACTIVE );

    return ( retState );
}


/**
 * \brief De-activates master / slave mode (MSM).
 *
 * \pre   Timer must have slave mode controller; otherwise \ref TIM_REQUEST_ERROR
 *        and no register is changed.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_MasterSlaveModeInactive( tim_PeriphId_t periphId )
{
    const tim_RequestState_t retState = Tim_Config_MasterSlaveMode( periphId, TIM_FUNCTION_INACTIVE );

    return ( retState );
}


/**
 * \brief Returns master / slave mode (MSM) state.
 *
 * \param periphId   [in]: Timer peripheral identification, value from \ref tim_PeriphId_t
 *                         (timer with slave mode controller).
 * \param modeState [out]: Pointer to store master / slave mode state. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_MasterSlaveMode( tim_PeriphId_t periphId, tim_FunctionState_t * const modeState )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId  ) &&
        ( TIM_NULL_PTR  != modeState )    )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_SLAVE, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        const uint32_t modeStateRaw = LL_TIM_IsEnabledMasterSlaveMode( tim_PeriphConf[ periphId ].PeriphReg );

        *modeState = Tim_Conv_RawToFunctionState( modeStateRaw );
    }
    else
    {
        /* Invalid parameters or timer has no slave mode controller */
    }

    return ( retState );
}


/**
 * \brief Configures slave mode controller of timer.
 *
 * Slave mode is disabled first, then the trigger input is selected (trigger
 * input must be changed only while slave mode is disabled) and the required
 * slave mode is set.
 *
 * \note  Slave mode shares the mode selection with external clock mode 1
 *        (\ref TIM_CLOCKSOURCE_EXTERNAL_CH_IN), setting a slave mode switches
 *        the counter to the internal clock (or external clock mode 2).
 *
 * \pre   Timer must have slave mode controller and the counter must be disabled;
 *        \ref TIM_SLAVE_MODE_EXTERNAL_CLOCK is configured by \ref Tim_Set_ClockSource,
 *        combined modes \ref TIM_SLAVE_MODE_RESET_TRIGGER / \ref TIM_SLAVE_MODE_GATED_RESET
 *        are not available on STM32F4; otherwise \ref TIM_REQUEST_ERROR and no
 *        register is changed.
 *
 * \param periphId     [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param slaveMode    [in]: Slave mode, value from \ref tim_SlaveMode_t.
 * \param triggerInput [in]: Trigger input, value from \ref tim_TriggerInput_t
 *                           (not used for \ref TIM_SLAVE_MODE_DISABLE).
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_SlaveMode( tim_PeriphId_t periphId, tim_SlaveMode_t slaveMode, tim_TriggerInput_t triggerInput )
{
    tim_RequestState_t  retState    = TIM_REQUEST_ERROR;
    tim_FunctionState_t periphState = TIM_FUNCTION_ACTIVE;

    /*------------------------- Parameters validation ------------------------*/
    if( ( TIM_PERIPH_CNT                           > periphId  ) &&
        ( TIM_SLAVE_MODE_CNT                       > slaveMode ) &&
        ( TIM_SLAVE_MODE_EXTERNAL_CLOCK           != slaveMode ) &&
        ( TIM_REG_VAL_UNAVAILABLE != tim_SlaveModeLut[ slaveMode ] )    )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_SLAVE, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( ( TIM_REQUEST_OK         == retState  ) &&
        ( TIM_SLAVE_MODE_DISABLE != slaveMode )    )
    {
        retState = Tim_Check_TriggerInput( periphId, triggerInput );
    }
    else
    {
        /* Trigger input is not used or invalid parameters */
    }

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Get_PeriphState( periphId, &periphState );

        if( ( TIM_REQUEST_OK        == retState    ) &&
            ( TIM_FUNCTION_INACTIVE == periphState )    )
        {
            retState = TIM_REQUEST_OK;
        }
        else
        {
            /* Slave mode must not be changed while counter is running */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        /* Invalid parameters, nothing is written */
    }

    /*------------------------ Slave mode configuration ----------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Config_SlaveModeReg( periphId, LL_TIM_SLAVEMODE_DISABLED );
    }
    else
    {
        /* Previous step failed */
    }

    if( ( TIM_REQUEST_OK         == retState  ) &&
        ( TIM_SLAVE_MODE_DISABLE != slaveMode )    )
    {
        TIM_TypeDef * const timReg      = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      triggerCode = TIM_BIT_MASK_DECODE_SELECTION_CODE( triggerInput );

        LL_TIM_SetTriggerInput( timReg, triggerCode );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = READ_BIT( timReg->SMCR, TIM_SMCR_TS );

            if( triggerCode == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }

        if( TIM_REQUEST_OK == retState )
        {
            retState = Tim_Config_SlaveModeReg( periphId, tim_SlaveModeLut[ slaveMode ] );
        }
        else
        {
            /* Trigger input configuration failed */
        }
    }
    else
    {
        /* Slave mode disabled or previous step failed */
    }

    return ( retState );
}


/**
 * \brief Returns slave mode and trigger input of timer.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t
 *                            (timer with slave mode controller).
 * \param slaveMode    [out]: Pointer to store slave mode. Must not be NULL.
 * \param triggerInput [out]: Pointer to store trigger input, item of \ref tim_ExtClkSource_t of the
 *                            timer (\ref TIM_TRIGGER_INPUT_UNUSED if slave mode is disabled).
 *                            Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_SlaveMode( tim_PeriphId_t periphId, tim_SlaveMode_t * const slaveMode, tim_TriggerInput_t * const triggerInput )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId     ) &&
        ( TIM_NULL_PTR  != slaveMode    ) &&
        ( TIM_NULL_PTR  != triggerInput )    )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_SLAVE, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg       = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      slaveModeReg = READ_BIT( timReg->SMCR, TIM_SMCR_SMS );
        const uint32_t      triggerReg   = READ_BIT( timReg->SMCR, TIM_SMCR_TS );

        *triggerInput = TIM_TRIGGER_INPUT_UNUSED;

        retState = TIM_REQUEST_ERROR;

        for( tim_SlaveMode_t modeIdx = TIM_SLAVE_MODE_DISABLE; TIM_SLAVE_MODE_CNT > modeIdx; modeIdx ++ )
        {
            if( tim_SlaveModeLut[ modeIdx ] == slaveModeReg )
            {
                *slaveMode = modeIdx;
                retState   = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Continue with next slave mode */
                retState = TIM_REQUEST_ERROR;
            }
        }

        if( ( TIM_REQUEST_OK         == retState  ) &&
            ( TIM_SLAVE_MODE_DISABLE != *slaveMode )    )
        {
            const tim_TriggerInput_t triggerItem = TIM_TRIGGER_INPUT_OF( periphId, triggerReg );

            retState = Tim_Check_TriggerInput( periphId, triggerItem );

            if( TIM_REQUEST_OK == retState )
            {
                *triggerInput = triggerItem;
            }
            else
            {
                /* Selection code of the register is not a trigger input of the device */
            }
        }
        else
        {
            /* Slave mode disabled (no trigger input in use) or slave mode not recognized */
        }
    }
    else
    {
        /* Invalid parameters or timer has no slave mode controller */
    }

    return ( retState );
}



/**
 * \brief Timer base initialization function.
 *
 * Configures in this order: counter clock source (\ref Tim_Set_ClockSource),
 * slave mode (\ref Tim_Set_SlaveMode), master trigger output (\ref Tim_Set_MasterTrigger,
 * timers with master mode), counter direction, update event generation,
 * prescaler, auto-reload preload, auto-reload value, update event generation
 * (loads preloaded registers) and break / trigger GPIO pins.
 *
 * Prescaler (and preloaded auto-reload / repetition counter) registers take
 * effect only at an update event, therefore an update event is generated by
 * software after the time base configuration and its update flag is cleared.
 * The first counter period runs with the configured timer frequency.
 *
 * \note  The software update event resets the counter and, with master trigger
 *        \ref TIM_MASTER_TRIGGER_RESET, is signalled on the trigger output. With
 *        update event generation inactive (UpdateEventState) shadow registers
 *        are not loaded (hardware behavior).
 *
 * With internal clock source the prescaler is calculated by \ref Tim_Set_ClkInternal
 * and the auto-reload value from the real timer frequency and required refresh
 * frequency. Refresh frequency must be in range where the auto-reload value fits
 * the timer resolution and must not exceed the timer frequency.
 *
 * \note  With external clock source the prescaler divides by 1 (counter counts
 *        each external clock edge) and the auto-reload value is set to the
 *        timer resolution (free running counter), TimerFrequency and
 *        RefreshFrequency are not used.
 *
 * \pre   Timer counter must be disabled; otherwise \ref TIM_REQUEST_ERROR
 *        (checked by \ref Tim_Set_ClockSource before any register is changed).
 *
 * \param timConfig [in]: Pointer to structure containing timer peripheral configuration. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_InitBase( tim_PeriphConfig_t * const timConfig )
{
    tim_RequestState_t retState        = TIM_REQUEST_ERROR;
    tim_FreqHz_t       timerFrequency  = 0u;
    uint32_t           prescalerValue  = TIM_PRESCALER_MIN - TIM_REG_DIV_OFFSET;
    uint32_t           autoreloadValue = 0u;

    if( ( TIM_NULL_PTR        != timConfig                         ) &&
        ( TIM_PERIPH_CNT       > timConfig->PeriphId               ) &&
        ( TIM_COUNTER_DIR_CNT  > timConfig->CounterDirection       ) &&
        ( TIM_FUNCTION_ACTIVE >= timConfig->UpdateEventState       ) &&
        ( TIM_FUNCTION_ACTIVE >= timConfig->AutoreloadPreloadState ) &&
        ( TIM_FUNCTION_ACTIVE >= timConfig->ArrPreloadState        ) &&
        ( TIM_BKIN2_PIN_UNUSED == timConfig->BreakIn2Pin           )    )
    {
        TIM_TypeDef * const timReg = tim_PeriphConf[ timConfig->PeriphId ].PeriphReg;

        /*-------------------------- Clock source ----------------------------*/
        retState = Tim_Set_ClockSource( timConfig->PeriphId, timConfig->ClockSource, timConfig->ExtClockSource );

        /*--------------------------- Slave mode -----------------------------*/
        if( ( TIM_REQUEST_OK         == retState             ) &&
            ( TIM_SLAVE_MODE_DISABLE != timConfig->SlaveMode )    )
        {
            if( TIM_CLOCKSOURCE_EXTERNAL_CH_IN == timConfig->ClockSource )
            {
                /* Slave mode and external clock mode 1 share the mode selection */
                retState = TIM_REQUEST_ERROR;
            }
            else
            {
                retState = Tim_Set_SlaveMode( timConfig->PeriphId, timConfig->SlaveMode, timConfig->SlaveTriggerInput );
            }
        }
        else
        {
            /* Slave mode is not used or previous step failed */
        }

        /*------------------------- Master trigger ---------------------------*/
        if( TIM_REQUEST_OK == retState )
        {
            const tim_RequestState_t masterAvail = Tim_Check_Feature( timConfig->PeriphId, TIM_FEATURE_AVAIL_MASTER, TIM_CHANNEL_CNT );

            if( TIM_REQUEST_OK == masterAvail )
            {
                retState = Tim_Set_MasterTrigger( timConfig->PeriphId, timConfig->MasterTrigger );
            }
            else if( TIM_MASTER_TRIGGER_RESET != timConfig->MasterTrigger )
            {
                /* Trigger output required on timer without master mode */
                retState = TIM_REQUEST_ERROR;
            }
            else
            {
                /* Timer without master mode, default trigger output */
            }
        }
        else
        {
            /* Error during initialization process */
        }

        /*------------------------ Counter direction -------------------------*/
        if( TIM_REQUEST_OK == retState )
        {
            retState = Tim_Config_CounterDirection( timConfig->PeriphId, timConfig->CounterDirection );
        }
        else
        {
            /* Error during initialization process */
        }

        /*------------------------ Update event state ------------------------*/
        if( TIM_REQUEST_OK == retState )
        {
            retState = Tim_Config_UpdateEvent( timConfig->PeriphId, timConfig->UpdateEventState );
        }
        else
        {
            /* Error during initialization process */
        }

        /*------------------------ Prescaler and period ----------------------*/
        if( ( TIM_REQUEST_OK          == retState               ) &&
            ( TIM_CLOCKSOURCE_INT_CLK == timConfig->ClockSource )    )
        {
            retState = Tim_Set_ClkInternal( timConfig->PeriphId, timConfig->TimerFrequency, &timerFrequency );

            /* Refresh frequency must be in range timer frequency / resolution - timer frequency */
            if( ( TIM_REQUEST_OK == retState                   ) &&
                ( 0u              < timConfig->RefreshFrequency ) &&
                ( timerFrequency >= timConfig->RefreshFrequency )    )
            {
                autoreloadValue = ( timerFrequency / timConfig->RefreshFrequency ) - TIM_REG_DIV_OFFSET;
            }
            else
            {
                /* Prescaler configuration failed or refresh frequency out of range */
                retState = TIM_REQUEST_ERROR;
            }
        }
        else if( TIM_REQUEST_OK == retState )
        {
            /* External clock: count each clock edge, free running counter */
            autoreloadValue = (uint32_t)tim_PeriphConf[ timConfig->PeriphId ].Resolution;

            LL_TIM_SetPrescaler( timReg, prescalerValue );

            for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
            {
                const uint32_t regValue = LL_TIM_GetPrescaler( timReg );

                if( prescalerValue == regValue )
                {
                    retState = TIM_REQUEST_OK;
                    break;
                }
                else
                {
                    /* Register value has not been correctly configured yet */
                    retState = TIM_REQUEST_ERROR;
                }
            }
        }
        else
        {
            /* Error during initialization process */
        }

        /*------------------------ Auto-reload preload ------------------------*/
        if( TIM_REQUEST_OK == retState )
        {
            /* ArrPreloadState is STM32G4 interface field with the same meaning */
            tim_FunctionState_t arrPreloadState = timConfig->AutoreloadPreloadState;

            if( TIM_FUNCTION_ACTIVE == timConfig->ArrPreloadState )
            {
                arrPreloadState = TIM_FUNCTION_ACTIVE;
            }
            else
            {
                /* Auto-reload preload state given by AutoreloadPreloadState */
            }

            retState = Tim_Config_ArrPreload( timConfig->PeriphId, arrPreloadState );
        }
        else
        {
            /* Error during initialization process */
        }

        /*------------------------ Auto-reload value --------------------------*/
        if( ( TIM_REQUEST_OK == retState                                                    ) &&
            ( (uint32_t)tim_PeriphConf[ timConfig->PeriphId ].Resolution >= autoreloadValue )    )
        {
            LL_TIM_SetAutoReload( timReg, autoreloadValue );

            for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
            {
                const uint32_t regValue = LL_TIM_GetAutoReload( timReg );

                if( autoreloadValue == regValue )
                {
                    retState = TIM_REQUEST_OK;
                    break;
                }
                else
                {
                    /* Register value has not been correctly configured yet */
                    retState = TIM_REQUEST_ERROR;
                }
            }
        }
        else
        {
            /* Previous step failed or auto-reload value exceeds timer resolution */
            retState = TIM_REQUEST_ERROR;
        }

        /*------------------ Load preloaded registers (UEV) -------------------*/
        if( TIM_REQUEST_OK == retState )
        {
            /* Prescaler is preloaded, without update event the first counter
             * period would run with the previous (reset) prescaler value */
            LL_TIM_GenerateEvent_UPDATE( timReg );

            /* Update flag set by software update event is not a counter overflow */
            LL_TIM_ClearFlag_UPDATE( timReg );
        }
        else
        {
            /* Error during initialization process */
        }

        /*------------- Peripheral GPIO initialization section ---------------*/

        const tim_BkinPin_t breakInPin = timConfig->BreakInPin;

        if( ( TIM_REQUEST_OK                            == retState            ) &&
            ( TIM_BIT_MASK_DECODE_PERIPH( breakInPin )  == timConfig->PeriphId ) &&
            ( TIM_BKIN_PIN_UNUSED                       != breakInPin          )    )
        {
            retState = Tim_InitBreakInputGpio( breakInPin );

            if( TIM_REQUEST_OK == retState )
            {
                retState = Tim_Config_BreakPinPolarity( timConfig->PeriphId, timConfig->BreakInPinPolarity );
            }
            else
            {
                /* Break input pin configuration failed */
            }
        }
        else
        {
            /* Break Input pin configuration is not used or previous step failed */
        }

        /* Break input 2 is not available on STM32F4 (BreakIn2Pin validated unused) */

        const tim_EtrPin_t triggerEventPin = timConfig->TriggerEventPin;

        if( ( TIM_REQUEST_OK                                == retState            ) &&
            ( TIM_BIT_MASK_DECODE_PERIPH( triggerEventPin ) == timConfig->PeriphId ) &&
            ( TIM_ETR_PIN_UNUSED                            != triggerEventPin     )    )
        {
            retState = Tim_InitTriggerEventGpio( triggerEventPin );
        }
        else
        {
            /* Trigger Event pin configuration is not used or previous step failed */
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Initializes timer channel (IO pins and channel mode).
 *
 * \note  Channel mode group selects the configuration function: PWM, forced
 *        output, output compare or input capture.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelConfig [in]: Timer channel configuration. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_InitChannel( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_NULL_PTR   != channelConfig                                     ) &&
        ( TIM_PERIPH_CNT  > periphId                                          ) &&
        ( tim_PeriphConf[ periphId ].ChannelCount > channelConfig->ChannelId ) )
    {
        /*------------- Channel GPIO initialization section --------------*/
        const tim_IoPin_t channelPin = channelConfig->IoPin;

        if( ( TIM_BIT_MASK_DECODE_PERIPH( channelPin )  == periphId                 ) &&
            ( TIM_BIT_MASK_DECODE_CHANNEL( channelPin ) == channelConfig->ChannelId ) &&
            ( TIM_CH_PIN_UNUSED                         != channelPin               )    )
        {
            retState = Tim_InitIOPin( channelPin );
        }
        else
        {
            /* Channel pin configuration is not used */
            retState = TIM_REQUEST_OK;
        }


        const tim_IOComplPin_t channelNPin = channelConfig->IoComplPin;

        if( ( TIM_REQUEST_OK                              == retState                 ) &&
            ( TIM_BIT_MASK_DECODE_PERIPH( channelNPin )  == periphId                 ) &&
            ( TIM_BIT_MASK_DECODE_CHANNEL( channelNPin ) == channelConfig->ChannelId ) &&
            ( TIM_CH_N_PIN_UNUSED                        != channelNPin              )    )
        {
            retState = Tim_InitIOComplPin( channelNPin );
        }
        else
        {
            /* Channel negative pin configuration is not used or previous step failed */
        }


        /*---------------------- Channel mode section ------------------------*/
        if( ( TIM_REQUEST_OK       == retState                   ) &&
            ( TIM_CHANNEL_MODE_CNT  > channelConfig->ChannelMode )    )
        {
            const tim_ModeGroup_t modeGroup = tim_ChannelModeConfig[ channelConfig->ChannelMode ].ModeGroup;

            if( TIM_MODE_GROUP_PWM == modeGroup )
            {
                retState = Tim_Set_Mode_Pwm( periphId, channelConfig );
            }
            else if( TIM_MODE_GROUP_FORCED == modeGroup )
            {
                retState = Tim_Set_Mode_ForcedOutput( periphId, channelConfig );
            }
            else if( TIM_MODE_GROUP_COMPARE == modeGroup )
            {
                retState = Tim_Set_Mode_OutputCompare( periphId, channelConfig );
            }
            else
            {
                retState = Tim_Set_Mode_InputCapture( periphId, channelConfig );
            }
        }
        else
        {
            /* Pin configuration failed or invalid channel mode */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Configures timer channel in input capture mode.
 *
 * Capture is disabled, input stage is configured (active input, capture
 * prescaler, input filter and captured edge from the channel configuration)
 * and capture is enabled again. Each field is verified by read-back.
 *
 * \pre   Channel must be available on the timer and have input stage (channels
 *        1 - 4), channel mode must be \ref TIM_CHANNEL_MODE_INPUT_CAPTURE;
 *        otherwise \ref TIM_REQUEST_ERROR and no register is changed.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelConfig [in]: Timer channel configuration. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_Mode_InputCapture( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig )
{
    tim_RequestState_t retState = Tim_Check_ChannelConfig( periphId, channelConfig, TIM_MODE_GROUP_INPUT );

    if( ( TIM_REQUEST_OK        == retState                 ) &&
        ( TIM_INPUT_CHANNEL_CNT  > channelConfig->ChannelId )    )
    {
        retState = Tim_Config_InputChannel( periphId, channelConfig->ChannelId, channelConfig->ActiveInput,
                                            channelConfig->InputPolarity, channelConfig->InputFilter,
                                            channelConfig->InputPrescaler );
    }
    else
    {
        /* Invalid parameters, channel without input stage or not input capture mode */
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Configures timer in encoder interface mode.
 *
 * Channels 1 and 2 are configured as direct inputs (polarity, filter), the
 * prescaler divides by 1 (every encoder edge is counted), the auto-reload
 * value is set to the encoder period and the slave mode controller is switched
 * to the required encoder mode. Counting direction is controlled by hardware
 * (\ref Tim_Get_CounterDirection), position is read by \ref Tim_Get_EncoderPosition.
 *
 * \note  Encoder mode uses the slave mode selection, clock source and slave
 *        mode configuration are replaced.
 *
 * \pre   Timer must have encoder interface (TIM1 / TIM2 / TIM3 / TIM4 / TIM5 /
 *        TIM8 / TIM9 / TIM12), counter must be disabled, encoder mode must be
 *        available on STM32F4 (\ref TIM_ENCODER_MODE_X2_TI1, \ref TIM_ENCODER_MODE_X2_TI2,
 *        \ref TIM_ENCODER_MODE_X4_TI12), input polarities must not be both
 *        edges and the period must fit the timer resolution; otherwise
 *        \ref TIM_REQUEST_ERROR and no register is changed.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param encoderConfig [in]: Encoder interface configuration. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_Mode_Encoder( tim_PeriphId_t periphId, const tim_EncoderConfig_t * const encoderConfig )
{
    tim_RequestState_t  retState    = TIM_REQUEST_ERROR;
    tim_FunctionState_t periphState = TIM_FUNCTION_ACTIVE;

    /*------------------------- Parameters validation ------------------------*/
    if( ( TIM_NULL_PTR   != encoderConfig ) &&
        ( TIM_PERIPH_CNT  > periphId      )    )
    {
        if( ( TIM_ENCODER_MODE_CNT                                     > encoderConfig->EncoderMode ) &&
            ( TIM_INPUT_FILTER_CNT                                     > encoderConfig->InputFilter ) &&
            ( (tim_Counter_t)tim_PeriphConf[ periphId ].Resolution    >= encoderConfig->Period      ) &&
            ( ( TIM_INPUT_POLARITY_NORMAL   == encoderConfig->Ti1Polarity )   ||
              ( TIM_INPUT_POLARITY_INVERTED == encoderConfig->Ti1Polarity )      )                     &&
            ( ( TIM_INPUT_POLARITY_NORMAL   == encoderConfig->Ti2Polarity )   ||
              ( TIM_INPUT_POLARITY_INVERTED == encoderConfig->Ti2Polarity )      )                        )
        {
            if( TIM_REG_VAL_UNAVAILABLE != tim_EncoderModeLut[ encoderConfig->EncoderMode ] )
            {
                retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_ENCODER, TIM_CHANNEL_CNT );
            }
            else
            {
                /* Encoder mode is not available on STM32F4 */
                retState = TIM_REQUEST_ERROR;
            }
        }
        else
        {
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Get_PeriphState( periphId, &periphState );

        if( ( TIM_REQUEST_OK        == retState    ) &&
            ( TIM_FUNCTION_INACTIVE == periphState )    )
        {
            retState = TIM_REQUEST_OK;
        }
        else
        {
            /* Mode must not be changed while counter is running */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        /* Invalid parameters or timer without encoder interface */
    }

    /*--------------------------- Encoder inputs -----------------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Config_InputChannel( periphId, TIM_CHANNEL_1, TIM_ACTIVE_INPUT_DIRECT, encoderConfig->Ti1Polarity,
                                            encoderConfig->InputFilter, TIM_INPUT_PRESCALER_DIV1 );
    }
    else
    {
        /* Previous step failed, nothing is written */
    }

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Config_InputChannel( periphId, TIM_CHANNEL_2, TIM_ACTIVE_INPUT_DIRECT, encoderConfig->Ti2Polarity,
                                            encoderConfig->InputFilter, TIM_INPUT_PRESCALER_DIV1 );
    }
    else
    {
        /* Previous configuration step failed */
    }

    /*------------------------ Prescaler and period --------------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg         = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      prescalerValue = TIM_PRESCALER_MIN - TIM_REG_DIV_OFFSET;

        LL_TIM_SetPrescaler( timReg, prescalerValue );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = LL_TIM_GetPrescaler( timReg );

            if( prescalerValue == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Previous configuration step failed */
    }

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Set_Period( periphId, encoderConfig->Period );
    }
    else
    {
        /* Previous configuration step failed */
    }

    /*--------------------------- Encoder mode -------------------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Config_SlaveModeReg( periphId, tim_EncoderModeLut[ encoderConfig->EncoderMode ] );
    }
    else
    {
        /* Previous configuration step failed */
    }

    return ( retState );
}


/**
 * \brief Configures timer in hall sensor interface mode.
 *
 * Hall inputs TI1 / TI2 / TI3 are combined by XOR into TI1, channel 1 captures
 * the time between hall edges (input TRC), the slave mode controller resets
 * the counter at each hall edge (trigger TI1F_ED, reset mode) and channel 2
 * generates delayed commutation pulse (OC2REF, PWM mode 2) which is used as
 * trigger output.
 *
 * \pre   Timer must have hall sensor interface (TIM1 / TIM2 / TIM3 / TIM4 /
 *        TIM5 / TIM8), counter must be disabled and the commutation delay must
 *        fit the timer resolution; otherwise \ref TIM_REQUEST_ERROR and no
 *        register is changed.
 *
 * \param periphId   [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param hallConfig [in]: Hall sensor interface configuration. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_Mode_HalSensor( tim_PeriphId_t periphId, const tim_HallSensorConfig_t * const hallConfig )
{
    tim_RequestState_t  retState    = TIM_REQUEST_ERROR;
    tim_FunctionState_t periphState = TIM_FUNCTION_ACTIVE;

    /*------------------------- Parameters validation ------------------------*/
    if( ( TIM_NULL_PTR   != hallConfig ) &&
        ( TIM_PERIPH_CNT  > periphId   )    )
    {
        if( ( TIM_INPUT_FILTER_CNT                                  > hallConfig->InputFilter      ) &&
            ( TIM_INPUT_PRESCALER_CNT                               > hallConfig->InputPrescaler   ) &&
            ( (tim_Counter_t)tim_PeriphConf[ periphId ].Resolution >= hallConfig->CommutationDelay )    )
        {
            retState = Tim_Check_InputPolarity( hallConfig->InputPolarity );
        }
        else
        {
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_HALL, TIM_CHANNEL_CNT );
    }
    else
    {
        /* Invalid parameters */
    }

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Get_PeriphState( periphId, &periphState );

        if( ( TIM_REQUEST_OK        == retState    ) &&
            ( TIM_FUNCTION_INACTIVE == periphState )    )
        {
            retState = TIM_REQUEST_OK;
        }
        else
        {
            /* Mode must not be changed while counter is running */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        /* Invalid parameters or timer without hall sensor interface */
    }

    /*------------------------- XOR of hall inputs ---------------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg = tim_PeriphConf[ periphId ].PeriphReg;

        LL_TIM_IC_EnableXORCombination( timReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t xorStateRaw = LL_TIM_IC_IsEnabledXORCombination( timReg );

            if( 0u != xorStateRaw )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* XOR combination has not been enabled yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Previous step failed, nothing is written */
    }

    /*---------------------- Capture of hall edge time -----------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Config_InputChannel( periphId, TIM_CHANNEL_1, TIM_ACTIVE_INPUT_TRC, hallConfig->InputPolarity,
                                            hallConfig->InputFilter, hallConfig->InputPrescaler );
    }
    else
    {
        /* Previous configuration step failed */
    }

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Set_SlaveMode( periphId, TIM_SLAVE_MODE_RESET, TIM_TRIGGER_INPUT_OF( periphId, LL_TIM_TS_TI1F_ED ) );
    }
    else
    {
        /* Previous configuration step failed */
    }

    /*------------------------ Commutation pulse -----------------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Set_ChannelMode( periphId, TIM_CHANNEL_2, TIM_CHANNEL_MODE_OUTPUT_PWM_INV );
    }
    else
    {
        /* Previous configuration step failed */
    }

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Set_CompareValue( periphId, TIM_CHANNEL_2, hallConfig->CommutationDelay );
    }
    else
    {
        /* Previous configuration step failed */
    }

    if( TIM_REQUEST_OK == retState )
    {
        /* All timers with hall sensor interface have master mode on STM32F4 */
        retState = Tim_Set_MasterTrigger( periphId, TIM_MASTER_TRIGGER_OC2REF );
    }
    else
    {
        /* Previous configuration step failed */
    }

    return ( retState );
}


/**
 * \brief Configures timer channel in PWM mode.
 *
 * Sets duty cycle to 0 %, then configures channel output (\ref Tim_Config_OutputChannel)
 * with output compare preload active (new duty cycle is applied at the next
 * update event) and generates update event to load preloaded registers.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelConfig [in]: Timer channel configuration (PWM channel mode from
 *                            \ref tim_ChannelMode_t). Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_Mode_Pwm( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig )
{
    tim_RequestState_t retState = Tim_Check_ChannelConfig( periphId, channelConfig, TIM_MODE_GROUP_PWM );

    if( TIM_REQUEST_OK == retState )
    {
        /* PWM starts with 0 % duty cycle */
        retState = Tim_Set_CompareValue( periphId, channelConfig->ChannelId, TIM_COUNTER_RESET_VAL );

        if( TIM_REQUEST_OK == retState )
        {
            retState = Tim_Config_OutputChannel( periphId, channelConfig, TIM_FUNCTION_ACTIVE );
        }
        else
        {
            /* Compare value configuration failed */
        }

        /* Force update generation (loads preloaded registers) */
        LL_TIM_GenerateEvent_UPDATE( tim_PeriphConf[ periphId ].PeriphReg );
    }
    else
    {
        /* Invalid parameters or not a PWM channel mode */
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Configures timer channel in forced output mode.
 *
 * Channel reference signal is forced active / inactive independently of the
 * compare value. Channel output is configured by \ref Tim_Config_OutputChannel
 * with output compare preload inactive.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelConfig [in]: Timer channel configuration (\ref TIM_CHANNEL_MODE_OUTPUT_FORCED_ACTIVE
 *                            or \ref TIM_CHANNEL_MODE_OUTPUT_FORCED_INACTIVE). Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_Mode_ForcedOutput( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig )
{
    tim_RequestState_t retState = Tim_Check_ChannelConfig( periphId, channelConfig, TIM_MODE_GROUP_FORCED );

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Config_OutputChannel( periphId, channelConfig, TIM_FUNCTION_INACTIVE );
    }
    else
    {
        /* Invalid parameters or not a forced output channel mode */
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Configures timer channel in output compare mode.
 *
 * Channel output is configured by \ref Tim_Config_OutputChannel with output
 * compare preload inactive (compare value set by \ref Tim_Set_CompareValue is
 * applied immediately). Compare value is not changed.
 *
 * \pre   \ref TIM_CHANNEL_MODE_OUTPUT_COMPARE_PULSE and \ref TIM_CHANNEL_MODE_OUTPUT_DIRECTION
 *        are available on channels 3 and 4 only; otherwise \ref TIM_REQUEST_ERROR
 *        and no register is changed.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelConfig [in]: Timer channel configuration (output compare channel mode from
 *                            \ref tim_ChannelMode_t). Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_Mode_OutputCompare( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig )
{
    tim_RequestState_t retState = Tim_Check_ChannelConfig( periphId, channelConfig, TIM_MODE_GROUP_COMPARE );

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Config_OutputChannel( periphId, channelConfig, TIM_FUNCTION_INACTIVE );
    }
    else
    {
        /* Invalid parameters or not an output compare channel mode */
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Returns minimal counter step time (resolution) of timer.
 *
 * Minimal step time corresponds to the timer kernel clock period (prescaler
 * divider \ref TIM_PRESCALER_MIN), rounded to the nearest nanosecond.
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param stepTime [out]: Pointer to store minimal step time in ns. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_TimStepTimeMin( tim_PeriphId_t periphId, tim_Time_ns_t * const stepTime )
{
    const tim_RequestState_t retState = Tim_Calc_StepTime( periphId, TIM_PRESCALER_MIN, stepTime );

    return ( retState );
}


/**
 * \brief Returns maximal counter step time of timer.
 *
 * Maximal step time corresponds to the maximal prescaler divider
 * (\ref TIM_PRESCALER_MAX + 1), rounded to the nearest nanosecond.
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param stepTime [out]: Pointer to store maximal step time in ns. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR (also if step time
 *         exceeds range of \ref tim_Time_ns_t).
 */
tim_RequestState_t Tim_Get_TimStepTimeMax( tim_PeriphId_t periphId, tim_Time_ns_t * const stepTime )
{
    const tim_RequestState_t retState = Tim_Calc_StepTime( periphId, TIM_PRESCALER_MAX + TIM_REG_DIV_OFFSET, stepTime );

    return ( retState );
}


/**
 * \brief Activates required timer peripheral (enables counter).
 *
 * \note  In one pulse mode the counter is stopped by hardware at the next
 *        update event, therefore the counter enable bit is not verified by
 *        read-back in this mode.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_PeriphActive( tim_PeriphId_t periphId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_PERIPH_CNT > periphId )
    {
        LL_TIM_EnableCounter( tim_PeriphConf[ periphId ].PeriphReg );

        const uint32_t onePulseMode = LL_TIM_GetOnePulseMode( tim_PeriphConf[ periphId ].PeriphReg );

        if( LL_TIM_ONEPULSEMODE_SINGLE == onePulseMode )
        {
            /* Counter may be already stopped by hardware, read-back not possible */
            retState = TIM_REQUEST_OK;
        }
        else
        {
            for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
            {
                const uint32_t counterState = LL_TIM_IsEnabledCounter( tim_PeriphConf[ periphId ].PeriphReg );

                if( 0u != counterState )
                {
                    retState = TIM_REQUEST_OK;
                    break;
                }
                else
                {
                    /* Counter has not been enabled yet */
                    retState = TIM_REQUEST_ERROR;
                }
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief De-activates required timer peripheral (disables counter).
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_PeriphInactive( tim_PeriphId_t periphId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_PERIPH_CNT > periphId )
    {
        LL_TIM_DisableCounter( tim_PeriphConf[ periphId ].PeriphReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t counterState = LL_TIM_IsEnabledCounter( tim_PeriphConf[ periphId ].PeriphReg );

            if( 0u == counterState )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Counter has not been disabled yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Configures PWM duty cycle of timer channel.
 *
 * Compare value is calculated from actual auto-reload value (64-bit arithmetic,
 * 32-bit timers can use full auto-reload range) and applied by \ref Tim_Set_CompareValue.
 *
 * \note  100 % duty cycle requires compare value auto-reload + 1, therefore it is
 *        not available when the auto-reload value equals the timer resolution.
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId [in]: Timer channel identification, value from \ref tim_ChannelId_t
 *                        (must be available on the timer).
 * \param dutyCycle [in]: Duty cycle in hundredths of percent (0 - \ref TIM_DUTY_CYCLE_MAX).
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_PwmMode_DutyCycle( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_CentiPercent_t dutyCycle )
{
    tim_RequestState_t retState = Tim_Check_ChannelId( periphId, channelId );

    if( ( TIM_REQUEST_OK      == retState  ) &&
        ( TIM_DUTY_CYCLE_MAX >= dutyCycle )    )
    {
        /* PWM signal period is determined by the value of the auto-reload register */
        const uint64_t period = (uint64_t)LL_TIM_GetAutoReload( tim_PeriphConf[ periphId ].PeriphReg ) + TIM_REG_DIV_OFFSET;

        /* Pulse duration is determined by the value of the compare register.  */
        /* Its value is calculated in order to match the requested duty cycle. */
        const uint64_t pulseDuration = ( (uint64_t)dutyCycle * period ) / TIM_DUTY_CYCLE_MAX;

        if( (uint64_t)tim_PeriphConf[ periphId ].Resolution >= pulseDuration )
        {
            retState = Tim_Set_CompareValue( periphId, channelId, (tim_Counter_t)pulseDuration );
        }
        else
        {
            /* Compare value does not fit the timer resolution */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        /* Invalid input parameters, nothing is written */
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Returns PWM duty cycle of timer channel.
 *
 * Duty cycle is calculated from the compare and auto-reload values and rounded
 * to the nearest hundredth of percent (limited to \ref TIM_DUTY_CYCLE_MAX).
 *
 * \param periphId   [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId  [in]: Timer channel identification, value from \ref tim_ChannelId_t.
 * \param dutyCycle [out]: Pointer to store duty cycle in hundredths of percent. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_PwmMode_DutyCycle( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_CentiPercent_t * const dutyCycle )
{
    tim_RequestState_t retState = Tim_Check_ChannelId( periphId, channelId );

    if( ( TIM_REQUEST_OK == retState  ) &&
        ( TIM_NULL_PTR   != dutyCycle )    )
    {
        TIM_TypeDef * const timReg       = tim_PeriphConf[ periphId ].PeriphReg;
        const uint64_t      period       = (uint64_t)LL_TIM_GetAutoReload( timReg ) + TIM_REG_DIV_OFFSET;
        const uint64_t      compareValue = tim_ChannelConfig[ channelId ].GetCompare( timReg );
        const uint64_t      dutyRaw      = ( ( compareValue * TIM_DUTY_CYCLE_MAX ) + ( period / 2u ) ) / period;

        if( TIM_DUTY_CYCLE_MAX >= dutyRaw )
        {
            *dutyCycle = (tim_CentiPercent_t)dutyRaw;
        }
        else
        {
            /* Compare value above period, output is permanently active */
            *dutyCycle = TIM_DUTY_CYCLE_MAX;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/*------------------ Output compare and PWM functionality --------------------*/

/**
 * \brief Sets output compare mode of timer channel.
 *
 * Only the output compare mode is changed (channel output, polarity and
 * preload stay unchanged), the mode can be changed while the counter runs.
 *
 * \pre   Channel must be available on the timer and configured as output (no
 *        input capture), mode must be available on STM32F4 (not pulse on compare,
 *        direction output, combined and asymmetric PWM); otherwise
 *        \ref TIM_REQUEST_ERROR and no register is changed.
 *
 * \param periphId    [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId   [in]: Timer channel identification, value from \ref tim_ChannelId_t.
 * \param channelMode [in]: Output channel mode, value from \ref tim_ChannelMode_t
 *                          (except \ref TIM_CHANNEL_MODE_INPUT_CAPTURE).
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_ChannelMode( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_ChannelMode_t channelMode )
{
    tim_RequestState_t retState = Tim_Check_ChannelId( periphId, channelId );

    if( ( TIM_REQUEST_OK       == retState    ) &&
        ( TIM_CHANNEL_MODE_CNT  > channelMode )    )
    {
        const tim_ModeGroup_t modeGroup   = tim_ChannelModeConfig[ channelMode ].ModeGroup;

        if( TIM_MODE_GROUP_INPUT == modeGroup )
        {
            /* Input capture is not an output compare mode */
            retState = TIM_REQUEST_ERROR;
        }
        else if( TIM_REG_VAL_UNAVAILABLE == tim_ChannelModeConfig[ channelMode ].ModeRegVal )
        {
            /* Output compare mode is not available on STM32F4 */
            retState = TIM_REQUEST_ERROR;
        }
        else
        {
            retState = TIM_REQUEST_OK;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg     = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      outputReg  = tim_ChannelConfig[ channelId ].ChannelOutputReg;
        const uint32_t      modeRegVal = tim_ChannelModeConfig[ channelMode ].ModeRegVal;

        LL_TIM_OC_SetMode( timReg, outputReg, modeRegVal );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = LL_TIM_OC_GetMode( timReg, outputReg );

            if( modeRegVal == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid parameters, nothing is written */
    }

    return ( retState );
}


/**
 * \brief Returns mode of timer channel.
 *
 * \param periphId     [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId    [in]: Timer channel identification, value from \ref tim_ChannelId_t.
 * \param channelMode [out]: Pointer to store channel mode (\ref TIM_CHANNEL_MODE_INPUT_CAPTURE
 *                           for channel configured as input). Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR (also for output compare
 *         modes not listed in \ref tim_ChannelMode_t).
 */
tim_RequestState_t Tim_Get_ChannelMode( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_ChannelMode_t * const channelMode )
{
    tim_RequestState_t retState = Tim_Check_ChannelId( periphId, channelId );

    if( ( TIM_REQUEST_OK == retState    ) &&
        ( TIM_NULL_PTR   != channelMode )    )
    {
        TIM_TypeDef * const timReg      = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      outputReg   = tim_ChannelConfig[ channelId ].ChannelOutputReg;
        uint32_t            activeInput = 0u;

        /* Channels 5 and 6 have no input stage */
        if( TIM_INPUT_CHANNEL_CNT > channelId )
        {
            activeInput = LL_TIM_IC_GetActiveInput( timReg, outputReg );
        }
        else
        {
            activeInput = 0u;
        }

        if( 0u != activeInput )
        {
            *channelMode = TIM_CHANNEL_MODE_INPUT_CAPTURE;
            retState     = TIM_REQUEST_OK;
        }
        else
        {
            const uint32_t modeRegVal = LL_TIM_OC_GetMode( timReg, outputReg );

            retState = TIM_REQUEST_ERROR;

            for( tim_ChannelMode_t modeIdx = TIM_CHANNEL_MODE_INPUT_CAPTURE; TIM_CHANNEL_MODE_CNT > modeIdx; modeIdx ++ )
            {
                if( ( TIM_MODE_GROUP_INPUT != tim_ChannelModeConfig[ modeIdx ].ModeGroup  ) &&
                    ( modeRegVal           == tim_ChannelModeConfig[ modeIdx ].ModeRegVal )    )
                {
                    *channelMode = modeIdx;
                    retState     = TIM_REQUEST_OK;
                    break;
                }
                else
                {
                    /* Continue with next channel mode */
                    retState = TIM_REQUEST_ERROR;
                }
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Sets compare value of timer channel.
 *
 * With output compare preload active the value is applied at the next update event.
 *
 * \param periphId     [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId    [in]: Timer channel identification, value from \ref tim_ChannelId_t.
 * \param compareValue [in]: Compare value (max. timer resolution, see \ref tim_Counter_t).
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_CompareValue( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_Counter_t compareValue )
{
    tim_RequestState_t retState = Tim_Check_ChannelId( periphId, channelId );

    if( ( TIM_REQUEST_OK                                         == retState     ) &&
        ( (tim_Counter_t)tim_PeriphConf[ periphId ].Resolution >= compareValue )    )
    {
        TIM_TypeDef * const timReg = tim_PeriphConf[ periphId ].PeriphReg;

        tim_ChannelConfig[ channelId ].SetCompare( timReg, compareValue );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = tim_ChannelConfig[ channelId ].GetCompare( timReg );

            if( compareValue == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Returns compare (or captured) value of timer channel.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId     [in]: Timer channel identification, value from \ref tim_ChannelId_t.
 * \param compareValue [out]: Pointer to store compare value. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_CompareValue( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_Counter_t * const compareValue )
{
    tim_RequestState_t retState = Tim_Check_ChannelId( periphId, channelId );

    if( ( TIM_REQUEST_OK == retState     ) &&
        ( TIM_NULL_PTR   != compareValue )    )
    {
        *compareValue = tim_ChannelConfig[ channelId ].GetCompare( tim_PeriphConf[ periphId ].PeriphReg );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Sets PWM pulse width of timer channel.
 *
 * Compare value is calculated from the actual timer counter frequency and
 * rounded to the nearest counter step.
 *
 * \pre   Timer must be clocked by internal clock and the pulse must not exceed
 *        the PWM period; otherwise \ref TIM_REQUEST_ERROR and no register is changed.
 *
 * \param periphId   [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId  [in]: Timer channel identification, value from \ref tim_ChannelId_t.
 * \param pulseWidth [in]: Required pulse width in ns.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_PwmMode_PulseWidth( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_Time_ns_t pulseWidth )
{
    tim_RequestState_t retState       = Tim_Check_ChannelId( periphId, channelId );
    tim_ClockSource_t  clockSource    = TIM_CLOCKSOURCE_EXTERNAL_ETR;
    tim_FreqHz_t       timerFrequency = 0u;

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Get_ClockSource( periphId, &clockSource );
    }
    else
    {
        /* Invalid peripheral or channel */
    }

    if( ( TIM_REQUEST_OK          == retState    ) &&
        ( TIM_CLOCKSOURCE_INT_CLK == clockSource )    )
    {
        retState = Tim_Get_ClkInternal( periphId, &timerFrequency );
    }
    else
    {
        /* Pulse width is defined only for internal clock source */
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        const uint64_t periodSteps = (uint64_t)LL_TIM_GetAutoReload( tim_PeriphConf[ periphId ].PeriphReg ) + TIM_REG_DIV_OFFSET;
        const uint64_t pulseSteps  = ( ( (uint64_t)pulseWidth * timerFrequency ) + ( TIM_NS_PER_S / 2u ) ) / TIM_NS_PER_S;

        if( periodSteps >= pulseSteps )
        {
            retState = Tim_Set_CompareValue( periphId, channelId, (tim_Counter_t)pulseSteps );
        }
        else
        {
            /* Pulse is longer than PWM period */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        /* Timer frequency is not available */
    }

    return ( retState );
}


/**
 * \brief Activates output compare preload of timer channel (compare value applied at update event).
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId [in]: Timer channel identification, value from \ref tim_ChannelId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_ComparePreloadActive( tim_PeriphId_t periphId, tim_ChannelId_t channelId )
{
    const tim_RequestState_t retState = Tim_Config_ComparePreload( periphId, channelId, TIM_FUNCTION_ACTIVE );

    return ( retState );
}


/**
 * \brief De-activates output compare preload of timer channel (compare value applied immediately).
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId [in]: Timer channel identification, value from \ref tim_ChannelId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_ComparePreloadInactive( tim_PeriphId_t periphId, tim_ChannelId_t channelId )
{
    const tim_RequestState_t retState = Tim_Config_ComparePreload( periphId, channelId, TIM_FUNCTION_INACTIVE );

    return ( retState );
}


/**
 * \brief Sets polarity of timer channel output.
 *
 * \param periphId       [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param outputId       [in]: Timer channel output identification, value from \ref tim_OutputId_t.
 * \param outputPolarity [in]: Output active level, value from \ref tim_Polarity_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_OutputPolarity( tim_PeriphId_t periphId, tim_OutputId_t outputId, tim_Polarity_t outputPolarity )
{
    tim_RequestState_t retState = Tim_Check_OutputId( periphId, outputId );

    if( ( TIM_REQUEST_OK    == retState       ) &&
        ( TIM_POLARITY_LOW  >= outputPolarity )    )
    {
        TIM_TypeDef * const timReg       = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      outputReg    = tim_OutputLut[ outputId ].OutputReg;
        uint32_t            polarityReg  = LL_TIM_OCPOLARITY_HIGH;

        if( TIM_POLARITY_HIGH == outputPolarity )
        {
            polarityReg = LL_TIM_OCPOLARITY_HIGH;
        }
        else
        {
            polarityReg = LL_TIM_OCPOLARITY_LOW;
        }

        LL_TIM_OC_SetPolarity( timReg, outputReg, polarityReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = LL_TIM_OC_GetPolarity( timReg, outputReg );

            if( polarityReg == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Returns polarity of timer channel output.
 *
 * \param periphId        [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param outputId        [in]: Timer channel output identification, value from \ref tim_OutputId_t.
 * \param outputPolarity [out]: Pointer to store output active level. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_OutputPolarity( tim_PeriphId_t periphId, tim_OutputId_t outputId, tim_Polarity_t * const outputPolarity )
{
    tim_RequestState_t retState = Tim_Check_OutputId( periphId, outputId );

    if( ( TIM_REQUEST_OK == retState       ) &&
        ( TIM_NULL_PTR   != outputPolarity )    )
    {
        const uint32_t polarityReg = LL_TIM_OC_GetPolarity( tim_PeriphConf[ periphId ].PeriphReg, tim_OutputLut[ outputId ].OutputReg );

        if( LL_TIM_OCPOLARITY_HIGH == polarityReg )
        {
            *outputPolarity = TIM_POLARITY_HIGH;
        }
        else
        {
            *outputPolarity = TIM_POLARITY_LOW;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Activates main output (MOE) of timer with break feature.
 *
 * \note  Main output is cleared by hardware on break event (unless automatic
 *        output enable is active).
 *
 * \pre   Timer must have break feature (TIM1 / TIM8);
 *        otherwise \ref TIM_REQUEST_ERROR and no register is changed.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_MainOutputActive( tim_PeriphId_t periphId )
{
    const tim_RequestState_t retState = Tim_Config_MainOutput( periphId, TIM_FUNCTION_ACTIVE );

    return ( retState );
}


/**
 * \brief De-activates main output (MOE) of timer with break feature.
 *
 * Outputs are switched to idle / off state according to the off-state configuration.
 *
 * \pre   Timer must have break feature (TIM1 / TIM8);
 *        otherwise \ref TIM_REQUEST_ERROR and no register is changed.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_MainOutputInactive( tim_PeriphId_t periphId )
{
    const tim_RequestState_t retState = Tim_Config_MainOutput( periphId, TIM_FUNCTION_INACTIVE );

    return ( retState );
}


/**
 * \brief Returns main output (MOE) state of timer with break feature.
 *
 * \param periphId     [in]: Timer peripheral identification, value from \ref tim_PeriphId_t
 *                           (timer with break feature).
 * \param outputState [out]: Pointer to store main output state. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_MainOutputState( tim_PeriphId_t periphId, tim_FunctionState_t * const outputState )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId    ) &&
        ( TIM_NULL_PTR  != outputState )    )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_BREAK, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        const uint32_t mainOutputRaw = LL_TIM_IsEnabledAllOutputs( tim_PeriphConf[ periphId ].PeriphReg );

        *outputState = Tim_Conv_RawToFunctionState( mainOutputRaw );
    }
    else
    {
        /* Invalid parameters or timer has no main output control */
    }

    return ( retState );
}


/*------------------------ Input capture functionality -----------------------*/

/**
 * \brief Returns last captured value of timer channel.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId     [in]: Timer channel identification, value from \ref tim_ChannelId_t
 *                            (channels 1 - 4 available on the timer).
 * \param captureValue [out]: Pointer to store captured counter value. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_CaptureValue( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_Counter_t * const captureValue )
{
    tim_RequestState_t retState = Tim_Check_InputChannel( periphId, channelId );

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Get_CompareValue( periphId, channelId, captureValue );
    }
    else
    {
        /* Invalid peripheral or channel without input stage */
    }

    return ( retState );
}


/**
 * \brief Sets digital input filter of timer channel.
 *
 * \param periphId    [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId   [in]: Timer channel identification, value from \ref tim_ChannelId_t
 *                          (channels 1 - 4 available on the timer).
 * \param inputFilter [in]: Input filter, value from \ref tim_InputFilter_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_InputFilter( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_InputFilter_t inputFilter )
{
    tim_RequestState_t retState = Tim_Check_InputChannel( periphId, channelId );

    if( ( TIM_REQUEST_OK       == retState    ) &&
        ( TIM_INPUT_FILTER_CNT  > inputFilter )    )
    {
        TIM_TypeDef * const timReg    = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      channelLl = tim_ChannelConfig[ channelId ].ChannelReg;
        const uint32_t      filterReg = tim_InputFilterLut[ inputFilter ];

        LL_TIM_IC_SetFilter( timReg, channelLl, filterReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = LL_TIM_IC_GetFilter( timReg, channelLl );

            if( filterReg == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Sets input capture prescaler of timer channel.
 *
 * \param periphId       [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId      [in]: Timer channel identification, value from \ref tim_ChannelId_t
 *                             (channels 1 - 4 available on the timer).
 * \param inputPrescaler [in]: Capture prescaler, value from \ref tim_InputPrescaler_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_InputPrescaler( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_InputPrescaler_t inputPrescaler )
{
    tim_RequestState_t retState = Tim_Check_InputChannel( periphId, channelId );

    if( ( TIM_REQUEST_OK          == retState       ) &&
        ( TIM_INPUT_PRESCALER_CNT  > inputPrescaler )    )
    {
        TIM_TypeDef * const timReg       = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      channelLl    = tim_ChannelConfig[ channelId ].ChannelReg;
        const uint32_t      prescalerReg = tim_InputPrescalerLut[ inputPrescaler ];

        LL_TIM_IC_SetPrescaler( timReg, channelLl, prescalerReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = LL_TIM_IC_GetPrescaler( timReg, channelLl );

            if( prescalerReg == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Sets captured edge(s) of timer channel input.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId     [in]: Timer channel identification, value from \ref tim_ChannelId_t
 *                            (channels 1 - 4 available on the timer).
 * \param inputPolarity [in]: Input polarity, value from \ref tim_InputPolarity_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_InputPolarity( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_InputPolarity_t inputPolarity )
{
    tim_RequestState_t retState = Tim_Check_InputChannel( periphId, channelId );

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Check_InputPolarity( inputPolarity );
    }
    else
    {
        /* Invalid peripheral or channel without input stage */
    }

    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg    = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      channelLl = tim_ChannelConfig[ channelId ].ChannelReg;

        LL_TIM_IC_SetPolarity( timReg, channelLl, (uint32_t)inputPolarity );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = LL_TIM_IC_GetPolarity( timReg, channelLl );

            if( (uint32_t)inputPolarity == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid parameters, nothing is written */
    }

    return ( retState );
}


/**
 * \brief Configures PWM input measurement mode.
 *
 * Input channel captures the period (direct input, required edge), the paired
 * channel captures the pulse width (indirect input, opposite edge) and the
 * slave mode controller resets the counter at each period start (trigger
 * TI1FP1 / TI2FP2, reset mode). The timer is started by \ref Tim_Start, the
 * result is read by \ref Tim_Get_InputPwm.
 *
 * \pre   Timer must have channels 1 and 2 and slave mode controller, counter
 *        must be disabled, input channel must be \ref TIM_CHANNEL_1 or
 *        \ref TIM_CHANNEL_2 and polarity must not be both edges; otherwise
 *        \ref TIM_REQUEST_ERROR and no register is changed.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param inputChannel  [in]: Channel with the measured signal (\ref TIM_CHANNEL_1 or \ref TIM_CHANNEL_2).
 * \param inputPolarity [in]: Period start edge (\ref TIM_INPUT_POLARITY_NORMAL or \ref TIM_INPUT_POLARITY_INVERTED).
 * \param inputFilter   [in]: Input filter, value from \ref tim_InputFilter_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_Mode_InputPwm( tim_PeriphId_t periphId, tim_ChannelId_t inputChannel, tim_InputPolarity_t inputPolarity, tim_InputFilter_t inputFilter )
{
    tim_RequestState_t  retState       = TIM_REQUEST_ERROR;
    tim_ChannelId_t     pairedChannel  = TIM_CHANNEL_2;
    tim_TriggerInput_t  triggerInput   = TIM_TRIGGER_INPUT_UNUSED;
    tim_InputPolarity_t pairedPolarity = TIM_INPUT_POLARITY_INVERTED;
    tim_FunctionState_t periphState    = TIM_FUNCTION_ACTIVE;

    /*------------------------- Parameters validation ------------------------*/
    if( ( ( TIM_CHANNEL_1               == inputChannel  )   ||
          ( TIM_CHANNEL_2               == inputChannel  )      ) &&
        ( ( TIM_INPUT_POLARITY_NORMAL   == inputPolarity )   ||
          ( TIM_INPUT_POLARITY_INVERTED == inputPolarity )      ) &&
        ( TIM_INPUT_FILTER_CNT           > inputFilter   )           )
    {
        /* Both channels 1 and 2 must be available */
        retState = Tim_Check_InputChannel( periphId, TIM_CHANNEL_2 );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_SLAVE, TIM_CHANNEL_CNT );
    }
    else
    {
        /* Invalid parameters */
    }

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Get_PeriphState( periphId, &periphState );

        if( ( TIM_REQUEST_OK        == retState    ) &&
            ( TIM_FUNCTION_INACTIVE == periphState )    )
        {
            retState = TIM_REQUEST_OK;
        }
        else
        {
            /* Mode must not be changed while counter is running */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        /* Timer without slave mode controller or invalid parameters */
    }

    /*------------------------ Channels configuration ------------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        if( TIM_CHANNEL_1 == inputChannel )
        {
            pairedChannel = TIM_CHANNEL_2;
            triggerInput  = TIM_TRIGGER_INPUT_OF( periphId, LL_TIM_TS_TI1FP1 );
        }
        else
        {
            pairedChannel = TIM_CHANNEL_1;
            triggerInput  = TIM_TRIGGER_INPUT_OF( periphId, LL_TIM_TS_TI2FP2 );
        }

        if( TIM_INPUT_POLARITY_NORMAL == inputPolarity )
        {
            pairedPolarity = TIM_INPUT_POLARITY_INVERTED;
        }
        else
        {
            pairedPolarity = TIM_INPUT_POLARITY_NORMAL;
        }

        retState = Tim_Config_InputChannel( periphId, inputChannel, TIM_ACTIVE_INPUT_DIRECT, inputPolarity,
                                            inputFilter, TIM_INPUT_PRESCALER_DIV1 );
    }
    else
    {
        /* Previous step failed, nothing is written */
    }

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Config_InputChannel( periphId, pairedChannel, TIM_ACTIVE_INPUT_INDIRECT, pairedPolarity,
                                            inputFilter, TIM_INPUT_PRESCALER_DIV1 );
    }
    else
    {
        /* Previous configuration step failed */
    }

    /*------------------- Counter reset at period start ----------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Set_SlaveMode( periphId, TIM_SLAVE_MODE_RESET, triggerInput );
    }
    else
    {
        /* Previous configuration step failed */
    }

    return ( retState );
}


/**
 * \brief Returns frequency and duty cycle measured in PWM input mode.
 *
 * \note  Values of the last complete period are returned; for input signal
 *        slower than the counter period the result is not valid.
 *
 * \pre   PWM input mode must be configured by \ref Tim_Set_Mode_InputPwm and the
 *        timer must be clocked by internal clock; otherwise (or if no period was
 *        captured yet) \ref TIM_REQUEST_ERROR.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param inputChannel  [in]: Channel with the measured signal (\ref TIM_CHANNEL_1 or \ref TIM_CHANNEL_2).
 * \param frequency    [out]: Pointer to store signal frequency in Hz. Must not be NULL.
 * \param dutyCycle    [out]: Pointer to store duty cycle in hundredths of percent. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_InputPwm( tim_PeriphId_t periphId, tim_ChannelId_t inputChannel, tim_FreqHz_t * const frequency, tim_CentiPercent_t * const dutyCycle )
{
    tim_RequestState_t retState       = TIM_REQUEST_ERROR;
    tim_ClockSource_t  clockSource    = TIM_CLOCKSOURCE_EXTERNAL_ETR;
    tim_FreqHz_t       timerFrequency = 0u;
    tim_Counter_t      periodSteps    = 0u;
    tim_Counter_t      pulseSteps     = 0u;
    tim_ChannelId_t    pairedChannel  = TIM_CHANNEL_2;

    if( ( ( TIM_CHANNEL_1 == inputChannel )   ||
          ( TIM_CHANNEL_2 == inputChannel )      ) &&
        ( TIM_NULL_PTR   != frequency     )        &&
        ( TIM_NULL_PTR   != dutyCycle     )           )
    {
        retState = Tim_Check_InputChannel( periphId, TIM_CHANNEL_2 );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Get_ClockSource( periphId, &clockSource );
    }
    else
    {
        /* Invalid parameters */
    }

    if( ( TIM_REQUEST_OK          == retState    ) &&
        ( TIM_CLOCKSOURCE_INT_CLK == clockSource )    )
    {
        retState = Tim_Get_ClkInternal( periphId, &timerFrequency );
    }
    else
    {
        /* Measurement is defined only for internal clock source */
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        if( TIM_CHANNEL_1 == inputChannel )
        {
            pairedChannel = TIM_CHANNEL_2;
        }
        else
        {
            pairedChannel = TIM_CHANNEL_1;
        }

        periodSteps = tim_ChannelConfig[ inputChannel ].GetCompare( tim_PeriphConf[ periphId ].PeriphReg );
        pulseSteps  = tim_ChannelConfig[ pairedChannel ].GetCompare( tim_PeriphConf[ periphId ].PeriphReg );

        if( TIM_COUNTER_RESET_VAL != periodSteps )
        {
            const uint64_t dutyRaw = ( ( (uint64_t)pulseSteps * TIM_DUTY_CYCLE_MAX ) + ( periodSteps / 2u ) ) / periodSteps;

            *frequency = timerFrequency / periodSteps;

            if( TIM_DUTY_CYCLE_MAX >= dutyRaw )
            {
                *dutyCycle = (tim_CentiPercent_t)dutyRaw;
            }
            else
            {
                *dutyCycle = TIM_DUTY_CYCLE_MAX;
            }
        }
        else
        {
            /* No period captured yet */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        /* Timer frequency is not available */
    }

    return ( retState );
}


/**
 * \brief Selects source of timer channel input (TIx).
 *
 * STM32F4 timers have the input remap (TIMx_OR) only on selected channels, the items of
 * \ref tim_InputSource_t are:
 *  - TIM5 channel 4: \ref TIM_INPUT_SOURCE_TIM5_CH4_PIN (GPIO), _LSI, _LSE and
 *    _RTC_WKUP (RTC wake-up interrupt)
 *  - TIM11 channel 1: \ref TIM_INPUT_SOURCE_TIM11_CH1_PIN (GPIO), _HSE_RTC and
 *    _SPDIFRX_FRAME_SYNC (devices with SPDIFRX)
 *
 * The item must belong to the timer and the channel of the call.
 *
 * \pre   Channel must be available on the timer (channels 1 - 4) and have an input
 *        remap; otherwise \ref TIM_REQUEST_ERROR and no register is changed.
 *
 * \param periphId    [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId   [in]: Timer channel identification, value from \ref tim_ChannelId_t.
 * \param inputSource [in]: Input source, item of \ref tim_InputSource_t of the timer channel.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_InputSource( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_InputSource_t inputSource )
{
    tim_RequestState_t retState = Tim_Check_InputChannel( periphId, channelId );

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Check_InputSource( periphId, channelId, inputSource );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        const tim_InputRemapConfig_t * const remapConfig = Tim_Get_InputRemap( periphId, channelId );
        TIM_TypeDef * const                  timReg      = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t                       selRegVal   = remapConfig->SelVal[ TIM_BIT_MASK_DECODE_INPUT_SOURCE_CODE( inputSource ) ];

        MODIFY_REG( timReg->OR, remapConfig->SelMask, selRegVal );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = READ_BIT( timReg->OR, remapConfig->SelMask );

            if( selRegVal == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid parameters or channel without input remap */
    }

    return ( retState );
}


/**
 * \brief Returns source of timer channel input (TIx).
 *
 * \pre   Channel must be available on the timer (channels 1 - 4) and have an input
 *        remap; otherwise \ref TIM_REQUEST_ERROR.
 *
 * \param periphId     [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId    [in]: Timer channel identification, value from \ref tim_ChannelId_t.
 * \param inputSource [out]: Pointer to store input source, item of \ref tim_InputSource_t of the
 *                           timer channel. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_InputSource( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_InputSource_t * const inputSource )
{
    tim_RequestState_t                   retState    = Tim_Check_InputChannel( periphId, channelId );
    const tim_InputRemapConfig_t * const remapConfig = Tim_Get_InputRemap( periphId, channelId );

    if( ( TIM_REQUEST_OK == retState    ) &&
        ( TIM_NULL_PTR   != remapConfig ) &&
        ( TIM_NULL_PTR   != inputSource )    )
    {
        const uint32_t selRegVal = READ_BIT( tim_PeriphConf[ periphId ].PeriphReg->OR, remapConfig->SelMask );
        uint32_t       selCode   = 0u;

        /* Remap values not listed in the table are additional GPIO selections */
        for( uint32_t codeIdx = 0u; TIM_INPUT_REMAP_SOURCE_CNT > codeIdx; codeIdx ++ )
        {
            if( remapConfig->SelVal[ codeIdx ] == selRegVal )
            {
                selCode = codeIdx;
                break;
            }
            else
            {
                /* Continue with next selection code */
            }
        }

        *inputSource = TIM_INPUT_SOURCE_OF( periphId, channelId, selCode );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Sets dead-time and sampling clock (fDTS) division.
 *
 * fDTS is used by input filters, dead-time generator and break filters.
 *
 * \pre   Timer must have clock division (all except TIM6 / TIM7); otherwise
 *        \ref TIM_REQUEST_ERROR and no register is changed.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param clockDiv [in]: Clock division, value from \ref tim_ClockDiv_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_ClockDivision( tim_PeriphId_t periphId, tim_ClockDiv_t clockDiv )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT    > periphId ) &&
        ( TIM_CLOCK_DIV_CNT > clockDiv )    )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_CLOCK_DIV, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg      = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      clockDivReg = tim_ClockDivLut[ clockDiv ];

        LL_TIM_SetClockDivision( timReg, clockDivReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = LL_TIM_GetClockDivision( timReg );

            if( clockDivReg == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid parameters or timer has no clock division */
    }

    return ( retState );
}


/**
 * \brief Returns dead-time and sampling clock (fDTS) division.
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param clockDiv [out]: Pointer to store clock division. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_ClockDivision( tim_PeriphId_t periphId, tim_ClockDiv_t * const clockDiv )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId ) &&
        ( TIM_NULL_PTR  != clockDiv )    )
    {
        const uint32_t clockDivReg = LL_TIM_GetClockDivision( tim_PeriphConf[ periphId ].PeriphReg );

        for( tim_ClockDiv_t divIdx = TIM_CLOCK_DIV_1; TIM_CLOCK_DIV_CNT > divIdx; divIdx ++ )
        {
            if( tim_ClockDivLut[ divIdx ] == clockDivReg )
            {
                *clockDiv = divIdx;
                retState  = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Continue with next clock division */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/*--------------------- Encoder interface functionality ----------------------*/

/**
 * \brief Returns encoder position (counter value) of timer in encoder mode.
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t
 *                        (timer with encoder interface).
 * \param position [out]: Pointer to store encoder position. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_EncoderPosition( tim_PeriphId_t periphId, tim_Counter_t * const position )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId ) &&
        ( TIM_NULL_PTR  != position )    )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_ENCODER, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        *position = LL_TIM_GetCounter( tim_PeriphConf[ periphId ].PeriphReg );
    }
    else
    {
        /* Invalid parameters or timer without encoder interface */
    }

    return ( retState );
}


/**
 * \brief Configures encoder index function (counter reset by index signal on ETR input).
 *
 * \note  Encoder index is not available on STM32F4 timers, the function is kept
 *        for compatibility of the module interface.
 *
 * \param periphId    [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param indexConfig [in]: Encoder index configuration (not used).
 *
 * \return Always \ref TIM_REQUEST_ERROR (no register is changed).
 */
tim_RequestState_t Tim_Set_EncoderIndex( tim_PeriphId_t periphId, const tim_EncoderIndexConfig_t * const indexConfig )
{
    (void)periphId;
    (void)indexConfig;

    return ( TIM_REQUEST_ERROR );
}


/*----------------------- Dead-time and break functionality ------------------*/

/**
 * \brief Sets dead-time of complementary outputs.
 *
 * Dead-time is calculated from the timer kernel clock and the dead-time and
 * sampling clock division (\ref Tim_Set_ClockDivision), rounded to whole fDTS
 * periods and truncated to the dead-time step of its range (0 - 127 step 1,
 * 128 - 254 step 2, 256 - 504 step 8, 512 - 1008 step 16 fDTS periods).
 *
 * \note  STM32F4 timers have symmetric dead-time only (single DTG field), rising
 *        and falling edge dead-time must result in the same generator value.
 *
 * \pre   Timer must have break feature (TIM1 / TIM8), dead-time must be in one
 *        of the ranges (255 and 505 - 511 fDTS periods are not representable),
 *        rising and falling dead-time must be equal and registers must not be
 *        locked (\ref Tim_Set_LockLevel); otherwise \ref TIM_REQUEST_ERROR (nothing
 *        is written for invalid parameters).
 *
 * \param periphId        [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param risingDeadTime  [in]: Dead-time inserted at rising edge of OCxREF in ns.
 * \param fallingDeadTime [in]: Dead-time inserted at falling edge of OCxREF in ns.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_DeadTime( tim_PeriphId_t periphId, tim_Time_ns_t risingDeadTime, tim_Time_ns_t fallingDeadTime )
{
    tim_RequestState_t retState    = TIM_REQUEST_ERROR;
    uint32_t           risingDtg   = 0u;
    uint32_t           fallingDtg  = 0u;

    if( TIM_PERIPH_CNT > periphId )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_BREAK, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    /*----------------------- Dead-time generator values ---------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Calc_DeadTimeReg( periphId, risingDeadTime, &risingDtg );
    }
    else
    {
        /* Invalid peripheral or timer without dead-time generator */
    }

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Calc_DeadTimeReg( periphId, fallingDeadTime, &fallingDtg );
    }
    else
    {
        /* Rising edge dead-time out of range */
    }

    if( ( TIM_REQUEST_OK == retState   ) &&
        ( risingDtg      != fallingDtg )    )
    {
        /* Asymmetric dead-time is not available on STM32F4 */
        retState = TIM_REQUEST_ERROR;
    }
    else
    {
        /* Symmetric dead-time or previous step failed */
    }

    /*--------------------------- Registers write ----------------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg = tim_PeriphConf[ periphId ].PeriphReg;

        LL_TIM_OC_SetDeadTime( timReg, risingDtg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t dtgRead = READ_BIT( timReg->BDTR, TIM_BDTR_DTG );

            if( risingDtg == dtgRead )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been configured (or is locked) */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid parameters, nothing is written */
    }

    return ( retState );
}


/**
 * \brief Configures break function (BKIN input).
 *
 * \note  STM32F4 break input has no digital filter and no bidirectional mode,
 *        only \ref TIM_INPUT_FILTER_INACTIVE and \ref TIM_BREAK_MODE_INPUT are
 *        accepted.
 *
 * \pre   Timer must have break feature and registers must not be locked;
 *        otherwise \ref TIM_REQUEST_ERROR.
 *
 * \param periphId    [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param breakConfig [in]: Break configuration. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_BreakConfig( tim_PeriphId_t periphId, const tim_BreakConfig_t * const breakConfig )
{
    tim_RequestState_t retState = Tim_Check_BreakConfig( periphId, breakConfig );

    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg      = tim_PeriphConf[ periphId ].PeriphReg;
        uint32_t            polarityReg = LL_TIM_BREAK_POLARITY_HIGH;
        uint32_t            enableReg   = TIM_REG_BIT_CLEARED;

        if( TIM_POLARITY_HIGH == breakConfig->BreakPolarity )
        {
            polarityReg = LL_TIM_BREAK_POLARITY_HIGH;
        }
        else
        {
            polarityReg = LL_TIM_BREAK_POLARITY_LOW;
        }

        LL_TIM_ConfigBRK( timReg, polarityReg );

        if( TIM_FUNCTION_ACTIVE == breakConfig->BreakState )
        {
            LL_TIM_EnableBRK( timReg );
            enableReg = TIM_BDTR_BKE;
        }
        else
        {
            LL_TIM_DisableBRK( timReg );
            enableReg = TIM_REG_BIT_CLEARED;
        }

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = READ_BIT( timReg->BDTR, TIM_BDTR_BKE | TIM_BDTR_BKP );

            if( ( enableReg | polarityReg ) == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register values have not been configured (or are locked) */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid parameters, nothing is written */
    }

    return ( retState );
}


/**
 * \brief Configures break 2 function (BKIN2).
 *
 * \note  Break 2 is not available on STM32F4 timers, the function is kept for
 *        compatibility of the module interface.
 *
 * \param periphId    [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param breakConfig [in]: Break 2 configuration (not used).
 *
 * \return Always \ref TIM_REQUEST_ERROR (no register is changed).
 */
tim_RequestState_t Tim_Set_Break2Config( tim_PeriphId_t periphId, const tim_BreakConfig_t * const breakConfig )
{
    (void)periphId;
    (void)breakConfig;

    return ( TIM_REQUEST_ERROR );
}


/**
 * \brief Activates automatic output enable (MOE set automatically at update event after break).
 *
 * \pre   Timer must have break feature; otherwise \ref TIM_REQUEST_ERROR.
 *
 * \warning Device errata "PWM re-enabled in automatic output enable mode despite of system
 *          break": the system break (clock security system) is not latched - with AOE the PWM
 *          outputs are re-armed on the next counter period after a clock failure. AOE is
 *          refused while the clock security system is enabled (RCC CR CSSON). Use the break
 *          function solely for fault protection with AOE inactive, control loops by output
 *          compare clear. CSS enabled after AOE is not detected.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_AutomaticOutputActive( tim_PeriphId_t periphId )
{
    const tim_RequestState_t retState = Tim_Config_AutomaticOutput( periphId, TIM_FUNCTION_ACTIVE );

    return ( retState );
}


/**
 * \brief De-activates automatic output enable (MOE set only by software after break).
 *
 * \pre   Timer must have break feature; otherwise \ref TIM_REQUEST_ERROR.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_AutomaticOutputInactive( tim_PeriphId_t periphId )
{
    const tim_RequestState_t retState = Tim_Config_AutomaticOutput( periphId, TIM_FUNCTION_INACTIVE );

    return ( retState );
}


/**
 * \brief Configures off-state of outputs.
 *
 * \pre   Timer must have break feature and registers must not be locked
 *        (lock level 2 or higher); otherwise \ref TIM_REQUEST_ERROR.
 *
 * \param periphId     [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param offStateIdle [in]: Off-state selection for idle mode (MOE = 0, OSSI), value from \ref tim_FunctionState_t.
 * \param offStateRun  [in]: Off-state selection for run mode (MOE = 1, OSSR), value from \ref tim_FunctionState_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_OffStateConfig( tim_PeriphId_t periphId, tim_FunctionState_t offStateIdle, tim_FunctionState_t offStateRun )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT       > periphId     ) &&
        ( TIM_FUNCTION_ACTIVE >= offStateIdle ) &&
        ( TIM_FUNCTION_ACTIVE >= offStateRun  )    )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_BREAK, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg  = tim_PeriphConf[ periphId ].PeriphReg;
        uint32_t            ossiReg = LL_TIM_OSSI_DISABLE;
        uint32_t            ossrReg = LL_TIM_OSSR_DISABLE;

        if( TIM_FUNCTION_ACTIVE == offStateIdle )
        {
            ossiReg = LL_TIM_OSSI_ENABLE;
        }
        else
        {
            ossiReg = LL_TIM_OSSI_DISABLE;
        }

        if( TIM_FUNCTION_ACTIVE == offStateRun )
        {
            ossrReg = LL_TIM_OSSR_ENABLE;
        }
        else
        {
            ossrReg = LL_TIM_OSSR_DISABLE;
        }

        LL_TIM_SetOffStates( timReg, ossiReg, ossrReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = READ_BIT( timReg->BDTR, TIM_BDTR_OSSI | TIM_BDTR_OSSR );

            if( ( ossiReg | ossrReg ) == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been configured (or is locked) */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid parameters or timer without break feature */
    }

    return ( retState );
}


/**
 * \brief Sets lock level of timer configuration registers.
 *
 * \note  Lock level can be written only once after reset, locked fields stay
 *        write protected until the next reset.
 *
 * \pre   Timer must have break feature; otherwise \ref TIM_REQUEST_ERROR.
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param lockLevel [in]: Lock level, value from \ref tim_LockLevel_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR (also if the lock
 *         level was already written).
 */
tim_RequestState_t Tim_Set_LockLevel( tim_PeriphId_t periphId, tim_LockLevel_t lockLevel )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT     > periphId  ) &&
        ( TIM_LOCK_LEVEL_CNT > lockLevel )    )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_BREAK, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg  = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      lockReg = tim_LockLevelLut[ lockLevel ];

        LL_TIM_CC_SetLockLevel( timReg, lockReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = READ_BIT( timReg->BDTR, TIM_BDTR_LOCK );

            if( lockReg == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Lock level has not been written (already locked) */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid parameters or timer without break feature */
    }

    return ( retState );
}


/**
 * \brief Activates preload of capture / compare control bits (commutation).
 *
 * CCxE, CCxNE and OCxM bits are preloaded and updated by commutation event
 * (\ref Tim_Generate_Event with \ref TIM_EVENT_COMMUTATION) or also by trigger input.
 *
 * \pre   Timer must have commutation feature (TIM1 / TIM8); otherwise
 *        \ref TIM_REQUEST_ERROR and no register is changed.
 *
 * \param periphId     [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param updateSource [in]: Update source of preloaded bits, value from \ref tim_CommutationUpdate_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_CommutationPreloadActive( tim_PeriphId_t periphId, tim_CommutationUpdate_t updateSource )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT             > periphId     ) &&
        ( TIM_COMMUTATION_UPDATE_CNT > updateSource )    )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_COMMUTATION, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg    = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      updateReg = tim_CommutationUpdateLut[ updateSource ];

        LL_TIM_CC_SetUpdate( timReg, updateReg );
        LL_TIM_CC_EnablePreload( timReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = READ_BIT( timReg->CR2, TIM_CR2_CCPC | TIM_CR2_CCUS );

            if( ( TIM_CR2_CCPC | updateReg ) == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid parameters or timer without commutation feature */
    }

    return ( retState );
}


/**
 * \brief De-activates preload of capture / compare control bits (bits written immediately).
 *
 * \pre   Timer must have commutation feature; otherwise \ref TIM_REQUEST_ERROR.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_CommutationPreloadInactive( tim_PeriphId_t periphId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_PERIPH_CNT > periphId )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_COMMUTATION, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg = tim_PeriphConf[ periphId ].PeriphReg;

        LL_TIM_CC_DisablePreload( timReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t preloadRaw = LL_TIM_CC_IsEnabledPreload( timReg );

            if( 0u == preloadRaw )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid peripheral or timer without commutation feature */
    }

    return ( retState );
}


/*--------------------- DMA and external trigger functionality ---------------*/

/**
 * \brief Activates timer DMA request.
 *
 * \note  DMA channel (transfer, request line, addresses) is configured by the
 *        DMA module, register address is provided by \ref Tim_Get_DmaRegAddr.
 *
 * \pre   DMA request must be available on the timer; otherwise
 *        \ref TIM_REQUEST_ERROR and no register is changed.
 *
 * \param periphId   [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param dmaRequest [in]: DMA request identification, value from \ref tim_DmaRequest_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_DmaRequestActive( tim_PeriphId_t periphId, tim_DmaRequest_t dmaRequest )
{
    const tim_RequestState_t retState = Tim_Config_DmaRequest( periphId, dmaRequest, TIM_FUNCTION_ACTIVE );

    return ( retState );
}


/**
 * \brief De-activates timer DMA request.
 *
 * \pre   DMA request must be available on the timer; otherwise
 *        \ref TIM_REQUEST_ERROR and no register is changed.
 *
 * \param periphId   [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param dmaRequest [in]: DMA request identification, value from \ref tim_DmaRequest_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_DmaRequestInactive( tim_PeriphId_t periphId, tim_DmaRequest_t dmaRequest )
{
    const tim_RequestState_t retState = Tim_Config_DmaRequest( periphId, dmaRequest, TIM_FUNCTION_INACTIVE );

    return ( retState );
}


/**
 * \brief Configures DMA burst transfer (DMA writes / reads consecutive timer
 *        registers through TIMx_DMAR).
 *
 * \note  STM32F4 timers have no burst source selection (DBSS), the burst is
 *        executed by the DMA stream of any enabled timer DMA request. The burst
 *        source is only validated (request available on the timer), the request
 *        itself is enabled by \ref Tim_Set_DmaRequestActive.
 *
 * \pre   Timer must have DMA burst feature, the burst source request must be
 *        available on the timer and the base register must exist on STM32F4
 *        (TIMx_CR1 - TIMx_BDTR); otherwise \ref TIM_REQUEST_ERROR and no
 *        register is changed.
 *
 * \param periphId    [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param burstSource [in]: DMA request triggering the burst, value from \ref tim_DmaRequest_t.
 * \param baseReg     [in]: First transferred register, value from \ref tim_DmaBurstReg_t.
 * \param burstLength [in]: Count of transferred registers (\ref TIM_DMA_BURST_LEN_MIN - \ref TIM_DMA_BURST_LEN_MAX).
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_DmaBurst( tim_PeriphId_t periphId, tim_DmaRequest_t burstSource, tim_DmaBurstReg_t baseReg, tim_DmaBurstLen_t burstLength )
{
    tim_RequestState_t retState = Tim_Check_DmaRequest( periphId, burstSource );

    if( ( TIM_REQUEST_OK        == retState    ) &&
        ( TIM_DMA_BURST_REG_CNT  > baseReg     ) &&
        ( TIM_DMA_BURST_LEN_MIN <= burstLength ) &&
        ( TIM_DMA_BURST_LEN_MAX >= burstLength ) &&
        ( TIM_REG_VAL_UNAVAILABLE != tim_DmaBurstRegLut[ baseReg ] ) )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_DMA_BURST, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg      = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      baseAddrReg = tim_DmaBurstRegLut[ baseReg ];
        const uint32_t      lengthReg   = ( (uint32_t)burstLength - TIM_DMA_BURST_LEN_MIN ) << TIM_DCR_DBL_Pos;

        /* STM32F4 has no burst source selection, burst is executed by any enabled DMA request */
        LL_TIM_ConfigDMABurst( timReg, baseAddrReg, lengthReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = READ_BIT( timReg->DCR, TIM_DCR_DBA | TIM_DCR_DBL );

            if( ( baseAddrReg | lengthReg ) == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid parameters, nothing is written */
    }

    return ( retState );
}


/**
 * \brief Returns address of timer register for DMA transfer configuration.
 *
 * \pre   Capture / compare register channel must be available on the timer,
 *        DMA burst register requires DMA burst feature; otherwise
 *        \ref TIM_REQUEST_ERROR.
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param dmaReg    [in]: Timer register, value from \ref tim_DmaReg_t.
 * \param regAddr  [out]: Pointer to store register address. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_DmaRegAddr( tim_PeriphId_t periphId, tim_DmaReg_t dmaReg, tim_RegAddr_t * const regAddr )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT  > periphId ) &&
        ( TIM_DMA_REG_CNT > dmaReg   ) &&
        ( TIM_NULL_PTR   != regAddr  )    )
    {
        retState = Tim_Check_Feature( periphId, tim_DmaRegLut[ dmaReg ].Availability, tim_DmaRegLut[ dmaReg ].ChannelId );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        *regAddr = (tim_RegAddr_t)( (uintptr_t)tim_PeriphConf[ periphId ].PeriphReg + tim_DmaRegLut[ dmaReg ].RegOffset );
    }
    else
    {
        /* Invalid parameters or register not available on the timer */
    }

    return ( retState );
}


/**
 * \brief Configures external trigger input (ETR) conditioning.
 *
 * \pre   Timer must have external trigger input; otherwise \ref TIM_REQUEST_ERROR
 *        and no register is changed.
 *
 * \param periphId     [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param etrPolarity  [in]: Active level / edge of ETR (\ref TIM_POLARITY_HIGH non-inverted).
 * \param etrPrescaler [in]: ETR prescaler, value from \ref tim_EtrPrescaler_t.
 * \param etrFilter    [in]: ETR digital filter, value from \ref tim_InputFilter_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_EtrConfig( tim_PeriphId_t periphId, tim_Polarity_t etrPolarity, tim_EtrPrescaler_t etrPrescaler, tim_InputFilter_t etrFilter )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT        > periphId     ) &&
        ( TIM_POLARITY_LOW     >= etrPolarity  ) &&
        ( TIM_ETR_PRESCALER_CNT > etrPrescaler ) &&
        ( TIM_INPUT_FILTER_CNT  > etrFilter    )    )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_ETR, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg       = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      prescalerReg = tim_EtrPrescalerLut[ etrPrescaler ];
        const uint32_t      filterReg    = tim_EtrFilterLut[ etrFilter ];
        uint32_t            polarityReg  = LL_TIM_ETR_POLARITY_NONINVERTED;

        if( TIM_POLARITY_HIGH == etrPolarity )
        {
            polarityReg = LL_TIM_ETR_POLARITY_NONINVERTED;
        }
        else
        {
            polarityReg = LL_TIM_ETR_POLARITY_INVERTED;
        }

        LL_TIM_ConfigETR( timReg, polarityReg, prescalerReg, filterReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = READ_BIT( timReg->SMCR, TIM_SMCR_ETP | TIM_SMCR_ETPS | TIM_SMCR_ETF );

            if( ( polarityReg | prescalerReg | filterReg ) == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid parameters or timer without external trigger input */
    }

    return ( retState );
}


/**
 * \brief Selects source of external trigger input (ETR).
 *
 * \note  ETR source selection is not available on STM32F4 timers (ETR is always
 *        connected to the ETR pin), the function is kept for compatibility of
 *        the module interface.
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param etrSource [in]: ETR source (not used).
 *
 * \return Always \ref TIM_REQUEST_ERROR (no register is changed).
 */
tim_RequestState_t Tim_Set_EtrSource( tim_PeriphId_t periphId, tim_EtrSource_t etrSource )
{
    (void)periphId;
    (void)etrSource;

    return ( TIM_REQUEST_ERROR );
}


/*------------- STM32H5 functionality (not available on STM32F4) -------------*/

/**
 * \brief Activates PWM dithering.
 *
 * \note  Dithering is not available on STM32F4 timers, the function is kept for
 *        compatibility of the module interface.
 *
 * \param periphId [in]: Timer peripheral identification (not used).
 *
 * \return Always \ref TIM_REQUEST_ERROR (no register is changed).
 */
tim_RequestState_t Tim_Set_DitheringActive( tim_PeriphId_t periphId )
{
    (void)periphId;

    return ( TIM_REQUEST_ERROR );
}


/**
 * \brief De-activates PWM dithering.
 *
 * \note  Dithering is not available on STM32F4 timers, the function is kept for
 *        compatibility of the module interface.
 *
 * \param periphId [in]: Timer peripheral identification (not used).
 *
 * \return Always \ref TIM_REQUEST_ERROR (no register is changed).
 */
tim_RequestState_t Tim_Set_DitheringInactive( tim_PeriphId_t periphId )
{
    (void)periphId;

    return ( TIM_REQUEST_ERROR );
}


/**
 * \brief Activates update interrupt flag remapping (UIF copy in counter bit 31).
 *
 * \note  UIF remapping is not available on STM32F4 timers, the function is kept
 *        for compatibility of the module interface.
 *
 * \param periphId [in]: Timer peripheral identification (not used).
 *
 * \return Always \ref TIM_REQUEST_ERROR (no register is changed).
 */
tim_RequestState_t Tim_Set_UifRemapActive( tim_PeriphId_t periphId )
{
    (void)periphId;

    return ( TIM_REQUEST_ERROR );
}


/**
 * \brief De-activates update interrupt flag remapping.
 *
 * \note  UIF remapping is not available on STM32F4 timers, the function is kept
 *        for compatibility of the module interface.
 *
 * \param periphId [in]: Timer peripheral identification (not used).
 *
 * \return Always \ref TIM_REQUEST_ERROR (no register is changed).
 */
tim_RequestState_t Tim_Set_UifRemapInactive( tim_PeriphId_t periphId )
{
    (void)periphId;

    return ( TIM_REQUEST_ERROR );
}


/**
 * \brief Returns counter value and update (overflow) flag read atomically.
 *
 * \note  UIF remapping is not available on STM32F4 timers, the function is kept
 *        for compatibility of the module interface.
 *
 * \param periphId      [in]: Timer peripheral identification (not used).
 * \param counterValue [out]: Pointer to store counter value (not written).
 * \param overflowFlag [out]: Pointer to store update flag copy (not written).
 *
 * \return Always \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_CounterWithOverflow( tim_PeriphId_t periphId, tim_Counter_t * const counterValue, tim_FlagState_t * const overflowFlag )
{
    (void)periphId;
    (void)counterValue;
    (void)overflowFlag;

    return ( TIM_REQUEST_ERROR );
}


/**
 * \brief Returns activation state of timer peripheral
 *
 * \param periphId     [in]: Timer peripheral identification
 * \param periphState [out]: Actual peripheral activation state
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
tim_RequestState_t Tim_Get_PeriphState( tim_PeriphId_t periphId, tim_FunctionState_t * const periphState )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId    ) &&
        ( TIM_NULL_PTR  != periphState )    )
    {
        uint32_t periphStateRaw = LL_TIM_IsEnabledCounter( tim_PeriphConf[ periphId ].PeriphReg );

        if( 0u != periphStateRaw )
        {
            *periphState = TIM_FUNCTION_ACTIVE;
        }
        else
        {
            *periphState = TIM_FUNCTION_INACTIVE;
        }

        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Configures timer counter clock source.
 *
 * For \ref TIM_CLOCKSOURCE_EXTERNAL_CH_IN (external clock mode 1) the trigger
 * input is selected first, then the slave mode controller is switched to
 * external clock mode. Input channel (polarity, filter) of TIx trigger inputs
 * is not configured by this function.
 *
 * \pre   Timer counter must be disabled (\ref Tim_Get_PeriphState returns
 *        \ref TIM_FUNCTION_INACTIVE); otherwise \ref TIM_REQUEST_ERROR and no
 *        register is changed.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param clockSource   [in]: Timer clock source, value from \ref tim_ClockSource_t.
 * \param triggerSource [in]: Trigger input used as external clock, value from \ref tim_ExtClkSource_t
 *                            (used and validated for \ref TIM_CLOCKSOURCE_EXTERNAL_CH_IN only).
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_ClockSource( tim_PeriphId_t periphId, tim_ClockSource_t clockSource, tim_ExtClkSource_t triggerSource )
{
    tim_RequestState_t  retState    = TIM_REQUEST_ERROR;
    tim_FunctionState_t periphState = TIM_FUNCTION_ACTIVE;

    /*------------------------- Parameters validation ------------------------*/
    if( TIM_PERIPH_CNT > periphId )
    {
        TIM_TypeDef * const timReg = tim_PeriphConf[ periphId ].PeriphReg;

        if( TIM_CLOCKSOURCE_INT_CLK == clockSource )
        {
            retState = TIM_REQUEST_OK;
        }
        else if( TIM_CLOCKSOURCE_EXTERNAL_ETR == clockSource )
        {
            const uint32_t etr2ModeAvailability = IS_TIM_CLOCKSOURCE_ETRMODE2_INSTANCE( timReg );

            if( 0u != etr2ModeAvailability )
            {
                retState = TIM_REQUEST_OK;
            }
            else
            {
                /* External clock mode 2 is not available on this timer */
                retState = TIM_REQUEST_ERROR;
            }
        }
        else if( TIM_CLOCKSOURCE_EXTERNAL_CH_IN == clockSource )
        {
            const uint32_t slaveModeAvailability = IS_TIM_SLAVE_INSTANCE( timReg );

            if( 0u != slaveModeAvailability )
            {
                retState = Tim_Check_TriggerInput( periphId, triggerSource );
            }
            else
            {
                /* External clock mode 1 is not available on this timer */
                retState = TIM_REQUEST_ERROR;
            }
        }
        else
        {
            /* Unknown clock source */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    /*--------------------- Peripheral state validation ----------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Get_PeriphState( periphId, &periphState );

        if( ( TIM_REQUEST_OK        == retState    ) &&
            ( TIM_FUNCTION_INACTIVE == periphState )    )
        {
            retState = TIM_REQUEST_OK;
        }
        else
        {
            /* Clock source must not be changed while counter is running */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        /* Invalid parameters, nothing is written */
    }

    /*------------------------- Trigger input selection ----------------------*/
    if( ( TIM_REQUEST_OK                 == retState    ) &&
        ( TIM_CLOCKSOURCE_EXTERNAL_CH_IN == clockSource )    )
    {
        TIM_TypeDef * const timReg      = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      triggerCode = TIM_BIT_MASK_DECODE_SELECTION_CODE( triggerSource );

        LL_TIM_SetTriggerInput( timReg, triggerCode );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = READ_BIT( timReg->SMCR, TIM_SMCR_TS );

            if( triggerCode == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Trigger input is not used by selected clock source */
    }

    /*------------------------- Clock source selection -----------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg = tim_PeriphConf[ periphId ].PeriphReg;

        /* Configures slave mode selection and external clock enable bits */
        LL_TIM_SetClockSource( timReg, clockSource );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = READ_BIT( timReg->SMCR, TIM_SMCR_SMS | TIM_SMCR_ECE );

            if( (uint32_t)clockSource == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Previous step failed */
    }

    return ( retState );
}


/**
 * \brief Returns timer counter clock source.
 *
 * \note  Slave modes other than external clock mode 1 (reset, gated, trigger)
 *        use internal clock, \ref TIM_CLOCKSOURCE_INT_CLK is returned for them.
 *
 * \param periphId     [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param clockSource [out]: Pointer to store timer clock source. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_ClockSource( tim_PeriphId_t periphId, tim_ClockSource_t * const clockSource )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId    ) &&
        ( TIM_NULL_PTR  != clockSource )    )
    {
        TIM_TypeDef * const timReg        = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      extClkEnabled = LL_TIM_IsEnabledExternalClock( timReg );
        const uint32_t      slaveModeReg  = READ_BIT( timReg->SMCR, TIM_SMCR_SMS );

        if( 0u != extClkEnabled )
        {
            *clockSource = TIM_CLOCKSOURCE_EXTERNAL_ETR;
        }
        else if( LL_TIM_CLOCKSOURCE_EXT_MODE1 == slaveModeReg )
        {
            *clockSource = TIM_CLOCKSOURCE_EXTERNAL_CH_IN;
        }
        else
        {
            *clockSource = TIM_CLOCKSOURCE_INT_CLK;
        }

        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Selects filtered input of timer channel as trigger input (TRGI) of the
 *        slave mode controller (STM32G4 interface function).
 *
 * \ref TIM_CHANNEL_1 selects TI1FP1, \ref TIM_CHANNEL_2 selects TI2FP2. The
 * trigger input may be changed only while the slave mode is disabled, therefore
 * the actual slave mode is disabled during the change and restored afterwards.
 *
 * \pre   Timer must have slave mode controller, counter must be disabled and the
 *        channel must be \ref TIM_CHANNEL_1 or \ref TIM_CHANNEL_2; otherwise
 *        \ref TIM_REQUEST_ERROR and no register is changed.
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId [in]: Timer channel used as trigger input (\ref TIM_CHANNEL_1 or \ref TIM_CHANNEL_2).
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_TriggerSource( tim_PeriphId_t periphId, tim_ChannelId_t channelId )
{
    tim_RequestState_t  retState     = TIM_REQUEST_ERROR;
    tim_FunctionState_t periphState  = TIM_FUNCTION_ACTIVE;
    uint32_t            triggerCode  = LL_TIM_TS_TI1FP1;

    if( ( TIM_PERIPH_CNT > periphId                ) &&
        ( ( TIM_CHANNEL_1 == channelId ) ||
          ( TIM_CHANNEL_2 == channelId )           )    )
    {
        retState = Tim_Check_InputChannel( periphId, channelId );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_SLAVE, TIM_CHANNEL_CNT );
    }
    else
    {
        /* Invalid parameters */
    }

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Get_PeriphState( periphId, &periphState );

        if( ( TIM_REQUEST_OK        == retState    ) &&
            ( TIM_FUNCTION_INACTIVE == periphState )    )
        {
            retState = TIM_REQUEST_OK;
        }
        else
        {
            /* Trigger input must not be changed while counter is running */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        /* Timer without slave mode controller or invalid parameters */
    }

    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg       = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      slaveModeReg = READ_BIT( timReg->SMCR, TIM_SMCR_SMS );

        if( TIM_CHANNEL_1 == channelId )
        {
            triggerCode = LL_TIM_TS_TI1FP1;
        }
        else
        {
            triggerCode = LL_TIM_TS_TI2FP2;
        }

        retState = Tim_Config_SlaveModeReg( periphId, LL_TIM_SLAVEMODE_DISABLED );

        if( TIM_REQUEST_OK == retState )
        {
            LL_TIM_SetTriggerInput( timReg, triggerCode );

            for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
            {
                const uint32_t regValue = READ_BIT( timReg->SMCR, TIM_SMCR_TS );

                if( triggerCode == regValue )
                {
                    retState = TIM_REQUEST_OK;
                    break;
                }
                else
                {
                    /* Register value has not been correctly configured yet */
                    retState = TIM_REQUEST_ERROR;
                }
            }

            /* Previous slave mode is restored also if trigger input write failed */
            const tim_RequestState_t restoreState = Tim_Config_SlaveModeReg( periphId, slaveModeReg );

            if( TIM_REQUEST_OK != restoreState )
            {
                retState = TIM_REQUEST_ERROR;
            }
            else
            {
                /* Slave mode restored */
            }
        }
        else
        {
            /* Slave mode could not be disabled */
        }
    }
    else
    {
        /* Previous step failed, nothing is written */
    }

    return ( retState );
}


/**
 * \brief Sets digital input filter of timer channel (STM32G4 interface function,
 *        same as \ref Tim_Set_InputFilter).
 *
 * \param periphId    [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId   [in]: Timer channel identification, value from \ref tim_ChannelId_t
 *                          (channels 1 - 4 available on the timer).
 * \param inputFilter [in]: Input filter, value from \ref tim_InputFilter_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_InputCaptureFilter( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_InputFilter_t inputFilter )
{
    const tim_RequestState_t retState = Tim_Set_InputFilter( periphId, channelId, inputFilter );

    return ( retState );
}


/**
 * \brief Activates auto-reload preload of timer.
 *
 * With auto-reload preload active, new auto-reload value is applied at the
 * next update event.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_AutoreloadActive( tim_PeriphId_t periphId )
{
    const tim_RequestState_t retState = Tim_Config_ArrPreload( periphId, TIM_FUNCTION_ACTIVE );

    return ( retState );
}


/**
 * \brief De-activates auto-reload preload of timer.
 *
 * With auto-reload preload inactive, new auto-reload value is applied immediately.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_AutoreloadInactive( tim_PeriphId_t periphId )
{
    const tim_RequestState_t retState = Tim_Config_ArrPreload( periphId, TIM_FUNCTION_INACTIVE );

    return ( retState );
}


/**
 * \brief Returns auto-reload preload state of timer.
 *
 * \param periphId   [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param modeState [out]: Pointer to store actual state. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_AutoreloadState( tim_PeriphId_t periphId, tim_FunctionState_t * const modeState )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId  ) &&
        ( TIM_NULL_PTR  != modeState )    )
    {
        const uint32_t periphStateRaw = LL_TIM_IsEnabledARRPreload( tim_PeriphConf[ periphId ].PeriphReg );

        if( 0u != periphStateRaw )
        {
            *modeState = TIM_FUNCTION_ACTIVE;
        }
        else
        {
            *modeState = TIM_FUNCTION_INACTIVE;
        }

        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Activates one pulse mode of timer.
 *
 * In one pulse mode the counter stops (counter enable cleared by hardware) at
 * the next update event.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_OnePulseModeActive( tim_PeriphId_t periphId )
{
    const tim_RequestState_t retState = Tim_Config_OnePulseMode( periphId, TIM_FUNCTION_ACTIVE );

    return ( retState );
}


/**
 * \brief De-activates one pulse mode of timer (counter runs continuously).
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_OnePulseModeInactive( tim_PeriphId_t periphId )
{
    const tim_RequestState_t retState = Tim_Config_OnePulseMode( periphId, TIM_FUNCTION_INACTIVE );

    return ( retState );
}


/**
 * \brief Returns one pulse mode state of timer.
 *
 * \param periphId   [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param modeState [out]: Pointer to store actual state. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_OnePulseMode( tim_PeriphId_t periphId, tim_FunctionState_t * const modeState )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId  ) &&
        ( TIM_NULL_PTR  != modeState )    )
    {
        const uint32_t onePulseModeReg = LL_TIM_GetOnePulseMode( tim_PeriphConf[ periphId ].PeriphReg );

        if( LL_TIM_ONEPULSEMODE_SINGLE == onePulseModeReg )
        {
            *modeState = TIM_FUNCTION_ACTIVE;
        }
        else
        {
            *modeState = TIM_FUNCTION_INACTIVE;
        }

        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Enables timer channel output.
 *
 * \pre   Output must be available on the timer (channel / complementary output
 *        exists); otherwise \ref TIM_REQUEST_ERROR and no register is changed.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param outputId [in]: Timer channel output identification, value from \ref tim_OutputId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_OutputActive( tim_PeriphId_t periphId, tim_OutputId_t outputId )
{
    tim_RequestState_t retState = Tim_Check_OutputId( periphId, outputId );

    if( TIM_REQUEST_OK == retState )
    {
        const uint32_t outputReg = tim_OutputLut[ outputId ].OutputReg;

        LL_TIM_CC_EnableChannel( tim_PeriphConf[ periphId ].PeriphReg, outputReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t outputState = LL_TIM_CC_IsEnabledChannel( tim_PeriphConf[ periphId ].PeriphReg, outputReg );

            if( 0u != outputState )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Channel output has not been enabled yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid peripheral or output not available on the timer */
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Disables timer channel output.
 *
 * \pre   Output must be available on the timer (channel / complementary output
 *        exists); otherwise \ref TIM_REQUEST_ERROR and no register is changed.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param outputId [in]: Timer channel output identification, value from \ref tim_OutputId_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Set_OutputInactive( tim_PeriphId_t periphId, tim_OutputId_t outputId )
{
    tim_RequestState_t retState = Tim_Check_OutputId( periphId, outputId );

    if( TIM_REQUEST_OK == retState )
    {
        const uint32_t outputReg = tim_OutputLut[ outputId ].OutputReg;

        LL_TIM_CC_DisableChannel( tim_PeriphConf[ periphId ].PeriphReg, outputReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t outputState = LL_TIM_CC_IsEnabledChannel( tim_PeriphConf[ periphId ].PeriphReg, outputReg );

            if( 0u == outputState )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Channel output has not been disabled yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid peripheral or output not available on the timer */
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Returns timer channel output state.
 *
 * \param periphId   [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param outputId   [in]: Timer channel output identification, value from \ref tim_OutputId_t.
 * \param modeState [out]: Pointer to store output enable state. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
tim_RequestState_t Tim_Get_OutputState( tim_PeriphId_t periphId, tim_OutputId_t outputId, tim_FunctionState_t * const modeState )
{
    tim_RequestState_t retState = Tim_Check_OutputId( periphId, outputId );

    if( ( TIM_REQUEST_OK == retState  ) &&
        ( TIM_NULL_PTR   != modeState )    )
    {
        const uint32_t outputStateRaw = LL_TIM_CC_IsEnabledChannel( tim_PeriphConf[ periphId ].PeriphReg, tim_OutputLut[ outputId ].OutputReg );

        *modeState = Tim_Conv_RawToFunctionState( outputStateRaw );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}



/*--------------------- GPIO configuration functionality ---------------------*/

/**
 * \brief Channel input/output pin GPIO configuration
 *
 * \param pinId [in] : Identification of required pin.
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
tim_RequestState_t Tim_InitIOPin( tim_IoPin_t pinId )
{
    tim_RequestState_t  retState      = TIM_REQUEST_ERROR;
    gpio_RequestState_t gpioInitState = GPIO_REQUEST_ERROR;
    gpio_Config_t       pinConfig     = { 0u };

    pinConfig.PortId         = TIM_BIT_MASK_DECODE_PORT( pinId );
    pinConfig.PinId          = TIM_BIT_MASK_DECODE_PIN( pinId );
    pinConfig.PinMode        = GPIO_PIN_MODE_ALTERNATE;
    pinConfig.PinPull        = GPIO_PIN_PULL_NONE;
    pinConfig.PinSpeed       = GPIO_PIN_SPEED_MEDIUM;
    pinConfig.PinOutType     = GPIO_PIN_OUTPUT_PUSHPULL;
    pinConfig.PinAltFunction = TIM_BIT_MASK_DECODE_AF( pinId );

    /* Initialize GPIO */
    gpioInitState = Gpio_Init( &pinConfig );

    if( GPIO_REQUEST_ERROR != gpioInitState )
    {
        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Negative channel input/output pin GPIO configuration
 *
 * \param pinId [in] : Identification of required pin.
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
tim_RequestState_t Tim_InitIOComplPin( tim_IOComplPin_t pinId )
{
    tim_RequestState_t  retState      = TIM_REQUEST_ERROR;
    gpio_RequestState_t gpioInitState = GPIO_REQUEST_ERROR;
    gpio_Config_t       pinConfig     = { 0u };

    pinConfig.PortId         = TIM_BIT_MASK_DECODE_PORT( pinId );
    pinConfig.PinId          = TIM_BIT_MASK_DECODE_PIN( pinId );
    pinConfig.PinMode        = GPIO_PIN_MODE_ALTERNATE;
    pinConfig.PinPull        = GPIO_PIN_PULL_NONE;
    pinConfig.PinSpeed       = GPIO_PIN_SPEED_MEDIUM;
    pinConfig.PinOutType     = GPIO_PIN_OUTPUT_PUSHPULL;
    pinConfig.PinAltFunction = TIM_BIT_MASK_DECODE_AF( pinId );

    /* Initialize GPIO */
    gpioInitState = Gpio_Init( &pinConfig );

    if( GPIO_REQUEST_ERROR != gpioInitState )
    {
        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Timer trigger event pin GPIO configuration
 *
 * \param pinId [in] : Identification of required pin.
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
tim_RequestState_t Tim_InitTriggerEventGpio( tim_EtrPin_t pinId )
{
    tim_RequestState_t  retState      = TIM_REQUEST_ERROR;
    gpio_RequestState_t gpioInitState = GPIO_REQUEST_ERROR;
    gpio_Config_t       pinConfig     = { 0u };

    pinConfig.PortId         = TIM_BIT_MASK_DECODE_PORT( pinId );
    pinConfig.PinId          = TIM_BIT_MASK_DECODE_PIN( pinId );
    pinConfig.PinMode        = GPIO_PIN_MODE_ALTERNATE;
    pinConfig.PinPull        = GPIO_PIN_PULL_NONE;
    pinConfig.PinSpeed       = GPIO_PIN_SPEED_MEDIUM;
    pinConfig.PinOutType     = GPIO_PIN_OUTPUT_PUSHPULL;
    pinConfig.PinAltFunction = TIM_BIT_MASK_DECODE_AF( pinId );

    /* Initialize GPIO */
    gpioInitState = Gpio_Init( &pinConfig );

    if( GPIO_REQUEST_ERROR != gpioInitState )
    {
        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Timer break input pin GPIO configuration
 *
 * \param pinId [in] : Identification of required pin.
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
tim_RequestState_t Tim_InitBreakInputGpio( tim_BkinPin_t pinId )
{
    tim_RequestState_t  retState      = TIM_REQUEST_ERROR;
    gpio_RequestState_t gpioInitState = GPIO_REQUEST_ERROR;
    gpio_Config_t       pinConfig     = { 0u };

    pinConfig.PortId         = TIM_BIT_MASK_DECODE_PORT( pinId );
    pinConfig.PinId          = TIM_BIT_MASK_DECODE_PIN( pinId );
    pinConfig.PinMode        = GPIO_PIN_MODE_ALTERNATE;
    pinConfig.PinPull        = GPIO_PIN_PULL_NONE;
    pinConfig.PinSpeed       = GPIO_PIN_SPEED_MEDIUM;
    pinConfig.PinOutType     = GPIO_PIN_OUTPUT_PUSHPULL;
    pinConfig.PinAltFunction = TIM_BIT_MASK_DECODE_AF( pinId );

    /* Initialize GPIO */
    gpioInitState = Gpio_Init( &pinConfig );

    if( GPIO_REQUEST_ERROR != gpioInitState )
    {
        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Timer break input 2 pin GPIO configuration
 *
 * \note  Break input 2 is not available on STM32F4 timers (\ref tim_Bkin2Pin_t
 *        has no pin), the function is kept for compatibility of the module interface.
 *
 * \param pinId [in] : Identification of required pin (not used).
 *
 * \return Always \ref TIM_REQUEST_ERROR (no GPIO is configured).
 */
tim_RequestState_t Tim_InitBreakInput2Gpio( tim_Bkin2Pin_t pinId )
{
    (void)pinId;

    return ( TIM_REQUEST_ERROR );
}


/* =========================== LOCAL FUNCTIONS ============================== */

/**
 * \brief Checks availability of DMA request on timer peripheral.
 *
 * \param periphId   [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param dmaRequest [in]: DMA request identification, value from \ref tim_DmaRequest_t.
 *
 * \return Returns \ref TIM_REQUEST_OK if the timer has DMA and the request is
 *         available, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Check_DmaRequest( tim_PeriphId_t periphId, tim_DmaRequest_t dmaRequest )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT      > periphId   ) &&
        ( TIM_DMA_REQUEST_CNT > dmaRequest )    )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_DMA, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Check_Feature( periphId, tim_DmaRequestLut[ dmaRequest ].Availability, tim_DmaRequestLut[ dmaRequest ].ChannelId );
    }
    else
    {
        /* Invalid parameters or timer without DMA */
    }

    /* Capture / compare requests require the channel on the timer */
    if( ( TIM_REQUEST_OK  == retState                                   ) &&
        ( TIM_CHANNEL_CNT != tim_DmaRequestLut[ dmaRequest ].ChannelId )    )
    {
        retState = Tim_Check_ChannelId( periphId, tim_DmaRequestLut[ dmaRequest ].ChannelId );
    }
    else
    {
        /* Request without channel or previous check failed */
    }

    return ( retState );
}


/**
 * \brief Configures DMA request enable state (DIER UDE / CCxDE / COMDE / TDE).
 *
 * \param periphId     [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param dmaRequest   [in]: DMA request identification, value from \ref tim_DmaRequest_t.
 * \param requestState [in]: Required request state, value from \ref tim_FunctionState_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Config_DmaRequest( tim_PeriphId_t periphId, tim_DmaRequest_t dmaRequest, tim_FunctionState_t requestState )
{
    tim_RequestState_t retState = Tim_Check_DmaRequest( periphId, dmaRequest );

    if( ( TIM_REQUEST_OK       == retState     ) &&
        ( TIM_FUNCTION_ACTIVE >= requestState )    )
    {
        TIM_TypeDef * const timReg = tim_PeriphConf[ periphId ].PeriphReg;

        if( TIM_FUNCTION_ACTIVE == requestState )
        {
            tim_DmaRequestLut[ dmaRequest ].EnableReq( timReg );
        }
        else
        {
            tim_DmaRequestLut[ dmaRequest ].DisableReq( timReg );
        }

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t            requestRaw  = tim_DmaRequestLut[ dmaRequest ].IsEnabledReq( timReg );
            const tim_FunctionState_t requestRead = Tim_Conv_RawToFunctionState( requestRaw );

            if( requestState == requestRead )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}

/**
 * \brief Calculates dead-time generator register value (DTG / DTGF encoding).
 *
 * Dead-time in fDTS periods (rounded) is encoded into one of four ranges:
 * 0 - 127 (step 1), 128 - 254 (step 2), 256 - 504 (step 8), 512 - 1008 (step 16).
 * Values between ranges (255, 505 - 511) are not representable and are rejected.
 *
 * \param periphId  [in]: Timer peripheral identification (valid, checked by caller).
 * \param deadTime  [in]: Required dead-time in ns.
 * \param dtgReg   [out]: Pointer to store encoded dead-time register value. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR (dead-time out of range
 *         or timer clock not available).
 */
static tim_RequestState_t Tim_Calc_DeadTimeReg( tim_PeriphId_t periphId, tim_Time_ns_t deadTime, uint32_t * const dtgReg )
{
    tim_RequestState_t retState        = TIM_REQUEST_ERROR;
    rcc_FreqHz_t       timerPeriphFreq = 0u;
    tim_ClockDiv_t     clockDiv        = TIM_CLOCK_DIV_1;

    const rcc_RequestState_t rccRetState = Rcc_Get_PeriphClk( tim_PeriphConf[ periphId ].PeriphRcc, &timerPeriphFreq );

    if( ( RCC_REQUEST_OK == rccRetState     ) &&
        ( 0u              < timerPeriphFreq ) &&
        ( TIM_NULL_PTR   != dtgReg          )    )
    {
        retState = Tim_Get_ClockDivision( periphId, &clockDiv );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        const uint64_t dtsFreq  = (uint64_t)timerPeriphFreq / tim_ClockDivFactorLut[ clockDiv ];
        const uint64_t dtSteps  = ( ( (uint64_t)deadTime * dtsFreq ) + ( TIM_NS_PER_S / 2u ) ) / TIM_NS_PER_S;

        if( DT_DELAY_1 >= dtSteps )
        {
            *dtgReg = (uint32_t)dtSteps;
        }
        else if( ( ( TIM_DTG_R2_OFFSET + DT_DELAY_2 ) * TIM_DTG_R2_STEP ) >= dtSteps )
        {
            *dtgReg = DT_RANGE_2 | (uint32_t)( ( dtSteps / TIM_DTG_R2_STEP ) - TIM_DTG_R2_OFFSET );
        }
        else if( ( ( TIM_DTG_R3_OFFSET * TIM_DTG_R3_STEP                ) <= dtSteps ) &&
                 ( ( ( TIM_DTG_R3_OFFSET + DT_DELAY_3 ) * TIM_DTG_R3_STEP ) >= dtSteps )    )
        {
            *dtgReg = DT_RANGE_3 | (uint32_t)( ( dtSteps / TIM_DTG_R3_STEP ) - TIM_DTG_R3_OFFSET );
        }
        else if( ( ( TIM_DTG_R4_OFFSET * TIM_DTG_R4_STEP                ) <= dtSteps ) &&
                 ( ( ( TIM_DTG_R4_OFFSET + DT_DELAY_4 ) * TIM_DTG_R4_STEP ) >= dtSteps )    )
        {
            *dtgReg = DT_RANGE_4 | (uint32_t)( ( dtSteps / TIM_DTG_R4_STEP ) - TIM_DTG_R4_OFFSET );
        }
        else
        {
            /* Dead-time between ranges (255 or 505 - 511 fDTS periods) or exceeds
               maximum of dead-time generator */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        /* Timer clock or clock division not available */
    }

    return ( retState );
}


/**
 * \brief Checks break configuration parameters and break feature availability.
 *
 * STM32F4 break input has no digital filter and no bidirectional mode, only
 * \ref TIM_INPUT_FILTER_INACTIVE and \ref TIM_BREAK_MODE_INPUT are valid.
 *
 * \param periphId     [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param breakConfig  [in]: Break configuration. Must not be NULL.
 *
 * \return Returns \ref TIM_REQUEST_OK if parameters are valid and the feature is
 *         available, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Check_BreakConfig( tim_PeriphId_t periphId, const tim_BreakConfig_t * const breakConfig )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_NULL_PTR        != breakConfig                ) &&
        ( TIM_PERIPH_CNT       > periphId                   )    )
    {
        if( ( TIM_FUNCTION_ACTIVE       >= breakConfig->BreakState    ) &&
            ( TIM_POLARITY_LOW          >= breakConfig->BreakPolarity ) &&
            ( TIM_INPUT_FILTER_INACTIVE == breakConfig->BreakFilter   ) &&
            ( TIM_BREAK_MODE_INPUT      == breakConfig->BreakMode     )    )
        {
            retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_BREAK, TIM_CHANNEL_CNT );
        }
        else
        {
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Configures automatic output enable state (AOE bit).
 *
 * \note  Device errata "PWM re-enabled in automatic output enable mode despite of system
 *        break": activation is refused while the clock security system is enabled.
 *
 * \param periphId    [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param outputState [in]: Required automatic output state, value from \ref tim_FunctionState_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Config_AutomaticOutput( tim_PeriphId_t periphId, tim_FunctionState_t outputState )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;
    const uint32_t     cssState = READ_BIT( RCC->CR, RCC_CR_CSSON );

    if( ( TIM_PERIPH_CNT       > periphId    ) &&
        ( TIM_FUNCTION_ACTIVE >= outputState ) &&
        ( ( TIM_FUNCTION_ACTIVE != outputState ) || ( 0u == cssState ) ) )
    {
        /* Device errata: system break (CSS) is not latched with AOE - AOE only without CSS */
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_BREAK, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg = tim_PeriphConf[ periphId ].PeriphReg;

        if( TIM_FUNCTION_ACTIVE == outputState )
        {
            LL_TIM_EnableAutomaticOutput( timReg );
        }
        else
        {
            LL_TIM_DisableAutomaticOutput( timReg );
        }

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t            aoeRaw  = LL_TIM_IsEnabledAutomaticOutput( timReg );
            const tim_FunctionState_t aoeRead = Tim_Conv_RawToFunctionState( aoeRaw );

            if( outputState == aoeRead )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been configured (or is locked) */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid parameters or timer without break feature */
    }

    return ( retState );
}


/**
 * \brief Configures polarity of break input pin (BKIN).
 *
 * STM32F4 timers have single break polarity (BKP), the pin polarity of the
 * timer configuration (BreakInPinPolarity) is therefore the break polarity:
 * \ref TIM_POLARITY_HIGH break is active on high level, \ref TIM_POLARITY_LOW
 * on low level. \ref Tim_Set_BreakConfig overwrites the polarity.
 *
 * \param periphId    [in]: Timer peripheral identification (valid, checked by caller).
 * \param pinPolarity [in]: Pin polarity, value from \ref tim_Polarity_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Config_BreakPinPolarity( tim_PeriphId_t periphId, tim_Polarity_t pinPolarity )
{
    tim_RequestState_t  retState    = TIM_REQUEST_ERROR;
    TIM_TypeDef * const timReg      = tim_PeriphConf[ periphId ].PeriphReg;
    uint32_t            polarityReg = LL_TIM_BREAK_POLARITY_HIGH;

    if( TIM_POLARITY_HIGH == pinPolarity )
    {
        polarityReg = LL_TIM_BREAK_POLARITY_HIGH;
    }
    else
    {
        polarityReg = LL_TIM_BREAK_POLARITY_LOW;
    }

    LL_TIM_ConfigBRK( timReg, polarityReg );

    for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
    {
        const uint32_t regValue = READ_BIT( timReg->BDTR, TIM_BDTR_BKP );

        if( polarityReg == regValue )
        {
            retState = TIM_REQUEST_OK;
            break;
        }
        else
        {
            /* Register value has not been configured (or is locked) */
            retState = TIM_REQUEST_ERROR;
        }
    }

    return ( retState );
}

/**
 * \brief Checks availability of channel with input stage (channels 1 - 4) on timer peripheral.
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId [in]: Timer channel identification, value from \ref tim_ChannelId_t.
 *
 * \return Returns \ref TIM_REQUEST_OK if the channel exists on the timer and has
 *         input stage, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Check_InputChannel( tim_PeriphId_t periphId, tim_ChannelId_t channelId )
{
    tim_RequestState_t retState = Tim_Check_ChannelId( periphId, channelId );

    if( ( TIM_REQUEST_OK        == retState  ) &&
        ( TIM_INPUT_CHANNEL_CNT  > channelId )    )
    {
        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Checks validity of input polarity value.
 *
 * \param inputPolarity [in]: Input polarity, value from \ref tim_InputPolarity_t.
 *
 * \return Returns \ref TIM_REQUEST_OK for a valid polarity, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Check_InputPolarity( tim_InputPolarity_t inputPolarity )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_INPUT_POLARITY_NORMAL     == inputPolarity ) ||
        ( TIM_INPUT_POLARITY_INVERTED   == inputPolarity ) ||
        ( TIM_INPUT_POLARITY_BOTH_EDGES == inputPolarity )    )
    {
        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Configures input stage of timer channel and enables the capture.
 *
 * Capture is disabled first (active input can be changed only while capture
 * is disabled), then active input, prescaler, filter and polarity are written
 * and verified by read-back and capture is enabled.
 *
 * \param periphId       [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId      [in]: Timer channel identification (channels 1 - 4 available on the timer).
 * \param activeInput    [in]: Active input, value from \ref tim_ActiveInput_t.
 * \param inputPolarity  [in]: Input polarity, value from \ref tim_InputPolarity_t.
 * \param inputFilter    [in]: Input filter, value from \ref tim_InputFilter_t.
 * \param inputPrescaler [in]: Capture prescaler, value from \ref tim_InputPrescaler_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Config_InputChannel( tim_PeriphId_t       periphId,
                                                   tim_ChannelId_t      channelId,
                                                   tim_ActiveInput_t    activeInput,
                                                   tim_InputPolarity_t  inputPolarity,
                                                   tim_InputFilter_t    inputFilter,
                                                   tim_InputPrescaler_t inputPrescaler )
{
    tim_RequestState_t retState = Tim_Check_InputChannel( periphId, channelId );

    /*------------------------- Parameters validation ------------------------*/
    if( ( TIM_REQUEST_OK          == retState       ) &&
        ( TIM_ACTIVE_INPUT_CNT     > activeInput    ) &&
        ( TIM_INPUT_FILTER_CNT     > inputFilter    ) &&
        ( TIM_INPUT_PRESCALER_CNT  > inputPrescaler )    )
    {
        retState = Tim_Check_InputPolarity( inputPolarity );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    /*------------------------- Capture disable ------------------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Set_OutputInactive( periphId, tim_ChannelConfig[ channelId ].OutputId );
    }
    else
    {
        /* Invalid parameters, nothing is written */
    }

    /*------------------------- Input stage configuration --------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg       = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      channelLl    = tim_ChannelConfig[ channelId ].ChannelReg;
        const uint32_t      inputReg     = tim_ActiveInputLut[ activeInput ];
        const uint32_t      prescalerReg = tim_InputPrescalerLut[ inputPrescaler ];
        const uint32_t      filterReg    = tim_InputFilterLut[ inputFilter ];
        const uint32_t      polarityReg  = (uint32_t)inputPolarity;

        LL_TIM_IC_Config( timReg, channelLl, ( inputReg | prescalerReg | filterReg | polarityReg ) );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t inputRead     = LL_TIM_IC_GetActiveInput( timReg, channelLl );
            const uint32_t prescalerRead = LL_TIM_IC_GetPrescaler( timReg, channelLl );
            const uint32_t filterRead    = LL_TIM_IC_GetFilter( timReg, channelLl );
            const uint32_t polarityRead  = LL_TIM_IC_GetPolarity( timReg, channelLl );

            if( ( inputReg     == inputRead     ) &&
                ( prescalerReg == prescalerRead ) &&
                ( filterReg    == filterRead    ) &&
                ( polarityReg  == polarityRead  )    )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register values have not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Previous step failed */
    }

    /*------------------------- Capture enable -------------------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Set_OutputActive( periphId, tim_ChannelConfig[ channelId ].OutputId );
    }
    else
    {
        /* Previous step failed */
    }

    return ( retState );
}

/**
 * \brief Configures master / slave mode state (MSM bit).
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param modeState [in]: Required master / slave mode state, value from \ref tim_FunctionState_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR (also for timers
 *         without slave mode controller).
 */
static tim_RequestState_t Tim_Config_MasterSlaveMode( tim_PeriphId_t periphId, tim_FunctionState_t modeState )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT       > periphId  ) &&
        ( TIM_FUNCTION_ACTIVE >= modeState )    )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_SLAVE, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg = tim_PeriphConf[ periphId ].PeriphReg;

        if( TIM_FUNCTION_ACTIVE == modeState )
        {
            LL_TIM_EnableMasterSlaveMode( timReg );
        }
        else
        {
            LL_TIM_DisableMasterSlaveMode( timReg );
        }

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t            modeStateRaw  = LL_TIM_IsEnabledMasterSlaveMode( timReg );
            const tim_FunctionState_t modeStateRead = Tim_Conv_RawToFunctionState( modeStateRaw );

            if( modeState == modeStateRead )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid parameters or timer has no slave mode controller */
    }

    return ( retState );
}


/**
 * \brief Writes slave mode selection (SMS bits) and verifies it by read-back.
 *
 * \param periphId     [in]: Timer peripheral identification (valid, checked by caller).
 * \param slaveModeReg [in]: LL slave mode register value (LL_TIM_SLAVEMODE_xxx).
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Config_SlaveModeReg( tim_PeriphId_t periphId, uint32_t slaveModeReg )
{
    tim_RequestState_t  retState = TIM_REQUEST_ERROR;
    TIM_TypeDef * const timReg   = tim_PeriphConf[ periphId ].PeriphReg;

    LL_TIM_SetSlaveMode( timReg, slaveModeReg );

    for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
    {
        const uint32_t regValue = READ_BIT( timReg->SMCR, TIM_SMCR_SMS );

        if( slaveModeReg == regValue )
        {
            retState = TIM_REQUEST_OK;
            break;
        }
        else
        {
            /* Register value has not been correctly configured yet */
            retState = TIM_REQUEST_ERROR;
        }
    }

    return ( retState );
}

/**
 * \brief Checks availability of channel on timer peripheral.
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId [in]: Timer channel identification, value from \ref tim_ChannelId_t.
 *
 * \return Returns \ref TIM_REQUEST_OK if peripheral and channel identification are
 *         valid and the channel exists on the timer, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Check_ChannelId( tim_PeriphId_t periphId, tim_ChannelId_t channelId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT                          > periphId  ) &&
        ( TIM_CHANNEL_CNT                         > channelId ) &&
        ( tim_PeriphConf[ periphId ].ChannelCount > channelId )    )
    {
        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Checks channel configuration for channel mode configuration functions.
 *
 * \param periphId      [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelConfig [in]: Timer channel configuration. Must not be NULL.
 * \param modeGroup     [in]: Required group of the channel mode, value from \ref tim_ModeGroup_t.
 *
 * \return Returns \ref TIM_REQUEST_OK if the channel exists on the timer and the channel
 *         mode belongs to the required group and is available on STM32F4,
 *         otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Check_ChannelConfig( tim_PeriphId_t periphId, const tim_ChannelConfig_t * const channelConfig, tim_ModeGroup_t modeGroup )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_NULL_PTR != channelConfig )
    {
        retState = Tim_Check_ChannelId( periphId, channelConfig->ChannelId );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( ( TIM_REQUEST_OK       == retState                   ) &&
        ( TIM_CHANNEL_MODE_CNT  > channelConfig->ChannelMode )    )
    {
        const tim_ModeGroup_t channelGroup = tim_ChannelModeConfig[ channelConfig->ChannelMode ].ModeGroup;

        if( modeGroup != channelGroup )
        {
            /* Channel mode belongs to other mode group */
            retState = TIM_REQUEST_ERROR;
        }
        else if( TIM_REG_VAL_UNAVAILABLE == tim_ChannelModeConfig[ channelConfig->ChannelMode ].ModeRegVal )
        {
            /* Channel mode is not available on STM32F4 */
            retState = TIM_REQUEST_ERROR;
        }
        else
        {
            retState = TIM_REQUEST_OK;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Configures output channel: mode, polarity, idle state, compare preload,
 *        complementary output, output enable and main output.
 *
 * Complementary output is enabled with the same polarity when the complementary
 * pin is configured and the channel has complementary output. Idle state and
 * main output (MOE) are configured on timers with break feature only (TIM1 /
 * TIM8), without MOE no signal is generated on these timers.
 *
 * \param periphId      [in]: Timer peripheral identification (valid, checked by caller).
 * \param channelConfig [in]: Timer channel configuration (valid, checked by caller).
 * \param preloadState  [in]: Output compare preload state, value from \ref tim_FunctionState_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Config_OutputChannel( tim_PeriphId_t periphId, const tim_ChannelConfig_t * const channelConfig, tim_FunctionState_t preloadState )
{
    const tim_ChannelId_t channelId   = channelConfig->ChannelId;
    const tim_OutputId_t  outputId    = tim_ChannelConfig[ channelId ].OutputId;
    TIM_TypeDef * const   timReg      = tim_PeriphConf[ periphId ].PeriphReg;
    const uint32_t        breakAvail  = IS_TIM_BREAK_INSTANCE( timReg );
    tim_FlagState_t       complUsed   = TIM_FLAG_INACTIVE;
    tim_RequestState_t    retState    = TIM_REQUEST_ERROR;

    /* Complementary output is used only if its pin is configured */
    if( ( tim_PeriphConf[ periphId ].ComplChannelCount > channelId                 ) &&
        ( TIM_CH_N_PIN_UNUSED                         != channelConfig->IoComplPin )    )
    {
        complUsed = TIM_FLAG_ACTIVE;
    }
    else
    {
        complUsed = TIM_FLAG_INACTIVE;
    }

    /*----------------------- Mode, polarity, preload ------------------------*/
    retState = Tim_Set_ChannelMode( periphId, channelId, channelConfig->ChannelMode );

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Set_OutputPolarity( periphId, outputId, channelConfig->OutputPolarity );
    }
    else
    {
        /* Previous configuration step failed */
    }

    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Config_ComparePreload( periphId, channelId, preloadState );
    }
    else
    {
        /* Previous configuration step failed */
    }

    /*------------------------------ Idle state ------------------------------*/
    if( ( TIM_REQUEST_OK == retState   ) &&
        ( 0u             != breakAvail )    )
    {
        const uint32_t outputReg = tim_ChannelConfig[ channelId ].ChannelOutputReg;
        uint32_t       idleReg   = LL_TIM_OCIDLESTATE_LOW;

        if( TIM_POLARITY_HIGH == channelConfig->IdleState )
        {
            idleReg = LL_TIM_OCIDLESTATE_HIGH;
        }
        else
        {
            idleReg = LL_TIM_OCIDLESTATE_LOW;
        }

        LL_TIM_OC_SetIdleState( timReg, outputReg, idleReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = LL_TIM_OC_GetIdleState( timReg, outputReg );

            if( idleReg == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Idle state is available only on timers with break feature */
    }

    /*------------------------- Complementary output -------------------------*/
    if( ( TIM_REQUEST_OK  == retState  ) &&
        ( TIM_FLAG_ACTIVE == complUsed )    )
    {
        const tim_OutputId_t complOutputId = tim_ChannelConfig[ channelId ].ComplOutputId;

        retState = Tim_Set_OutputPolarity( periphId, complOutputId, channelConfig->OutputPolarity );

        if( TIM_REQUEST_OK == retState )
        {
            retState = Tim_Set_OutputActive( periphId, complOutputId );
        }
        else
        {
            /* Complementary output polarity configuration failed */
        }
    }
    else
    {
        /* Complementary output is not used or previous step failed */
    }

    /*--------------------------- Output enable ------------------------------*/
    if( TIM_REQUEST_OK == retState )
    {
        retState = Tim_Set_OutputActive( periphId, outputId );
    }
    else
    {
        /* Previous configuration step failed */
    }

    if( ( TIM_REQUEST_OK == retState   ) &&
        ( 0u             != breakAvail )    )
    {
        retState = Tim_Config_MainOutput( periphId, TIM_FUNCTION_ACTIVE );
    }
    else
    {
        /* Timer has no main output control or previous step failed */
    }

    return ( retState );
}


/**
 * \brief Configures output compare preload state (OCxPE bit) of timer channel.
 *
 * \param periphId     [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId    [in]: Timer channel identification, value from \ref tim_ChannelId_t.
 * \param preloadState [in]: Required preload state, value from \ref tim_FunctionState_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Config_ComparePreload( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_FunctionState_t preloadState )
{
    tim_RequestState_t retState = Tim_Check_ChannelId( periphId, channelId );

    if( ( TIM_REQUEST_OK       == retState     ) &&
        ( TIM_FUNCTION_ACTIVE >= preloadState )    )
    {
        TIM_TypeDef * const timReg    = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      outputReg = tim_ChannelConfig[ channelId ].ChannelOutputReg;

        if( TIM_FUNCTION_ACTIVE == preloadState )
        {
            LL_TIM_OC_EnablePreload( timReg, outputReg );
        }
        else
        {
            LL_TIM_OC_DisablePreload( timReg, outputReg );
        }

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t            preloadRaw   = LL_TIM_OC_IsEnabledPreload( timReg, outputReg );
            const tim_FunctionState_t preloadRead  = Tim_Conv_RawToFunctionState( preloadRaw );

            if( preloadState == preloadRead )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Configures main output state (MOE bit) of timer with break feature.
 *
 * \param periphId    [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param outputState [in]: Required main output state, value from \ref tim_FunctionState_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR (also for timers without
 *         break feature).
 */
static tim_RequestState_t Tim_Config_MainOutput( tim_PeriphId_t periphId, tim_FunctionState_t outputState )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT       > periphId    ) &&
        ( TIM_FUNCTION_ACTIVE >= outputState )    )
    {
        retState = Tim_Check_Feature( periphId, TIM_FEATURE_AVAIL_BREAK, TIM_CHANNEL_CNT );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    if( TIM_REQUEST_OK == retState )
    {
        TIM_TypeDef * const timReg = tim_PeriphConf[ periphId ].PeriphReg;

        if( TIM_FUNCTION_ACTIVE == outputState )
        {
            LL_TIM_EnableAllOutputs( timReg );
        }
        else
        {
            LL_TIM_DisableAllOutputs( timReg );
        }

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t            mainOutputRaw  = LL_TIM_IsEnabledAllOutputs( timReg );
            const tim_FunctionState_t mainOutputRead = Tim_Conv_RawToFunctionState( mainOutputRaw );

            if( outputState == mainOutputRead )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Main output has not been configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Invalid parameters or timer has no main output control */
    }

    return ( retState );
}

/**
 * \brief Checks availability of timer feature on timer peripheral.
 *
 * Availability is evaluated by CMSIS instance macros of the device, features not
 * existing on STM32F4 timers (\ref TIM_FEATURE_AVAIL_NONE) are never available.
 *
 * \param periphId     [in]: Timer peripheral identification (must be valid, checked by caller).
 * \param availability [in]: Feature availability condition, value from \ref tim_FeatureAvail_t.
 * \param channelId    [in]: Channel identification (\ref TIM_FEATURE_AVAIL_CHANNEL only).
 *
 * \return Returns \ref TIM_REQUEST_OK if the feature is available on the timer,
 *         otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Check_Feature( tim_PeriphId_t periphId, tim_FeatureAvail_t availability, tim_ChannelId_t channelId )
{
    tim_RequestState_t  retState     = TIM_REQUEST_ERROR;
    TIM_TypeDef * const timReg       = tim_PeriphConf[ periphId ].PeriphReg;
    tim_FunctionState_t featureState = TIM_FUNCTION_INACTIVE;

    if( TIM_FEATURE_AVAIL_ALWAYS == availability )
    {
        featureState = TIM_FUNCTION_ACTIVE;
    }
    else if( TIM_FEATURE_AVAIL_CHANNEL == availability )
    {
        if( tim_PeriphConf[ periphId ].ChannelCount > channelId )
        {
            featureState = TIM_FUNCTION_ACTIVE;
        }
        else
        {
            featureState = TIM_FUNCTION_INACTIVE;
        }
    }
    else if( TIM_FEATURE_AVAIL_COMMUTATION == availability )
    {
        featureState = Tim_Conv_RawToFunctionState( IS_TIM_COMMUTATION_EVENT_INSTANCE( timReg ) );
    }
    else if( TIM_FEATURE_AVAIL_SLAVE == availability )
    {
        featureState = Tim_Conv_RawToFunctionState( IS_TIM_SLAVE_INSTANCE( timReg ) );
    }
    else if( TIM_FEATURE_AVAIL_BREAK == availability )
    {
        featureState = Tim_Conv_RawToFunctionState( IS_TIM_BREAK_INSTANCE( timReg ) );
    }
    else if( TIM_FEATURE_AVAIL_ENCODER == availability )
    {
        featureState = Tim_Conv_RawToFunctionState( IS_TIM_ENCODER_INTERFACE_INSTANCE( timReg ) );
    }
    else if( TIM_FEATURE_AVAIL_MASTER == availability )
    {
        featureState = Tim_Conv_RawToFunctionState( IS_TIM_MASTER_INSTANCE( timReg ) );
    }
    else if( TIM_FEATURE_AVAIL_CLOCK_DIV == availability )
    {
        featureState = Tim_Conv_RawToFunctionState( IS_TIM_CLOCK_DIVISION_INSTANCE( timReg ) );
    }
    else if( TIM_FEATURE_AVAIL_HALL == availability )
    {
        featureState = Tim_Conv_RawToFunctionState( IS_TIM_HALL_SENSOR_INTERFACE_INSTANCE( timReg ) );
    }
    else if( TIM_FEATURE_AVAIL_DMA == availability )
    {
        featureState = Tim_Conv_RawToFunctionState( IS_TIM_DMA_INSTANCE( timReg ) );
    }
    else if( TIM_FEATURE_AVAIL_DMA_CC == availability )
    {
        featureState = Tim_Conv_RawToFunctionState( IS_TIM_DMA_CC_INSTANCE( timReg ) );
    }
    else if( TIM_FEATURE_AVAIL_DMA_BURST == availability )
    {
        featureState = Tim_Conv_RawToFunctionState( IS_TIM_DMABURST_INSTANCE( timReg ) );
    }
    else if( TIM_FEATURE_AVAIL_ETR == availability )
    {
        featureState = Tim_Conv_RawToFunctionState( IS_TIM_ETR_INSTANCE( timReg ) );
    }
    else
    {
        /* Feature is not available on STM32F4 timers (TIM_FEATURE_AVAIL_NONE) */
        featureState = TIM_FUNCTION_INACTIVE;
    }

    if( TIM_FUNCTION_ACTIVE == featureState )
    {
        retState = TIM_REQUEST_OK;
    }
    else
    {
        /* Feature is not available on this timer */
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Checks availability of interrupt request / status flag on timer peripheral.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param irqId    [in]: Interrupt request identification, value from \ref tim_IrqId_t.
 *
 * \return Returns \ref TIM_REQUEST_OK if peripheral and interrupt identification are
 *         valid and the interrupt is available on the timer, otherwise returns
 *         \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Check_IrqId( tim_PeriphId_t periphId, tim_IrqId_t irqId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId ) &&
        ( TIM_IRQ_CNT    > irqId    )    )
    {
        retState = Tim_Check_Feature( periphId, tim_IrqLut[ irqId ].Availability, tim_IrqLut[ irqId ].ChannelId );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Returns NVIC interrupt line and module handler of timer interrupt group.
 *
 * Timers with a single (global) interrupt line use it for all interrupt groups.
 *
 * \param periphId   [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param irqLine    [in]: Interrupt group, value from \ref tim_IrqLine_t (except \ref TIM_IRQ_LINE_ALL).
 * \param nvicIrqId [out]: Pointer to store NVIC interrupt line. Must not be NULL.
 * \param nvicIsr   [out]: Pointer to store module interrupt handler of the line. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if the timer has
 *         NVIC line for the group, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Get_NvicLine( tim_PeriphId_t periphId, tim_IrqLine_t irqLine, nvic_PeriphIrqList_t * const nvicIrqId, nvic_IsrCallback_t * const nvicIsr )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT   > periphId  ) &&
        ( TIM_IRQ_LINE_CNT > irqLine   ) &&
        ( TIM_NULL_PTR    != nvicIrqId ) &&
        ( TIM_NULL_PTR    != nvicIsr   )    )
    {
        const tim_IsrConfig_t * const isrConfig = &tim_PeriphConf[ periphId ].IsrConfig;
        nvic_PeriphIrqList_t          lineIrqId = isrConfig->GeneralIrqId;
        nvic_IsrCallback_t            lineIsr   = isrConfig->GeneralIsr;

        if( TIM_IRQ_LINE_UPDATE == irqLine )
        {
            lineIrqId = isrConfig->UpdateIrqId;
            lineIsr   = isrConfig->UpdateIsr;
        }
        else if( TIM_IRQ_LINE_CAPTURE_COMPARE == irqLine )
        {
            lineIrqId = isrConfig->CaptureCompareIrqId;
            lineIsr   = isrConfig->CaptureCompareIsr;
        }
        else if( TIM_IRQ_LINE_TRIGGER_COMM == irqLine )
        {
            lineIrqId = isrConfig->TriggerCommIrqId;
            lineIsr   = isrConfig->TriggerCommIsr;
        }
        else
        {
            lineIrqId = isrConfig->BreakIrqId;
            lineIsr   = isrConfig->BreakIsr;
        }

        if( NVIC_PERIPH_IRQ_SIZE == lineIrqId )
        {
            /* Group has no dedicated line, global timer interrupt is used */
            lineIrqId = isrConfig->GeneralIrqId;
            lineIsr   = isrConfig->GeneralIsr;
        }
        else
        {
            /* Dedicated interrupt line of the group */
        }

        if( NVIC_PERIPH_IRQ_SIZE != lineIrqId )
        {
            *nvicIrqId = lineIrqId;
            *nvicIsr   = lineIsr;
            retState   = TIM_REQUEST_OK;
        }
        else
        {
            /* Timer has no interrupt line */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Processes pending interrupt requests of timer interrupt group.
 *
 * For every interrupt of the group that is enabled and has active status flag
 * \ref Tim_Handle_Irq is called.
 *
 * \param periphId [in]: Timer peripheral identification (constant from module interrupt handler).
 * \param irqLine  [in]: Interrupt group, value from \ref tim_IrqLine_t (\ref TIM_IRQ_LINE_ALL for global line).
 */
static void Tim_Process_Irq( tim_PeriphId_t periphId, tim_IrqLine_t irqLine )
{
    TIM_TypeDef * const timReg = tim_PeriphConf[ periphId ].PeriphReg;

    for( tim_IrqId_t irqId = TIM_IRQ_UPDATE; TIM_IRQ_CNT > irqId; irqId ++ )
    {
        const tim_IrqLine_t entryLine = tim_IrqLut[ irqId ].IrqLine;

        if( TIM_FEATURE_AVAIL_NONE == tim_IrqLut[ irqId ].Availability )
        {
            /* Interrupt does not exist on STM32F4 timers */
        }
        else if( ( TIM_IRQ_LINE_ALL == irqLine   ) ||
                 ( entryLine        == irqLine   )    )
        {
            const uint32_t irqEnabledRaw = tim_IrqLut[ irqId ].IsEnabledIt( timReg );
            const uint32_t flagActiveRaw = tim_IrqLut[ irqId ].IsActiveFlag( timReg );

            if( ( 0u != irqEnabledRaw ) &&
                ( 0u != flagActiveRaw )    )
            {
                Tim_Handle_Irq( periphId, irqId );
            }
            else
            {
                /* Interrupt is not enabled or not pending */
            }
        }
        else
        {
            /* Interrupt belongs to other interrupt group */
        }
    }
}


/**
 * \brief Handles pending timer interrupt: clears status flag(s) and calls user callback.
 *
 * Capture / compare callback receives over-capture state.
 *
 * \param periphId [in]: Timer peripheral identification (valid, from \ref Tim_Process_Irq).
 * \param irqId    [in]: Pending interrupt identification (valid, from \ref Tim_Process_Irq).
 */
static void Tim_Handle_Irq( tim_PeriphId_t periphId, tim_IrqId_t irqId )
{
    TIM_TypeDef * const             timReg  = tim_PeriphConf[ periphId ].PeriphReg;
    const tim_IsrCallback_t * const userIsr = &tim_UserCallbacks[ periphId ];

    if( TIM_FEATURE_AVAIL_CHANNEL == tim_IrqLut[ irqId ].Availability )
    {
        /*------------------------- Capture / compare ------------------------*/
        const tim_ChannelId_t                   channelId   = tim_IrqLut[ irqId ].ChannelId;
        const uint32_t                          ovrFlagRaw  = tim_CcOvrLut[ channelId ].IsActiveFlag( timReg );
        tim_CaptureComapreIsrCallback_t * const callback    = userIsr->CaptureCompareIsr[ channelId ];
        tim_OvercaptureFlag_t                   overcapture = TIM_OVERCAPTURE_INACTIVE;

        if( 0u != ovrFlagRaw )
        {
            overcapture = TIM_OVERCAPTURE_ACTIVE;
            tim_CcOvrLut[ channelId ].ClearFlag( timReg );
        }
        else
        {
            overcapture = TIM_OVERCAPTURE_INACTIVE;
        }

        tim_IrqLut[ irqId ].ClearFlag( timReg );

        if( TIM_NULL_PTR != callback )
        {
            callback( overcapture );
        }
        else
        {
            /* User callback is not registered */
        }
    }
    else
    {
        /*------------------- Events without callback parameter -------------*/
        tim_UpdateIsrCallback_t * callback = userIsr->UpdateIsr;

        if( TIM_IRQ_COMMUTATION == irqId )
        {
            callback = userIsr->CommutationIsr;
        }
        else if( TIM_IRQ_TRIGGER == irqId )
        {
            callback = userIsr->TriggerIsr;
        }
        else if( TIM_IRQ_BREAK == irqId )
        {
            callback = userIsr->BreakIsr;
        }
        else
        {
            /* Update interrupt, callback already selected */
            callback = userIsr->UpdateIsr;
        }

        tim_IrqLut[ irqId ].ClearFlag( timReg );

        if( TIM_NULL_PTR != callback )
        {
            callback();
        }
        else
        {
            /* User callback is not registered */
        }
    }
}


/**
 * \brief Configures update event (UEV) generation state (UDIS bit).
 *
 * \param periphId   [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param eventState [in]: Required update event generation state, value from \ref tim_FunctionState_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Config_UpdateEvent( tim_PeriphId_t periphId, tim_FunctionState_t eventState )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT       > periphId   ) &&
        ( TIM_FUNCTION_ACTIVE >= eventState )    )
    {
        TIM_TypeDef * const timReg = tim_PeriphConf[ periphId ].PeriphReg;

        if( TIM_FUNCTION_ACTIVE == eventState )
        {
            LL_TIM_EnableUpdateEvent( timReg );
        }
        else
        {
            LL_TIM_DisableUpdateEvent( timReg );
        }

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t            updateEventRaw   = LL_TIM_IsEnabledUpdateEvent( timReg );
            const tim_FunctionState_t updateEventState = Tim_Conv_RawToFunctionState( updateEventRaw );

            if( eventState == updateEventState )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Configures auto-reload preload state (ARPE bit).
 *
 * \param periphId     [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param preloadState [in]: Required auto-reload preload state, value from \ref tim_FunctionState_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Config_ArrPreload( tim_PeriphId_t periphId, tim_FunctionState_t preloadState )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT       > periphId     ) &&
        ( TIM_FUNCTION_ACTIVE >= preloadState )    )
    {
        TIM_TypeDef * const timReg = tim_PeriphConf[ periphId ].PeriphReg;

        if( TIM_FUNCTION_ACTIVE == preloadState )
        {
            LL_TIM_EnableARRPreload( timReg );
        }
        else
        {
            LL_TIM_DisableARRPreload( timReg );
        }

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t            arrPreloadRaw   = LL_TIM_IsEnabledARRPreload( timReg );
            const tim_FunctionState_t arrPreloadState = Tim_Conv_RawToFunctionState( arrPreloadRaw );

            if( preloadState == arrPreloadState )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Configures one pulse mode state (OPM bit).
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param modeState [in]: Required one pulse mode state, value from \ref tim_FunctionState_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Config_OnePulseMode( tim_PeriphId_t periphId, tim_FunctionState_t modeState )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT       > periphId  ) &&
        ( TIM_FUNCTION_ACTIVE >= modeState )    )
    {
        TIM_TypeDef * const timReg          = tim_PeriphConf[ periphId ].PeriphReg;
        uint32_t            onePulseModeReg = LL_TIM_ONEPULSEMODE_REPETITIVE;

        if( TIM_FUNCTION_ACTIVE == modeState )
        {
            onePulseModeReg = LL_TIM_ONEPULSEMODE_SINGLE;
        }
        else
        {
            onePulseModeReg = LL_TIM_ONEPULSEMODE_REPETITIVE;
        }

        LL_TIM_SetOnePulseMode( timReg, onePulseModeReg );

        for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
        {
            const uint32_t regValue = LL_TIM_GetOnePulseMode( timReg );

            if( onePulseModeReg == regValue )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Register value has not been correctly configured yet */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Checks availability of software generated event on timer peripheral.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param eventId  [in]: Event identification, value from \ref tim_EventId_t.
 *
 * \return Returns \ref TIM_REQUEST_OK if peripheral and event identification are
 *         valid and the event is available on the timer, otherwise returns
 *         \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Check_EventId( tim_PeriphId_t periphId, tim_EventId_t eventId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId ) &&
        ( TIM_EVENT_CNT  > eventId  )    )
    {
        retState = Tim_Check_Feature( periphId, tim_EventLut[ eventId ].Availability, tim_EventLut[ eventId ].ChannelId );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}

/**
 * \brief Converts raw LL state value (0 / non-zero) to module function state.
 *
 * \param rawState [in]: Raw state returned by LL_TIM_IsEnabledXxx / LL_TIM_IsActiveXxx.
 *
 * \return \ref TIM_FUNCTION_ACTIVE for non-zero raw state, otherwise \ref TIM_FUNCTION_INACTIVE.
 */
static tim_FunctionState_t Tim_Conv_RawToFunctionState( uint32_t rawState )
{
    tim_FunctionState_t retState = TIM_FUNCTION_INACTIVE;

    if( 0u != rawState )
    {
        retState = TIM_FUNCTION_ACTIVE;
    }
    else
    {
        retState = TIM_FUNCTION_INACTIVE;
    }

    return ( retState );
}


/**
 * \brief Configures counter direction (counting mode) of timer.
 *
 * \pre   Timers without counter mode selection (all except TIM1 / TIM2 / TIM3 /
 *        TIM4 / TIM5 / TIM8) support only \ref TIM_COUNTER_DIR_UP; otherwise
 *        \ref TIM_REQUEST_ERROR and no register is changed. Switching between
 *        edge-aligned and center-aligned mode is allowed only while counter is
 *        disabled (ensured by caller).
 *
 * \param periphId   [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param counterDir [in]: Counter direction, value from \ref tim_CounterDir_t.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Config_CounterDirection( tim_PeriphId_t periphId, tim_CounterDir_t counterDir )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT      > periphId   ) &&
        ( TIM_COUNTER_DIR_CNT > counterDir )    )
    {
        TIM_TypeDef * const timReg              = tim_PeriphConf[ periphId ].PeriphReg;
        const uint32_t      modeSelAvailability = IS_TIM_COUNTER_MODE_SELECT_INSTANCE( timReg );

        if( ( 0u                 != modeSelAvailability ) ||
            ( TIM_COUNTER_DIR_UP == counterDir          )    )
        {
            const uint32_t counterModeReg = tim_CounterModeLut[ counterDir ];

            LL_TIM_SetCounterMode( timReg, counterModeReg );

            for( uint32_t iterationCnt = 0u; TIM_TIMEOUT_RAW > iterationCnt; iterationCnt ++ )
            {
                const uint32_t regValue = LL_TIM_GetCounterMode( timReg );

                if( counterModeReg == regValue )
                {
                    retState = TIM_REQUEST_OK;
                    break;
                }
                else
                {
                    /* Register value has not been correctly configured yet */
                    retState = TIM_REQUEST_ERROR;
                }
            }
        }
        else
        {
            /* Timer supports up-counting only */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}

/**
 * \brief Checks a trigger input (slave mode controller / external clock mode 1 input) of the timer.
 *
 * The item must belong to the timer and its SMCR.TS code must be a trigger selection of the device.
 *
 * \param periphId     [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param triggerInput [in]: Trigger input, item of \ref tim_ExtClkSource_t.
 *
 * \return Returns \ref TIM_REQUEST_OK if the item is a trigger input of the timer,
 *         otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Check_TriggerInput( tim_PeriphId_t periphId, tim_ExtClkSource_t triggerInput )
{
    tim_RequestState_t retState   = TIM_REQUEST_ERROR;
    const uint32_t     itemPeriph = TIM_BIT_MASK_DECODE_SELECTION_PERIPH( triggerInput );
    const uint32_t     itemCode   = TIM_BIT_MASK_DECODE_SELECTION_CODE( triggerInput );

    if( ( TIM_PERIPH_CNT      > periphId   ) &&
        ( (uint32_t)periphId == itemPeriph )    )
    {
        for( uint32_t codeIdx = 0u; TIM_TRIGGER_CODE_CNT > codeIdx; codeIdx ++ )
        {
            if( tim_TriggerCodeLut[ codeIdx ] == itemCode )
            {
                retState = TIM_REQUEST_OK;
                break;
            }
            else
            {
                /* Continue with next trigger selection code */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        /* Trigger input of another timer or timer out of range */
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Checks availability of channel output on timer peripheral.
 *
 * \param periphId [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param outputId [in]: Timer channel output identification, value from \ref tim_OutputId_t.
 *
 * \return Returns \ref TIM_REQUEST_OK if peripheral and output identification are
 *         valid and the output exists on the timer, otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Check_OutputId( tim_PeriphId_t periphId, tim_OutputId_t outputId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_PERIPH_CNT > periphId ) &&
        ( TIM_OUTPUT_CNT > outputId )    )
    {
        const tim_ChannelId_t channelId = tim_OutputLut[ outputId ].ChannelId;

        if( TIM_FLAG_ACTIVE == tim_OutputLut[ outputId ].ComplOutput )
        {
            if( tim_PeriphConf[ periphId ].ComplChannelCount > channelId )
            {
                retState = TIM_REQUEST_OK;
            }
            else
            {
                /* Channel has no complementary output on this timer */
                retState = TIM_REQUEST_ERROR;
            }
        }
        else
        {
            if( tim_PeriphConf[ periphId ].ChannelCount > channelId )
            {
                retState = TIM_REQUEST_OK;
            }
            else
            {
                /* Channel is not available on this timer */
                retState = TIM_REQUEST_ERROR;
            }
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Returns input remap configuration of timer channel.
 *
 * \param periphId  [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId [in]: Timer channel identification, value from \ref tim_ChannelId_t.
 *
 * \return Pointer to input remap configuration of the channel, NULL if the
 *         channel has no input remap (or identifications are invalid).
 */
static const tim_InputRemapConfig_t * Tim_Get_InputRemap( tim_PeriphId_t periphId, tim_ChannelId_t channelId )
{
    const tim_InputRemapConfig_t * remapConfig = TIM_NULL_PTR;

    if( TIM_PERIPH_CNT > periphId )
    {
        for( uint32_t remapIdx = 0u; TIM_INPUT_REMAP_CNT > remapIdx; remapIdx ++ )
        {
            if( ( tim_PeriphConf[ periphId ].PeriphReg == tim_InputRemapLut[ remapIdx ].PeriphReg ) &&
                ( channelId                            == tim_InputRemapLut[ remapIdx ].ChannelId )    )
            {
                remapConfig = &tim_InputRemapLut[ remapIdx ];
                break;
            }
            else
            {
                /* Continue with next remap configuration */
            }
        }
    }
    else
    {
        /* Invalid peripheral identification */
    }

    return ( remapConfig );
}


/**
 * \brief Checks an input source of a timer channel with input remap.
 *
 * The item must belong to the timer and the channel and its selection code must be available on the device.
 *
 * \param periphId    [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param channelId   [in]: Timer channel identification, value from \ref tim_ChannelId_t.
 * \param inputSource [in]: Input source, item of \ref tim_InputSource_t.
 *
 * \return Returns \ref TIM_REQUEST_OK if the item is an input source of the timer channel,
 *         otherwise returns \ref TIM_REQUEST_ERROR.
 */
static tim_RequestState_t Tim_Check_InputSource( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_InputSource_t inputSource )
{
    tim_RequestState_t                   retState    = TIM_REQUEST_ERROR;
    const tim_InputRemapConfig_t * const remapConfig = Tim_Get_InputRemap( periphId, channelId );
    const uint32_t                       itemPeriph  = TIM_BIT_MASK_DECODE_SELECTION_PERIPH( inputSource );
    const uint32_t                       itemChannel = TIM_BIT_MASK_DECODE_INPUT_SOURCE_CHANNEL( inputSource );
    const uint32_t                       itemCode    = TIM_BIT_MASK_DECODE_INPUT_SOURCE_CODE( inputSource );

    if( ( TIM_NULL_PTR              != remapConfig ) &&
        ( (uint32_t)periphId        == itemPeriph  ) &&
        ( (uint32_t)channelId       == itemChannel ) &&
        ( TIM_INPUT_REMAP_SOURCE_CNT > itemCode    )    )
    {
        if( TIM_REG_VAL_UNAVAILABLE != remapConfig->SelVal[ itemCode ] )
        {
            retState = TIM_REQUEST_OK;
        }
        else
        {
            /* Selection code is not available on the device */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        /* Input source of another timer / channel or channel without input remap */
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Calculates duration of one counter step for required clock divider.
 *
 * Step time is calculated from the actual timer kernel clock and rounded to
 * the nearest nanosecond.
 *
 * \param periphId   [in]: Timer peripheral identification, value from \ref tim_PeriphId_t.
 * \param clkDivider [in]: Timer kernel clock divider (\ref TIM_PRESCALER_MIN - \ref TIM_PRESCALER_MAX + 1).
 * \param stepTime  [out]: Pointer to store counter step time in ns. Must not be NULL.
 *
 * \return State of request execution. Returns \ref TIM_REQUEST_OK if request was
 *         success, otherwise returns \ref TIM_REQUEST_ERROR (also if step time
 *         exceeds \ref TIM_TIME_NS_MAX).
 */
static tim_RequestState_t Tim_Calc_StepTime( tim_PeriphId_t periphId, uint32_t clkDivider, tim_Time_ns_t * const stepTime )
{
    tim_RequestState_t retState        = TIM_REQUEST_ERROR;
    rcc_FreqHz_t       timerPeriphFreq = 0u;

    if( ( TIM_PERIPH_CNT > periphId   ) &&
        ( TIM_NULL_PTR  != stepTime   )    )
    {
        const rcc_RequestState_t rccRetState = Rcc_Get_PeriphClk( tim_PeriphConf[ periphId ].PeriphRcc, &timerPeriphFreq );

        if( ( RCC_REQUEST_OK == rccRetState     ) &&
            ( 0u              < timerPeriphFreq )    )
        {
            /* Rounded step time: divider / f = divider * 1e9 / f [ns] */
            const uint64_t stepTimeNs = ( ( (uint64_t)clkDivider * TIM_NS_PER_S ) + ( timerPeriphFreq / 2u ) ) / timerPeriphFreq;

            if( TIM_TIME_NS_MAX >= stepTimeNs )
            {
                *stepTime = (tim_Time_ns_t)stepTimeNs;
                retState  = TIM_REQUEST_OK;
            }
            else
            {
                /* Step time is not representable in tim_Time_ns_t */
                retState = TIM_REQUEST_ERROR;
            }
        }
        else
        {
            /* Timer clock is not available */
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}

/* ============================ ISR FUNCTIONS =============================== */

/*------------------ Timer 1 Interrupt Service Routines ---------------------*/
#ifdef TIM1
/**
 * \brief TIM1 break and TIM9 global interrupt service routine (shared NVIC line).
 */
static void Tim_Tim1_Break_IsrHandler( void )
{
    Tim_Process_Irq( TIM_PERIPH_1, TIM_IRQ_LINE_BREAK );
#ifdef TIM9
    Tim_Process_Irq( TIM_PERIPH_9, TIM_IRQ_LINE_ALL );
#endif
}


/**
 * \brief TIM1 update and TIM10 global interrupt service routine (shared NVIC line).
 */
static void Tim_Tim1_Update_IsrHandler( void )
{
    Tim_Process_Irq( TIM_PERIPH_1, TIM_IRQ_LINE_UPDATE );
#ifdef TIM10
    Tim_Process_Irq( TIM_PERIPH_10, TIM_IRQ_LINE_ALL );
#endif
}


/**
 * \brief TIM1 trigger and commutation and TIM11 global interrupt service routine (shared NVIC line).
 */
static void Tim_Tim1_Trigger_IsrHandler( void )
{
    Tim_Process_Irq( TIM_PERIPH_1, TIM_IRQ_LINE_TRIGGER_COMM );
#ifdef TIM11
    Tim_Process_Irq( TIM_PERIPH_11, TIM_IRQ_LINE_ALL );
#endif
}


/**
 * \brief TIM1 capture / compare interrupt service routine.
 */
static void Tim_Tim1_CaptureCompare_IsrHandler( void )
{
    Tim_Process_Irq( TIM_PERIPH_1, TIM_IRQ_LINE_CAPTURE_COMPARE );
}
#endif


/*------------------ Timer 8 Interrupt Service Routines ---------------------*/
#ifdef TIM8
/**
 * \brief TIM8 break and TIM12 global interrupt service routine (shared NVIC line).
 */
static void Tim_Tim8_Break_IsrHandler( void )
{
    Tim_Process_Irq( TIM_PERIPH_8, TIM_IRQ_LINE_BREAK );
#ifdef TIM12
    Tim_Process_Irq( TIM_PERIPH_12, TIM_IRQ_LINE_ALL );
#endif
}


/**
 * \brief TIM8 update and TIM13 global interrupt service routine (shared NVIC line).
 */
static void Tim_Tim8_Update_IsrHandler( void )
{
    Tim_Process_Irq( TIM_PERIPH_8, TIM_IRQ_LINE_UPDATE );
#ifdef TIM13
    Tim_Process_Irq( TIM_PERIPH_13, TIM_IRQ_LINE_ALL );
#endif
}


/**
 * \brief TIM8 trigger and commutation and TIM14 global interrupt service routine (shared NVIC line).
 */
static void Tim_Tim8_Trigger_IsrHandler( void )
{
    Tim_Process_Irq( TIM_PERIPH_8, TIM_IRQ_LINE_TRIGGER_COMM );
#ifdef TIM14
    Tim_Process_Irq( TIM_PERIPH_14, TIM_IRQ_LINE_ALL );
#endif
}


/**
 * \brief TIM8 capture / compare interrupt service routine.
 */
static void Tim_Tim8_CaptureCompare_IsrHandler( void )
{
    Tim_Process_Irq( TIM_PERIPH_8, TIM_IRQ_LINE_CAPTURE_COMPARE );
}
#endif


/*------------------ Timer 2 Interrupt Service Routines ---------------------*/
#ifdef TIM2
/**
 * \brief TIM2 global interrupt service routine (all interrupt groups).
 */
static void Tim_Tim2_General_IsrHandler( void )
{
    Tim_Process_Irq( TIM_PERIPH_2, TIM_IRQ_LINE_ALL );
}
#endif


/*------------------ Timer 3 Interrupt Service Routines ---------------------*/
#ifdef TIM3
/**
 * \brief TIM3 global interrupt service routine (all interrupt groups).
 */
static void Tim_Tim3_General_IsrHandler( void )
{
    Tim_Process_Irq( TIM_PERIPH_3, TIM_IRQ_LINE_ALL );
}
#endif


/*------------------ Timer 4 Interrupt Service Routines ---------------------*/
#ifdef TIM4
/**
 * \brief TIM4 global interrupt service routine (all interrupt groups).
 */
static void Tim_Tim4_General_IsrHandler( void )
{
    Tim_Process_Irq( TIM_PERIPH_4, TIM_IRQ_LINE_ALL );
}
#endif


/*------------------ Timer 5 Interrupt Service Routines ---------------------*/
#ifdef TIM5
/**
 * \brief TIM5 global interrupt service routine (all interrupt groups).
 */
static void Tim_Tim5_General_IsrHandler( void )
{
    Tim_Process_Irq( TIM_PERIPH_5, TIM_IRQ_LINE_ALL );
}
#endif


/*------------------ Timer 6 Interrupt Service Routines ---------------------*/
#ifdef TIM6
/**
 * \brief TIM6 global interrupt service routine (all interrupt groups).
 */
static void Tim_Tim6_General_IsrHandler( void )
{
    Tim_Process_Irq( TIM_PERIPH_6, TIM_IRQ_LINE_ALL );
}
#endif


/*------------------ Timer 7 Interrupt Service Routines ---------------------*/
#ifdef TIM7
/**
 * \brief TIM7 global interrupt service routine (all interrupt groups).
 */
static void Tim_Tim7_General_IsrHandler( void )
{
    Tim_Process_Irq( TIM_PERIPH_7, TIM_IRQ_LINE_ALL );
}
#endif


/* =========================== INTERRUPT HANDLERS =========================== */

/* ================================ TASKS =================================== */
