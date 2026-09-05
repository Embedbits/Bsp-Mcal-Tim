/**
 * \author Mr.Nobody
 * \file Tim.h
 * \ingroup Tim
 * \brief Tim module common functionality
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
/* ============================== TYPEDEFS ================================== */

typedef struct
{
    tim_ChannelId_t ChannelId; /**< Channel identification */
    uint32_t        ChannelReg;
    uint32_t        ChannelOutputReg;
    uint32_t        ChannelComplOutputReg;
}   tim_ChannelConfigStruct_t;


/** \brief Configuration structure of Interrupt Service Routines */
typedef struct
{
    tim_ErrIsrCallback_t            *ErrorIsr;            /**< Error interrupt user callback pointer        */
    tim_UpdateIsrCallback_t         *UpdateIsr;           /**< Update interrupt user callback pointer       */
    tim_CaptureComapreIsrCallback_t *CaptureCompareIsr;   /**< Capture&Compare user callback pointer        */
    tim_TriggerIsrCallback_t        *TriggerIsr;          /**< Trigger interrupt user callback pointer      */
    tim_CommutationIsrCallback_t    *CommutationIsr;      /**< Commutation interrupt user callback pointer  */
    tim_BreakIsrCallback_t          *BreakIsr;            /**< Break interrupt user callback pointer        */
    tim_Break2IsrCallback_t         *Break2Isr;           /**< Break 2 interrupt user callback pointer      */
    tim_SystemBreakIsrCallback_t    *SysBreakIsr;         /**< System break interrupt user callback pointer */
    tim_DirectionIsrCallback_t      *DirectionIsr;        /**< Direction interrupt user callback pointer    */
    tim_IndexIsrCallback_t          *IndexIsr;            /**< Index interrupt user callback pointer        */
}   tim_UserIsr_t;


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
    TIM_TypeDef     *PeriphReg;    /**< Timer configuration register           */
    rcc_PeriphId_t   PeriphRcc;    /**< Timer RCC configuration ID             */
    tim_Resolution_t Resolution;   /**< Timer resolution in bits               */
    tim_ChannelCnt_t ChannelCount; /**< Count of available channels for timer  */
    tim_IsrConfig_t  IsrConfig;    /**< Configuration of module internal ISR's */
}   tim_ConfigStruct_t;


typedef struct
{
    tim_UpdateIsrCallback_t         *UpdateIsr;
    tim_CaptureComapreIsrCallback_t *CaptureCompareIsr[ TIM_CHANNEL_CNT ];
    tim_CommutationIsrCallback_t    *CommutationIsr;
    tim_TriggerIsrCallback_t        *TriggerIsr;
    tim_IndexIsrCallback_t          *IndexIsr;
    tim_DirectionIsrCallback_t      *DirectionIsr;
    tim_BreakIsrCallback_t          *BreakIsr;
    tim_Break2IsrCallback_t         *Break2Isr;
    tim_SystemBreakIsrCallback_t    *SysBreakIsr;
    tim_ErrIsrCallback_t            *ErrorIsr;
}   tim_IsrCallback_t;


typedef struct
{
    tim_ChannelMode_t ChannelModeId;
    uint32_t          ModeRegVal;
}   tim_ChannelModeConfig_t;

/* ======================== FORWARD DECLARATIONS ============================ */


static void Tim_Tim1_Break_Tim15_General_IsrHandler( void );
static void Tim_Tim1_Update_Tim16_General_IsrHandler( void );
static void Tim_Tim1_Trigger_Tim17_General_IsrHandler( void );

#ifdef TIM1
static void Tim_Tim1_General_IsrHandler( void );
static void Tim_Tim1_Break_IsrHandler( void );
static void Tim_Tim1_Update_IsrHandler( void );
static void Tim_Tim1_Trigger_IsrHandler( void );
static void Tim_Tim1_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM2
static void Tim_Tim2_General_IsrHandler( void );
static void Tim_Tim2_Break_IsrHandler( void );
static void Tim_Tim2_Update_IsrHandler( void );
static void Tim_Tim2_Trigger_IsrHandler( void );
static void Tim_Tim2_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM3
static void Tim_Tim3_General_IsrHandler( void );
static void Tim_Tim3_Break_IsrHandler( void );
static void Tim_Tim3_Update_IsrHandler( void );
static void Tim_Tim3_Trigger_IsrHandler( void );
static void Tim_Tim3_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM4
static void Tim_Tim4_General_IsrHandler( void );
static void Tim_Tim4_Break_IsrHandler( void );
static void Tim_Tim4_Update_IsrHandler( void );
static void Tim_Tim4_Trigger_IsrHandler( void );
static void Tim_Tim4_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM5
static void Tim_Tim5_General_IsrHandler( void );
static void Tim_Tim5_Break_IsrHandler( void );
static void Tim_Tim5_Update_IsrHandler( void );
static void Tim_Tim5_Trigger_IsrHandler( void );
static void Tim_Tim5_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM6
static void Tim_Tim6_General_IsrHandler( void );
static void Tim_Tim6_Break_IsrHandler( void );
static void Tim_Tim6_Update_IsrHandler( void );
static void Tim_Tim6_Trigger_IsrHandler( void );
static void Tim_Tim6_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM7
static void Tim_Tim7_General_IsrHandler( void );
static void Tim_Tim7_Break_IsrHandler( void );
static void Tim_Tim7_Update_IsrHandler( void );
static void Tim_Tim7_Trigger_IsrHandler( void );
static void Tim_Tim7_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM8
static void Tim_Tim8_General_IsrHandler( void );
static void Tim_Tim8_Break_IsrHandler( void );
static void Tim_Tim8_Update_IsrHandler( void );
static void Tim_Tim8_Trigger_IsrHandler( void );
static void Tim_Tim8_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM9
static void Tim_Tim9_General_IsrHandler( void );
static void Tim_Tim9_Break_IsrHandler( void );
static void Tim_Tim9_Update_IsrHandler( void );
static void Tim_Tim9_Trigger_IsrHandler( void );
static void Tim_Tim9_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM10
static void Tim_Tim10_General_IsrHandler( void );
static void Tim_Tim10_Break_IsrHandler( void );
static void Tim_Tim10_Update_IsrHandler( void );
static void Tim_Tim10_Trigger_IsrHandler( void );
static void Tim_Tim10_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM11
static void Tim_Tim11_General_IsrHandler( void );
static void Tim_Tim11_Break_IsrHandler( void );
static void Tim_Tim11_Update_IsrHandler( void );
static void Tim_Tim11_Trigger_IsrHandler( void );
static void Tim_Tim11_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM12
static void Tim_Tim12_General_IsrHandler( void );
static void Tim_Tim12_Break_IsrHandler( void );
static void Tim_Tim12_Update_IsrHandler( void );
static void Tim_Tim12_Trigger_IsrHandler( void );
static void Tim_Tim12_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM13
static void Tim_Tim13_General_IsrHandler( void );
static void Tim_Tim13_Break_IsrHandler( void );
static void Tim_Tim13_Update_IsrHandler( void );
static void Tim_Tim13_Trigger_IsrHandler( void );
static void Tim_Tim13_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM14
static void Tim_Tim14_General_IsrHandler( void );
static void Tim_Tim14_Break_IsrHandler( void );
static void Tim_Tim14_Update_IsrHandler( void );
static void Tim_Tim14_Trigger_IsrHandler( void );
static void Tim_Tim14_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM15
static void Tim_Tim15_General_IsrHandler( void );
static void Tim_Tim15_Break_IsrHandler( void );
static void Tim_Tim15_Update_IsrHandler( void );
static void Tim_Tim15_Trigger_IsrHandler( void );
static void Tim_Tim15_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM16
static void Tim_Tim16_General_IsrHandler( void );
static void Tim_Tim16_Break_IsrHandler( void );
static void Tim_Tim16_Update_IsrHandler( void );
static void Tim_Tim16_Trigger_IsrHandler( void );
static void Tim_Tim16_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM17
static void Tim_Tim17_General_IsrHandler( void );
static void Tim_Tim17_Break_IsrHandler( void );
static void Tim_Tim17_Update_IsrHandler( void );
static void Tim_Tim17_Trigger_IsrHandler( void );
static void Tim_Tim17_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM18
static void Tim_Tim18_General_IsrHandler( void );
static void Tim_Tim18_Break_IsrHandler( void );
static void Tim_Tim18_Update_IsrHandler( void );
static void Tim_Tim18_Trigger_IsrHandler( void );
static void Tim_Tim18_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM19
static void Tim_Tim19_General_IsrHandler( void );
static void Tim_Tim19_Break_IsrHandler( void );
static void Tim_Tim19_Update_IsrHandler( void );
static void Tim_Tim19_Trigger_IsrHandler( void );
static void Tim_Tim19_CapatureCompare_IsrHandler( void );
#endif

#ifdef TIM20
static void Tim_Tim20_General_IsrHandler( void );
static void Tim_Tim20_Break_IsrHandler( void );
static void Tim_Tim20_Update_IsrHandler( void );
static void Tim_Tim20_Trigger_IsrHandler( void );
static void Tim_Tim20_CapatureCompare_IsrHandler( void );
#endif

/* ========================== SYMBOLIC CONSTANTS ============================ */

/** Value of major version of SW module */
#define TIM_MAJOR_VERSION           ( 1u )

/** Value of minor version of SW module */
#define TIM_MINOR_VERSION           ( 0u )

/** Value of patch version of SW module */
#define TIM_PATCH_VERSION           ( 0u )


/** Maximum wait time for configuration request confirmation */
#define TIM_TIMEOUT_RAW             ( 0x84FCB )

/** Minimum value of timer prescaler register */
#define TIM_PRESCALER_MIN           ( 1u )

/** Maximum value of timer prescaler register */
#define TIM_PRESCALER_MAX           ( 65535u )

/* =============================== MACROS =================================== */

/* ========================== EXPORTED VARIABLES ============================ */

/* =========================== LOCAL VARIABLES ============================== */


/** USART/UART peripherals configuration array */
static tim_ConfigStruct_t const         tim_PeriphConf[ TIM_PERIPH_CNT ] =
{
#ifdef TIM1
  { .PeriphReg = TIM1, .PeriphRcc = RCC_PERIPH_TIM1, .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 6u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim1_General_IsrHandler              , .GeneralIrqId        = NVIC_PERIPH_IRQ_SIZE,
                   .BreakIsr          = Tim_Tim1_Break_Tim15_General_IsrHandler  , .BreakIrqId          = NVIC_PERIPH_IRQ_TIM1_BRK,
                   .UpdateIsr         = Tim_Tim1_Update_Tim16_General_IsrHandler , .UpdateIrqId         = NVIC_PERIPH_IRQ_TIM1_UP,
                   .TriggerCommIsr    = Tim_Tim1_Trigger_Tim17_General_IsrHandler, .TriggerCommIrqId    = NVIC_PERIPH_IRQ_TIM1_TRG_COM,
                   .CaptureCompareIsr = Tim_Tim1_CapatureCompare_IsrHandler      , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_TIM1_CC  } },
#endif
#ifdef TIM2
  { .PeriphReg = TIM2, .PeriphRcc = RCC_PERIPH_TIM2, .Resolution = TIM_RESOLUTION_32BIT, .ChannelCount = 4u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim2_General_IsrHandler              , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM2,
                   .BreakIsr          = Tim_Tim2_Break_IsrHandler                , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = Tim_Tim2_Update_IsrHandler               , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = Tim_Tim2_Trigger_IsrHandler              , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = Tim_Tim2_CapatureCompare_IsrHandler      , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM3
  { .PeriphReg = TIM3 , .PeriphRcc = RCC_PERIPH_TIM3 , .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 4u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim3_General_IsrHandler              , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM3,
                   .BreakIsr          = Tim_Tim3_Break_IsrHandler                , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = Tim_Tim3_Update_IsrHandler               , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = Tim_Tim3_Trigger_IsrHandler              , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = Tim_Tim3_CapatureCompare_IsrHandler      , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM4
  { .PeriphReg = TIM4 , .PeriphRcc = RCC_PERIPH_TIM4 , .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 4u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim4_General_IsrHandler              , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM4,
                   .BreakIsr          = Tim_Tim4_Break_IsrHandler                , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = Tim_Tim4_Update_IsrHandler               , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = Tim_Tim4_Trigger_IsrHandler              , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = Tim_Tim4_CapatureCompare_IsrHandler      , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM5
  { .PeriphReg = TIM5 , .PeriphRcc = RCC_PERIPH_TIM5 , .Resolution = TIM_RESOLUTION_32BIT, .ChannelCount = 4u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim5_General_IsrHandler              , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM5,
                   .BreakIsr          = Tim_Tim5_Break_IsrHandler                , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = Tim_Tim5_Update_IsrHandler               , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = Tim_Tim5_Trigger_IsrHandler              , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = Tim_Tim5_CapatureCompare_IsrHandler      , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM6
  { .PeriphReg = TIM6 , .PeriphRcc = RCC_PERIPH_TIM6 , .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 1u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim6_General_IsrHandler              , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM6,
                   .BreakIsr          = Tim_Tim6_Break_IsrHandler                , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = Tim_Tim6_Update_IsrHandler               , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = Tim_Tim6_Trigger_IsrHandler              , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = Tim_Tim6_CapatureCompare_IsrHandler      , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM7
  { .PeriphReg = TIM7 , .PeriphRcc = RCC_PERIPH_TIM7 , .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 1u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim7_General_IsrHandler              , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM7,
                   .BreakIsr          = Tim_Tim7_Break_IsrHandler                , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = Tim_Tim7_Update_IsrHandler               , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = Tim_Tim7_Trigger_IsrHandler              , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = Tim_Tim7_CapatureCompare_IsrHandler      , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM8
  { .PeriphReg = TIM8 , .PeriphRcc = RCC_PERIPH_TIM8 , .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 6u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim8_General_IsrHandler              , .GeneralIrqId        = NVIC_PERIPH_IRQ_SIZE,
                   .BreakIsr          = Tim_Tim8_Break_IsrHandler                , .BreakIrqId          = NVIC_PERIPH_IRQ_TIM8_BRK,
                   .UpdateIsr         = Tim_Tim8_Update_IsrHandler               , .UpdateIrqId         = NVIC_PERIPH_IRQ_TIM8_UP,
                   .TriggerCommIsr    = Tim_Tim8_Trigger_IsrHandler              , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_TIM8_TRG_COM,
                   .CaptureCompareIsr = Tim_Tim8_CapatureCompare_IsrHandler      , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_TIM8_CC  } },
#endif
#ifdef TIM9
  { .PeriphReg = TIM9 , .PeriphRcc = RCC_PERIPH_TIM9 , .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim9_General_IsrHandler              , .GeneralIrqId        = NVIC_PERIPH_IRQ_SIZE,
                   .BreakIsr          = Tim_Tim9_Break_IsrHandler                , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = Tim_Tim9_Update_IsrHandler               , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = Tim_Tim9_Trigger_IsrHandler              , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = Tim_Tim9_CapatureCompare_IsrHandler      , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM10
  { .PeriphReg = TIM10, .PeriphRcc = RCC_PERIPH_TIM10, .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim10_General_IsrHandler             , .GeneralIrqId        = NVIC_PERIPH_IRQ_SIZE,
                   .BreakIsr          = Tim_Tim10_Break_IsrHandler               , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = Tim_Tim10_Update_IsrHandler              , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = Tim_Tim10_Trigger_IsrHandler             , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = Tim_Tim10_CapatureCompare_IsrHandler     , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM11
  { .PeriphReg = TIM11, .PeriphRcc = RCC_PERIPH_TIM11, .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim11_General_IsrHandler             , .GeneralIrqId        = NVIC_PERIPH_IRQ_SIZE,
                   .BreakIsr          = Tim_Tim11_Break_IsrHandler               , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = Tim_Tim11_Update_IsrHandler              , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = Tim_Tim11_Trigger_IsrHandler             , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = Tim_Tim11_CapatureCompare_IsrHandler     , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM12
  { .PeriphReg = TIM12, .PeriphRcc = RCC_PERIPH_TIM12, .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim12_General_IsrHandler             , .GeneralIrqId        = NVIC_PERIPH_IRQ_SIZE,
                   .BreakIsr          = Tim_Tim12_Break_IsrHandler               , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = Tim_Tim12_Update_IsrHandler              , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = Tim_Tim12_Trigger_IsrHandler             , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = Tim_Tim12_CapatureCompare_IsrHandler     , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM13
  { .PeriphReg = TIM13, .PeriphRcc = RCC_PERIPH_TIM13, .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim13_General_IsrHandler             , .GeneralIrqId        = NVIC_PERIPH_IRQ_SIZE,
                   .BreakIsr          = Tim_Tim13_Break_IsrHandler               , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = Tim_Tim13_Update_IsrHandler              , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = Tim_Tim13_Trigger_IsrHandler             , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = Tim_Tim13_CapatureCompare_IsrHandler     , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM14
  { .PeriphReg = TIM14, .PeriphRcc = RCC_PERIPH_TIM14, .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim14_General_IsrHandler             , .GeneralIrqId        = NVIC_PERIPH_IRQ_SIZE,
                   .BreakIsr          = Tim_Tim14_Break_IsrHandler               , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = Tim_Tim14_Update_IsrHandler              , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = Tim_Tim14_Trigger_IsrHandler             , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = Tim_Tim14_CapatureCompare_IsrHandler     , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM15
  { .PeriphReg = TIM15, .PeriphRcc = RCC_PERIPH_TIM15, .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 6u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim1_Break_Tim15_General_IsrHandler  , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM15,
                   .BreakIsr          = Tim_Tim15_Break_IsrHandler               , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = Tim_Tim15_Update_IsrHandler              , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = Tim_Tim15_Trigger_IsrHandler             , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = Tim_Tim15_CapatureCompare_IsrHandler     , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM16
  { .PeriphReg = TIM16, .PeriphRcc = RCC_PERIPH_TIM16, .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 6u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim1_Update_Tim16_General_IsrHandler , .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM16,
                   .BreakIsr          = Tim_Tim16_Break_IsrHandler               , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = Tim_Tim16_Update_IsrHandler              , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = Tim_Tim16_Trigger_IsrHandler             , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = Tim_Tim16_CapatureCompare_IsrHandler     , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM17
  { .PeriphReg = TIM17, .PeriphRcc = RCC_PERIPH_TIM17, .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 6u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim1_Trigger_Tim17_General_IsrHandler, .GeneralIrqId        = NVIC_PERIPH_IRQ_TIM17,
                   .BreakIsr          = Tim_Tim17_Break_IsrHandler               , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = Tim_Tim17_Update_IsrHandler              , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = Tim_Tim17_Trigger_IsrHandler             , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = Tim_Tim17_CapatureCompare_IsrHandler     , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM18
  { .PeriphReg = TIM18, .PeriphRcc = RCC_PERIPH_TIM18, .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim18_General_IsrHandler             , .GeneralIrqId        = NVIC_PERIPH_IRQ_SIZE,
                   .BreakIsr          = Tim_Tim18_Break_IsrHandler               , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = Tim_Tim18_Update_IsrHandler              , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = Tim_Tim18_Trigger_IsrHandler             , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = Tim_Tim18_CapatureCompare_IsrHandler     , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM19
  { .PeriphReg = TIM19, .PeriphRcc = RCC_PERIPH_TIM19, .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 0u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim19_General_IsrHandler             , .GeneralIrqId        = NVIC_PERIPH_IRQ_SIZE,
                   .BreakIsr          = Tim_Tim19_Break_IsrHandler               , .BreakIrqId          = NVIC_PERIPH_IRQ_SIZE,
                   .UpdateIsr         = Tim_Tim19_Update_IsrHandler              , .UpdateIrqId         = NVIC_PERIPH_IRQ_SIZE,
                   .TriggerCommIsr    = Tim_Tim19_Trigger_IsrHandler             , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_SIZE,
                   .CaptureCompareIsr = Tim_Tim19_CapatureCompare_IsrHandler     , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_SIZE  } },
#endif
#ifdef TIM20
  { .PeriphReg = TIM20, .PeriphRcc = RCC_PERIPH_TIM20, .Resolution = TIM_RESOLUTION_16BIT, .ChannelCount = 6u,
    .IsrConfig = { .GeneralIsr        = Tim_Tim20_General_IsrHandler             , .GeneralIrqId        = NVIC_PERIPH_IRQ_SIZE,
                   .BreakIsr          = Tim_Tim20_Break_IsrHandler               , .BreakIrqId          = NVIC_PERIPH_IRQ_TIM20_BRK,
                   .UpdateIsr         = Tim_Tim20_Update_IsrHandler              , .UpdateIrqId         = NVIC_PERIPH_IRQ_TIM20_UP,
                   .TriggerCommIsr    = Tim_Tim20_Trigger_IsrHandler             , .TriggerCommIrqId    = NVIC_PERIPH_IRQ_TIM20_TRG_COM,
                   .CaptureCompareIsr = Tim_Tim20_CapatureCompare_IsrHandler     , .CaptureCompareIrqId = NVIC_PERIPH_IRQ_TIM20_CC  } },
#endif
};


static const tim_ChannelModeConfig_t    tim_ChannelModeConfig[ TIM_CHANNEL_MODE_CNT ] =
{
    { .ChannelModeId = TIM_CHANNEL_MODE_INPUT_CAPTURE                  , .ModeRegVal = 0u },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_FORCED_ACTIVE           , .ModeRegVal = LL_TIM_OCMODE_FORCED_ACTIVE    },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_FORCED_INACTIVE         , .ModeRegVal = LL_TIM_OCMODE_FORCED_INACTIVE  },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_COMPARE_PULSE           , .ModeRegVal = LL_TIM_OCMODE_PULSE_ON_COMPARE },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_COMPARE_FORCED_ACTIVE   , .ModeRegVal = LL_TIM_OCMODE_ACTIVE           },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_COMPARE_FORCED_INACTIVE , .ModeRegVal = LL_TIM_OCMODE_INACTIVE         },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_COMPARE_TOGGLE          , .ModeRegVal = LL_TIM_OCMODE_TOGGLE           },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_PWM                     , .ModeRegVal = LL_TIM_OCMODE_PWM1             },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_PWM_INV                 , .ModeRegVal = LL_TIM_OCMODE_PWM2             },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_COMBINED_PWM            , .ModeRegVal = LL_TIM_OCMODE_COMBINED_PWM1    },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_COMBINED_PWM_INV        , .ModeRegVal = LL_TIM_OCMODE_COMBINED_PWM2    },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_ASSYMETRIC_PWM          , .ModeRegVal = LL_TIM_OCMODE_ASSYMETRIC_PWM1  },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_ASSYMETRIC_PWM_INV      , .ModeRegVal = LL_TIM_OCMODE_ASSYMETRIC_PWM2  },
    { .ChannelModeId = TIM_CHANNEL_MODE_OUTPUT_DIRECTION               , .ModeRegVal = LL_TIM_OCMODE_DIRECTION_OUTPUT },
};


static const tim_ChannelConfigStruct_t  tim_ChannelConfig[ TIM_CHANNEL_CNT ] =
{
    { .ChannelId = TIM_CHANNEL_1, .ChannelReg = LL_TIM_CHANNEL_CH1, .ChannelOutputReg = LL_TIM_CHANNEL_CH1, .ChannelComplOutputReg = LL_TIM_CHANNEL_CH1N },
    { .ChannelId = TIM_CHANNEL_2, .ChannelReg = LL_TIM_CHANNEL_CH2, .ChannelOutputReg = LL_TIM_CHANNEL_CH2, .ChannelComplOutputReg = LL_TIM_CHANNEL_CH2N },
    { .ChannelId = TIM_CHANNEL_3, .ChannelReg = LL_TIM_CHANNEL_CH3, .ChannelOutputReg = LL_TIM_CHANNEL_CH3, .ChannelComplOutputReg = LL_TIM_CHANNEL_CH3N },
    { .ChannelId = TIM_CHANNEL_4, .ChannelReg = LL_TIM_CHANNEL_CH4, .ChannelOutputReg = LL_TIM_CHANNEL_CH4, .ChannelComplOutputReg = LL_TIM_CHANNEL_CH4N },
    { .ChannelId = TIM_CHANNEL_5, .ChannelReg = LL_TIM_CHANNEL_CH5, .ChannelOutputReg = 0u                , .ChannelComplOutputReg = 0u                  },
    { .ChannelId = TIM_CHANNEL_6, .ChannelReg = LL_TIM_CHANNEL_CH6, .ChannelOutputReg = 0u                , .ChannelComplOutputReg = 0u                  },
};

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
 * \param timConfig [in]: Pointer to structure containing timer peripheral configuration
 * \return Request processing status
 */
tim_RequestState_t Tim_Init( tim_PeriphConfig_t * const timConfig )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_NULL_PTR != timConfig )
    {
        rcc_FunctionState_t rccActivationState = RCC_FUNCTION_INACTIVE;
        rcc_RequestState_t  rccRequestState    = RCC_REQUEST_ERROR;

        /*---------------- Peripheral clock activation section ---------------*/
        rccRequestState = Rcc_Get_PeriphState( tim_PeriphConf[ timConfig->PeriphId ].PeriphRcc, &rccActivationState );

        if( ( RCC_REQUEST_ERROR     != rccRequestState    ) &&
            ( RCC_FUNCTION_INACTIVE == rccActivationState )    )
        {
            rccRequestState = Rcc_Set_PeriphActive( tim_PeriphConf[ timConfig->PeriphId ].PeriphRcc );

            if( RCC_REQUEST_ERROR == rccRequestState )
            {
                retState = TIM_REQUEST_ERROR;
            }
        }
        else
        {
            /* Clock is already active. */
        }

        if( TIM_REQUEST_ERROR != retState )
        {
            tim_FunctionState_t timActivationState = TIM_FUNCTION_INACTIVE;

            retState = Tim_Get_PeriphState( timConfig->PeriphId, &timActivationState );

            if( TIM_REQUEST_ERROR != retState )
            {
                if( TIM_FUNCTION_ACTIVE == timActivationState )
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
        }
        else
        {
            /* Error during initialization process */
        }

        /*---------------- Timer base initialization section -----------------*/

        if( TIM_REQUEST_ERROR != retState )
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

        /* Loop through all channels in configurations */
        for( tim_ChannelId_t timerChannelId = 0u; timerChannelsCount > timerChannelId; timerChannelId ++ )
        {
            if( TIM_FUNCTION_INACTIVE != timConfig->ChannelConfig[ timerChannelId ].ChannelState )
            {
                Tim_InitChannel( timConfig->PeriphId, &timConfig->ChannelConfig[ timerChannelId ] );
            }
            else
            {
                continue;
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
 * \brief De-initializes module Tim
 *
 * This function shall call every necessary sub-module de-initialization function
 * and free all the resources allocated by the module. In case of failure, the
 * function shall handle it by itself and shall not be transferred to AppMain
 * layer.
 */
tim_RequestState_t Tim_Deinit( tim_PeriphConfig_t * const timConfig )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_NULL_PTR != timConfig )
    {

    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Main task of module Tim
 *
 * This function shall be called in the main loop of the application or the task
 * scheduler. It shall be called periodically, depending on the module's
 * requirements.
 */
void Tim_Task( void )
{

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
        timConfig->SlaveMode                = TIM_SLAVE_MODE_DISABLE;
        timConfig->TimerFrequency           = 10000000u;
        timConfig->AutoreloadPreloadState   = TIM_FUNCTION_INACTIVE;
        timConfig->UpdateEventState         = TIM_FUNCTION_INACTIVE;
        timConfig->CounterDirection         = TIM_COUNTER_DIR_UP;

        timConfig->RefreshFrequency         = 1000000u;
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
        }
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
 * \param periphId [in] : Timer peripheral identification.
 * \param reqFreq  [in] : Value required frequency in Hz.
 * \param trueFreq [out]: Real value of timer frequency in Hz.
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
tim_RequestState_t Tim_Set_ClkInternal( tim_PeriphId_t periphId, tim_FreqHz_t reqFreq, tim_FreqHz_t * const trueFreq )
{
    tim_RequestState_t retState        = TIM_REQUEST_OK;
    rcc_FreqHz_t       timerPeriphFreq = 0u;

    rcc_RequestState_t rccRetState = Rcc_Get_PeriphClk( tim_PeriphConf[ periphId ].PeriphRcc, &timerPeriphFreq );

    uint32_t prescalerValue = __LL_TIM_CALC_PSC( timerPeriphFreq, reqFreq );

    if( ( RCC_REQUEST_ERROR == rccRetState    ) ||
        ( 0u                == prescalerValue )    )
    {
        retState = TIM_REQUEST_ERROR;

        if( TIM_NULL_PTR != trueFreq )
        {
            *trueFreq = 0u;
        }
        else
        {
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        if( TIM_NULL_PTR != trueFreq )
        {
            *trueFreq = timerPeriphFreq / prescalerValue;
        }
        else
        {
            retState = TIM_REQUEST_ERROR;
        }

        LL_TIM_SetPrescaler( tim_PeriphConf[ periphId ].PeriphReg, prescalerValue );
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
    tim_RequestState_t retState        = TIM_REQUEST_OK;
    rcc_FreqHz_t       timerPeriphFreq = 0u;

    if( TIM_NULL_PTR != timFreq )
    {
        rcc_RequestState_t rccRetState = Rcc_Get_PeriphClk( tim_PeriphConf[ periphId ].PeriphRcc, &timerPeriphFreq );

        if( RCC_REQUEST_ERROR != rccRetState )
        {
            uint32_t prescaler = LL_TIM_GetPrescaler( tim_PeriphConf[ periphId ].PeriphReg );

            *timFreq = ( timerPeriphFreq / ( prescaler + 1u ) );

            retState = TIM_REQUEST_OK;
        }
        else
        {

        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Timer base initialization function.
 *
 * \param timConfig [in]: Pointer to structure containing timer peripheral configuration
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
tim_RequestState_t Tim_InitBase( tim_PeriphConfig_t * const timConfig )
{
    tim_RequestState_t retState        = TIM_REQUEST_ERROR;
    tim_FreqHz_t       timerFrequency  = 0u;
    uint32_t           autoreloadValue = 0u;

    if( TIM_NULL_PTR != timConfig )
    {
        retState = Tim_Set_ClkInternal( timConfig->PeriphId, timConfig->TimerFrequency, &timerFrequency );

        if( TIM_REQUEST_ERROR != retState )
        {
            if( TIM_FUNCTION_INACTIVE == timConfig->ArrPreloadState )
            {
                LL_TIM_DisableARRPreload( tim_PeriphConf[ timConfig->PeriphId ].PeriphReg );
            }
            else
            {
                LL_TIM_EnableARRPreload( tim_PeriphConf[ timConfig->PeriphId ].PeriphReg );
            }
        }
        else
        {
            /* Error during initialization process */
        }

        if( TIM_REQUEST_ERROR != retState )
        {
            if( 0u != timConfig->RefreshFrequency )
            {
                autoreloadValue = ( timerFrequency / timConfig->RefreshFrequency ) - 1u;

                if( tim_PeriphConf[ timConfig->PeriphId ].Resolution < autoreloadValue )
                {
                    autoreloadValue = 0u;
                }
                else
                {
                    /* Auto-reload value is in correct range */
                }
            }
            else
            {
                autoreloadValue = 0u;
            }

            LL_TIM_SetAutoReload( tim_PeriphConf[ timConfig->PeriphId ].PeriphReg, autoreloadValue );
        }
        else
        {
            /* Error during initialization process */
        }

        /*------------- Peripheral GPIO initialization section ---------------*/

        const tim_BkinPin_t breakInPin = timConfig->BreakInPin;

        if( ( TIM_BIT_MASK_DECODE_PERIPH( breakInPin )  == timConfig->PeriphId ) &&
            ( TIM_BKIN_PIN_UNUSED                       != breakInPin          )    )
        {
            Tim_InitBreakInputGpio( breakInPin );
        }
        else
        {
            /* Break Input pin configuration is not used */
        }

        const tim_Bkin2Pin_t breakIn2Pin = timConfig->BreakIn2Pin;

        if( ( TIM_BIT_MASK_DECODE_PERIPH( breakIn2Pin ) == timConfig->PeriphId ) &&
            ( TIM_BKIN2_PIN_UNUSED                      != breakIn2Pin         )    )
        {
            Tim_InitBreakInput2Gpio( breakIn2Pin );
        }
        else
        {
            /* Break input 2 pin configuration is not used */
        }

        const tim_EtrPin_t triggerEventPin = timConfig->TriggerEventPin;

        if( ( TIM_BIT_MASK_DECODE_PERIPH( triggerEventPin ) == timConfig->PeriphId ) &&
            ( TIM_ETR_PIN_UNUSED                            != triggerEventPin     )    )
        {
            Tim_InitTriggerEventGpio( triggerEventPin );
        }
        else
        {
            /* Trigger Event pin configuration is not used */
        }
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


tim_RequestState_t Tim_InitChannel( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_NULL_PTR != channelConfig )
    {
        /*------------- Channel GPIO initialization section --------------*/
        const tim_IoPin_t channelPin = channelConfig->IoPin;

        if( ( TIM_BIT_MASK_DECODE_PERIPH( channelPin )  == periphId                 ) &&
            ( TIM_BIT_MASK_DECODE_CHANNEL( channelPin ) == channelConfig->ChannelId ) &&
            ( TIM_CH_PIN_UNUSED                         != channelPin               )    )
        {
            Tim_InitIOPin( channelPin );
        }
        else
        {
            /* Channel pin configuration is not used */
        }


        const tim_IOComplPin_t channelNPin = channelConfig->IoComplPin;

        if( ( TIM_BIT_MASK_DECODE_PERIPH( channelNPin )  == periphId                 ) &&
            ( TIM_BIT_MASK_DECODE_CHANNEL( channelNPin ) == channelConfig->ChannelId ) &&
            ( TIM_CH_N_PIN_UNUSED                        != channelNPin              )    )
        {
            Tim_InitIOComplPin( channelNPin );
        }
        else
        {
            /* Channel negative pin configuration is not used */
        }


        if( ( TIM_CHANNEL_MODE_OUTPUT_PWM                == channelConfig->ChannelMode ) ||
            ( TIM_CHANNEL_MODE_OUTPUT_PWM_INV            == channelConfig->ChannelMode ) ||
            ( TIM_CHANNEL_MODE_OUTPUT_COMBINED_PWM       == channelConfig->ChannelMode ) ||
            ( TIM_CHANNEL_MODE_OUTPUT_COMBINED_PWM_INV   == channelConfig->ChannelMode ) ||
            ( TIM_CHANNEL_MODE_OUTPUT_ASSYMETRIC_PWM     == channelConfig->ChannelMode ) ||
            ( TIM_CHANNEL_MODE_OUTPUT_ASSYMETRIC_PWM_INV == channelConfig->ChannelMode )    )
        {
            Tim_Set_Mode_Pwm( periphId, channelConfig );
        }

    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


tim_RequestState_t Tim_Set_Mode_InputCapture( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    (void)periphId;
    (void)channelConfig;

    return ( retState );
}


tim_RequestState_t Tim_Set_Mode_Encoder( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    (void)periphId;
    (void)channelConfig;

    return ( retState );
}


tim_RequestState_t Tim_Set_Mode_HalSensor( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    (void)periphId;
    (void)channelConfig;

    return ( retState );
}


/** \brief Configures timer channel in PWM mode.
 *
 * \param periphId      [in]: Timer peripheral identification
 * \param channelConfig [in]: Pointer to structure containing channel configuration
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
tim_RequestState_t Tim_Set_Mode_Pwm( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( ( TIM_NULL_PTR  != channelConfig ) &&
        ( TIM_PERIPH_CNT > periphId      )    )
    {
        if( ( TIM_CHANNEL_MODE_OUTPUT_PWM                == channelConfig->ChannelMode ) ||
            ( TIM_CHANNEL_MODE_OUTPUT_PWM_INV            == channelConfig->ChannelMode ) ||
            ( TIM_CHANNEL_MODE_OUTPUT_COMBINED_PWM       == channelConfig->ChannelMode ) ||
            ( TIM_CHANNEL_MODE_OUTPUT_COMBINED_PWM_INV   == channelConfig->ChannelMode ) ||
            ( TIM_CHANNEL_MODE_OUTPUT_ASSYMETRIC_PWM     == channelConfig->ChannelMode ) ||
            ( TIM_CHANNEL_MODE_OUTPUT_ASSYMETRIC_PWM_INV == channelConfig->ChannelMode )    )
        {
            /* Configure channel mode */
            LL_TIM_OC_SetMode( tim_PeriphConf[ periphId ].PeriphReg,
                               tim_ChannelConfig[ channelConfig->ChannelId ].ChannelOutputReg,
                               tim_ChannelModeConfig[ channelConfig->ChannelMode ].ModeRegVal );

            /* Set output state */
            if( TIM_POLARITY_HIGH == channelConfig->OutputPolarity )
            {
                LL_TIM_OC_SetPolarity( tim_PeriphConf[ periphId ].PeriphReg,
                                       tim_ChannelConfig[ channelConfig->ChannelId ].ChannelOutputReg,
                                       LL_TIM_OCPOLARITY_HIGH );
            }
            else
            {
                LL_TIM_OC_SetPolarity( tim_PeriphConf[ periphId ].PeriphReg,
                                       tim_ChannelConfig[ channelConfig->ChannelId ].ChannelOutputReg,
                                       LL_TIM_OCPOLARITY_LOW );
            }

            /* Set idle state */
            if( TIM_POLARITY_HIGH == channelConfig->IdleState )
            {
                LL_TIM_OC_SetIdleState( tim_PeriphConf[ periphId ].PeriphReg,
                                        tim_ChannelConfig[ channelConfig->ChannelId ].ChannelOutputReg,
                                        LL_TIM_OCIDLESTATE_HIGH );
            }
            else
            {
                LL_TIM_OC_SetIdleState( tim_PeriphConf[ periphId ].PeriphReg,
                                        tim_ChannelConfig[ channelConfig->ChannelId ].ChannelOutputReg,
                                        LL_TIM_OCIDLESTATE_LOW );
            }

            LL_TIM_CC_EnableChannel( tim_PeriphConf[ periphId ].PeriphReg,
                                     tim_ChannelConfig[ channelConfig->ChannelId ].ChannelOutputReg );

            Tim_Set_PwmMode_DutyCycle( periphId, channelConfig->ChannelId, 0u );

            /* Force update generation */
            LL_TIM_GenerateEvent_UPDATE( tim_PeriphConf[ periphId ].PeriphReg );

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


tim_RequestState_t Tim_Set_Mode_ForcedOutput( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    (void)periphId;
    (void)channelConfig;

    return ( retState );
}


tim_RequestState_t Tim_Set_Mode_OutputCompare( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    (void)periphId;
    (void)channelConfig;

    return ( retState );
}


/**
 * \brief Activates required timer peripheral
 *
 * \param periphId [in]: Timer peripheral identification
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
tim_RequestState_t Tim_Set_PeriphActive( tim_PeriphId_t periphId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_PERIPH_CNT > periphId )
    {
        LL_TIM_EnableCounter( tim_PeriphConf[ periphId ].PeriphReg );

        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief De-activates required timer peripheral
 *
 * \param periphId [in]: Timer peripheral identification
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
tim_RequestState_t Tim_Set_PeriphInactive( tim_PeriphId_t periphId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_PERIPH_CNT > periphId )
    {
        LL_TIM_DisableCounter( tim_PeriphConf[ periphId ].PeriphReg );

        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


tim_RequestState_t Tim_Set_PwmMode_DutyCycle( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_CentiPercent_t dutyCycle )
{
    tim_RequestState_t retState      = TIM_REQUEST_OK;
    uint32_t           pulseDuration = 0u;
    uint32_t           period        = 0u;

    /* PWM signal period is determined by the value of the auto-reload register */
    period = LL_TIM_GetAutoReload( tim_PeriphConf[ periphId ].PeriphReg ) + 1u;

    /* Pulse duration is determined by the value of the compare register.  */
    /* Its value is calculated in order to match the requested duty cycle. */
    pulseDuration = ( dutyCycle * period ) / 10000u;

    if( TIM_CHANNEL_1 == channelId )
    {
        LL_TIM_OC_SetCompareCH1( tim_PeriphConf[ periphId ].PeriphReg, pulseDuration );
    }
    else if( TIM_CHANNEL_2 == channelId )
    {
        LL_TIM_OC_SetCompareCH2( tim_PeriphConf[ periphId ].PeriphReg, pulseDuration );
    }
    else if( TIM_CHANNEL_3 == channelId )
    {
        LL_TIM_OC_SetCompareCH3( tim_PeriphConf[ periphId ].PeriphReg, pulseDuration );
    }
    else if( TIM_CHANNEL_4 == channelId )
    {
        LL_TIM_OC_SetCompareCH4( tim_PeriphConf[ periphId ].PeriphReg, pulseDuration );
    }
    else if( TIM_CHANNEL_5 == channelId )
    {
        LL_TIM_OC_SetCompareCH5( tim_PeriphConf[ periphId ].PeriphReg, pulseDuration );
    }
    else if( TIM_CHANNEL_6 == channelId )
    {
        LL_TIM_OC_SetCompareCH6( tim_PeriphConf[ periphId ].PeriphReg, pulseDuration );
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


tim_RequestState_t Tim_Get_PwmMode_DutyCycle( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_CentiPercent_t * const dutyCycle )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    (void)periphId;
    (void)channelId;
    (void)dutyCycle;

    return ( retState );
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

    if( TIM_PERIPH_CNT > periphId )
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


tim_RequestState_t Tim_Set_ClockSource( tim_PeriphId_t periphId, tim_ClockSource_t clockSource, tim_ExtClkSource_t triggerSource )
{
    tim_RequestState_t retState = TIM_REQUEST_OK;
// TODO: Add other external clock options configuration
    LL_TIM_SetClockSource( tim_PeriphConf[ periphId ].PeriphReg, clockSource );

    if( TIM_CLOCKSOURCE_EXTERNAL_ETR == clockSource )
    {
        uint32_t etr2ModeAvailability = IS_TIM_CLOCKSOURCE_ETRMODE2_INSTANCE( tim_PeriphConf[ periphId ].PeriphReg );

        if( 0u != etr2ModeAvailability )
        {
            LL_TIM_EnableExternalClock( tim_PeriphConf[ periphId ].PeriphReg );
        }
        else
        {
            retState = TIM_REQUEST_ERROR;
        }
    }
    else
    {
        LL_TIM_DisableExternalClock( tim_PeriphConf[ periphId ].PeriphReg );
    }

    if( TIM_CLOCKSOURCE_EXTERNAL_CH_IN == clockSource )
    {
        LL_TIM_SetTriggerInput( tim_PeriphConf[ periphId ].PeriphReg, triggerSource );
    }
    else
    {

    }

    return ( retState );
}


tim_RequestState_t Tim_Get_ClockSource( tim_PeriphId_t periphId, tim_ClockSource_t * const clockSource )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    (void)periphId;
    (void)clockSource;

    return ( retState );
}


tim_RequestState_t Tim_Set_AutoreloadActive( tim_PeriphId_t periphId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

//    LL_TIM_CC_EnablePreload();

    (void)periphId;

    return ( retState );
}


tim_RequestState_t Tim_Set_AutoreloadInactive( tim_PeriphId_t periphId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    (void)periphId;

    return ( retState );
}


tim_RequestState_t Tim_Get_AutoreloadState( tim_PeriphId_t periphId, tim_FunctionState_t * const modeState )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_PERIPH_CNT > periphId )
    {
        uint32_t periphStateRaw = LL_TIM_IsEnabledCounter( tim_PeriphConf[ periphId ].PeriphReg );

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


tim_RequestState_t Tim_Set_OnePulseModeActive( tim_PeriphId_t periphId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    (void)periphId;

    return ( retState );
}


tim_RequestState_t Tim_Set_OnePulseModeInactive( tim_PeriphId_t periphId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    (void)periphId;

    return ( retState );
}


tim_RequestState_t Tim_Get_OnePulseMode( tim_PeriphId_t periphId, tim_FunctionState_t * const modeState )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    (void)periphId;
    (void)modeState;

    return ( retState );
}


tim_RequestState_t Tim_Set_OutputActive( tim_PeriphId_t periphId, tim_OutputId_t channelId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    if( TIM_PERIPH_CNT > periphId )
    {
        LL_TIM_CC_EnableChannel( tim_PeriphConf[ periphId ].PeriphReg, channelId );

        retState = TIM_REQUEST_OK;
    }
    else
    {
        retState = TIM_REQUEST_ERROR;
    }

    return ( retState );
}


tim_RequestState_t Tim_Set_OutputInactive( tim_PeriphId_t periphId, tim_OutputId_t channelId )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    (void)periphId;
    (void)channelId;

    return ( retState );
}


tim_RequestState_t Tim_Get_OutputState( tim_PeriphId_t periphId, tim_OutputId_t channelId, tim_FunctionState_t * const modeState )
{
    tim_RequestState_t retState = TIM_REQUEST_ERROR;

    (void)periphId;
    (void)channelId;
    (void)modeState;

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
    gpio_Config_t       pinConfig;

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
    gpio_Config_t       pinConfig;

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
    gpio_Config_t       pinConfig;

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
    gpio_Config_t       pinConfig;

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
 * \param pinId [in] : Identification of required pin.
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
tim_RequestState_t Tim_InitBreakInput2Gpio( tim_Bkin2Pin_t pinId )
{
    tim_RequestState_t  retState      = TIM_REQUEST_ERROR;
    gpio_RequestState_t gpioInitState = GPIO_REQUEST_ERROR;
    gpio_Config_t       pinConfig;

    pinConfig.PortId         = TIM_BIT_MASK_DECODE_PORT( pinId );
    pinConfig.PinId          = TIM_BIT_MASK_DECODE_PIN( pinId );
    pinConfig.PinMode        = GPIO_PIN_MODE_ALTERNATE;
    pinConfig.PinPull        = GPIO_PIN_PULL_NONE;
    pinConfig.PinSpeed       = GPIO_PIN_SPEED_MEDIUM;
    pinConfig.PinOutType     = GPIO_PIN_OUTPUT_PUSHPULL;
    pinConfig.PinAltFunction = TIM_BIT_MASK_DECODE_AF( pinId );;

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


/* =========================== LOCAL FUNCTIONS ============================== */

/**
 * \brief Common ISR callback for Timer1 Break event and Timer 15 general event.
 *
 * This has to be "extra" function because the morons from ST are not able to
 * use different IRQ's for different sources!  They even connected DAC
 * interrupt's with Timer 6!!! You just had 240 IRQ channels. Why to use them
 * properly? (SARCASM)
 *
 */
static void Tim_Tim1_Break_Tim15_General_IsrHandler( void )
{
    Tim_Tim1_Break_IsrHandler();
    Tim_Tim15_General_IsrHandler();
}


/**
 * \brief Common ISR callback for Timer1 Update event and Timer 16 general event.
 *
 * This has to be "extra" function because the morons from ST are not able to
 * use different IRQ's for different sources!  They even connected DAC
 * interrupt's with Timer 6!!! You just had 240 IRQ channels. Why to use them
 * properly? (SARCASM)
 *
 */
static void Tim_Tim1_Update_Tim16_General_IsrHandler( void )
{
    Tim_Tim1_Update_IsrHandler();
    Tim_Tim16_General_IsrHandler();
}


/**
 * \brief Common ISR callback for Timer1 Trigger event and Timer 17 general event.
 *
 * This has to be "extra" function because the morons from ST are not able to
 * use different IRQ's for different sources!  They even connected DAC
 * interrupt's with Timer 6!!! You just had 240 IRQ channels. Why to use them
 * properly? (SARCASM)
 *
 */
static void Tim_Tim1_Trigger_Tim17_General_IsrHandler( void )
{
    Tim_Tim1_Trigger_IsrHandler();
    Tim_Tim17_General_IsrHandler();
}


/* ============================ ISR FUNCTIONS =============================== */

#ifdef TIM1
/**
 * \brief Timer 1 General interrupt handler
 *
 */
static void Tim_Tim1_General_IsrHandler( void )
{
    Tim_Tim1_Break_IsrHandler();
    Tim_Tim1_Update_IsrHandler();
    Tim_Tim1_Trigger_IsrHandler();
    Tim_Tim1_CapatureCompare_IsrHandler();
}


/**
 * \brief Timer 1 Break and Timer 15 common interrupt service  routine
 *
 */
static void Tim_Tim1_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

}


/**
 * \brief Timer 1 Update and Timer 16 common interrupt service  routine
 *
 */
static void Tim_Tim1_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */
}


/**
 * \brief Timer 1 Trigger and Timer 17 common interrupt service  routine
 *
 */
static void Tim_Tim1_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */
}


/**
 * \brief Timer 1 Capture/Compare interrupt service  routine
 *
 * The four Capture/compare channels are checked in interrupt. BECAUSE the ST
 * in their omniscient mighty minds ignored the last two channels if are
 * available. Another great idea ST.
 *
 */
static void Tim_Tim1_CapatureCompare_IsrHandler( void )
{
    /* Check Timer 1 Capture/Compare interrupt */
    if( 0u != LL_TIM_IsActiveFlag_CC1( tim_PeriphConf[ TIM_PERIPH_1 ].PeriphReg ) )
    {

    }
    else
    {
        /* Channel 1 did not trigger interrupt */
    }

    if( 0u != LL_TIM_IsActiveFlag_CC2( tim_PeriphConf[ TIM_PERIPH_1 ].PeriphReg ) )
    {

    }
    else
    {
        /* Channel 1 did not trigger interrupt */
    }

    if( 0u != LL_TIM_IsActiveFlag_CC3( tim_PeriphConf[ TIM_PERIPH_1 ].PeriphReg ) )
    {

    }
    else
    {
        /* Channel 1 did not trigger interrupt */
    }

    if( 0u != LL_TIM_IsActiveFlag_CC4( tim_PeriphConf[ TIM_PERIPH_1 ].PeriphReg ) )
    {

    }
    else
    {
        /* Channel 1 did not trigger interrupt */
    }
}
#endif

/*------------------ Timer 2 Interrupt Service Routines ----------------------*/

#ifdef TIM2
static void Tim_Tim2_General_IsrHandler( void )
{
    Tim_Tim2_Break_IsrHandler();
    Tim_Tim2_Update_IsrHandler();
    Tim_Tim2_Trigger_IsrHandler();
    Tim_Tim2_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 2 Break interrupt service  routine
 *
 */
static void Tim_Tim2_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 2 Update interrupt service  routine
 *
 */
static void Tim_Tim2_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 2 Trigger interrupt service  routine
 *
 */
static void Tim_Tim2_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 2 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim2_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 3 Interrupt Service Routines ----------------------*/

#ifdef TIM3
static void Tim_Tim3_General_IsrHandler( void )
{
    Tim_Tim3_Break_IsrHandler();
    Tim_Tim3_Update_IsrHandler();
    Tim_Tim3_Trigger_IsrHandler();
    Tim_Tim3_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 3 Break interrupt service  routine
 *
 */
static void Tim_Tim3_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 3 Update interrupt service  routine
 *
 */
static void Tim_Tim3_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 3 Trigger interrupt service  routine
 *
 */
static void Tim_Tim3_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 3 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim3_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 4 Interrupt Service Routines ----------------------*/

#ifdef TIM4
static void Tim_Tim4_General_IsrHandler( void )
{
    Tim_Tim4_Break_IsrHandler();
    Tim_Tim4_Update_IsrHandler();
    Tim_Tim4_Trigger_IsrHandler();
    Tim_Tim4_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 4 Break interrupt service  routine
 *
 */
static void Tim_Tim4_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 4 Update interrupt service  routine
 *
 */
static void Tim_Tim4_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 4 Trigger interrupt service  routine
 *
 */
static void Tim_Tim4_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 4 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim4_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 5 Interrupt Service Routines ----------------------*/

#ifdef TIM5
static void Tim_Tim5_General_IsrHandler( void )
{
    Tim_Tim5_Break_IsrHandler();
    Tim_Tim5_Update_IsrHandler();
    Tim_Tim5_Trigger_IsrHandler();
    Tim_Tim5_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 5 Break interrupt service  routine
 *
 */
static void Tim_Tim5_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 5 Update interrupt service  routine
 *
 */
static void Tim_Tim5_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 5 Trigger interrupt service  routine
 *
 */
static void Tim_Tim5_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 5 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim5_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 6 Interrupt Service Routines ----------------------*/

#ifdef TIM6
static void Tim_Tim6_General_IsrHandler( void )
{
    Tim_Tim6_Break_IsrHandler();
    Tim_Tim6_Update_IsrHandler();
    Tim_Tim6_Trigger_IsrHandler();
    Tim_Tim6_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 6 Break interrupt service  routine
 *
 */
static void Tim_Tim6_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 6 Update interrupt service  routine
 *
 */
static void Tim_Tim6_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 6 Trigger interrupt service  routine
 *
 */
static void Tim_Tim6_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 6 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim6_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 7 Interrupt Service Routines ----------------------*/

#ifdef TIM7
static void Tim_Tim7_General_IsrHandler( void )
{
    Tim_Tim7_Break_IsrHandler();
    Tim_Tim7_Update_IsrHandler();
    Tim_Tim7_Trigger_IsrHandler();
    Tim_Tim7_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 7 Break interrupt service  routine
 *
 */
static void Tim_Tim7_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 7 Update interrupt service  routine
 *
 */
static void Tim_Tim7_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 7 Trigger interrupt service  routine
 *
 */
static void Tim_Tim7_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 7 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim7_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 8 Interrupt Service Routines ----------------------*/

#ifdef TIM8
static void Tim_Tim8_General_IsrHandler( void )
{
    Tim_Tim8_Break_IsrHandler();
    Tim_Tim8_Update_IsrHandler();
    Tim_Tim8_Trigger_IsrHandler();
    Tim_Tim8_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 8 Break interrupt service  routine
 *
 */
static void Tim_Tim8_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 8 Update interrupt service  routine
 *
 */
static void Tim_Tim8_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 8 Trigger interrupt service  routine
 *
 */
static void Tim_Tim8_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 8 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim8_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 9 Interrupt Service Routines ----------------------*/

#ifdef TIM9
static void Tim_Tim9_General_IsrHandler( void )
{
    Tim_Tim9_Break_IsrHandler();
    Tim_Tim9_Update_IsrHandler();
    Tim_Tim9_Trigger_IsrHandler();
    Tim_Tim9_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 9 Break interrupt service  routine
 *
 */
static void Tim_Tim9_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 9 Update interrupt service  routine
 *
 */
static void Tim_Tim9_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 9 Trigger interrupt service  routine
 *
 */
static void Tim_Tim9_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 9 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim9_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 10 Interrupt Service Routines ----------------------*/

#ifdef TIM10
static void Tim_Tim10_General_IsrHandler( void )
{
    Tim_Tim10_Break_IsrHandler();
    Tim_Tim10_Update_IsrHandler();
    Tim_Tim10_Trigger_IsrHandler();
    Tim_Tim10_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 10 Break interrupt service  routine
 *
 */
static void Tim_Tim10_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 10 Update interrupt service  routine
 *
 */
static void Tim_Tim10_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 10 Trigger interrupt service  routine
 *
 */
static void Tim_Tim10_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 10 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim10_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 11 Interrupt Service Routines ----------------------*/

#ifdef TIM11
static void Tim_Tim11_General_IsrHandler( void )
{
    Tim_Tim11_Break_IsrHandler();
    Tim_Tim11_Update_IsrHandler();
    Tim_Tim11_Trigger_IsrHandler();
    Tim_Tim11_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 11 Break interrupt service  routine
 *
 */
static void Tim_Tim11_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 11 Update interrupt service  routine
 *
 */
static void Tim_Tim11_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 11 Trigger interrupt service  routine
 *
 */
static void Tim_Tim11_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 11 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim11_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 12 Interrupt Service Routines ----------------------*/

#ifdef TIM12
static void Tim_Tim12_General_IsrHandler( void )
{
    Tim_Tim12_Break_IsrHandler();
    Tim_Tim12_Update_IsrHandler();
    Tim_Tim12_Trigger_IsrHandler();
    Tim_Tim12_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 12 Break interrupt service  routine
 *
 */
static void Tim_Tim12_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 12 Update interrupt service  routine
 *
 */
static void Tim_Tim12_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 12 Trigger interrupt service  routine
 *
 */
static void Tim_Tim12_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 12 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim12_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 13 Interrupt Service Routines ----------------------*/

#ifdef TIM13
static void Tim_Tim13_General_IsrHandler( void )
{
    Tim_Tim13_Break_IsrHandler();
    Tim_Tim13_Update_IsrHandler();
    Tim_Tim13_Trigger_IsrHandler();
    Tim_Tim13_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 13 Break interrupt service  routine
 *
 */
static void Tim_Tim13_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 13 Update interrupt service  routine
 *
 */
static void Tim_Tim13_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 13 Trigger interrupt service  routine
 *
 */
static void Tim_Tim13_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 13 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim13_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 14 Interrupt Service Routines ----------------------*/

#ifdef TIM14
static void Tim_Tim14_General_IsrHandler( void )
{
    Tim_Tim14_Break_IsrHandler();
    Tim_Tim14_Update_IsrHandler();
    Tim_Tim14_Trigger_IsrHandler();
    Tim_Tim14_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 14 Break interrupt service  routine
 *
 */
static void Tim_Tim14_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 14 Update interrupt service  routine
 *
 */
static void Tim_Tim14_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 14 Trigger interrupt service  routine
 *
 */
static void Tim_Tim14_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 14 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim14_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 15 Interrupt Service Routines ----------------------*/

#ifdef TIM15
static void Tim_Tim15_General_IsrHandler( void )
{
    Tim_Tim15_Break_IsrHandler();
    Tim_Tim15_Update_IsrHandler();
    Tim_Tim15_Trigger_IsrHandler();
    Tim_Tim15_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 15 Break interrupt service  routine
 *
 */
static void Tim_Tim15_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 15 Update interrupt service  routine
 *
 */
static void Tim_Tim15_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 15 Trigger interrupt service  routine
 *
 */
static void Tim_Tim15_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 15 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim15_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 16 Interrupt Service Routines ----------------------*/

#ifdef TIM16
static void Tim_Tim16_General_IsrHandler( void )
{
    Tim_Tim16_Break_IsrHandler();
    Tim_Tim16_Update_IsrHandler();
    Tim_Tim16_Trigger_IsrHandler();
    Tim_Tim16_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 16 Break interrupt service  routine
 *
 */
static void Tim_Tim16_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 16 Update interrupt service  routine
 *
 */
static void Tim_Tim16_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 16 Trigger interrupt service  routine
 *
 */
static void Tim_Tim16_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 16 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim16_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 17 Interrupt Service Routines ----------------------*/

#ifdef TIM17
static void Tim_Tim17_General_IsrHandler( void )
{
    Tim_Tim17_Break_IsrHandler();
    Tim_Tim17_Update_IsrHandler();
    Tim_Tim17_Trigger_IsrHandler();
    Tim_Tim17_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 17 Break interrupt service  routine
 *
 */
static void Tim_Tim17_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 17 Update interrupt service  routine
 *
 */
static void Tim_Tim17_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 17 Trigger interrupt service  routine
 *
 */
static void Tim_Tim17_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 17 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim17_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 18 Interrupt Service Routines ----------------------*/

#ifdef TIM18
static void Tim_Tim18_General_IsrHandler( void )
{
    Tim_Tim18_Break_IsrHandler();
    Tim_Tim18_Update_IsrHandler();
    Tim_Tim18_Trigger_IsrHandler();
    Tim_Tim18_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 18 Break interrupt service  routine
 *
 */
static void Tim_Tim18_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 18 Update interrupt service  routine
 *
 */
static void Tim_Tim18_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 18 Trigger interrupt service  routine
 *
 */
static void Tim_Tim18_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 18 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim18_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 19 Interrupt Service Routines ----------------------*/

#ifdef TIM19
static void Tim_Tim19_General_IsrHandler( void )
{
    Tim_Tim19_Break_IsrHandler();
    Tim_Tim19_Update_IsrHandler();
    Tim_Tim19_Trigger_IsrHandler();
    Tim_Tim19_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 19 Break interrupt service  routine
 *
 */
static void Tim_Tim19_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 19 Update interrupt service  routine
 *
 */
static void Tim_Tim19_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 19 Trigger interrupt service  routine
 *
 */
static void Tim_Tim19_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 19 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim19_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif

/*------------------ Timer 20 Interrupt Service Routines ----------------------*/

#ifdef TIM20
static void Tim_Tim20_General_IsrHandler( void )
{
    Tim_Tim20_Break_IsrHandler();
    Tim_Tim20_Update_IsrHandler();
    Tim_Tim20_Trigger_IsrHandler();
    Tim_Tim20_CapatureCompare_IsrHandler();
}

/**
 * \brief Timer 20 Break interrupt service  routine
 *
 */
static void Tim_Tim20_Break_IsrHandler( void )
{
    /* Check Timer 1 Break interrupt */

    /* Check Timer 15 interrupts */

}


/**
 * \brief Timer 20 Update interrupt service  routine
 *
 */
static void Tim_Tim20_Update_IsrHandler( void )
{
    /* Check Timer 1 Update interrupt */

    /* Check Timer 16 interrupts */
}


/**
 * \brief Timer 20 Trigger interrupt service  routine
 *
 */
static void Tim_Tim20_Trigger_IsrHandler( void )
{
    /* Check Timer 1 Trigger interrupt */

    /* Check Timer 17 interrupts */
}


/**
 * \brief Timer 20 Capture&Compare interrupt service  routine
 *
 */
static void Tim_Tim20_CapatureCompare_IsrHandler( void )
{
    /* Check Timer Capture&Compare interrupt */
}
#endif


/* =========================== INTERRUPT HANDLERS =========================== */

/* ================================ TASKS =================================== */

