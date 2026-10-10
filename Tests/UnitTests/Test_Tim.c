/**
 * \author Mr.Nobody
 * \file Test_Tim.c
 * \ingroup Tim
 * \brief Unit tests of Timer (TIM) module.
 *
 * Tim.c is compiled unchanged with real LL drivers. TIM registers are emulated
 * by RegMem, RCC, NVIC and GPIO modules are mocked by CMock. Timer ISRs
 * registered in NVIC are captured by stub and called directly to test interrupt
 * handling.
 *
 * Tests use timers of STM32F7 (TIM1 advanced with channels 5 / 6 and break 2,
 * TIM2 / TIM5 32-bit general purpose, TIM3 16-bit general purpose, TIM6 basic,
 * TIM9 / TIM11 general purpose with shared interrupt lines). Functions of
 * features not available on STM32F7 timers are tested to return error without
 * register access.
 *
 * \note Emulated registers are plain memory. Status register (SR) flags are
 *       write-0-to-clear on HW - emulated SR keeps the last written value
 *       (~flag), tests check that the flag bit was cleared by the write.
 *       Register write read-back never fails on emulated registers, timeout
 *       branches of read-back loops are not tested.
 */

/* ============================= INCLUDES =================================== */
#include "unity.h"                          /* Unity testing framework        */
#include "UtCommon.h"                       /* Common test helpers            */
#include "RegMem.h"                         /* Register memory emulation      */
#include "Tim_Port.h"                       /* Module under test              */
#include "MockRcc_Port.h"                   /* RCC module mock                */
#include "MockNvic_Port.h"                  /* NVIC module mock               */
#include "MockGpio_Port.h"                  /* GPIO module mock               */
#include "Stm32_tim.h"                      /* TIM registers definition       */
#include <stdint.h>                         /* Register addresses (uintptr_t) */
/* ============================= TYPEDEFS =================================== */

/* ======================= FORWARD DECLARATIONS ============================= */

static nvic_RequestState_t  Ut_Tim_NvicSetHandlerStub   ( nvic_PeriphIrqList_t irqId, const nvic_IsrCallback_t irqHandler, int callCnt );
static void                 Ut_Tim_Expect_PeriphClk     ( rcc_PeriphId_t rccId, rcc_FreqHz_t periphClk );
static void                 Ut_Tim_Expect_ClockActivation( rcc_PeriphId_t rccId );
static void                 Ut_Tim_Expect_GpioInit      ( gpio_PortId_t portId, gpio_PinId_t pinId, gpio_AltFunction_t altFunc, gpio_RequestState_t retState );
static void                 Ut_Tim_Expect_Deinit        ( rcc_PeriphId_t rccId, const nvic_PeriphIrqList_t * const irqLines );
static void                 Ut_Tim_Enable_Irq           ( tim_PeriphId_t periphId, tim_IrqId_t irqId, nvic_PeriphIrqList_t nvicIrqId );
static void                 Ut_Tim_Reset_Callbacks      ( void );
static tim_PeriphConfig_t   Ut_Tim_Get_Config           ( tim_PeriphId_t periphId );
static tim_ChannelConfig_t  Ut_Tim_Get_ChannelConfig    ( tim_ChannelId_t channelId, tim_ChannelMode_t channelMode );

static void                 Ut_Tim_UpdateCallback       ( void );
static void                 Ut_Tim_CaptureCompareCallback( tim_OvercaptureFlag_t overcaptureFlag );
static void                 Ut_Tim_TriggerCallback      ( void );
static void                 Ut_Tim_CommutationCallback  ( void );
static void                 Ut_Tim_BreakCallback        ( void );
static void                 Ut_Tim_Break2Callback       ( void );

/* ========================= SYMBOLIC CONSTANTS ============================= */

/** Timer kernel clock returned by RCC mock [Hz] */
#define UT_TIM_CLK_HZ                       ( 250000000u )

/** Timer kernel clock used by dead-time tests (10 ns per fDTS period) [Hz] */
#define UT_TIM_DT_CLK_HZ                    ( 100000000u )

/** Prescaler register value dividing \ref UT_TIM_CLK_HZ to 10 MHz */
#define UT_TIM_PSC_10MHZ                    ( 24u )

/** Prescaler register value dividing \ref UT_TIM_CLK_HZ to 1 MHz */
#define UT_TIM_PSC_1MHZ                     ( 249u )

/** Maximum value of 16-bit timer registers */
#define UT_TIM_16BIT_MAX                    ( 0xFFFFu )

/** Interrupt priority used by tests */
#define UT_TIM_PRIO                         ( 5u )

/** Count of NVIC interrupt groups of timer (update, CC, trigger / commutation, break) */
#define UT_TIM_IRQ_LINE_CNT                 ( 4u )

/** Count of stored values returned through pointers by mocks in one test */
#define UT_TIM_MOCK_VALUE_CNT               ( 8u )

/** Count of stored expected GPIO configurations in one test */
#define UT_TIM_GPIO_CFG_CNT                 ( 4u )

/** Count of channels with capture / compare interrupt callback */
#define UT_TIM_CC_CALLBACK_CNT              ( 4u )

/** Invalid trigger input (item of the general purpose timer with a code that is not an SMCR.TS selection) */
#define UT_TIM_INVALID_TRIGGER              ( (tim_ExtClkSource_t)TIM_TRIGGER_INPUT_BIT_MASK_ENCODE( TIM_PERIPH_3, 0x0Fu ) )

/** Trigger input of another timer (TI1FP1 of TIM1) - refused for the general purpose timer */
#define UT_TIM_FOREIGN_TRIGGER              ( TIM_TRIGGER_INPUT_TIM1_TI1FP1 )

/** Trigger inputs TI1FP1 / TI2FP2 of the general purpose timer of the tests (valid on every device) */
#define UT_TIM_GP_TRG_TI1FP1                ( TIM_TRIGGER_INPUT_TIM3_TI1FP1 )
#define UT_TIM_GP_TRG_TI2FP2                ( TIM_TRIGGER_INPUT_TIM3_TI2FP2 )

/** Invalid input polarity (CCxNP without CCxP) */
#define UT_TIM_INVALID_IC_POLARITY          ( (tim_InputPolarity_t)TIM_CCER_CC1NP )

/* ============================== MACROS ==================================== */

/* ========================== LOCAL VARIABLES =============================== */

/** ISR registered in NVIC, indexed by NVIC interrupt line */
static nvic_IsrCallback_t   utTim_Isr[ NVIC_PERIPH_IRQ_SIZE ];

/** Count of Nvic_Set_PeriphIrq_Handler calls */
static uint32_t             utTim_HandlerCallCnt;

/** Timer kernel clock values returned by Rcc_Get_PeriphClk mock */
static rcc_FreqHz_t         utTim_PeriphClk[ UT_TIM_MOCK_VALUE_CNT ];

/** Index of next free item of \ref utTim_PeriphClk */
static uint32_t             utTim_PeriphClkIdx;

/** Expected GPIO configurations of Gpio_Init mock */
static gpio_Config_t        utTim_GpioCfg[ UT_TIM_GPIO_CFG_CNT ];

/** Index of next free item of \ref utTim_GpioCfg */
static uint32_t             utTim_GpioCfgIdx;

/** Counts of user callback calls */
static uint32_t             utTim_UpdateCnt;
static uint32_t             utTim_CaptureCompareCnt;
static uint32_t             utTim_TriggerCnt;
static uint32_t             utTim_CommutationCnt;
static uint32_t             utTim_BreakCnt;
static uint32_t             utTim_Break2Cnt;

/** Parameters of last user callback calls */
static tim_OvercaptureFlag_t utTim_LastOvercapture;

/** NVIC lines of TIM1 in order of processing by the module (update, CC, trigger / commutation, break - shared with TIM10 / TIM11 / TIM9) */
static const nvic_PeriphIrqList_t utTim_Tim1IrqLines[ UT_TIM_IRQ_LINE_CNT ] =
{
    NVIC_PERIPH_IRQ_TIM1_UP_TIM10, NVIC_PERIPH_IRQ_TIM1_CC, NVIC_PERIPH_IRQ_TIM1_TRG_COM_TIM11, NVIC_PERIPH_IRQ_TIM1_BRK_TIM9
};

/** NVIC lines of TIM3 (all interrupt groups share the global line) */
static const nvic_PeriphIrqList_t utTim_Tim3IrqLines[ UT_TIM_IRQ_LINE_CNT ] =
{
    NVIC_PERIPH_IRQ_TIM3, NVIC_PERIPH_IRQ_TIM3, NVIC_PERIPH_IRQ_TIM3, NVIC_PERIPH_IRQ_TIM3
};

/* ============================ TEST FIXTURE ================================ */

void setUp( void )
{
    TEST_ASSERT_EQUAL( REGMEM_REQUEST_OK, RegMem_Reset() );

    for( uint32_t irqId = 0u; NVIC_PERIPH_IRQ_SIZE > irqId; irqId++ )
    {
        utTim_Isr[ irqId ] = NULL;
    }

    utTim_HandlerCallCnt    = 0u;
    utTim_PeriphClkIdx      = 0u;
    utTim_GpioCfgIdx        = 0u;
    utTim_UpdateCnt         = 0u;
    utTim_CaptureCompareCnt = 0u;
    utTim_TriggerCnt        = 0u;
    utTim_CommutationCnt    = 0u;
    utTim_BreakCnt          = 0u;
    utTim_Break2Cnt         = 0u;
    utTim_LastOvercapture   = TIM_OVERCAPTURE_INACTIVE;

    /* Module keeps user callbacks between tests */
    Ut_Tim_Reset_Callbacks();

    Nvic_Set_PeriphIrq_Handler_Stub( Ut_Tim_NvicSetHandlerStub );
}


void tearDown( void )
{
    /* Mocks are verified by generated runner */
}

/* ========================== MODULE VERSION ================================ */

/**
 * \brief   Tim_Get_ModuleVersion() returns version of the module.
 *
 * \details Reads the module version structure.
 *
 * \par Expected results
 * - Version is 1.0.0 (Major 1, Minor 0, Patch 0).
 */
void Ut_Tim_Get_ModuleVersion_ReturnsVersion( void )
{
    tim_ModuleVersion_t version = Tim_Get_ModuleVersion();

    TEST_ASSERT_EQUAL_UINT8( 1u, version.Major );
    TEST_ASSERT_EQUAL_UINT8( 0u, version.Minor );
    TEST_ASSERT_EQUAL_UINT8( 0u, version.Patch );
}


/**
 * \brief   Tim_Task() does not access peripherals.
 *
 * \details Calls Tim_Task() without any expected mock call.
 *
 * \par Expected results
 * - No RCC / NVIC / GPIO call (strict mocks), TIM3 CR1 stays 0.
 */
void Ut_Tim_Task_DoesNotAccessPeripherals( void )
{
    /* No mock call expected - strict mocks fail on any call */
    Tim_Task();

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CR1 );
}

/* =========================== DEFAULT CONFIG =============================== */

/**
 * \brief   Tim_Get_DefaultConfig() fills default configuration.
 *
 * \details Reads default configuration.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, peripheral not selected (TIM_PERIPH_CNT), internal clock, slave
 *   mode disabled, master trigger reset.
 * - Auto-reload preload inactive, update event active, up-counting.
 * - Break, break 2 and ETR pins unused, timer frequency >= refresh frequency.
 * - All channels inactive with own channel identification and unused pins.
 */
void Ut_Tim_Get_DefaultConfig_ReturnsDefaults( void )
{
    tim_PeriphConfig_t config;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_DefaultConfig( &config ) );

    TEST_ASSERT_EQUAL( TIM_PERIPH_CNT,           config.PeriphId );
    TEST_ASSERT_EQUAL( TIM_CLOCKSOURCE_INT_CLK,  config.ClockSource );
    TEST_ASSERT_EQUAL( TIM_SLAVE_MODE_DISABLE,   config.SlaveMode );
    TEST_ASSERT_EQUAL( TIM_MASTER_TRIGGER_RESET, config.MasterTrigger );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_INACTIVE,    config.AutoreloadPreloadState );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_ACTIVE,      config.UpdateEventState );
    TEST_ASSERT_EQUAL( TIM_COUNTER_DIR_UP,       config.CounterDirection );
    TEST_ASSERT_EQUAL( TIM_BKIN_PIN_UNUSED,      config.BreakInPin );
    TEST_ASSERT_EQUAL( TIM_BKIN2_PIN_UNUSED,     config.BreakIn2Pin );
    TEST_ASSERT_EQUAL( TIM_ETR_PIN_UNUSED,       config.TriggerEventPin );
    TEST_ASSERT_TRUE( config.TimerFrequency >= config.RefreshFrequency );

    for( uint32_t channelId = 0u; TIM_CHANNEL_CNT > channelId; channelId++ )
    {
        TEST_ASSERT_EQUAL( channelId,             config.ChannelConfig[ channelId ].ChannelId );
        TEST_ASSERT_EQUAL( TIM_FUNCTION_INACTIVE, config.ChannelConfig[ channelId ].ChannelState );
        TEST_ASSERT_EQUAL( TIM_CH_PIN_UNUSED,     config.ChannelConfig[ channelId ].IoPin );
        TEST_ASSERT_EQUAL( TIM_CH_N_PIN_UNUSED,   config.ChannelConfig[ channelId ].IoComplPin );
    }
}


/**
 * \brief   Tim_Get_DefaultConfig() rejects NULL pointer.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR.
 */
void Ut_Tim_Get_DefaultConfig_NullPtr_ReturnsError( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_DefaultConfig( NULL ) );
}

/* ======================== COUNTER START / STOP ============================ */

/**
 * \brief   Tim_Start() enables the counter.
 *
 * \details Starts TIM3.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, CR1 = CEN.
 */
void Ut_Tim_Start_EnablesCounter( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( TIM_PERIPH_3 ) );

    TEST_ASSERT_EQUAL_HEX32( TIM_CR1_CEN, TIM3->CR1 );
}


/**
 * \brief   Tim_Start() in one pulse mode does not verify counter enable.
 *
 * \details CR1.OPM preset (CEN is cleared by HW at the end of the pulse), starts TIM3.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, CR1 = OPM | CEN.
 */
void Ut_Tim_Start_OnePulseMode_ReturnsOkWithoutReadBack( void )
{
    /* Counter enable is cleared by HW at the end of the pulse - it is not verified */
    TIM3->CR1 = TIM_CR1_OPM;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( TIM_PERIPH_3 ) );

    TEST_ASSERT_EQUAL_HEX32( TIM_CR1_OPM | TIM_CR1_CEN, TIM3->CR1 );
}


/**
 * \brief   Tim_Start() rejects peripheral out of range.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR.
 */
void Ut_Tim_Start_InvalidPeriph_ReturnsError( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Start( TIM_PERIPH_CNT ) );
}


/**
 * \brief   Tim_Stop() disables the counter and resets its value.
 *
 * \details CR1 = CEN | ARPE, CNT = 0x1234, stops TIM3.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, CR1 = ARPE (other bits kept), CNT = 0.
 */
void Ut_Tim_Stop_DisablesCounterAndResetsCounterValue( void )
{
    TIM3->CR1 = TIM_CR1_CEN | TIM_CR1_ARPE;
    TIM3->CNT = 0x1234u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Stop( TIM_PERIPH_3 ) );

    TEST_ASSERT_EQUAL_HEX32( TIM_CR1_ARPE, TIM3->CR1 );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CNT );
}


/**
 * \brief   Tim_Pause() disables the counter and keeps its value.
 *
 * \details CR1 = CEN, CNT = 0x1234, pauses TIM3.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, CR1 = 0, CNT = 0x1234.
 */
void Ut_Tim_Pause_DisablesCounterAndKeepsCounterValue( void )
{
    TIM3->CR1 = TIM_CR1_CEN;
    TIM3->CNT = 0x1234u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Pause( TIM_PERIPH_3 ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CR1 );
    TEST_ASSERT_EQUAL_HEX32( 0x1234u, TIM3->CNT );
}


/**
 * \brief   Tim_Stop() and Tim_Pause() reject peripheral out of range.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in both cases.
 */
void Ut_Tim_Stop_InvalidPeriph_ReturnsError( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Stop( TIM_PERIPH_CNT ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Pause( TIM_PERIPH_CNT ) );
}


/**
 * \brief   Tim_Get_PeriphState() reads counter enable bit.
 *
 * \details TIM2 CR1 = CEN, TIM3 CR1 = 0. Reads state of both timers.
 *
 * \par Expected results
 * - TIM2 active, TIM3 inactive.
 */
void Ut_Tim_Get_PeriphState_ReadsCounterEnable( void )
{
    tim_FunctionState_t periphState = TIM_FUNCTION_INACTIVE;

    TIM2->CR1 = TIM_CR1_CEN;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_PeriphState( TIM_PERIPH_2, &periphState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_ACTIVE, periphState );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_PeriphState( TIM_PERIPH_3, &periphState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_INACTIVE, periphState );
}


/**
 * \brief   Tim_Get_PeriphState() rejects invalid arguments.
 *
 * \details Calls the function with peripheral out of range and with NULL pointer.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in both cases.
 */
void Ut_Tim_Get_PeriphState_InvalidArgs_ReturnsError( void )
{
    tim_FunctionState_t periphState = TIM_FUNCTION_INACTIVE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_PeriphState( TIM_PERIPH_CNT, &periphState ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_PeriphState( TIM_PERIPH_3, NULL ) );
}

/* ============================ INTERNAL CLOCK ============================== */

/**
 * \brief   Tim_Set_ClkInternal() writes prescaler for exact divider.
 *
 * \details Kernel clock 250 MHz, requested counter frequency 1 MHz.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, PSC = 249, true frequency 1 MHz.
 */
void Ut_Tim_Set_ClkInternal_ExactDivider_WritesPrescaler( void )
{
    tim_FreqHz_t trueFreq = 0u;

    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_ClkInternal( TIM_PERIPH_3, 1000000u, &trueFreq ) );

    TEST_ASSERT_EQUAL_HEX32( UT_TIM_PSC_1MHZ, TIM3->PSC );
    TEST_ASSERT_EQUAL_UINT32( 1000000u, trueFreq );
}


/**
 * \brief   Tim_Set_ClkInternal() rounds divider to nearest value.
 *
 * \details Kernel clock 250 MHz, requested counter frequency 3 MHz (divider 83.3).
 *
 * \par Expected results
 * - TIM_REQUEST_OK, PSC = 82 (divider 83), true frequency 250 MHz / 83.
 */
void Ut_Tim_Set_ClkInternal_InexactDivider_RoundsToNearest( void )
{
    tim_FreqHz_t trueFreq = 0u;

    /* 250 MHz / 3 MHz = 83.3 -> divider 83 */
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_ClkInternal( TIM_PERIPH_3, 3000000u, &trueFreq ) );

    TEST_ASSERT_EQUAL_HEX32( 82u, TIM3->PSC );
    TEST_ASSERT_EQUAL_UINT32( UT_TIM_CLK_HZ / 83u, trueFreq );
}


/**
 * \brief   Tim_Set_ClkInternal() rejects frequency below prescaler range.
 *
 * \details Kernel clock 250 MHz, requested counter frequency 1 Hz (divider does not
 *          fit 16-bit prescaler).
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, PSC stays 0.
 */
void Ut_Tim_Set_ClkInternal_FrequencyTooLow_ReturnsErrorWithoutWrite( void )
{
    tim_FreqHz_t trueFreq = 0u;

    /* Divider 250000000 does not fit 16-bit prescaler */
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ClkInternal( TIM_PERIPH_3, 1u, &trueFreq ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->PSC );
}


/**
 * \brief   Tim_Set_ClkInternal() rejects frequency above kernel clock.
 *
 * \details Kernel clock 250 MHz, requested counter frequency 250 MHz + 1 Hz.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR.
 */
void Ut_Tim_Set_ClkInternal_FrequencyAboveKernelClock_ReturnsError( void )
{
    tim_FreqHz_t trueFreq = 0u;

    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ClkInternal( TIM_PERIPH_3, UT_TIM_CLK_HZ + 1u, &trueFreq ) );
}


/**
 * \brief   Tim_Set_ClkInternal() reports RCC error.
 *
 * \details RCC mock returns error when kernel clock is read.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, PSC stays 0.
 */
void Ut_Tim_Set_ClkInternal_RccError_ReturnsErrorWithoutWrite( void )
{
    tim_FreqHz_t trueFreq = 0u;

    Rcc_Get_PeriphClk_ExpectAndReturn( RCC_PERIPH_TIM3, NULL, RCC_REQUEST_ERROR );
    Rcc_Get_PeriphClk_IgnoreArg_periphClk();

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ClkInternal( TIM_PERIPH_3, 1000000u, &trueFreq ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->PSC );
}


/**
 * \brief   Tim_Set_ClkInternal() rejects invalid arguments.
 *
 * \details Calls the function with peripheral out of range, frequency 0 and NULL
 *          pointer.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, no RCC call (strict mock).
 */
void Ut_Tim_Set_ClkInternal_InvalidArgs_ReturnsErrorWithoutRccAccess( void )
{
    tim_FreqHz_t trueFreq = 0u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ClkInternal( TIM_PERIPH_CNT, 1000000u, &trueFreq ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ClkInternal( TIM_PERIPH_3,   0u,       &trueFreq ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ClkInternal( TIM_PERIPH_3,   1000000u, NULL ) );
}


/**
 * \brief   Tim_Get_ClkInternal() calculates counter frequency.
 *
 * \details Kernel clock 250 MHz, PSC = 24.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, counter frequency 10 MHz.
 */
void Ut_Tim_Get_ClkInternal_CalculatesCounterFrequency( void )
{
    tim_FreqHz_t timFreq = 0u;

    TIM3->PSC = UT_TIM_PSC_10MHZ;
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_ClkInternal( TIM_PERIPH_3, &timFreq ) );
    TEST_ASSERT_EQUAL_UINT32( 10000000u, timFreq );
}


/**
 * \brief   Tim_Get_ClkInternal() rejects NULL pointer.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR.
 */
void Ut_Tim_Get_ClkInternal_NullPtr_ReturnsError( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_ClkInternal( TIM_PERIPH_3, NULL ) );
}


/**
 * \brief   Minimal and maximal counter step time are calculated.
 *
 * \details Kernel clock 250 MHz. Reads minimal and maximal step time of TIM3.
 *
 * \par Expected results
 * - Minimal step time 4 ns (prescaler 1).
 * - Maximal step time 262144 ns (prescaler 65536).
 */
void Ut_Tim_Get_TimStepTime_MinAndMax( void )
{
    tim_Time_ns_t stepTime = 0u;

    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_TimStepTimeMin( TIM_PERIPH_3, &stepTime ) );
    TEST_ASSERT_EQUAL_UINT32( 4u, stepTime );

    /* Prescaler divider 65536 */
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_TimStepTimeMax( TIM_PERIPH_3, &stepTime ) );
    TEST_ASSERT_EQUAL_UINT32( 262144u, stepTime );
}


/**
 * \brief   Maximal step time not representable in nanoseconds is rejected.
 *
 * \details Kernel clock 1 Hz (maximal step time 65536 s does not fit 32-bit ns).
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR.
 */
void Ut_Tim_Get_TimStepTimeMax_NotRepresentable_ReturnsError( void )
{
    tim_Time_ns_t stepTime = 0u;

    /* 65536 s at 1 Hz kernel clock does not fit into nanoseconds (32-bit) */
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, 1u );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_TimStepTimeMax( TIM_PERIPH_3, &stepTime ) );
}

/* ============================ COUNTER, PERIOD ============================= */

/**
 * \brief   Tim_Set_Counter() writes counter of stopped timer.
 *
 * \details Sets TIM3 counter 0x1234.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, CNT = 0x1234.
 */
void Ut_Tim_Set_Counter_StoppedTimer_WritesCounter( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Counter( TIM_PERIPH_3, 0x1234u ) );

    TEST_ASSERT_EQUAL_HEX32( 0x1234u, TIM3->CNT );
}


/**
 * \brief   Tim_Set_Counter() writes counter of running timer without read-back.
 *
 * \details CR1 = CEN, sets TIM3 counter 0x55 (running counter cannot be verified).
 *
 * \par Expected results
 * - TIM_REQUEST_OK, CNT = 0x55.
 */
void Ut_Tim_Set_Counter_RunningTimer_WritesCounterWithoutReadBack( void )
{
    TIM3->CR1 = TIM_CR1_CEN;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Counter( TIM_PERIPH_3, 0x55u ) );

    TEST_ASSERT_EQUAL_HEX32( 0x55u, TIM3->CNT );
}


/**
 * \brief   Tim_Set_Counter() accepts 32-bit value for 32-bit timer.
 *
 * \details Sets TIM2 counter 0x89ABCDEF.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, CNT = 0x89ABCDEF.
 */
void Ut_Tim_Set_Counter_32BitTimer_AcceptsFullRange( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Counter( TIM_PERIPH_2, 0x89ABCDEFu ) );

    TEST_ASSERT_EQUAL_HEX32( 0x89ABCDEFu, TIM2->CNT );
}


/**
 * \brief   Tim_Set_Counter() rejects value above timer resolution.
 *
 * \details Sets TIM3 (16-bit) counter 0x10000.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, CNT stays 0.
 */
void Ut_Tim_Set_Counter_ValueAboveResolution_ReturnsErrorWithoutWrite( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Counter( TIM_PERIPH_3, UT_TIM_16BIT_MAX + 1u ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CNT );
}


/**
 * \brief   Tim_Get_Counter() reads counter.
 *
 * \details CNT = 0x4321, reads TIM3 counter, then calls the function with NULL.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, counter 0x4321.
 * - NULL pointer: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Get_Counter_ReadsCounter( void )
{
    tim_Counter_t counterValue = 0u;

    TIM3->CNT = 0x4321u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Counter( TIM_PERIPH_3, &counterValue ) );
    TEST_ASSERT_EQUAL_HEX32( 0x4321u, counterValue );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_Counter( TIM_PERIPH_3, NULL ) );
}


/**
 * \brief   Tim_Set_Period() writes auto-reload register.
 *
 * \details Sets TIM3 period 999 and reads it back.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, ARR = 999, period reads back 999.
 */
void Ut_Tim_Set_Period_WritesAutoReload( void )
{
    tim_Counter_t autoreloadValue = 0u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Period( TIM_PERIPH_3, 999u ) );

    TEST_ASSERT_EQUAL_HEX32( 999u, TIM3->ARR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Period( TIM_PERIPH_3, &autoreloadValue ) );
    TEST_ASSERT_EQUAL_UINT32( 999u, autoreloadValue );
}


/**
 * \brief   Tim_Set_Period() rejects invalid arguments.
 *
 * \details Sets TIM3 (16-bit) period 0x10000 and period of peripheral out of range.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in both cases, ARR stays 0.
 */
void Ut_Tim_Set_Period_ValueAboveResolution_ReturnsErrorWithoutWrite( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Period( TIM_PERIPH_3, UT_TIM_16BIT_MAX + 1u ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Period( TIM_PERIPH_CNT, 1u ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->ARR );
}


/**
 * \brief   Tim_Set_RefreshFreq() calculates auto-reload from counter frequency.
 *
 * \details Kernel clock 250 MHz, PSC = 24 (10 MHz). Sets refresh frequency 1 kHz.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, ARR = 9999, true frequency 1 kHz.
 */
void Ut_Tim_Set_RefreshFreq_InternalClock_WritesAutoReload( void )
{
    tim_FreqHz_t trueFreq = 0u;

    TIM3->PSC = UT_TIM_PSC_10MHZ;
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_RefreshFreq( TIM_PERIPH_3, 1000u, &trueFreq ) );

    TEST_ASSERT_EQUAL_HEX32( 9999u, TIM3->ARR );
    TEST_ASSERT_EQUAL_UINT32( 1000u, trueFreq );
}


/**
 * \brief   Tim_Set_RefreshFreq() rejects period above timer resolution.
 *
 * \details Counter frequency 10 MHz, refresh frequency 100 Hz (100000 steps above
 *          16-bit).
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, ARR stays 0.
 */
void Ut_Tim_Set_RefreshFreq_PeriodAboveResolution_ReturnsError( void )
{
    tim_FreqHz_t trueFreq = 0u;

    /* 10 MHz / 100 Hz = 100000 steps > 16-bit */
    TIM3->PSC = UT_TIM_PSC_10MHZ;
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_RefreshFreq( TIM_PERIPH_3, 100u, &trueFreq ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->ARR );
}


/**
 * \brief   Tim_Set_RefreshFreq() rejects externally clocked timer.
 *
 * \details SMCR.ECE set (external clock mode 2), sets refresh frequency 1 kHz.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, no RCC call (strict mock).
 */
void Ut_Tim_Set_RefreshFreq_ExternalClock_ReturnsErrorWithoutRccAccess( void )
{
    tim_FreqHz_t trueFreq = 0u;

    TIM3->SMCR = TIM_SMCR_ECE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_RefreshFreq( TIM_PERIPH_3, 1000u, &trueFreq ) );
}


/**
 * \brief   Tim_Get_RefreshFreq() calculates period frequency.
 *
 * \details Kernel clock 250 MHz, PSC = 24, ARR = 9999.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, refresh frequency 1 kHz.
 */
void Ut_Tim_Get_RefreshFreq_CalculatesPeriodFrequency( void )
{
    tim_FreqHz_t refreshFreq = 0u;

    TIM3->PSC = UT_TIM_PSC_10MHZ;
    TIM3->ARR = 9999u;
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_RefreshFreq( TIM_PERIPH_3, &refreshFreq ) );
    TEST_ASSERT_EQUAL_UINT32( 1000u, refreshFreq );
}

/* ========================== COUNTER DIRECTION ============================= */

/**
 * \brief   Center-aligned counter mode is written and read back.
 *
 * \details Sets TIM3 direction center-aligned up / down and reads it back.
 *
 * \par Expected results
 * - CR1 = center-aligned mode 3, direction reads back center up / down.
 */
void Ut_Tim_Set_CounterDirection_CenterAligned_WritesCounterMode( void )
{
    tim_CounterDir_t counterDir = TIM_COUNTER_DIR_UP;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CounterDirection( TIM_PERIPH_3, TIM_COUNTER_DIR_CENTER_UP_DOWN ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_COUNTERMODE_CENTER_UP_DOWN, TIM3->CR1 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_CounterDirection( TIM_PERIPH_3, &counterDir ) );
    TEST_ASSERT_EQUAL( TIM_COUNTER_DIR_CENTER_UP_DOWN, counterDir );
}


/**
 * \brief   Down-counting is written to DIR bit.
 *
 * \details Sets TIM1 direction down.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, CR1 = DIR.
 */
void Ut_Tim_Set_CounterDirection_Down_WritesDir( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CounterDirection( TIM_PERIPH_1, TIM_COUNTER_DIR_DOWN ) );

    TEST_ASSERT_EQUAL_HEX32( TIM_CR1_DIR, TIM1->CR1 );
}


/**
 * \brief   Basic timer supports up-counting only.
 *
 * \details Sets TIM6 direction down, then up.
 *
 * \par Expected results
 * - Down: TIM_REQUEST_ERROR, CR1 stays 0.
 * - Up: TIM_REQUEST_OK.
 */
void Ut_Tim_Set_CounterDirection_BasicTimer_UpOnly( void )
{
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_CounterDirection( TIM_PERIPH_6, TIM_COUNTER_DIR_DOWN ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM6->CR1 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CounterDirection( TIM_PERIPH_6, TIM_COUNTER_DIR_UP ) );
#else
    TEST_IGNORE_MESSAGE( "MCU without basic timer TIM6" );
#endif /* TIM6 */
}


/**
 * \brief   Direction of running timer cannot be changed.
 *
 * \details CR1 = CEN, sets TIM3 direction down.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, CR1 unchanged.
 */
void Ut_Tim_Set_CounterDirection_RunningTimer_ReturnsErrorWithoutWrite( void )
{
    TIM3->CR1 = TIM_CR1_CEN;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_CounterDirection( TIM_PERIPH_3, TIM_COUNTER_DIR_DOWN ) );

    TEST_ASSERT_EQUAL_HEX32( TIM_CR1_CEN, TIM3->CR1 );
}


/**
 * \brief   Counter direction functions reject invalid arguments.
 *
 * \details Calls setter with direction and peripheral out of range, getter with NULL
 *          pointer and peripheral out of range.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases.
 */
void Ut_Tim_Set_CounterDirection_InvalidArgs_ReturnsError( void )
{
    tim_CounterDir_t counterDir = TIM_COUNTER_DIR_UP;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_CounterDirection( TIM_PERIPH_3, TIM_COUNTER_DIR_CNT ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_CounterDirection( TIM_PERIPH_CNT, TIM_COUNTER_DIR_UP ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_CounterDirection( TIM_PERIPH_3, NULL ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_CounterDirection( TIM_PERIPH_CNT, &counterDir ) );
}

/* ========================= REPETITION COUNTER ============================= */

/**
 * \brief   Advanced timer accepts 16-bit repetition counter.
 *
 * \details Sets TIM1 repetition counter 0xFFFF and reads it back.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, RCR = 0xFFFF (16-bit RCR on STM32F7), reads back 0xFFFF.
 */
void Ut_Tim_Set_RepetitionCounter_AdvancedTimer_Accepts16Bit( void )
{
    tim_RepCnt_t repetitionCnt = 0u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_RepetitionCounter( TIM_PERIPH_1, 0xFFFFu ) );

    TEST_ASSERT_EQUAL_HEX32( 0xFFFFu, TIM1->RCR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_RepetitionCounter( TIM_PERIPH_1, &repetitionCnt ) );
    TEST_ASSERT_EQUAL_HEX16( 0xFFFFu, repetitionCnt );
}


/**
 * \brief   Timer without repetition counter rejects the access.
 *
 * \details Sets and reads TIM3 repetition counter.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in both cases, RCR stays 0.
 */
void Ut_Tim_Set_RepetitionCounter_TimerWithoutRcr_ReturnsError( void )
{
    tim_RepCnt_t repetitionCnt = 0u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_RepetitionCounter( TIM_PERIPH_3, 1u ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_RepetitionCounter( TIM_PERIPH_3, &repetitionCnt ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->RCR );
}


/* ===================== UPDATE EVENT, PRELOAD, OPM ========================= */

/**
 * \brief   Update event is disabled and enabled by UDIS bit.
 *
 * \details Deactivates update event of TIM3, reads the state, activates it and reads
 *          the state.
 *
 * \par Expected results
 * - CR1 = UDIS, state inactive.
 * - CR1 = 0, state active.
 */
void Ut_Tim_Set_UpdateEvent_TogglesUdis( void )
{
    tim_FunctionState_t eventState = TIM_FUNCTION_ACTIVE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_UpdateEventInactive( TIM_PERIPH_3 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_CR1_UDIS, TIM3->CR1 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_UpdateEventState( TIM_PERIPH_3, &eventState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_INACTIVE, eventState );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_UpdateEventActive( TIM_PERIPH_3 ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CR1 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_UpdateEventState( TIM_PERIPH_3, &eventState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_ACTIVE, eventState );
}


/**
 * \brief   Update source counter only is written to URS bit.
 *
 * \details Sets TIM3 update source counter only, reads it back, then sets source out
 *          of range.
 *
 * \par Expected results
 * - CR1 = URS, source reads back counter only.
 * - Source out of range: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Set_UpdateSource_CounterOnly_WritesUrs( void )
{
    tim_UpdateSource_t updateSource = TIM_UPDATE_SOURCE_ANY;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_UpdateSource( TIM_PERIPH_3, TIM_UPDATE_SOURCE_COUNTER ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_CR1_URS, TIM3->CR1 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_UpdateSource( TIM_PERIPH_3, &updateSource ) );
    TEST_ASSERT_EQUAL( TIM_UPDATE_SOURCE_COUNTER, updateSource );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_UpdateSource( TIM_PERIPH_3, TIM_UPDATE_SOURCE_CNT ) );
}


/**
 * \brief   Auto-reload preload is enabled and disabled by ARPE bit.
 *
 * \details Activates auto-reload preload of TIM3, reads the state and deactivates it.
 *
 * \par Expected results
 * - CR1 = ARPE, state active.
 * - CR1 = 0 after deactivation.
 */
void Ut_Tim_Set_Autoreload_TogglesArpe( void )
{
    tim_FunctionState_t modeState = TIM_FUNCTION_INACTIVE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_AutoreloadActive( TIM_PERIPH_3 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_CR1_ARPE, TIM3->CR1 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_AutoreloadState( TIM_PERIPH_3, &modeState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_ACTIVE, modeState );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_AutoreloadInactive( TIM_PERIPH_3 ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CR1 );
}


/**
 * \brief   One pulse mode is enabled and disabled by OPM bit.
 *
 * \details Activates one pulse mode of TIM3, reads the state and deactivates it, then
 *          activates it for peripheral out of range.
 *
 * \par Expected results
 * - CR1 = OPM, state active.
 * - CR1 = 0 after deactivation.
 * - Peripheral out of range: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Set_OnePulseMode_TogglesOpm( void )
{
    tim_FunctionState_t modeState = TIM_FUNCTION_INACTIVE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_OnePulseModeActive( TIM_PERIPH_3 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_CR1_OPM, TIM3->CR1 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_OnePulseMode( TIM_PERIPH_3, &modeState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_ACTIVE, modeState );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_OnePulseModeInactive( TIM_PERIPH_3 ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CR1 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_OnePulseModeActive( TIM_PERIPH_CNT ) );
}

/* ========================== EVENT GENERATION ============================== */

/**
 * \brief   Software update event is written to UG bit.
 *
 * \details Generates update event of TIM3.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, EGR = UG.
 */
void Ut_Tim_Generate_Event_Update_WritesUg( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Generate_Event( TIM_PERIPH_3, TIM_EVENT_UPDATE ) );

    TEST_ASSERT_EQUAL_HEX32( TIM_EGR_UG, TIM3->EGR );
}


/**
 * \brief   Software break event of advanced timer is written to BG bit.
 *
 * \details Generates break event of TIM1.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, EGR = BG.
 */
void Ut_Tim_Generate_Event_Break_AdvancedTimer_WritesBg( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Generate_Event( TIM_PERIPH_1, TIM_EVENT_BREAK ) );

    TEST_ASSERT_EQUAL_HEX32( TIM_EGR_BG, TIM1->EGR );
}


/**
 * \brief   Software break 2 event of advanced timer is written to B2G bit.
 *
 * \details Generates break 2 event of TIM1.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, EGR = B2G.
 */
void Ut_Tim_Generate_Event_Break2_AdvancedTimer_WritesB2g( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Generate_Event( TIM_PERIPH_1, TIM_EVENT_BREAK2 ) );

    TEST_ASSERT_EQUAL_HEX32( TIM_EGR_B2G, TIM1->EGR );
}

/**
 * \brief   Event not available on the timer is rejected.
 *
 * \details Generates CC1 event of TIM6 (basic timer without channels), commutation and
 *          break 2 event of TIM3 (general purpose timer) and event out of range.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, EGR of TIM6 and TIM3 stays 0.
 */
void Ut_Tim_Generate_Event_NotAvailable_ReturnsErrorWithoutWrite( void )
{
    /* Basic timer has no channel, general purpose timer has no commutation / break */
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Generate_Event( TIM_PERIPH_6, TIM_EVENT_CC1 ) );
#endif /* TIM6 */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Generate_Event( TIM_PERIPH_3, TIM_EVENT_COMMUTATION ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Generate_Event( TIM_PERIPH_3, TIM_EVENT_BREAK2 ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Generate_Event( TIM_PERIPH_3, TIM_EVENT_CNT ) );

#if defined(TIM6)
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM6->EGR );
#endif /* TIM6 */
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->EGR );
}

/* ============================= CLOCK SOURCE =============================== */

/**
 * \brief   External clock from channel input selects trigger and external clock mode 1.
 *
 * \details Sets TIM3 clock source external channel input TI1FP1 and reads it back.
 *
 * \par Expected results
 * - SMCR.TS = TI1FP1, SMCR.SMS = external clock mode 1.
 * - Clock source reads back external channel input.
 */
void Ut_Tim_Set_ClockSource_ExternalInput_WritesTriggerAndSlaveMode( void )
{
    tim_ClockSource_t clockSource = TIM_CLOCKSOURCE_INT_CLK;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_ClockSource( TIM_PERIPH_3, TIM_CLOCKSOURCE_EXTERNAL_CH_IN, UT_TIM_GP_TRG_TI1FP1 ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_TS_TI1FP1,             TIM3->SMCR & TIM_SMCR_TS );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_CLOCKSOURCE_EXT_MODE1, TIM3->SMCR & TIM_SMCR_SMS );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_ClockSource( TIM_PERIPH_3, &clockSource ) );
    TEST_ASSERT_EQUAL( TIM_CLOCKSOURCE_EXTERNAL_CH_IN, clockSource );
}


/**
 * \brief   External clock from ETR enables external clock mode 2.
 *
 * \details Sets TIM3 clock source ETR and reads it back.
 *
 * \par Expected results
 * - SMCR = ECE, clock source reads back ETR.
 */
void Ut_Tim_Set_ClockSource_Etr_WritesEce( void )
{
    tim_ClockSource_t clockSource = TIM_CLOCKSOURCE_INT_CLK;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_ClockSource( TIM_PERIPH_3, TIM_CLOCKSOURCE_EXTERNAL_ETR, TIM_TRIGGER_INPUT_UNUSED ) );

    TEST_ASSERT_EQUAL_HEX32( TIM_SMCR_ECE, TIM3->SMCR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_ClockSource( TIM_PERIPH_3, &clockSource ) );
    TEST_ASSERT_EQUAL( TIM_CLOCKSOURCE_EXTERNAL_ETR, clockSource );
}


/**
 * \brief   Internal clock source clears external clock modes.
 *
 * \details SMCR = ECE | external clock mode 1, sets TIM3 internal clock source.
 *
 * \par Expected results
 * - SMCR = 0, clock source reads back internal.
 */
void Ut_Tim_Set_ClockSource_Internal_ClearsExternalClock( void )
{
    tim_ClockSource_t clockSource = TIM_CLOCKSOURCE_EXTERNAL_ETR;

    TIM3->SMCR = TIM_SMCR_ECE | LL_TIM_CLOCKSOURCE_EXT_MODE1;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_ClockSource( TIM_PERIPH_3, TIM_CLOCKSOURCE_INT_CLK, TIM_TRIGGER_INPUT_UNUSED ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SMCR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_ClockSource( TIM_PERIPH_3, &clockSource ) );
    TEST_ASSERT_EQUAL( TIM_CLOCKSOURCE_INT_CLK, clockSource );
}


/**
 * \brief   Basic timer rejects external clock sources.
 *
 * \details Sets TIM6 clock source ETR and external channel input.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in both cases, SMCR stays 0.
 */
void Ut_Tim_Set_ClockSource_NotAvailableOnBasicTimer_ReturnsErrorWithoutWrite( void )
{
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ClockSource( TIM_PERIPH_6, TIM_CLOCKSOURCE_EXTERNAL_ETR,   TIM_TRIGGER_INPUT_UNUSED ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ClockSource( TIM_PERIPH_6, TIM_CLOCKSOURCE_EXTERNAL_CH_IN, TIM_TRIGGER_INPUT_UNUSED ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM6->SMCR );
#else
    TEST_IGNORE_MESSAGE( "MCU without basic timer TIM6" );
#endif /* TIM6 */
}


/**
 * \brief   Tim_Set_ClockSource() rejects invalid arguments.
 *
 * \details Calls the function with invalid trigger input, invalid clock source and
 *          peripheral out of range.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, SMCR stays 0.
 */
void Ut_Tim_Set_ClockSource_InvalidArgs_ReturnsErrorWithoutWrite( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ClockSource( TIM_PERIPH_3, TIM_CLOCKSOURCE_EXTERNAL_CH_IN, UT_TIM_INVALID_TRIGGER ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ClockSource( TIM_PERIPH_3, TIM_CLOCKSOURCE_EXTERNAL_CH_IN, UT_TIM_FOREIGN_TRIGGER ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ClockSource( TIM_PERIPH_3, TIM_CLOCKSOURCE_EXTERNAL_CH_IN, TIM_TRIGGER_INPUT_UNUSED ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ClockSource( TIM_PERIPH_3, (tim_ClockSource_t)TIM_SMCR_SMS_0, TIM_TRIGGER_INPUT_UNUSED ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ClockSource( TIM_PERIPH_CNT, TIM_CLOCKSOURCE_INT_CLK, TIM_TRIGGER_INPUT_UNUSED ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SMCR );
}


/**
 * \brief   Clock source of running timer cannot be changed.
 *
 * \details CR1 = CEN, sets TIM3 clock source ETR.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, SMCR stays 0.
 */
void Ut_Tim_Set_ClockSource_RunningTimer_ReturnsErrorWithoutWrite( void )
{
    TIM3->CR1 = TIM_CR1_CEN;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ClockSource( TIM_PERIPH_3, TIM_CLOCKSOURCE_EXTERNAL_ETR, TIM_TRIGGER_INPUT_UNUSED ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SMCR );
}

/* ============================ INITIALIZATION ============================== */

/**
 * \brief   Tim_Init() configures time base of the timer.
 *
 * \details TIM3 configuration with counter frequency 10 MHz and refresh frequency 1 MHz,
 *          kernel clock 250 MHz, clock inactive in RCC.
 *
 * \par Expected results
 * - Clock enabled, TIM_REQUEST_OK.
 * - PSC = 24, ARR = 9, CR1 = 0 (up-counting, update event enabled, no preload,
 *   stopped), SMCR = 0, CR2 = 0, CCER = 0.
 * - EGR = UG (prescaler loaded), update flag cleared.
 */
void Ut_Tim_Init_DefaultConfig_ConfiguresTimeBase( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_3 );

    Ut_Tim_Expect_ClockActivation( RCC_PERIPH_TIM3 );
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Init( &config ) );

    /* 250 MHz / 25 = 10 MHz counter clock, 10 MHz / 10 = 1 MHz refresh */
    TEST_ASSERT_EQUAL_HEX32( UT_TIM_PSC_10MHZ, TIM3->PSC );
    TEST_ASSERT_EQUAL_HEX32( 9u, TIM3->ARR );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CR1 );     /* Up-counting, UEV enabled, no preload, stopped */
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SMCR );    /* Internal clock, slave mode disabled */
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CR2 );     /* TRGO = reset */
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCER );    /* No channel used */

    /* Update event loads preloaded prescaler, its update flag is cleared */
    TEST_ASSERT_EQUAL_HEX32( TIM_EGR_UG, TIM3->EGR );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SR & TIM_SR_UIF );
}


/**
 * \brief   Tim_Init() does not enable already active clock.
 *
 * \details RCC mock reports active clock of TIM3.
 *
 * \par Expected results
 * - Rcc_Set_PeriphActive() is not called, TIM_REQUEST_OK.
 */
void Ut_Tim_Init_ClockAlreadyActive_DoesNotReactivate( void )
{
    static rcc_FunctionState_t clockState = RCC_FUNCTION_ACTIVE;
    tim_PeriphConfig_t         config     = Ut_Tim_Get_Config( TIM_PERIPH_3 );

    Rcc_Get_PeriphState_ExpectAndReturn( RCC_PERIPH_TIM3, NULL, RCC_REQUEST_OK );
    Rcc_Get_PeriphState_IgnoreArg_funcState();
    Rcc_Get_PeriphState_ReturnThruPtr_funcState( &clockState );
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Init( &config ) );
}


/**
 * \brief   Tim_Init() configures PWM channel and its pin.
 *
 * \details TIM3 channel 1 active in PWM mode with pin PA6.
 *
 * \par Expected results
 * - Gpio_Init() called for PA6 with alternate function 2, TIM_REQUEST_OK.
 * - Channel 1 in PWM mode 1 with preload, CCER = CC1E, CCR1 = 0.
 * - BDTR stays 0 (general purpose timer without main output).
 */
void Ut_Tim_Init_PwmChannelWithPin_ConfiguresPinAndChannel( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_3 );

    config.ChannelConfig[ TIM_CHANNEL_1 ].ChannelState = TIM_FUNCTION_ACTIVE;
    config.ChannelConfig[ TIM_CHANNEL_1 ].ChannelMode  = TIM_CHANNEL_MODE_OUTPUT_PWM;
    config.ChannelConfig[ TIM_CHANNEL_1 ].IoPin        = TIM_3_CH1_PA6;

    Ut_Tim_Expect_ClockActivation( RCC_PERIPH_TIM3 );
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );
    Ut_Tim_Expect_GpioInit( GPIO_PORT_A, GPIO_PIN_ID_6, GPIO_ALT_FUNC_2, GPIO_REQUEST_OK );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Init( &config ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_OCMODE_PWM1, LL_TIM_OC_GetMode( TIM3, LL_TIM_CHANNEL_CH1 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_CCMR1_OC1PE, TIM3->CCMR1 & TIM_CCMR1_OC1PE );
    TEST_ASSERT_EQUAL_HEX32( TIM_CCER_CC1E, TIM3->CCER );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCR1 );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->BDTR );    /* General purpose timer has no main output */
}


/**
 * \brief   Tim_Init() rejects NULL configuration.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, no RCC / GPIO / NVIC call (strict mocks).
 */
void Ut_Tim_Init_NullConfig_ReturnsErrorWithoutAccess( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Init( NULL ) );
}


/**
 * \brief   Tim_Init() rejects peripheral out of range.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, no RCC / GPIO / NVIC call (strict mocks).
 */
void Ut_Tim_Init_InvalidPeriph_ReturnsErrorWithoutAccess( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_CNT );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Init( &config ) );
}


/**
 * \brief   Tim_Init() stops on RCC error.
 *
 * \details RCC mock returns error when clock state is read.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, PSC and ARR stay 0.
 */
void Ut_Tim_Init_RccError_ReturnsErrorWithoutRegisterAccess( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_3 );

    Rcc_Get_PeriphState_ExpectAndReturn( RCC_PERIPH_TIM3, NULL, RCC_REQUEST_ERROR );
    Rcc_Get_PeriphState_IgnoreArg_funcState();

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Init( &config ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->PSC );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->ARR );
}


/**
 * \brief   Tim_Init() rejects running timer.
 *
 * \details CR1 = CEN, ARR = 0x1234, initializes TIM3.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, ARR unchanged.
 */
void Ut_Tim_Init_RunningTimer_ReturnsErrorWithoutReconfiguration( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_3 );

    TIM3->CR1 = TIM_CR1_CEN;
    TIM3->ARR = 0x1234u;
    Ut_Tim_Expect_ClockActivation( RCC_PERIPH_TIM3 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Init( &config ) );

    TEST_ASSERT_EQUAL_HEX32( 0x1234u, TIM3->ARR );
}


/**
 * \brief   Tim_Init() stops on channel pin configuration error.
 *
 * \details TIM3 channel 1 in PWM mode with pin PA6, GPIO mock returns error.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, CCMR1 and CCER stay 0.
 */
void Ut_Tim_Init_ChannelPinError_ReturnsErrorWithoutChannelConfiguration( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_3 );

    config.ChannelConfig[ TIM_CHANNEL_1 ].ChannelState = TIM_FUNCTION_ACTIVE;
    config.ChannelConfig[ TIM_CHANNEL_1 ].ChannelMode  = TIM_CHANNEL_MODE_OUTPUT_PWM;
    config.ChannelConfig[ TIM_CHANNEL_1 ].IoPin        = TIM_3_CH1_PA6;

    Ut_Tim_Expect_ClockActivation( RCC_PERIPH_TIM3 );
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );
    Ut_Tim_Expect_GpioInit( GPIO_PORT_A, GPIO_PIN_ID_6, GPIO_ALT_FUNC_2, GPIO_REQUEST_ERROR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Init( &config ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCMR1 );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCER );
}


/**
 * \brief   Tim_InitBase() rejects refresh frequency above counter frequency.
 *
 * \details Counter frequency 1 kHz, refresh frequency 2 kHz.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, ARR stays 0, no update event (EGR = 0).
 */
void Ut_Tim_InitBase_RefreshFrequencyAboveTimerFrequency_ReturnsError( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_3 );

    config.TimerFrequency   = 1000u;
    config.RefreshFrequency = 2000u;
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_InitBase( &config ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->ARR );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->EGR );     /* No update event on error */
}


/**
 * \brief   Tim_InitBase() with ETR clock configures free running counter.
 *
 * \details PSC preset 0x55, TIM3 configuration with clock source ETR.
 *
 * \par Expected results
 * - TIM_REQUEST_OK without RCC call (counter counts ETR edges).
 * - SMCR = ECE, PSC = 0, ARR = 0xFFFF.
 */
void Ut_Tim_InitBase_ExternalEtrClock_FreeRunningCounter( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_3 );

    TIM3->PSC          = 0x55u;
    config.ClockSource = TIM_CLOCKSOURCE_EXTERNAL_ETR;

    /* No RCC clock request - counter counts ETR edges */
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_InitBase( &config ) );

    TEST_ASSERT_EQUAL_HEX32( TIM_SMCR_ECE, TIM3->SMCR );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->PSC );
    TEST_ASSERT_EQUAL_HEX32( UT_TIM_16BIT_MAX, TIM3->ARR );
}


/**
 * \brief   Tim_InitBase() configures slave mode, master trigger and counter options.
 *
 * \details TIM3 in reset slave mode with trigger TI2FP2, master trigger update,
 *          down-counting and auto-reload preload.
 *
 * \par Expected results
 * - SMCR.TS = TI2FP2, SMCR.SMS = reset mode, CR2.MMS = update.
 * - CR1 = DIR | ARPE.
 */
void Ut_Tim_InitBase_SlaveModeAndMasterTrigger_ConfiguresSynchronization( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_3 );

    config.SlaveMode         = TIM_SLAVE_MODE_RESET;
    config.SlaveTriggerInput = UT_TIM_GP_TRG_TI2FP2;
    config.MasterTrigger     = TIM_MASTER_TRIGGER_UPDATE;
    config.CounterDirection  = TIM_COUNTER_DIR_DOWN;
    config.AutoreloadPreloadState = TIM_FUNCTION_ACTIVE;
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_InitBase( &config ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_TS_TI2FP2,         TIM3->SMCR & TIM_SMCR_TS );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_SLAVEMODE_RESET, TIM3->SMCR & TIM_SMCR_SMS );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_TRGO_UPDATE,     TIM3->CR2 & TIM_CR2_MMS );
    TEST_ASSERT_EQUAL_HEX32( TIM_CR1_DIR | TIM_CR1_ARPE, TIM3->CR1 );
}


/**
 * \brief   Slave mode cannot be combined with external clock mode 1.
 *
 * \details TIM3 with clock source external channel input and gated slave mode (both
 *          use SMS field).
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, ARR stays 0.
 */
void Ut_Tim_InitBase_SlaveModeWithExternalInputClock_ReturnsError( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_3 );

    /* Slave mode and external clock mode 1 share SMS field */
    config.ClockSource       = TIM_CLOCKSOURCE_EXTERNAL_CH_IN;
    config.ExtClockSource    = UT_TIM_GP_TRG_TI1FP1;
    config.SlaveMode         = TIM_SLAVE_MODE_GATED;
    config.SlaveTriggerInput = UT_TIM_GP_TRG_TI2FP2;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_InitBase( &config ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->ARR );
}


/**
 * \brief   Tim_InitBase() rejects invalid configurations.
 *
 * \details Calls the function with counter direction out of range, invalid update
 *          event state, invalid auto-reload preload state and NULL configuration.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, CR1 and SMCR stay 0.
 */
void Ut_Tim_InitBase_InvalidConfig_ReturnsErrorWithoutWrite( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_3 );

    config.CounterDirection = TIM_COUNTER_DIR_CNT;
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_InitBase( &config ) );

    config = Ut_Tim_Get_Config( TIM_PERIPH_3 );
    config.UpdateEventState = (tim_FunctionState_t)( TIM_FUNCTION_ACTIVE + 1u );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_InitBase( &config ) );

    config = Ut_Tim_Get_Config( TIM_PERIPH_3 );
    config.AutoreloadPreloadState = (tim_FunctionState_t)( TIM_FUNCTION_ACTIVE + 1u );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_InitBase( &config ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_InitBase( NULL ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CR1 );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SMCR );
}


/**
 * \brief   Tim_InitBase() configures break, break 2 and ETR pins.
 *
 * \details TIM1 with break input pin PA6 (polarity high), break input 2 pin PE6
 *          (polarity high) and ETR pin PA12. BDTR preset 0 (breaks active low).
 *
 * \par Expected results
 * - Gpio_Init() called for PA6, PE6 and PA12 with alternate function 1.
 * - TIM_REQUEST_OK, BDTR.BKP and BDTR.BK2P = polarity high (STM32F7 single break
 *   polarities).
 */
void Ut_Tim_InitBase_BreakAndEtrPins_ConfiguresGpioAndBreakPolarity( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_1 );

    config.BreakInPin          = TIM_1_BKIN_PA6;
    config.BreakInPinPolarity  = TIM_POLARITY_HIGH;
    config.BreakIn2Pin         = TIM_1_BKIN2_PE6;
    config.BreakIn2PinPolarity = TIM_POLARITY_HIGH;
    config.TriggerEventPin     = TIM_1_ETR_PA12;

    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_CLK_HZ );
    Ut_Tim_Expect_GpioInit( GPIO_PORT_A, GPIO_PIN_ID_6,  GPIO_ALT_FUNC_1, GPIO_REQUEST_OK );
    Ut_Tim_Expect_GpioInit( GPIO_PORT_E, GPIO_PIN_ID_6,  GPIO_ALT_FUNC_1, GPIO_REQUEST_OK );
    Ut_Tim_Expect_GpioInit( GPIO_PORT_A, GPIO_PIN_ID_12, GPIO_ALT_FUNC_1, GPIO_REQUEST_OK );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_InitBase( &config ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_BREAK_POLARITY_HIGH | LL_TIM_BREAK2_POLARITY_HIGH, TIM1->BDTR & ( TIM_BDTR_BKP | TIM_BDTR_BK2P ) );
}


/**
 * \brief   Tim_InitBase() stops on break input 2 pin error.
 *
 * \details TIM1 with break input 2 pin PE6 (polarity high), Gpio_Init() of the pin
 *          fails. ETR pin PA12 configured.
 *
 * \par Expected results
 * - Gpio_Init() called for PE6 only (no ETR pin configuration), TIM_REQUEST_ERROR.
 * - BDTR.BK2P stays 0.
 */
void Ut_Tim_InitBase_Break2PinGpioError_ReturnsError( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_1 );

    config.BreakIn2Pin         = TIM_1_BKIN2_PE6;
    config.BreakIn2PinPolarity = TIM_POLARITY_HIGH;
    config.TriggerEventPin     = TIM_1_ETR_PA12;

    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_CLK_HZ );
    Ut_Tim_Expect_GpioInit( GPIO_PORT_E, GPIO_PIN_ID_6, GPIO_ALT_FUNC_1, GPIO_REQUEST_ERROR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_InitBase( &config ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->BDTR & TIM_BDTR_BK2P );
}


/**
 * \brief   Pin of other timer in the configuration is ignored.
 *
 * \details TIM3 configuration with ETR pin of TIM1.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, Gpio_Init() is not called.
 */
void Ut_Tim_InitBase_PinOfOtherTimer_IsIgnored( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_3 );

    /* ETR pin of TIM1 in configuration of TIM3 - no GPIO configuration expected */
    config.TriggerEventPin = TIM_1_ETR_PA12;
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_InitBase( &config ) );
}


/**
 * \brief   Master trigger is rejected on timer without master mode.
 *
 * \details TIM9 (slave mode controller only) configuration with master trigger update.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR without RCC call (strict mock), CR2 stays 0.
 */
void Ut_Tim_InitBase_MasterTriggerOnTimerWithoutMasterMode_ReturnsError( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_9 );

    config.MasterTrigger = TIM_MASTER_TRIGGER_UPDATE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_InitBase( &config ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM9->CR2 );
}

/* =========================== DE-INITIALIZATION ============================ */

/**
 * \brief   Tim_Deinit() of advanced timer disables all interrupt lines and resets the
 *          peripheral.
 *
 * \details CR1 = CEN, CNT = 0x100, deinitializes TIM1.
 *
 * \par Expected results
 * - NVIC lines update, CC, trigger / commutation and break disabled, peripheral reset
 *   activated and released, clock disabled.
 * - TIM_REQUEST_OK, counter stopped, CNT = 0.
 */
void Ut_Tim_Deinit_AdvancedTimer_DeactivatesAllIrqLinesAndResetsPeripheral( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_1 );

    TIM1->CR1 = TIM_CR1_CEN;
    TIM1->CNT = 0x100u;
    Ut_Tim_Expect_Deinit( RCC_PERIPH_TIM1, utTim_Tim1IrqLines );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Deinit( &config ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->CR1 );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->CNT );
}


/**
 * \brief   Tim_Deinit() removes user callbacks.
 *
 * \details TIM3 with update callback and update interrupt is deinitialized, then ISR
 *          is called with update flag (interrupt enable kept in emulated registers).
 *
 * \par Expected results
 * - TIM_REQUEST_OK, update callback is not called.
 */
void Ut_Tim_Deinit_RemovesUserCallbacks( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_3 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_UpdateCallback( TIM_PERIPH_3, Ut_Tim_UpdateCallback ) );
    Ut_Tim_Enable_Irq( TIM_PERIPH_3, TIM_IRQ_UPDATE, NVIC_PERIPH_IRQ_TIM3 );
    Ut_Tim_Expect_Deinit( RCC_PERIPH_TIM3, utTim_Tim3IrqLines );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Deinit( &config ) );

    /* Interrupt still enabled in emulated registers (RCC reset is mocked) */
    TIM3->SR = TIM_SR_UIF;
    utTim_Isr[ NVIC_PERIPH_IRQ_TIM3 ]();

    TEST_ASSERT_EQUAL_UINT32( 0u, utTim_UpdateCnt );
}


/**
 * \brief   Tim_Deinit() executes all steps despite NVIC error.
 *
 * \details NVIC mock returns error for the first interrupt line of TIM3.
 *
 * \par Expected results
 * - All interrupt lines disabled, peripheral reset and clock disabled.
 * - TIM_REQUEST_ERROR.
 */
void Ut_Tim_Deinit_NvicError_ExecutesAllStepsAndReturnsError( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_3 );

    Nvic_Set_PeriphIrq_Inactive_ExpectAndReturn( NVIC_PERIPH_IRQ_TIM3, NVIC_REQUEST_ERROR );
    Nvic_Set_PeriphIrq_Inactive_ExpectAndReturn( NVIC_PERIPH_IRQ_TIM3, NVIC_REQUEST_OK );
    Nvic_Set_PeriphIrq_Inactive_ExpectAndReturn( NVIC_PERIPH_IRQ_TIM3, NVIC_REQUEST_OK );
    Nvic_Set_PeriphIrq_Inactive_ExpectAndReturn( NVIC_PERIPH_IRQ_TIM3, NVIC_REQUEST_OK );
    Rcc_Set_ResetActive_ExpectAndReturn( RCC_PERIPH_TIM3, RCC_REQUEST_OK );
    Rcc_Set_ResetInactive_ExpectAndReturn( RCC_PERIPH_TIM3, RCC_REQUEST_OK );
    Rcc_Set_PeriphInactive_ExpectAndReturn( RCC_PERIPH_TIM3, RCC_REQUEST_OK );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Deinit( &config ) );
}


/**
 * \brief   Tim_Deinit() rejects invalid arguments.
 *
 * \details Calls the function with NULL and with peripheral out of range.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in both cases, no RCC / NVIC call (strict mocks).
 */
void Ut_Tim_Deinit_InvalidArgs_ReturnsErrorWithoutAccess( void )
{
    tim_PeriphConfig_t config = Ut_Tim_Get_Config( TIM_PERIPH_CNT );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Deinit( NULL ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Deinit( &config ) );
}

/* ============================== INTERRUPTS ================================ */

/**
 * \brief   Tim_Set_IrqActive() registers ISR and enables interrupt.
 *
 * \details Enables update interrupt of TIM3 (global interrupt line).
 *
 * \par Expected results
 * - ISR registered and TIM3 NVIC line enabled, TIM_REQUEST_OK.
 * - DIER = UIE, update flag cleared (pending request discarded).
 */
void Ut_Tim_Set_IrqActive_GlobalLine_RegistersIsrAndEnablesInterrupt( void )
{
    Nvic_Set_PeriphIrq_Active_ExpectAndReturn( NVIC_PERIPH_IRQ_TIM3, NVIC_REQUEST_OK );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_IrqActive( TIM_PERIPH_3, TIM_IRQ_UPDATE ) );

    TEST_ASSERT_NOT_NULL( utTim_Isr[ NVIC_PERIPH_IRQ_TIM3 ] );
    TEST_ASSERT_EQUAL_HEX32( TIM_DIER_UIE, TIM3->DIER );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SR & TIM_SR_UIF );   /* Pending request discarded */
}


/**
 * \brief   Advanced timer uses dedicated interrupt line.
 *
 * \details Enables CC1 interrupt of TIM1.
 *
 * \par Expected results
 * - ISR registered and TIM1_CC NVIC line enabled, DIER = CC1IE.
 */
void Ut_Tim_Set_IrqActive_AdvancedTimer_UsesDedicatedLine( void )
{
    Nvic_Set_PeriphIrq_Active_ExpectAndReturn( NVIC_PERIPH_IRQ_TIM1_CC, NVIC_REQUEST_OK );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_IrqActive( TIM_PERIPH_1, TIM_IRQ_CAPTURE_COMPARE_CH1 ) );

    TEST_ASSERT_NOT_NULL( utTim_Isr[ NVIC_PERIPH_IRQ_TIM1_CC ] );
    TEST_ASSERT_EQUAL_HEX32( TIM_DIER_CC1IE, TIM1->DIER );
}


/**
 * \brief   Interrupts not existing on STM32F7 timers are rejected.
 *
 * \details Enables index, direction, system break and encoder error interrupt of TIM1.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, no ISR registered, no NVIC call (strict mock),
 *   DIER stays 0.
 */
void Ut_Tim_Set_IrqActive_NotAvailableOnStm32F7_ReturnsError( void )
{
    tim_FlagState_t     flagState = TIM_FLAG_INACTIVE;
    tim_FunctionState_t irqState  = TIM_FUNCTION_INACTIVE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_IrqActive( TIM_PERIPH_1, TIM_IRQ_INDEX ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_IrqActive( TIM_PERIPH_1, TIM_IRQ_DIRECTION ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_IrqActive( TIM_PERIPH_1, TIM_IRQ_SYSTEM_BREAK ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_IrqActive( TIM_PERIPH_1, TIM_IRQ_ERROR ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_IrqInactive( TIM_PERIPH_1, TIM_IRQ_ERROR ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_IrqState( TIM_PERIPH_1, TIM_IRQ_INDEX, &irqState ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_Flag( TIM_PERIPH_1, TIM_IRQ_DIRECTION, &flagState ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Clear_Flag( TIM_PERIPH_1, TIM_IRQ_SYSTEM_BREAK ) );

    /* Break 2 interrupt exists on advanced timers only */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_IrqActive( TIM_PERIPH_3, TIM_IRQ_BREAK2 ) );

    TEST_ASSERT_EQUAL_UINT32( 0u, utTim_HandlerCallCnt );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->DIER );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->DIER );
}


/**
 * \brief   Interrupt not available on the timer is rejected.
 *
 * \details Enables CC1 interrupt of TIM6, break and commutation interrupt of TIM3,
 *          interrupt out of range and interrupt of peripheral out of range.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, no ISR registered, DIER of TIM3 and TIM6 stays 0.
 */
void Ut_Tim_Set_IrqActive_NotAvailable_ReturnsErrorWithoutNvicAccess( void )
{
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_IrqActive( TIM_PERIPH_6, TIM_IRQ_CAPTURE_COMPARE_CH1 ) );
#endif /* TIM6 */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_IrqActive( TIM_PERIPH_3, TIM_IRQ_BREAK ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_IrqActive( TIM_PERIPH_3, TIM_IRQ_COMMUTATION ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_IrqActive( TIM_PERIPH_3, TIM_IRQ_CNT ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_IrqActive( TIM_PERIPH_CNT, TIM_IRQ_UPDATE ) );

    TEST_ASSERT_EQUAL_UINT32( 0u, utTim_HandlerCallCnt );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->DIER );
#if defined(TIM6)
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM6->DIER );
#endif /* TIM6 */
}


/**
 * \brief   NVIC error of interrupt activation is reported.
 *
 * \details NVIC mock returns error on line enabling of TIM3.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, DIER stays 0.
 */
void Ut_Tim_Set_IrqActive_NvicError_ReturnsErrorWithoutEnable( void )
{
    Nvic_Set_PeriphIrq_Active_ExpectAndReturn( NVIC_PERIPH_IRQ_TIM3, NVIC_REQUEST_ERROR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_IrqActive( TIM_PERIPH_3, TIM_IRQ_UPDATE ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->DIER );
}


/**
 * \brief   Tim_Set_IrqInactive() disables only the requested interrupt.
 *
 * \details DIER = UIE | CC2IE. Reads CC2 interrupt state, disables it and reads the
 *          state again.
 *
 * \par Expected results
 * - State active before, inactive after disabling.
 * - DIER = UIE (update interrupt kept).
 */
void Ut_Tim_Set_IrqInactive_DisablesInterrupt( void )
{
    tim_FunctionState_t irqState = TIM_FUNCTION_INACTIVE;

    TIM3->DIER = TIM_DIER_UIE | TIM_DIER_CC2IE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_IrqState( TIM_PERIPH_3, TIM_IRQ_CAPTURE_COMPARE_CH2, &irqState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_ACTIVE, irqState );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_IrqInactive( TIM_PERIPH_3, TIM_IRQ_CAPTURE_COMPARE_CH2 ) );

    TEST_ASSERT_EQUAL_HEX32( TIM_DIER_UIE, TIM3->DIER );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_IrqState( TIM_PERIPH_3, TIM_IRQ_CAPTURE_COMPARE_CH2, &irqState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_INACTIVE, irqState );
}


/**
 * \brief   Interrupt state functions reject invalid arguments.
 *
 * \details Calls state getter with NULL pointer and with trigger interrupt of TIM6,
 *          disables interrupt out of range.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases.
 */
void Ut_Tim_Get_IrqState_InvalidArgs_ReturnsError( void )
{
    tim_FunctionState_t irqState = TIM_FUNCTION_INACTIVE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_IrqState( TIM_PERIPH_3, TIM_IRQ_UPDATE, NULL ) );
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_IrqState( TIM_PERIPH_6, TIM_IRQ_TRIGGER, &irqState ) );
#endif /* TIM6 */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_IrqInactive( TIM_PERIPH_3, TIM_IRQ_CNT ) );
}


/**
 * \brief   Interrupt priority of advanced timer is set on all its lines.
 *
 * \details Sets priority 5 of TIM1.
 *
 * \par Expected results
 * - NVIC priority 5 set for update, CC, trigger / commutation and break line.
 * - TIM_REQUEST_OK.
 */
void Ut_Tim_Set_IrqPriority_AdvancedTimer_SetsAllLines( void )
{
    for( uint32_t lineIdx = 0u; UT_TIM_IRQ_LINE_CNT > lineIdx; lineIdx++ )
    {
        Nvic_Set_PeriphIrq_Prio_ExpectAndReturn( utTim_Tim1IrqLines[ lineIdx ], UT_TIM_PRIO, NVIC_REQUEST_OK );
    }

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_IrqPriority( TIM_PERIPH_1, UT_TIM_PRIO ) );
}


/**
 * \brief   NVIC error of priority setting is reported.
 *
 * \details NVIC mock returns error for TIM3 line, then priority of peripheral out of
 *          range is set.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in both cases, no further NVIC call after the error.
 */
void Ut_Tim_Set_IrqPriority_NvicError_StopsAndReturnsError( void )
{
    Nvic_Set_PeriphIrq_Prio_ExpectAndReturn( NVIC_PERIPH_IRQ_TIM3, UT_TIM_PRIO, NVIC_REQUEST_ERROR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_IrqPriority( TIM_PERIPH_3, UT_TIM_PRIO ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_IrqPriority( TIM_PERIPH_CNT, UT_TIM_PRIO ) );
}


/**
 * \brief   Interrupt priority is read from update line.
 *
 * \details NVIC mock returns priority 5 of TIM1_UP_TIM10 line, then the getter is called
 *          with NULL pointer.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, priority 5.
 * - NULL pointer: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Get_IrqPriority_ReadsUpdateLinePriority( void )
{
    static nvic_IrqPrio_t nvicPrio = UT_TIM_PRIO;
    tim_IrqPrio_t         irqPrio  = 0u;

    Nvic_Get_PeriphIrq_Prio_ExpectAndReturn( NVIC_PERIPH_IRQ_TIM1_UP_TIM10, NULL, NVIC_REQUEST_OK );
    Nvic_Get_PeriphIrq_Prio_IgnoreArg_irqPrio();
    Nvic_Get_PeriphIrq_Prio_ReturnThruPtr_irqPrio( &nvicPrio );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_IrqPriority( TIM_PERIPH_1, &irqPrio ) );
    TEST_ASSERT_EQUAL_UINT32( UT_TIM_PRIO, irqPrio );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_IrqPriority( TIM_PERIPH_1, NULL ) );
}


/**
 * \brief   Tim_Get_Flag() reads status flag.
 *
 * \details SR = UIF. Reads update and CC1 flag of TIM3, then calls the function with
 *          NULL pointer and CC1 flag of TIM6.
 *
 * \par Expected results
 * - Update flag active, CC1 flag inactive.
 * - NULL pointer, flag not available: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Get_Flag_ReadsStatusFlag( void )
{
    tim_FlagState_t flagState = TIM_FLAG_INACTIVE;

    TIM3->SR = TIM_SR_UIF;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Flag( TIM_PERIPH_3, TIM_IRQ_UPDATE, &flagState ) );
    TEST_ASSERT_EQUAL( TIM_FLAG_ACTIVE, flagState );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Flag( TIM_PERIPH_3, TIM_IRQ_CAPTURE_COMPARE_CH1, &flagState ) );
    TEST_ASSERT_EQUAL( TIM_FLAG_INACTIVE, flagState );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_Flag( TIM_PERIPH_3, TIM_IRQ_UPDATE, NULL ) );
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_Flag( TIM_PERIPH_6, TIM_IRQ_CAPTURE_COMPARE_CH1, &flagState ) );
#endif /* TIM6 */
}


/**
 * \brief   Tim_Clear_Flag() writes zero only to the flag bit.
 *
 * \details SR = UIF | CC1IF, clears CC1 flag of TIM3, then clears break flag of TIM3.
 *
 * \par Expected results
 * - SR = ~CC1IF (rc_w0 register - only the flag bit written with 0).
 * - Break flag of TIM3: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Clear_Flag_WritesZeroToFlag( void )
{
    TIM3->SR = TIM_SR_UIF | TIM_SR_CC1IF;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Clear_Flag( TIM_PERIPH_3, TIM_IRQ_CAPTURE_COMPARE_CH1 ) );

    /* rc_w0: only the flag bit is written with 0 */
    TEST_ASSERT_EQUAL_HEX32( ~TIM_SR_CC1IF, TIM3->SR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Clear_Flag( TIM_PERIPH_3, TIM_IRQ_BREAK ) );
}

/* =========================== INTERRUPT HANDLING =========================== */

/**
 * \brief   Update interrupt calls the update callback and clears the flag.
 *
 * \details TIM3 with update callback and update interrupt, ISR called with UIF.
 *
 * \par Expected results
 * - Update callback called once, UIF cleared.
 */
void Ut_Tim_Isr_Update_CallsCallbackAndClearsFlag( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_UpdateCallback( TIM_PERIPH_3, Ut_Tim_UpdateCallback ) );
    Ut_Tim_Enable_Irq( TIM_PERIPH_3, TIM_IRQ_UPDATE, NVIC_PERIPH_IRQ_TIM3 );

    TIM3->SR = TIM_SR_UIF;
    utTim_Isr[ NVIC_PERIPH_IRQ_TIM3 ]();

    TEST_ASSERT_EQUAL_UINT32( 1u, utTim_UpdateCnt );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SR & TIM_SR_UIF );
}


/**
 * \brief   Capture / compare interrupt reports over-capture to the callback.
 *
 * \details TIM3 with CC2 callback and CC2 interrupt. ISR called with CC2IF | CC2OF,
 *          then with CC2IF only.
 *
 * \par Expected results
 * - 1st call: callback 1x with over-capture active, CC2IF cleared.
 * - 2nd call: callback 2x, over-capture inactive.
 */
void Ut_Tim_Isr_CaptureCompare_ReportsOvercapture( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CaptureCompareCallback( TIM_PERIPH_3, TIM_CHANNEL_2, Ut_Tim_CaptureCompareCallback ) );
    Ut_Tim_Enable_Irq( TIM_PERIPH_3, TIM_IRQ_CAPTURE_COMPARE_CH2, NVIC_PERIPH_IRQ_TIM3 );

    TIM3->SR = TIM_SR_CC2IF | TIM_SR_CC2OF;
    utTim_Isr[ NVIC_PERIPH_IRQ_TIM3 ]();

    TEST_ASSERT_EQUAL_UINT32( 1u, utTim_CaptureCompareCnt );
    TEST_ASSERT_EQUAL( TIM_OVERCAPTURE_ACTIVE, utTim_LastOvercapture );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SR & TIM_SR_CC2IF );

    TIM3->SR = TIM_SR_CC2IF;
    utTim_Isr[ NVIC_PERIPH_IRQ_TIM3 ]();

    TEST_ASSERT_EQUAL_UINT32( 2u, utTim_CaptureCompareCnt );
    TEST_ASSERT_EQUAL( TIM_OVERCAPTURE_INACTIVE, utTim_LastOvercapture );
}


/**
 * \brief   Shared NVIC line processes interrupts of both timers.
 *
 * \details TIM1 break callback with break interrupt and TIM9 update callback with update
 *          interrupt (both on line TIM1_BRK_TIM9). ISR of the line called with TIM1 BIF
 *          and TIM9 UIF.
 *
 * \par Expected results
 * - Same ISR registered for both interrupts.
 * - Break and update callback called once each, both flags cleared (TIM1 SR keeps
 *   the last clear write of the break line - break 2 flag is checked after the
 *   break flag, emulated SR is not write-0-to-clear).
 */
void Ut_Tim_Isr_SharedLine_ProcessesBothTimers( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_BreakCallback( TIM_PERIPH_1, Ut_Tim_BreakCallback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_UpdateCallback( TIM_PERIPH_9, Ut_Tim_UpdateCallback ) );
    Ut_Tim_Enable_Irq( TIM_PERIPH_1, TIM_IRQ_BREAK,  NVIC_PERIPH_IRQ_TIM1_BRK_TIM9 );
    Ut_Tim_Enable_Irq( TIM_PERIPH_9, TIM_IRQ_UPDATE, NVIC_PERIPH_IRQ_TIM1_BRK_TIM9 );

    TEST_ASSERT_EQUAL_UINT32( 2u, utTim_HandlerCallCnt );

    TIM1->SR = TIM_SR_BIF;
    TIM9->SR = TIM_SR_UIF;
    utTim_Isr[ NVIC_PERIPH_IRQ_TIM1_BRK_TIM9 ]();

    TEST_ASSERT_EQUAL_UINT32( 1u, utTim_BreakCnt );
    TEST_ASSERT_EQUAL_UINT32( 1u, utTim_UpdateCnt );
    TEST_ASSERT_EQUAL_HEX32( ~TIM_SR_B2IF, TIM1->SR );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM9->SR & TIM_SR_UIF );
}


/**
 * \brief   Timer with global interrupt on shared line uses the line of the advanced timer.
 *
 * \details Enables CC1 interrupt of TIM11 (line TIM1_TRG_COM_TIM11) with CC1 callback,
 *          ISR called with CC1IF.
 *
 * \par Expected results
 * - TIM1_TRG_COM_TIM11 NVIC line enabled, DIER = CC1IE.
 * - Capture / compare callback called once.
 */
void Ut_Tim_Isr_Tim11_UsesTim1TriggerLine( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CaptureCompareCallback( TIM_PERIPH_11, TIM_CHANNEL_1, Ut_Tim_CaptureCompareCallback ) );
    Ut_Tim_Enable_Irq( TIM_PERIPH_11, TIM_IRQ_CAPTURE_COMPARE_CH1, NVIC_PERIPH_IRQ_TIM1_TRG_COM_TIM11 );

    TEST_ASSERT_EQUAL_HEX32( TIM_DIER_CC1IE, TIM11->DIER );

    TIM11->SR = TIM_SR_CC1IF;
    utTim_Isr[ NVIC_PERIPH_IRQ_TIM1_TRG_COM_TIM11 ]();

    TEST_ASSERT_EQUAL_UINT32( 1u, utTim_CaptureCompareCnt );
}


/**
 * \brief   Break interrupt line calls break and break 2 callbacks.
 *
 * \details TIM1 with break and break 2 callbacks and break interrupt (shared enable
 *          bit), system break callback is rejected (not available on STM32F7). ISR of
 *          break line called with BIF | B2IF.
 *
 * \par Expected results
 * - System break callback: TIM_REQUEST_ERROR.
 * - Break and break 2 callback called once each, SR = ~B2IF (last clear write,
 *   emulated SR is not write-0-to-clear).
 */
void Ut_Tim_Isr_BreakLine_CallsBreakAndBreak2Callbacks( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_BreakCallback( TIM_PERIPH_1, Ut_Tim_BreakCallback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Break2Callback( TIM_PERIPH_1, Ut_Tim_Break2Callback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_SystemBreakCallback( TIM_PERIPH_1, Ut_Tim_BreakCallback ) );
    Ut_Tim_Enable_Irq( TIM_PERIPH_1, TIM_IRQ_BREAK, NVIC_PERIPH_IRQ_TIM1_BRK_TIM9 );

    /* Break and break 2 share one interrupt enable bit */
    TIM1->SR = TIM_SR_BIF | TIM_SR_B2IF;
    utTim_Isr[ NVIC_PERIPH_IRQ_TIM1_BRK_TIM9 ]();

    TEST_ASSERT_EQUAL_UINT32( 1u, utTim_BreakCnt );
    TEST_ASSERT_EQUAL_UINT32( 1u, utTim_Break2Cnt );
    TEST_ASSERT_EQUAL_HEX32( ~TIM_SR_B2IF, TIM1->SR );
}


/**
 * \brief   Trigger / commutation interrupt line calls trigger and commutation callbacks.
 *
 * \details TIM1 with trigger and commutation callbacks and interrupts, index and
 *          direction callbacks are rejected (not available on STM32F7). ISR of trigger /
 *          commutation line called with TIF | COMIF.
 *
 * \par Expected results
 * - Index / direction callback: TIM_REQUEST_ERROR.
 * - Trigger and commutation callback called once each.
 */
void Ut_Tim_Isr_TriggerCommutationLine_CallsTriggerAndCommutationCallbacks( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_TriggerCallback( TIM_PERIPH_1, Ut_Tim_TriggerCallback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CommutationCallback( TIM_PERIPH_1, Ut_Tim_CommutationCallback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_IndexCallback( TIM_PERIPH_1, Ut_Tim_TriggerCallback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DirectionCallback( TIM_PERIPH_1, Ut_Tim_TriggerCallback ) );
    Ut_Tim_Enable_Irq( TIM_PERIPH_1, TIM_IRQ_TRIGGER,     NVIC_PERIPH_IRQ_TIM1_TRG_COM_TIM11 );
    Ut_Tim_Enable_Irq( TIM_PERIPH_1, TIM_IRQ_COMMUTATION, NVIC_PERIPH_IRQ_TIM1_TRG_COM_TIM11 );

    TIM1->SR = TIM_SR_TIF | TIM_SR_COMIF;
    utTim_Isr[ NVIC_PERIPH_IRQ_TIM1_TRG_COM_TIM11 ]();

    TEST_ASSERT_EQUAL_UINT32( 1u, utTim_TriggerCnt );
    TEST_ASSERT_EQUAL_UINT32( 1u, utTim_CommutationCnt );
}


/**
 * \brief   ISR of dedicated line processes only its interrupt group.
 *
 * \details TIM1 with update and CC1 callbacks and interrupts. ISR of update line
 *          called with UIF | CC1IF.
 *
 * \par Expected results
 * - Update callback 1x, CC callback not called.
 */
void Ut_Tim_Isr_DedicatedLine_ProcessesOnlyOwnGroup( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_UpdateCallback( TIM_PERIPH_1, Ut_Tim_UpdateCallback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CaptureCompareCallback( TIM_PERIPH_1, TIM_CHANNEL_1, Ut_Tim_CaptureCompareCallback ) );
    Ut_Tim_Enable_Irq( TIM_PERIPH_1, TIM_IRQ_UPDATE,              NVIC_PERIPH_IRQ_TIM1_UP_TIM10 );
    Ut_Tim_Enable_Irq( TIM_PERIPH_1, TIM_IRQ_CAPTURE_COMPARE_CH1, NVIC_PERIPH_IRQ_TIM1_CC );

    TIM1->SR = TIM_SR_UIF | TIM_SR_CC1IF;
    utTim_Isr[ NVIC_PERIPH_IRQ_TIM1_UP_TIM10 ]();

    TEST_ASSERT_EQUAL_UINT32( 1u, utTim_UpdateCnt );
    TEST_ASSERT_EQUAL_UINT32( 0u, utTim_CaptureCompareCnt );
}


/**
 * \brief   Pending flag of disabled interrupt is not handled.
 *
 * \details TIM3 with update and CC1 callbacks, only update interrupt enabled. ISR
 *          called with UIF | CC1IF.
 *
 * \par Expected results
 * - Update callback 1x, CC callback not called.
 */
void Ut_Tim_Isr_DisabledInterrupt_IsNotHandled( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_UpdateCallback( TIM_PERIPH_3, Ut_Tim_UpdateCallback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CaptureCompareCallback( TIM_PERIPH_3, TIM_CHANNEL_1, Ut_Tim_CaptureCompareCallback ) );
    Ut_Tim_Enable_Irq( TIM_PERIPH_3, TIM_IRQ_UPDATE, NVIC_PERIPH_IRQ_TIM3 );

    /* CC1 flag pending, but CC1 interrupt is not enabled */
    TIM3->SR = TIM_SR_UIF | TIM_SR_CC1IF;
    utTim_Isr[ NVIC_PERIPH_IRQ_TIM3 ]();

    TEST_ASSERT_EQUAL_UINT32( 1u, utTim_UpdateCnt );
    TEST_ASSERT_EQUAL_UINT32( 0u, utTim_CaptureCompareCnt );
}


/**
 * \brief   Interrupt without registered callback clears the flag.
 *
 * \details TIM3 update interrupt enabled without callback, ISR called with UIF.
 *
 * \par Expected results
 * - UIF cleared (no endless interrupt).
 */
void Ut_Tim_Isr_NoCallbackRegistered_ClearsFlag( void )
{
    Ut_Tim_Enable_Irq( TIM_PERIPH_3, TIM_IRQ_UPDATE, NVIC_PERIPH_IRQ_TIM3 );

    TIM3->SR = TIM_SR_UIF;
    utTim_Isr[ NVIC_PERIPH_IRQ_TIM3 ]();

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SR & TIM_SR_UIF );
}


/**
 * \brief   Callback setters reject invalid arguments.
 *
 * \details Sets update, trigger, break 2 and error callback of peripheral out of range, error
 *          callback of TIM1 (encoder error not available on STM32F7), CC callback of
 *          TIM1 channel 5 (no interrupt of channel 5) and of TIM6 (no channel).
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases.
 */
void Ut_Tim_Set_Callback_InvalidArgs_ReturnsError( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_UpdateCallback( TIM_PERIPH_CNT, Ut_Tim_UpdateCallback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_TriggerCallback( TIM_PERIPH_CNT, Ut_Tim_TriggerCallback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Break2Callback( TIM_PERIPH_CNT, Ut_Tim_Break2Callback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ErrorCallback( TIM_PERIPH_CNT, NULL ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ErrorCallback( TIM_PERIPH_1, NULL ) );     /* Not available on STM32F7 */

    /* Channels 5 / 6 have no interrupt, basic timer has no channel */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_CaptureCompareCallback( TIM_PERIPH_1, TIM_CHANNEL_5, Ut_Tim_CaptureCompareCallback ) );
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_CaptureCompareCallback( TIM_PERIPH_6, TIM_CHANNEL_1, Ut_Tim_CaptureCompareCallback ) );
#endif /* TIM6 */
}

/* ======================= MASTER / SLAVE SYNCHRONIZATION =================== */

/**
 * \brief   Master trigger is written to MMS field.
 *
 * \details Sets TIM6 master trigger update, reads it back, then sets trigger out of
 *          range.
 *
 * \par Expected results
 * - CR2 = TRGO update, trigger reads back update.
 * - Trigger out of range: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Set_MasterTrigger_WritesMms( void )
{
#if defined(TIM6)
    tim_MasterTrigger_t masterTrigger = TIM_MASTER_TRIGGER_RESET;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_MasterTrigger( TIM_PERIPH_6, TIM_MASTER_TRIGGER_UPDATE ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_TRGO_UPDATE, TIM6->CR2 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_MasterTrigger( TIM_PERIPH_6, &masterTrigger ) );
    TEST_ASSERT_EQUAL( TIM_MASTER_TRIGGER_UPDATE, masterTrigger );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_MasterTrigger( TIM_PERIPH_6, TIM_MASTER_TRIGGER_CNT ) );
#else
    TEST_IGNORE_MESSAGE( "MCU without basic timer TIM6" );
#endif /* TIM6 */
}


/**
 * \brief   Trigger outputs not available on the timer are rejected.
 *
 * \details Sets and reads master trigger 2 of TIM3 (general purpose timer), sets
 *          master trigger encoder clock of TIM3 (not available on STM32F7) and master
 *          trigger 2 of TIM1 out of range.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, CR2 of TIM1 and TIM3 stays 0.
 */
void Ut_Tim_Set_MasterTrigger_NotAvailableOnStm32F7_ReturnsErrorWithoutWrite( void )
{
    tim_MasterTrigger2_t masterTrigger2 = TIM_MASTER_TRIGGER2_RESET;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_MasterTrigger2( TIM_PERIPH_3, TIM_MASTER_TRIGGER2_UPDATE ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_MasterTrigger2( TIM_PERIPH_3, &masterTrigger2 ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_MasterTrigger( TIM_PERIPH_3, TIM_MASTER_TRIGGER_ENCODER_CLK ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_MasterTrigger2( TIM_PERIPH_1, TIM_MASTER_TRIGGER2_CNT ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_MasterTrigger2( TIM_PERIPH_1, NULL ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->CR2 );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CR2 );
}


/**
 * \brief   Master trigger 2 of advanced timer is written to MMS2 field.
 *
 * \details Sets TIM1 master trigger 2 OC4REF rising / falling and reads it back, then
 *          OC6REF.
 *
 * \par Expected results
 * - CR2 = TRGO2 OC4 rising / falling, trigger 2 reads back.
 * - CR2 = TRGO2 OC6, trigger 2 reads back.
 */
void Ut_Tim_Set_MasterTrigger2_AdvancedTimer_WritesMms2( void )
{
    tim_MasterTrigger2_t masterTrigger2 = TIM_MASTER_TRIGGER2_RESET;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_MasterTrigger2( TIM_PERIPH_1, TIM_MASTER_TRIGGER2_OC4REF_RISING_FALLING ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_TRGO2_OC4_RISINGFALLING, TIM1->CR2 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_MasterTrigger2( TIM_PERIPH_1, &masterTrigger2 ) );
    TEST_ASSERT_EQUAL( TIM_MASTER_TRIGGER2_OC4REF_RISING_FALLING, masterTrigger2 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_MasterTrigger2( TIM_PERIPH_1, TIM_MASTER_TRIGGER2_OC6REF ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_TRGO2_OC6, TIM1->CR2 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_MasterTrigger2( TIM_PERIPH_1, &masterTrigger2 ) );
    TEST_ASSERT_EQUAL( TIM_MASTER_TRIGGER2_OC6REF, masterTrigger2 );
}


/**
 * \brief   Master / slave mode is enabled and disabled by MSM bit.
 *
 * \details Activates master / slave mode of TIM3, reads it, deactivates it, then
 *          activates it on TIM6.
 *
 * \par Expected results
 * - SMCR = MSM, state active; SMCR = 0 after deactivation.
 * - TIM6 (no slave mode controller): TIM_REQUEST_ERROR.
 */
void Ut_Tim_Set_MasterSlaveMode_TogglesMsm( void )
{
    tim_FunctionState_t modeState = TIM_FUNCTION_INACTIVE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_MasterSlaveModeActive( TIM_PERIPH_3 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_SMCR_MSM, TIM3->SMCR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_MasterSlaveMode( TIM_PERIPH_3, &modeState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_ACTIVE, modeState );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_MasterSlaveModeInactive( TIM_PERIPH_3 ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SMCR );

    /* Basic timer has no slave mode controller */
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_MasterSlaveModeActive( TIM_PERIPH_6 ) );
#endif /* TIM6 */
}


/**
 * \brief   Gated slave mode with trigger input is written and read back.
 *
 * \details Sets TIM3 gated slave mode with trigger TI2FP2 and reads it back.
 *
 * \par Expected results
 * - SMCR = TS TI2FP2 | SMS gated.
 * - Slave mode gated and trigger TI2FP2 read back.
 */
void Ut_Tim_Set_SlaveMode_Gated_WritesTriggerAndMode( void )
{
    tim_SlaveMode_t    slaveMode    = TIM_SLAVE_MODE_DISABLE;
    tim_TriggerInput_t triggerInput = TIM_TRIGGER_INPUT_UNUSED;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_SlaveMode( TIM_PERIPH_3, TIM_SLAVE_MODE_GATED, UT_TIM_GP_TRG_TI2FP2 ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_TS_TI2FP2 | LL_TIM_SLAVEMODE_GATED, TIM3->SMCR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_SlaveMode( TIM_PERIPH_3, &slaveMode, &triggerInput ) );
    TEST_ASSERT_EQUAL( TIM_SLAVE_MODE_GATED, slaveMode );
    TEST_ASSERT_EQUAL( UT_TIM_GP_TRG_TI2FP2, triggerInput );
}


/**
 * \brief   Disabled slave mode clears SMS without trigger validation.
 *
 * \details SMCR = TI2FP2 | gated, sets slave mode disabled with invalid trigger input.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, SMCR.SMS = 0.
 */
void Ut_Tim_Set_SlaveMode_Disable_ClearsSlaveMode( void )
{
    TIM3->SMCR = LL_TIM_TS_TI2FP2 | LL_TIM_SLAVEMODE_GATED;

    /* Trigger input is not used (not validated) when slave mode is disabled */
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_SlaveMode( TIM_PERIPH_3, TIM_SLAVE_MODE_DISABLE, UT_TIM_INVALID_TRIGGER ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SMCR & TIM_SMCR_SMS );
}


/**
 * \brief   Combined reset + trigger slave mode is written to SMS bit 3.
 *
 * \details Sets TIM3 slave mode reset + trigger with trigger input TI1FP1 and reads it
 *          back.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, SMCR.SMS = combined reset + trigger, SMCR.TS = TI1FP1.
 * - Slave mode reads back reset + trigger with trigger input TI1FP1.
 */
void Ut_Tim_Set_SlaveMode_ResetTrigger_WritesCombinedMode( void )
{
    tim_SlaveMode_t    slaveMode    = TIM_SLAVE_MODE_DISABLE;
    tim_TriggerInput_t triggerInput = TIM_TRIGGER_INPUT_UNUSED;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_SlaveMode( TIM_PERIPH_3, TIM_SLAVE_MODE_RESET_TRIGGER, UT_TIM_GP_TRG_TI1FP1 ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_SLAVEMODE_COMBINED_RESETTRIGGER, TIM3->SMCR & TIM_SMCR_SMS );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_TS_TI1FP1, TIM3->SMCR & TIM_SMCR_TS );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_SlaveMode( TIM_PERIPH_3, &slaveMode, &triggerInput ) );
    TEST_ASSERT_EQUAL( TIM_SLAVE_MODE_RESET_TRIGGER, slaveMode );
    TEST_ASSERT_EQUAL( UT_TIM_GP_TRG_TI1FP1, triggerInput );
}

/**
 * \brief   Tim_Set_SlaveMode() rejects invalid arguments.
 *
 * \details Sets external clock slave mode (configured by Tim_Set_ClockSource), slave
 *          mode out of range, invalid trigger input, slave mode of TIM6 and combined
 *          gated + reset slave mode (not available on STM32F7).
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, SMCR of TIM3 and TIM6 stays 0.
 */
void Ut_Tim_Set_SlaveMode_InvalidArgs_ReturnsErrorWithoutWrite( void )
{
    /* External clock mode is configured by Tim_Set_ClockSource */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_SlaveMode( TIM_PERIPH_3, TIM_SLAVE_MODE_EXTERNAL_CLOCK, UT_TIM_GP_TRG_TI2FP2 ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_SlaveMode( TIM_PERIPH_3, TIM_SLAVE_MODE_CNT, UT_TIM_GP_TRG_TI2FP2 ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_SlaveMode( TIM_PERIPH_3, TIM_SLAVE_MODE_RESET, UT_TIM_INVALID_TRIGGER ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_SlaveMode( TIM_PERIPH_3, TIM_SLAVE_MODE_RESET, UT_TIM_FOREIGN_TRIGGER ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_SlaveMode( TIM_PERIPH_3, TIM_SLAVE_MODE_RESET, TIM_TRIGGER_INPUT_UNUSED ) );
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_SlaveMode( TIM_PERIPH_6, TIM_SLAVE_MODE_RESET, UT_TIM_GP_TRG_TI2FP2 ) );
#endif /* TIM6 */

    /* Combined gated + reset slave mode is not available on STM32F7 */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_SlaveMode( TIM_PERIPH_3, TIM_SLAVE_MODE_GATED_RESET, UT_TIM_GP_TRG_TI2FP2 ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SMCR );
#if defined(TIM6)
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM6->SMCR );
#endif /* TIM6 */
}


/**
 * \brief   Slave mode of running timer cannot be changed.
 *
 * \details CR1 = CEN, sets TIM3 trigger slave mode.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, SMCR stays 0.
 */
void Ut_Tim_Set_SlaveMode_RunningTimer_ReturnsErrorWithoutWrite( void )
{
    TIM3->CR1 = TIM_CR1_CEN;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_SlaveMode( TIM_PERIPH_3, TIM_SLAVE_MODE_TRIGGER, UT_TIM_GP_TRG_TI2FP2 ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SMCR );
}

/**
 * \brief   Internal trigger input of a timer is written and read back.
 *
 * \details SMCR.TS = TI1FP1, sets reset slave mode of TIM1 ITR0 (TIM5 TRGO) and reads it back.
 *
 * \par Expected results
 * - SMCR = TS code of the input | SMS reset.
 * - Slave mode reset and the trigger input item read back.
 */
void Ut_Tim_Set_SlaveMode_InternalTrigger_WritesTsOfTimer( void )
{
    tim_SlaveMode_t    slaveMode    = TIM_SLAVE_MODE_DISABLE;
    tim_TriggerInput_t triggerInput = TIM_TRIGGER_INPUT_UNUSED;

    TIM1->SMCR = LL_TIM_TS_TI1FP1;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_SlaveMode( TIM_PERIPH_1, TIM_SLAVE_MODE_RESET, TIM_TRIGGER_INPUT_TIM1_ITR0_TIM5_TRGO ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_TS_ITR0 | LL_TIM_SLAVEMODE_RESET, TIM1->SMCR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_SlaveMode( TIM_PERIPH_1, &slaveMode, &triggerInput ) );
    TEST_ASSERT_EQUAL( TIM_SLAVE_MODE_RESET, slaveMode );
    TEST_ASSERT_EQUAL( TIM_TRIGGER_INPUT_TIM1_ITR0_TIM5_TRGO, triggerInput );
}


/**
 * \brief   Internal trigger inputs of other timers are written.
 *
 * \details Sets the trigger inputs of TIM1 from TIM2 TRGO (ITR1, gated) and from TIM4 TRGO (ITR3, trigger mode) on the devices with the timers, otherwise the test is
 *          ignored.
 *
 * \par Expected results
 * - SMCR = TS code of the input | SMS of the mode.
 */
void Ut_Tim_Set_SlaveMode_InternalTriggerOfOtherTimers_WritesTs( void )
{
#if defined(TIM2) && \
    defined(TIM3) && \
    defined(TIM4)
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_SlaveMode( TIM_PERIPH_1, TIM_SLAVE_MODE_DISABLE, TIM_TRIGGER_INPUT_UNUSED ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_SlaveMode( TIM_PERIPH_1, TIM_SLAVE_MODE_GATED, TIM_TRIGGER_INPUT_TIM1_ITR1_TIM2_TRGO ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_TS_ITR1 | LL_TIM_SLAVEMODE_GATED, TIM1->SMCR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_SlaveMode( TIM_PERIPH_1, TIM_SLAVE_MODE_DISABLE, TIM_TRIGGER_INPUT_UNUSED ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_SlaveMode( TIM_PERIPH_1, TIM_SLAVE_MODE_TRIGGER, TIM_TRIGGER_INPUT_TIM1_ITR3_TIM4_TRGO ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_TS_ITR3 | LL_TIM_SLAVEMODE_TRIGGER, TIM1->SMCR );
#else
    TEST_IGNORE_MESSAGE( "The device lacks the timers of the connections" );
#endif
}


/**
 * \brief   Trigger input of the slave mode is not reported while the slave mode is disabled.
 *
 * \details SMCR.TS = TI2FP2 with slave mode disabled, reads the slave mode.
 *
 * \par Expected results
 * - Slave mode disabled, trigger input TIM_TRIGGER_INPUT_UNUSED.
 */
void Ut_Tim_Get_SlaveMode_Disabled_ReportsUnusedTrigger( void )
{
    tim_SlaveMode_t    slaveMode    = TIM_SLAVE_MODE_GATED;
    tim_TriggerInput_t triggerInput = UT_TIM_GP_TRG_TI2FP2;

    TIM3->SMCR = LL_TIM_TS_TI2FP2;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_SlaveMode( TIM_PERIPH_3, &slaveMode, &triggerInput ) );
    TEST_ASSERT_EQUAL( TIM_SLAVE_MODE_DISABLE, slaveMode );
    TEST_ASSERT_EQUAL( TIM_TRIGGER_INPUT_UNUSED, triggerInput );
}

/* ============================= CHANNEL MODE =============================== */

/**
 * \brief   Toggle output compare mode is written and read back.
 *
 * \details Sets TIM3 channel 2 mode toggle and reads it back.
 *
 * \par Expected results
 * - OC2M = toggle, mode reads back toggle.
 */
void Ut_Tim_Set_ChannelMode_Toggle_WritesOutputCompareMode( void )
{
    tim_ChannelMode_t channelMode = TIM_CHANNEL_MODE_CNT;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_ChannelMode( TIM_PERIPH_3, TIM_CHANNEL_2, TIM_CHANNEL_MODE_OUTPUT_COMPARE_TOGGLE ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_OCMODE_TOGGLE, LL_TIM_OC_GetMode( TIM3, LL_TIM_CHANNEL_CH2 ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_ChannelMode( TIM_PERIPH_3, TIM_CHANNEL_2, &channelMode ) );
    TEST_ASSERT_EQUAL( TIM_CHANNEL_MODE_OUTPUT_COMPARE_TOGGLE, channelMode );
}


/**
 * \brief   Modes of channels 5 / 6 are written to CCMR3.
 *
 * \details Sets TIM1 channel 5 mode PWM and channel 6 mode combined PWM, reads mode
 *          of channel 6.
 *
 * \par Expected results
 * - OC5M = PWM mode 1, OC6M = combined PWM mode 1, CCMR1 and CCMR2 stay 0.
 * - Channel 6 mode reads back combined PWM.
 */
void Ut_Tim_Set_ChannelMode_Channel5And6_WritesCcmr3( void )
{
    tim_ChannelMode_t channelMode = TIM_CHANNEL_MODE_CNT;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_ChannelMode( TIM_PERIPH_1, TIM_CHANNEL_5, TIM_CHANNEL_MODE_OUTPUT_PWM ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_ChannelMode( TIM_PERIPH_1, TIM_CHANNEL_6, TIM_CHANNEL_MODE_OUTPUT_COMBINED_PWM ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_OCMODE_PWM1, LL_TIM_OC_GetMode( TIM1, LL_TIM_CHANNEL_CH5 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_OCMODE_COMBINED_PWM1, LL_TIM_OC_GetMode( TIM1, LL_TIM_CHANNEL_CH6 ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->CCMR1 );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->CCMR2 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_ChannelMode( TIM_PERIPH_1, TIM_CHANNEL_6, &channelMode ) );
    TEST_ASSERT_EQUAL( TIM_CHANNEL_MODE_OUTPUT_COMBINED_PWM, channelMode );
}


/**
 * \brief   Output compare modes not available on STM32F7 are rejected, 4-bit OCxM
 *          modes are written.
 *
 * \details Sets pulse on compare and direction output modes on TIM3 channel 3, then
 *          combined PWM 2, asymmetric PWM 1 and asymmetric PWM 2 modes.
 *
 * \par Expected results
 * - Pulse on compare / direction output: TIM_REQUEST_ERROR, CCMR2 stays 0.
 * - 4-bit modes: TIM_REQUEST_OK, OC3M written (OC3M bit 3 included), mode reads back.
 */
void Ut_Tim_Set_ChannelMode_ModesNotAvailableOnStm32F7_ReturnsError( void )
{
    tim_ChannelMode_t channelMode = TIM_CHANNEL_MODE_CNT;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ChannelMode( TIM_PERIPH_3, TIM_CHANNEL_3, TIM_CHANNEL_MODE_OUTPUT_COMPARE_PULSE ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ChannelMode( TIM_PERIPH_3, TIM_CHANNEL_3, TIM_CHANNEL_MODE_OUTPUT_DIRECTION ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCMR2 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_ChannelMode( TIM_PERIPH_3, TIM_CHANNEL_3, TIM_CHANNEL_MODE_OUTPUT_COMBINED_PWM_INV ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_OCMODE_COMBINED_PWM2, LL_TIM_OC_GetMode( TIM3, LL_TIM_CHANNEL_CH3 ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_ChannelMode( TIM_PERIPH_3, TIM_CHANNEL_3, &channelMode ) );
    TEST_ASSERT_EQUAL( TIM_CHANNEL_MODE_OUTPUT_COMBINED_PWM_INV, channelMode );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_ChannelMode( TIM_PERIPH_3, TIM_CHANNEL_3, TIM_CHANNEL_MODE_OUTPUT_ASSYMETRIC_PWM ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_OCMODE_ASYMMETRIC_PWM1, LL_TIM_OC_GetMode( TIM3, LL_TIM_CHANNEL_CH3 ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_ChannelMode( TIM_PERIPH_3, TIM_CHANNEL_3, TIM_CHANNEL_MODE_OUTPUT_ASSYMETRIC_PWM_INV ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_OCMODE_ASYMMETRIC_PWM2, LL_TIM_OC_GetMode( TIM3, LL_TIM_CHANNEL_CH3 ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_ChannelMode( TIM_PERIPH_3, TIM_CHANNEL_3, &channelMode ) );
    TEST_ASSERT_EQUAL( TIM_CHANNEL_MODE_OUTPUT_ASSYMETRIC_PWM_INV, channelMode );
}


/**
 * \brief   Tim_Set_ChannelMode() rejects invalid arguments.
 *
 * \details Sets input capture mode (configured by Tim_Set_Mode_InputCapture), mode out
 *          of range, mode of TIM3 channel 5 and mode of TIM6 channel 1.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, CCMR1 stays 0.
 */
void Ut_Tim_Set_ChannelMode_InvalidArgs_ReturnsErrorWithoutWrite( void )
{
    /* Input capture is configured by Tim_Set_Mode_InputCapture */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ChannelMode( TIM_PERIPH_3, TIM_CHANNEL_1, TIM_CHANNEL_MODE_INPUT_CAPTURE ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ChannelMode( TIM_PERIPH_3, TIM_CHANNEL_1, TIM_CHANNEL_MODE_CNT ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ChannelMode( TIM_PERIPH_3, TIM_CHANNEL_5, TIM_CHANNEL_MODE_OUTPUT_PWM ) );
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ChannelMode( TIM_PERIPH_6, TIM_CHANNEL_1, TIM_CHANNEL_MODE_OUTPUT_PWM ) );
#endif /* TIM6 */

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCMR1 );
}


/**
 * \brief   Channel mapped on input is reported as input capture.
 *
 * \details CCMR1.CC1S = TI1, reads channel 1 mode, then calls the getter with NULL.
 *
 * \par Expected results
 * - Mode input capture.
 * - NULL pointer: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Get_ChannelMode_InputChannel_ReturnsInputCapture( void )
{
    tim_ChannelMode_t channelMode = TIM_CHANNEL_MODE_CNT;

    TIM3->CCMR1 = TIM_CCMR1_CC1S_0;     /* IC1 mapped on TI1 */

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_ChannelMode( TIM_PERIPH_3, TIM_CHANNEL_1, &channelMode ) );
    TEST_ASSERT_EQUAL( TIM_CHANNEL_MODE_INPUT_CAPTURE, channelMode );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_ChannelMode( TIM_PERIPH_3, TIM_CHANNEL_1, NULL ) );
}

/* ======================= COMPARE VALUE, DUTY CYCLE ======================== */

/**
 * \brief   Compare value is written to CCR and read back.
 *
 * \details Sets TIM3 channel 4 compare value 0xFFFF and reads it back.
 *
 * \par Expected results
 * - CCR4 = 0xFFFF, compare value reads back 0xFFFF.
 */
void Ut_Tim_Set_CompareValue_WritesCcr( void )
{
    tim_Counter_t compareValue = 0u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CompareValue( TIM_PERIPH_3, TIM_CHANNEL_4, UT_TIM_16BIT_MAX ) );
    TEST_ASSERT_EQUAL_HEX32( UT_TIM_16BIT_MAX, TIM3->CCR4 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_CompareValue( TIM_PERIPH_3, TIM_CHANNEL_4, &compareValue ) );
    TEST_ASSERT_EQUAL_HEX32( UT_TIM_16BIT_MAX, compareValue );
}


/**
 * \brief   32-bit timer accepts 32-bit compare value.
 *
 * \details Sets TIM2 channel 1 compare value 0x89ABCDEF.
 *
 * \par Expected results
 * - CCR1 = 0x89ABCDEF.
 */
void Ut_Tim_Set_CompareValue_32BitTimer_AcceptsFullRange( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CompareValue( TIM_PERIPH_2, TIM_CHANNEL_1, 0x89ABCDEFu ) );

    TEST_ASSERT_EQUAL_HEX32( 0x89ABCDEFu, TIM2->CCR1 );
}


/**
 * \brief   Compare value functions reject invalid arguments.
 *
 * \details Sets TIM3 (16-bit) compare value 0x10000, compare value of channel 5 and
 *          channel out of range, reads with NULL pointer and from TIM6.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, CCR1 stays 0.
 */
void Ut_Tim_Set_CompareValue_InvalidArgs_ReturnsErrorWithoutWrite( void )
{
    tim_Counter_t compareValue = 0u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_CompareValue( TIM_PERIPH_3, TIM_CHANNEL_1, UT_TIM_16BIT_MAX + 1u ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_CompareValue( TIM_PERIPH_3, TIM_CHANNEL_5, 1u ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_CompareValue( TIM_PERIPH_3, TIM_CHANNEL_CNT, 1u ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_CompareValue( TIM_PERIPH_3, TIM_CHANNEL_1, NULL ) );
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_CompareValue( TIM_PERIPH_6, TIM_CHANNEL_1, &compareValue ) );
#endif /* TIM6 */

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCR1 );
}


/**
 * \brief   PWM duty cycle is converted to compare value.
 *
 * \details ARR = 999 (1000 steps). Sets duty cycle 25 %, 100 % and 0 %.
 *
 * \par Expected results
 * - CCR1 = 250, 1000 (above period - output permanently active) and 0.
 */
void Ut_Tim_Set_PwmMode_DutyCycle_CalculatesCompareValue( void )
{
    TIM3->ARR = 999u;   /* Period of 1000 steps */

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_PwmMode_DutyCycle( TIM_PERIPH_3, TIM_CHANNEL_1, 2500u ) );
    TEST_ASSERT_EQUAL_HEX32( 250u, TIM3->CCR1 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_PwmMode_DutyCycle( TIM_PERIPH_3, TIM_CHANNEL_1, 10000u ) );
    TEST_ASSERT_EQUAL_HEX32( 1000u, TIM3->CCR1 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_PwmMode_DutyCycle( TIM_PERIPH_3, TIM_CHANNEL_1, 0u ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCR1 );
}


/**
 * \brief   PWM duty cycle above 100 % is rejected.
 *
 * \details ARR = 999, CCR1 = 1, sets duty cycle 100.01 %.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, CCR1 unchanged.
 */
void Ut_Tim_Set_PwmMode_DutyCycle_AboveMaximum_ReturnsErrorWithoutWrite( void )
{
    TIM3->ARR  = 999u;
    TIM3->CCR1 = 1u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_PwmMode_DutyCycle( TIM_PERIPH_3, TIM_CHANNEL_1, 10001u ) );

    TEST_ASSERT_EQUAL_HEX32( 1u, TIM3->CCR1 );
}


/**
 * \brief   PWM duty cycle is calculated from compare value.
 *
 * \details ARR = 999. Reads duty cycle with CCR1 = 333 and CCR1 = 2000.
 *
 * \par Expected results
 * - CCR1 = 333: 33.30 %.
 * - CCR1 = 2000 (above period): 100 %.
 */
void Ut_Tim_Get_PwmMode_DutyCycle_RoundsToCentiPercent( void )
{
    tim_CentiPercent_t dutyCycle = 0u;

    TIM3->ARR  = 999u;
    TIM3->CCR1 = 333u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_PwmMode_DutyCycle( TIM_PERIPH_3, TIM_CHANNEL_1, &dutyCycle ) );
    TEST_ASSERT_EQUAL_UINT16( 3330u, dutyCycle );

    /* Compare value above period - output permanently active */
    TIM3->CCR1 = 2000u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_PwmMode_DutyCycle( TIM_PERIPH_3, TIM_CHANNEL_1, &dutyCycle ) );
    TEST_ASSERT_EQUAL_UINT16( 10000u, dutyCycle );
}


/**
 * \brief   PWM pulse width is converted to compare value.
 *
 * \details Kernel clock 250 MHz, PSC = 249 (1 us per step), ARR = 999. Sets channel 2
 *          pulse width 250 us.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, CCR2 = 250.
 */
void Ut_Tim_Set_PwmMode_PulseWidth_CalculatesCompareValue( void )
{
    TIM3->PSC = UT_TIM_PSC_1MHZ;    /* 1 us per step */
    TIM3->ARR = 999u;
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_PwmMode_PulseWidth( TIM_PERIPH_3, TIM_CHANNEL_2, 250000u ) );

    TEST_ASSERT_EQUAL_HEX32( 250u, TIM3->CCR2 );
}


/**
 * \brief   PWM pulse width longer than period is rejected.
 *
 * \details Counter 1 MHz, period 1 ms. Sets channel 2 pulse width 2 ms.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, CCR2 stays 0.
 */
void Ut_Tim_Set_PwmMode_PulseWidth_LongerThanPeriod_ReturnsErrorWithoutWrite( void )
{
    TIM3->PSC = UT_TIM_PSC_1MHZ;
    TIM3->ARR = 999u;
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_PwmMode_PulseWidth( TIM_PERIPH_3, TIM_CHANNEL_2, 2000000u ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCR2 );
}


/**
 * \brief   PWM pulse width is rejected for externally clocked timer.
 *
 * \details SMCR.ECE set, sets channel 2 pulse width.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, no RCC call (strict mock).
 */
void Ut_Tim_Set_PwmMode_PulseWidth_ExternalClock_ReturnsErrorWithoutRccAccess( void )
{
    TIM3->SMCR = TIM_SMCR_ECE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_PwmMode_PulseWidth( TIM_PERIPH_3, TIM_CHANNEL_2, 1000u ) );
}

/* ====================== OUTPUT CONFIGURATION ============================== */

/**
 * \brief   Compare preload is enabled and disabled by OCxPE bit.
 *
 * \details Activates and deactivates compare preload of TIM3 channel 1, then activates
 *          it on TIM6.
 *
 * \par Expected results
 * - CCMR1 = OC1PE, then 0.
 * - TIM6: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Set_ComparePreload_TogglesOcpe( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_ComparePreloadActive( TIM_PERIPH_3, TIM_CHANNEL_1 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_CCMR1_OC1PE, TIM3->CCMR1 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_ComparePreloadInactive( TIM_PERIPH_3, TIM_CHANNEL_1 ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCMR1 );

#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ComparePreloadActive( TIM_PERIPH_6, TIM_CHANNEL_1 ) );
#endif /* TIM6 */
}


/**
 * \brief   Polarity of complementary output is written to CCxNP bit.
 *
 * \details Sets TIM1 output 1N polarity low, reads polarity of output 1N and 1.
 *
 * \par Expected results
 * - CCER = CC1NP.
 * - Output 1N polarity low, output 1 polarity high.
 */
void Ut_Tim_Set_OutputPolarity_ComplementaryOutput_WritesCcnp( void )
{
    tim_Polarity_t outputPolarity = TIM_POLARITY_HIGH;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_OutputPolarity( TIM_PERIPH_1, TIM_OUTPUT_1_N, TIM_POLARITY_LOW ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_CCER_CC1NP, TIM1->CCER );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_OutputPolarity( TIM_PERIPH_1, TIM_OUTPUT_1_N, &outputPolarity ) );
    TEST_ASSERT_EQUAL( TIM_POLARITY_LOW, outputPolarity );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_OutputPolarity( TIM_PERIPH_1, TIM_OUTPUT_1, &outputPolarity ) );
    TEST_ASSERT_EQUAL( TIM_POLARITY_HIGH, outputPolarity );
}


/**
 * \brief   Output polarity of unavailable output is rejected.
 *
 * \details Sets polarity of TIM3 output 1N, output 5, output out of range and invalid
 *          polarity.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, CCER stays 0.
 */
void Ut_Tim_Set_OutputPolarity_OutputNotAvailable_ReturnsErrorWithoutWrite( void )
{
    /* General purpose timer has no complementary outputs and no channel 5 */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_OutputPolarity( TIM_PERIPH_3, TIM_OUTPUT_1_N, TIM_POLARITY_LOW ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_OutputPolarity( TIM_PERIPH_3, TIM_OUTPUT_5,   TIM_POLARITY_LOW ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_OutputPolarity( TIM_PERIPH_3, TIM_OUTPUT_CNT, TIM_POLARITY_LOW ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_OutputPolarity( TIM_PERIPH_3, TIM_OUTPUT_1,   (tim_Polarity_t)( TIM_POLARITY_LOW + 1u ) ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCER );
}


/**
 * \brief   Output is enabled and disabled by CCxE bit.
 *
 * \details Activates TIM3 output 2, reads its state, deactivates it, then activates
 *          output 2N and reads state with NULL pointer.
 *
 * \par Expected results
 * - CCER = CC2E, state active; CCER = 0 after deactivation.
 * - Output 2N, NULL pointer: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Set_Output_TogglesChannelEnable( void )
{
    tim_FunctionState_t outputState = TIM_FUNCTION_INACTIVE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_OutputActive( TIM_PERIPH_3, TIM_OUTPUT_2 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_CCER_CC2E, TIM3->CCER );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_OutputState( TIM_PERIPH_3, TIM_OUTPUT_2, &outputState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_ACTIVE, outputState );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_OutputInactive( TIM_PERIPH_3, TIM_OUTPUT_2 ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCER );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_OutputActive( TIM_PERIPH_3, TIM_OUTPUT_2_N ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_OutputState( TIM_PERIPH_3, TIM_OUTPUT_2, NULL ) );
}


/**
 * \brief   Main output of advanced timer is enabled and disabled by MOE bit.
 *
 * \details Activates TIM1 main output, reads its state and deactivates it.
 *
 * \par Expected results
 * - BDTR = MOE, state active; BDTR = 0 after deactivation.
 */
void Ut_Tim_Set_MainOutput_AdvancedTimer_TogglesMoe( void )
{
    tim_FunctionState_t outputState = TIM_FUNCTION_INACTIVE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_MainOutputActive( TIM_PERIPH_1 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_BDTR_MOE, TIM1->BDTR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_MainOutputState( TIM_PERIPH_1, &outputState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_ACTIVE, outputState );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_MainOutputInactive( TIM_PERIPH_1 ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->BDTR );
}


/**
 * \brief   General purpose timer rejects main output functions.
 *
 * \details Activates and reads main output of TIM3.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in both cases, BDTR stays 0.
 */
void Ut_Tim_Set_MainOutput_GeneralTimer_ReturnsError( void )
{
    tim_FunctionState_t outputState = TIM_FUNCTION_INACTIVE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_MainOutputActive( TIM_PERIPH_3 ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_MainOutputState( TIM_PERIPH_3, &outputState ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->BDTR );
}

/* ======================= OUTPUT COMPARE / PWM MODES ======================= */

/**
 * \brief   PWM mode of advanced timer enables output, complementary output and main
 *          output.
 *
 * \details TIM1 channel 1 PWM with complementary pin PA7, polarity low, idle state
 *          high, CCR1 preset 0x100.
 *
 * \par Expected results
 * - OC1M = PWM mode 1 with preload.
 * - CCER = CC1E | CC1P | CC1NE | CC1NP, CR2 = OIS1, BDTR = MOE.
 * - CCR1 = 0 (starts with 0 % duty cycle), EGR = UG (preloaded registers applied).
 */
void Ut_Tim_Set_Mode_Pwm_AdvancedTimerWithComplOutput_EnablesAllOutputs( void )
{
    tim_ChannelConfig_t channelConfig = Ut_Tim_Get_ChannelConfig( TIM_CHANNEL_1, TIM_CHANNEL_MODE_OUTPUT_PWM );

    channelConfig.IoComplPin     = TIM_1_CH1N_PA7;
    channelConfig.OutputPolarity = TIM_POLARITY_LOW;
    channelConfig.IdleState      = TIM_POLARITY_HIGH;
    TIM1->CCR1                   = 0x100u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Mode_Pwm( TIM_PERIPH_1, &channelConfig ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_OCMODE_PWM1, LL_TIM_OC_GetMode( TIM1, LL_TIM_CHANNEL_CH1 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_CCMR1_OC1PE, TIM1->CCMR1 & TIM_CCMR1_OC1PE );
    TEST_ASSERT_EQUAL_HEX32( TIM_CCER_CC1E | TIM_CCER_CC1P | TIM_CCER_CC1NE | TIM_CCER_CC1NP, TIM1->CCER );
    TEST_ASSERT_EQUAL_HEX32( TIM_CR2_OIS1, TIM1->CR2 );
    TEST_ASSERT_EQUAL_HEX32( TIM_BDTR_MOE, TIM1->BDTR );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->CCR1 );          /* Starts with 0 % duty cycle */
    TEST_ASSERT_EQUAL_HEX32( TIM_EGR_UG, TIM1->EGR );   /* Preloaded registers applied */
}


/**
 * \brief   PWM mode without complementary pin keeps complementary output disabled.
 *
 * \details TIM1 channel 2 inverted PWM without complementary pin.
 *
 * \par Expected results
 * - OC2M = PWM mode 2, CCER = CC2E only.
 */
void Ut_Tim_Set_Mode_Pwm_WithoutComplPin_KeepsComplOutputDisabled( void )
{
    tim_ChannelConfig_t channelConfig = Ut_Tim_Get_ChannelConfig( TIM_CHANNEL_2, TIM_CHANNEL_MODE_OUTPUT_PWM_INV );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Mode_Pwm( TIM_PERIPH_1, &channelConfig ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_OCMODE_PWM2, LL_TIM_OC_GetMode( TIM1, LL_TIM_CHANNEL_CH2 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_CCER_CC2E, TIM1->CCER );
}


/**
 * \brief   Tim_Set_Mode_Pwm() rejects non PWM mode and NULL configuration.
 *
 * \details Calls the function with toggle mode and with NULL.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in both cases, CCMR1, CCER and EGR stay 0.
 */
void Ut_Tim_Set_Mode_Pwm_NonPwmMode_ReturnsErrorWithoutWrite( void )
{
    tim_ChannelConfig_t channelConfig = Ut_Tim_Get_ChannelConfig( TIM_CHANNEL_1, TIM_CHANNEL_MODE_OUTPUT_COMPARE_TOGGLE );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_Pwm( TIM_PERIPH_3, &channelConfig ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_Pwm( TIM_PERIPH_3, NULL ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCMR1 );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCER );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->EGR );
}


/**
 * \brief   Output compare mode is configured without compare preload.
 *
 * \details CCMR1 = OC1PE preset, TIM3 channel 1 in toggle mode.
 *
 * \par Expected results
 * - OC1M = toggle, OC1PE cleared, CCER = CC1E.
 */
void Ut_Tim_Set_Mode_OutputCompare_Toggle_ConfiguresWithoutPreload( void )
{
    tim_ChannelConfig_t channelConfig = Ut_Tim_Get_ChannelConfig( TIM_CHANNEL_1, TIM_CHANNEL_MODE_OUTPUT_COMPARE_TOGGLE );

    TIM3->CCMR1 = TIM_CCMR1_OC1PE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Mode_OutputCompare( TIM_PERIPH_3, &channelConfig ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_OCMODE_TOGGLE, LL_TIM_OC_GetMode( TIM3, LL_TIM_CHANNEL_CH1 ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCMR1 & TIM_CCMR1_OC1PE );
    TEST_ASSERT_EQUAL_HEX32( TIM_CCER_CC1E, TIM3->CCER );
}


/**
 * \brief   Output compare rejects mode not available on STM32F7.
 *
 * \details TIM3 channel 3 in pulse on compare mode.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, CCMR2 and CCER stay 0.
 */
void Ut_Tim_Set_Mode_OutputCompare_PulseModeNotAvailable_ReturnsError( void )
{
    tim_ChannelConfig_t channelConfig = Ut_Tim_Get_ChannelConfig( TIM_CHANNEL_3, TIM_CHANNEL_MODE_OUTPUT_COMPARE_PULSE );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_OutputCompare( TIM_PERIPH_3, &channelConfig ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCMR2 );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCER );
}


/**
 * \brief   PWM mode accepts combined PWM (4-bit OCxM on STM32F7).
 *
 * \details TIM3 channel 1 in combined PWM mode.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, OC1M = combined PWM mode 1 (OC1M bit 3 set), CCER.CC1E set.
 */
void Ut_Tim_Set_Mode_Pwm_CombinedPwm_WritesOcmBit3( void )
{
    tim_ChannelConfig_t channelConfig = Ut_Tim_Get_ChannelConfig( TIM_CHANNEL_1, TIM_CHANNEL_MODE_OUTPUT_COMBINED_PWM );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Mode_Pwm( TIM_PERIPH_3, &channelConfig ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_OCMODE_COMBINED_PWM1, LL_TIM_OC_GetMode( TIM3, LL_TIM_CHANNEL_CH1 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_CCMR1_OC1M_3, TIM3->CCMR1 & TIM_CCMR1_OC1M_3 );
    TEST_ASSERT_EQUAL_HEX32( TIM_CCER_CC1E, TIM3->CCER & TIM_CCER_CC1E );
}


/**
 * \brief   Channel 4 of advanced timer has no complementary output.
 *
 * \details TIM1 channel 4 PWM with complementary pin configured (TIM_1_CH1N_PA7, not
 *          used - channel 4 has no complementary output on STM32F7), then output 4N
 *          is activated.
 *
 * \par Expected results
 * - PWM: TIM_REQUEST_OK, CCER = CC4E only, BDTR = MOE.
 * - Output 4N: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Set_Mode_Pwm_Channel4_HasNoComplementaryOutput( void )
{
    tim_ChannelConfig_t channelConfig = Ut_Tim_Get_ChannelConfig( TIM_CHANNEL_4, TIM_CHANNEL_MODE_OUTPUT_PWM );

    channelConfig.IoComplPin = TIM_1_CH1N_PA7;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Mode_Pwm( TIM_PERIPH_1, &channelConfig ) );

    TEST_ASSERT_EQUAL_HEX32( TIM_CCER_CC4E, TIM1->CCER );
    TEST_ASSERT_EQUAL_HEX32( TIM_BDTR_MOE, TIM1->BDTR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_OutputActive( TIM_PERIPH_1, TIM_OUTPUT_4_N ) );
}


/**
 * \brief   Forced output mode is written and enabled.
 *
 * \details TIM3 channel 4 in forced active mode, then the function is called with PWM
 *          mode.
 *
 * \par Expected results
 * - OC4M = forced active, CCER = CC4E.
 * - PWM mode: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Set_Mode_ForcedOutput_WritesForcedMode( void )
{
    tim_ChannelConfig_t channelConfig = Ut_Tim_Get_ChannelConfig( TIM_CHANNEL_4, TIM_CHANNEL_MODE_OUTPUT_FORCED_ACTIVE );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Mode_ForcedOutput( TIM_PERIPH_3, &channelConfig ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_OCMODE_FORCED_ACTIVE, LL_TIM_OC_GetMode( TIM3, LL_TIM_CHANNEL_CH4 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_CCER_CC4E, TIM3->CCER );

    channelConfig.ChannelMode = TIM_CHANNEL_MODE_OUTPUT_PWM;
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_ForcedOutput( TIM_PERIPH_3, &channelConfig ) );
}


/**
 * \brief   Tim_InitChannel() rejects invalid arguments.
 *
 * \details Initializes TIM3 channel 5 (only 4 channels), channel with mode out of
 *          range, NULL configuration and channel of peripheral out of range.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, CCMR1 and CCER stay 0.
 */
void Ut_Tim_InitChannel_InvalidArgs_ReturnsErrorWithoutWrite( void )
{
    tim_ChannelConfig_t channelConfig = Ut_Tim_Get_ChannelConfig( TIM_CHANNEL_5, TIM_CHANNEL_MODE_OUTPUT_PWM );

    /* General purpose timer has only 4 channels */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_InitChannel( TIM_PERIPH_3, &channelConfig ) );

    channelConfig = Ut_Tim_Get_ChannelConfig( TIM_CHANNEL_1, TIM_CHANNEL_MODE_CNT );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_InitChannel( TIM_PERIPH_3, &channelConfig ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_InitChannel( TIM_PERIPH_3, NULL ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_InitChannel( TIM_PERIPH_CNT, &channelConfig ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCMR1 );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCER );
}

/* ============================= INPUT CAPTURE ============================== */

/**
 * \brief   Input capture mode configures the input stage.
 *
 * \details TIM3 channel 2 input capture with indirect input, both edges, filter
 *          fDTS/8 N=6 and prescaler 4.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, active input indirect TI, polarity both edges, filter fDTS/8 N=6,
 *   prescaler 4.
 * - CCER.CC2E set.
 */
void Ut_Tim_Set_Mode_InputCapture_ConfiguresInputStage( void )
{
    tim_ChannelConfig_t channelConfig = Ut_Tim_Get_ChannelConfig( TIM_CHANNEL_2, TIM_CHANNEL_MODE_INPUT_CAPTURE );

    channelConfig.ActiveInput    = TIM_ACTIVE_INPUT_INDIRECT;
    channelConfig.InputPolarity  = TIM_INPUT_POLARITY_BOTH_EDGES;
    channelConfig.InputFilter    = TIM_INPUT_FILTER_FDIV8_N6;
    channelConfig.InputPrescaler = TIM_INPUT_PRESCALER_DIV4;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Mode_InputCapture( TIM_PERIPH_3, &channelConfig ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_ACTIVEINPUT_INDIRECTTI,  LL_TIM_IC_GetActiveInput( TIM3, LL_TIM_CHANNEL_CH2 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_IC_POLARITY_BOTHEDGE,    LL_TIM_IC_GetPolarity( TIM3, LL_TIM_CHANNEL_CH2 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_IC_FILTER_FDIV8_N6,      LL_TIM_IC_GetFilter( TIM3, LL_TIM_CHANNEL_CH2 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_ICPSC_DIV4,              LL_TIM_IC_GetPrescaler( TIM3, LL_TIM_CHANNEL_CH2 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_CCER_CC2E, TIM3->CCER & TIM_CCER_CC2E );
}


/**
 * \brief   Input capture mode rejects invalid configurations.
 *
 * \details Configures TIM3 channel 1 with invalid polarity, filter out of range,
 *          output PWM mode and TIM1 channel 5 (no input stage of channel 5).
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, TIM3 CCMR1 / CCER stay 0.
 */
void Ut_Tim_Set_Mode_InputCapture_InvalidConfig_ReturnsErrorWithoutWrite( void )
{
    tim_ChannelConfig_t channelConfig = Ut_Tim_Get_ChannelConfig( TIM_CHANNEL_1, TIM_CHANNEL_MODE_INPUT_CAPTURE );

    channelConfig.InputPolarity = UT_TIM_INVALID_IC_POLARITY;
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_InputCapture( TIM_PERIPH_3, &channelConfig ) );

    channelConfig = Ut_Tim_Get_ChannelConfig( TIM_CHANNEL_1, TIM_CHANNEL_MODE_INPUT_CAPTURE );
    channelConfig.InputFilter = TIM_INPUT_FILTER_CNT;
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_InputCapture( TIM_PERIPH_3, &channelConfig ) );

    /* Output mode passed to input capture configuration */
    channelConfig = Ut_Tim_Get_ChannelConfig( TIM_CHANNEL_1, TIM_CHANNEL_MODE_OUTPUT_PWM );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_InputCapture( TIM_PERIPH_3, &channelConfig ) );

    /* Channel 5 has no input stage */
    channelConfig = Ut_Tim_Get_ChannelConfig( TIM_CHANNEL_5, TIM_CHANNEL_MODE_INPUT_CAPTURE );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_InputCapture( TIM_PERIPH_1, &channelConfig ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCMR1 );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCER );
}


/**
 * \brief   Tim_InitChannel() configures input capture channel.
 *
 * \details TIM3 channel 1 input capture with inverted polarity.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, active input direct TI, CCER = CC1E | CC1P.
 */
void Ut_Tim_InitChannel_InputCapture_ConfiguresInputStage( void )
{
    tim_ChannelConfig_t channelConfig = Ut_Tim_Get_ChannelConfig( TIM_CHANNEL_1, TIM_CHANNEL_MODE_INPUT_CAPTURE );

    channelConfig.InputPolarity = TIM_INPUT_POLARITY_INVERTED;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_InitChannel( TIM_PERIPH_3, &channelConfig ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_ACTIVEINPUT_DIRECTTI, LL_TIM_IC_GetActiveInput( TIM3, LL_TIM_CHANNEL_CH1 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_CCER_CC1E | TIM_CCER_CC1P, TIM3->CCER );
}


/**
 * \brief   Tim_Get_CaptureValue() reads CCR.
 *
 * \details CCR2 = 0x4321, reads TIM3 channel 2 capture value, then TIM1 channel 5 and
 *          NULL pointer.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, capture value 0x4321.
 * - Channel 5, NULL pointer: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Get_CaptureValue_ReadsCcr( void )
{
    tim_Counter_t captureValue = 0u;

    TIM3->CCR2 = 0x4321u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_CaptureValue( TIM_PERIPH_3, TIM_CHANNEL_2, &captureValue ) );
    TEST_ASSERT_EQUAL_HEX32( 0x4321u, captureValue );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_CaptureValue( TIM_PERIPH_1, TIM_CHANNEL_5, &captureValue ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_CaptureValue( TIM_PERIPH_3, TIM_CHANNEL_2, NULL ) );
}


/**
 * \brief   Input filter, prescaler and polarity are written.
 *
 * \details Sets TIM3 channel 3 filter fDTS/32 N=8, prescaler 8 and inverted polarity.
 *
 * \par Expected results
 * - Filter fDTS/32 N=8, prescaler 8, polarity falling.
 */
void Ut_Tim_Set_InputFilterPrescalerPolarity_WritesInputStage( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_InputFilter( TIM_PERIPH_3, TIM_CHANNEL_3, TIM_INPUT_FILTER_FDIV32_N8 ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_InputPrescaler( TIM_PERIPH_3, TIM_CHANNEL_3, TIM_INPUT_PRESCALER_DIV8 ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_InputPolarity( TIM_PERIPH_3, TIM_CHANNEL_3, TIM_INPUT_POLARITY_INVERTED ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_IC_FILTER_FDIV32_N8,  LL_TIM_IC_GetFilter( TIM3, LL_TIM_CHANNEL_CH3 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_ICPSC_DIV8,           LL_TIM_IC_GetPrescaler( TIM3, LL_TIM_CHANNEL_CH3 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_IC_POLARITY_FALLING,  LL_TIM_IC_GetPolarity( TIM3, LL_TIM_CHANNEL_CH3 ) );
}


/**
 * \brief   Input stage setters reject invalid arguments.
 *
 * \details Sets filter, prescaler and polarity out of range on TIM3 channel 1 and
 *          filter of TIM6.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, CCMR1 and CCER stay 0.
 */
void Ut_Tim_Set_InputFilterPrescalerPolarity_InvalidArgs_ReturnsErrorWithoutWrite( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_InputFilter( TIM_PERIPH_3, TIM_CHANNEL_1, TIM_INPUT_FILTER_CNT ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_InputPrescaler( TIM_PERIPH_3, TIM_CHANNEL_1, TIM_INPUT_PRESCALER_CNT ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_InputPolarity( TIM_PERIPH_3, TIM_CHANNEL_1, UT_TIM_INVALID_IC_POLARITY ) );
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_InputFilter( TIM_PERIPH_6, TIM_CHANNEL_1, TIM_INPUT_FILTER_INACTIVE ) );
#endif /* TIM6 */

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCMR1 );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCER );
}


/**
 * \brief   PWM input on channel 1 configures paired channels and reset slave mode.
 *
 * \details Sets TIM3 PWM input on channel 1, normal polarity, filter fDTS N=2.
 *
 * \par Expected results
 * - Channel 1: direct TI, rising edge.
 * - Channel 2: indirect TI, falling edge, filter fDTS N=2.
 * - SMCR = TS TI1FP1 | SMS reset.
 */
void Ut_Tim_Set_Mode_InputPwm_Channel1_ConfiguresPairedChannelsAndResetMode( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Mode_InputPwm( TIM_PERIPH_3, TIM_CHANNEL_1, TIM_INPUT_POLARITY_NORMAL, TIM_INPUT_FILTER_FDIV1_N2 ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_ACTIVEINPUT_DIRECTTI,   LL_TIM_IC_GetActiveInput( TIM3, LL_TIM_CHANNEL_CH1 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_IC_POLARITY_RISING,     LL_TIM_IC_GetPolarity( TIM3, LL_TIM_CHANNEL_CH1 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_ACTIVEINPUT_INDIRECTTI, LL_TIM_IC_GetActiveInput( TIM3, LL_TIM_CHANNEL_CH2 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_IC_POLARITY_FALLING,    LL_TIM_IC_GetPolarity( TIM3, LL_TIM_CHANNEL_CH2 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_IC_FILTER_FDIV1_N2,     LL_TIM_IC_GetFilter( TIM3, LL_TIM_CHANNEL_CH2 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_TS_TI1FP1 | LL_TIM_SLAVEMODE_RESET, TIM3->SMCR );
}


/**
 * \brief   PWM input on channel 2 uses TI2 as trigger.
 *
 * \details Sets TIM3 PWM input on channel 2, inverted polarity.
 *
 * \par Expected results
 * - Channel 1: indirect TI, rising edge; channel 2: falling edge.
 * - SMCR = TS TI2FP2 | SMS reset.
 */
void Ut_Tim_Set_Mode_InputPwm_Channel2_UsesTi2AsTrigger( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Mode_InputPwm( TIM_PERIPH_3, TIM_CHANNEL_2, TIM_INPUT_POLARITY_INVERTED, TIM_INPUT_FILTER_INACTIVE ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_ACTIVEINPUT_INDIRECTTI, LL_TIM_IC_GetActiveInput( TIM3, LL_TIM_CHANNEL_CH1 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_IC_POLARITY_RISING,     LL_TIM_IC_GetPolarity( TIM3, LL_TIM_CHANNEL_CH1 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_IC_POLARITY_FALLING,    LL_TIM_IC_GetPolarity( TIM3, LL_TIM_CHANNEL_CH2 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_TS_TI2FP2 | LL_TIM_SLAVEMODE_RESET, TIM3->SMCR );
}


/**
 * \brief   PWM input rejects invalid arguments and running timer.
 *
 * \details Sets PWM input on channel 3, with both edges polarity, filter out of range,
 *          on TIM6 and on running TIM3.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, CCMR1 and SMCR stay 0.
 */
void Ut_Tim_Set_Mode_InputPwm_InvalidArgs_ReturnsErrorWithoutWrite( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_InputPwm( TIM_PERIPH_3, TIM_CHANNEL_3, TIM_INPUT_POLARITY_NORMAL,     TIM_INPUT_FILTER_INACTIVE ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_InputPwm( TIM_PERIPH_3, TIM_CHANNEL_1, TIM_INPUT_POLARITY_BOTH_EDGES, TIM_INPUT_FILTER_INACTIVE ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_InputPwm( TIM_PERIPH_3, TIM_CHANNEL_1, TIM_INPUT_POLARITY_NORMAL,     TIM_INPUT_FILTER_CNT ) );
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_InputPwm( TIM_PERIPH_6, TIM_CHANNEL_1, TIM_INPUT_POLARITY_NORMAL,     TIM_INPUT_FILTER_INACTIVE ) );
#endif /* TIM6 */

    TIM3->CR1 = TIM_CR1_CEN;
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_InputPwm( TIM_PERIPH_3, TIM_CHANNEL_1, TIM_INPUT_POLARITY_NORMAL,     TIM_INPUT_FILTER_INACTIVE ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCMR1 );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SMCR );
}


/**
 * \brief   PWM input frequency and duty cycle are calculated from captures.
 *
 * \details Counter 1 MHz, CCR1 = 1000 (period), CCR2 = 250 (pulse).
 *
 * \par Expected results
 * - TIM_REQUEST_OK, frequency 1 kHz, duty cycle 25 %.
 */
void Ut_Tim_Get_InputPwm_CalculatesFrequencyAndDutyCycle( void )
{
    tim_FreqHz_t       frequency = 0u;
    tim_CentiPercent_t dutyCycle = 0u;

    TIM3->PSC  = UT_TIM_PSC_1MHZ;
    TIM3->CCR1 = 1000u;     /* Period: 1000 us */
    TIM3->CCR2 = 250u;      /* Pulse:   250 us */
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_InputPwm( TIM_PERIPH_3, TIM_CHANNEL_1, &frequency, &dutyCycle ) );

    TEST_ASSERT_EQUAL_UINT32( 1000u, frequency );
    TEST_ASSERT_EQUAL_UINT16( 2500u, dutyCycle );
}


/**
 * \brief   PWM input without captured period reports error.
 *
 * \details Counter 1 MHz, CCR1 = 0. Reads PWM input, then calls the function with NULL
 *          frequency pointer.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in both cases (no division by zero).
 */
void Ut_Tim_Get_InputPwm_NoPeriodCaptured_ReturnsError( void )
{
    tim_FreqHz_t       frequency = 0u;
    tim_CentiPercent_t dutyCycle = 0u;

    TIM3->PSC = UT_TIM_PSC_1MHZ;
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM3, UT_TIM_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_InputPwm( TIM_PERIPH_3, TIM_CHANNEL_1, &frequency, &dutyCycle ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_InputPwm( TIM_PERIPH_3, TIM_CHANNEL_1, NULL, &dutyCycle ) );
}


/**
 * \brief   Input source of TIM5 channel 4 is written to TIM5_OR remap.
 *
 * \details Sets TIM5 channel 4 input source LSI and reads it back, then sets GPIO input (pin).
 *
 * \par Expected results
 * - OR.TI4_RMP = LSI, the item TIM_INPUT_SOURCE_TIM5_CH4_LSI reads back.
 * - Pin: OR.TI4_RMP = 0, the item TIM_INPUT_SOURCE_TIM5_CH4_PIN reads back.
 */
void Ut_Tim_Set_InputSource_Tim5Channel4_WritesRemap( void )
{
    tim_InputSource_t inputSource = TIM_INPUT_SOURCE_TIM5_CH4_RTC_WKUP;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_InputSource( TIM_PERIPH_5, TIM_CHANNEL_4, TIM_INPUT_SOURCE_TIM5_CH4_LSI ) );
    TEST_ASSERT_EQUAL_HEX32( TIM5_OR_TI4_RMP_0, TIM5->OR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_InputSource( TIM_PERIPH_5, TIM_CHANNEL_4, &inputSource ) );
    TEST_ASSERT_EQUAL( TIM_INPUT_SOURCE_TIM5_CH4_LSI, inputSource );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_InputSource( TIM_PERIPH_5, TIM_CHANNEL_4, TIM_INPUT_SOURCE_TIM5_CH4_PIN ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM5->OR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_InputSource( TIM_PERIPH_5, TIM_CHANNEL_4, &inputSource ) );
    TEST_ASSERT_EQUAL( TIM_INPUT_SOURCE_TIM5_CH4_PIN, inputSource );
}


/**
 * \brief   Input source of TIM11 channel 1 selects SPDIFRX, HSE_RTC or MCO1.
 *
 * \details Sets TIM11 channel 1 input source HSE_RTC and reads it back, then MCO1 and SPDIFRX frame
 *          synchronization (devices with SPDIFRX).
 *
 * \par Expected results
 * - HSE_RTC: OR.TI1_RMP = HSE_RTC, the item reads back.
 * - MCO1: OR.TI1_RMP = MCO1.
 * - SPDIFRX: OR.TI1_RMP = SPDIFRX on the devices with SPDIFRX, otherwise code 1 is refused
 *   (TIM_REQUEST_ERROR) and OR is unchanged.
 */
void Ut_Tim_Set_InputSource_Tim11Channel1_SelectsHseRtc( void )
{
    tim_InputSource_t inputSource = TIM_INPUT_SOURCE_TIM11_CH1_PIN;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_InputSource( TIM_PERIPH_11, TIM_CHANNEL_1, TIM_INPUT_SOURCE_TIM11_CH1_HSE_RTC ) );
    TEST_ASSERT_EQUAL_HEX32( TIM11_OR_TI1_RMP_1, TIM11->OR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_InputSource( TIM_PERIPH_11, TIM_CHANNEL_1, &inputSource ) );
    TEST_ASSERT_EQUAL( TIM_INPUT_SOURCE_TIM11_CH1_HSE_RTC, inputSource );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_InputSource( TIM_PERIPH_11, TIM_CHANNEL_1, TIM_INPUT_SOURCE_TIM11_CH1_MCO1 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM11_OR_TI1_RMP, TIM11->OR );

#if defined(SPDIFRX)
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_InputSource( TIM_PERIPH_11, TIM_CHANNEL_1, TIM_INPUT_SOURCE_TIM11_CH1_SPDIFRX_FRAME_SYNC ) );
    TEST_ASSERT_EQUAL_HEX32( TIM11_OR_TI1_RMP_0, TIM11->OR );
#else
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_InputSource( TIM_PERIPH_11, TIM_CHANNEL_1, (tim_InputSource_t)TIM_INPUT_SOURCE_BIT_MASK_ENCODE( TIM_PERIPH_11, TIM_CHANNEL_1, 1u ) ) );
    TEST_ASSERT_EQUAL_HEX32( TIM11_OR_TI1_RMP, TIM11->OR );
#endif /* SPDIFRX */
}


/**
 * \brief   Channels without input remap and items of other timers / channels are refused.
 *
 * \details Sets an item for a channel without input remap, items of another timer and of another channel, a
 *          selection code out of the remap field, a channel out of range and reads the source of a channel
 *          without input remap and with NULL pointer.
 *
 * \par Expected results
 * - All calls: TIM_REQUEST_ERROR, the source variable and OR of the timers stay unchanged.
 */
void Ut_Tim_Set_InputSource_ChannelWithoutRemap_ReturnsError( void )
{
    tim_InputSource_t inputSource = TIM_INPUT_SOURCE_TIM5_CH4_PIN;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_InputSource( TIM_PERIPH_3, TIM_CHANNEL_1, (tim_InputSource_t)TIM_INPUT_SOURCE_BIT_MASK_ENCODE( TIM_PERIPH_3, TIM_CHANNEL_1, 0u ) ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_InputSource( TIM_PERIPH_3, TIM_CHANNEL_1, &inputSource ) );
    TEST_ASSERT_EQUAL( TIM_INPUT_SOURCE_TIM5_CH4_PIN, inputSource );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_InputSource( TIM_PERIPH_5, TIM_CHANNEL_4, TIM_INPUT_SOURCE_TIM11_CH1_HSE_RTC ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_InputSource( TIM_PERIPH_5, TIM_CHANNEL_3, TIM_INPUT_SOURCE_TIM5_CH4_LSI ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_InputSource( TIM_PERIPH_5, TIM_CHANNEL_4, (tim_InputSource_t)TIM_INPUT_SOURCE_BIT_MASK_ENCODE( TIM_PERIPH_5, TIM_CHANNEL_4, 4u ) ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_InputSource( TIM_PERIPH_1, TIM_CHANNEL_5, TIM_INPUT_SOURCE_TIM5_CH4_PIN ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_InputSource( TIM_PERIPH_5, TIM_CHANNEL_4, NULL ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->OR );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM5->OR );
}


/**
 * \brief   Clock division is written to CKD and read back.
 *
 * \details Sets TIM3 clock division 4 and reads it back, then sets division of TIM6
 *          and division out of range.
 *
 * \par Expected results
 * - CR1 = CKD /4, division reads back 4.
 * - TIM6 (no clock division), division out of range: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Set_ClockDivision_WritesCkd( void )
{
    tim_ClockDiv_t clockDiv = TIM_CLOCK_DIV_1;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_ClockDivision( TIM_PERIPH_3, TIM_CLOCK_DIV_4 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_CLOCKDIVISION_DIV4, TIM3->CR1 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_ClockDivision( TIM_PERIPH_3, &clockDiv ) );
    TEST_ASSERT_EQUAL( TIM_CLOCK_DIV_4, clockDiv );

    /* Basic timer has no clock division */
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ClockDivision( TIM_PERIPH_6, TIM_CLOCK_DIV_2 ) );
#endif /* TIM6 */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_ClockDivision( TIM_PERIPH_3, TIM_CLOCK_DIV_CNT ) );
}

/* =========================== ENCODER, HALL SENSOR ========================= */

/**
 * \brief   Encoder mode configures inputs, prescaler, period and encoder mode.
 *
 * \details PSC preset 0x55. TIM3 encoder mode x4 TI1 / TI2, TI1 inverted, TI2 normal
 *          polarity, filter fDTS N=8, period 1999.
 *
 * \par Expected results
 * - SMCR.SMS = encoder x4 TI12, PSC = 0, ARR = 1999.
 * - Channels 1 and 2 direct TI, channel 1 falling, channel 2 rising edge, filter fDTS N=8.
 */
void Ut_Tim_Set_Mode_Encoder_ConfiguresInputsAndEncoderMode( void )
{
    const tim_EncoderConfig_t encoderConfig =
    {
        .EncoderMode = TIM_ENCODER_MODE_X4_TI12,
        .Ti1Polarity = TIM_INPUT_POLARITY_INVERTED,
        .Ti2Polarity = TIM_INPUT_POLARITY_NORMAL,
        .InputFilter = TIM_INPUT_FILTER_FDIV1_N8,
        .Period      = 1999u
    };

    TIM3->PSC = 0x55u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Mode_Encoder( TIM_PERIPH_3, &encoderConfig ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_ENCODERMODE_X4_TI12,  TIM3->SMCR & TIM_SMCR_SMS );
    TEST_ASSERT_EQUAL_HEX32( 1999u, TIM3->ARR );
    TEST_ASSERT_EQUAL_HEX32( 0u,    TIM3->PSC );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_ACTIVEINPUT_DIRECTTI, LL_TIM_IC_GetActiveInput( TIM3, LL_TIM_CHANNEL_CH1 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_ACTIVEINPUT_DIRECTTI, LL_TIM_IC_GetActiveInput( TIM3, LL_TIM_CHANNEL_CH2 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_IC_POLARITY_FALLING,  LL_TIM_IC_GetPolarity( TIM3, LL_TIM_CHANNEL_CH1 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_IC_POLARITY_RISING,   LL_TIM_IC_GetPolarity( TIM3, LL_TIM_CHANNEL_CH2 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_IC_FILTER_FDIV1_N8,   LL_TIM_IC_GetFilter( TIM3, LL_TIM_CHANNEL_CH1 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_IC_FILTER_FDIV1_N8,   LL_TIM_IC_GetFilter( TIM3, LL_TIM_CHANNEL_CH2 ) );
}


/**
 * \brief   Encoder mode is not available on 2-channel timer TIM9 of STM32F7.
 *
 * \details TIM9 encoder mode x2 TI2, normal polarity, period 0xFFFF.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, SMCR, CCMR1 and ARR stay 0.
 */
void Ut_Tim_Set_Mode_Encoder_Tim9_ReturnsErrorWithoutWrite( void )
{
    const tim_EncoderConfig_t encoderConfig =
    {
        .EncoderMode = TIM_ENCODER_MODE_X2_TI2,
        .Ti1Polarity = TIM_INPUT_POLARITY_NORMAL,
        .Ti2Polarity = TIM_INPUT_POLARITY_NORMAL,
        .InputFilter = TIM_INPUT_FILTER_INACTIVE,
        .Period      = 0xFFFFu
    };

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_Encoder( TIM_PERIPH_9, &encoderConfig ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM9->SMCR );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM9->CCMR1 );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM9->ARR );
}


/**
 * \brief   Encoder mode rejects invalid configurations and running timer.
 *
 * \details Configures encoder with both edges polarity on TI1 and on TI2, mode x1 (not
 *          available on STM32F7), mode out of range, filter out of range, period over
 *          16-bit resolution of TIM3, on TIM6 and TIM10, with NULL configuration and on
 *          running TIM3.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, SMCR, CCMR1, PSC and ARR of TIM3 stay 0.
 */
void Ut_Tim_Set_Mode_Encoder_InvalidConfig_ReturnsErrorWithoutWrite( void )
{
    tim_EncoderConfig_t encoderConfig =
    {
        .EncoderMode = TIM_ENCODER_MODE_X2_TI1,
        .Ti1Polarity = TIM_INPUT_POLARITY_BOTH_EDGES,           /* Not allowed in encoder mode */
        .Ti2Polarity = TIM_INPUT_POLARITY_NORMAL,
        .InputFilter = TIM_INPUT_FILTER_INACTIVE,
        .Period      = 100u
    };

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_Encoder( TIM_PERIPH_3, &encoderConfig ) );

    encoderConfig.Ti1Polarity = TIM_INPUT_POLARITY_NORMAL;
    encoderConfig.Ti2Polarity = TIM_INPUT_POLARITY_BOTH_EDGES;
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_Encoder( TIM_PERIPH_3, &encoderConfig ) );

    encoderConfig.Ti2Polarity = TIM_INPUT_POLARITY_NORMAL;
    encoderConfig.EncoderMode = TIM_ENCODER_MODE_X1_TI1;        /* Not available on STM32F7 */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_Encoder( TIM_PERIPH_3, &encoderConfig ) );

    encoderConfig.EncoderMode = TIM_ENCODER_MODE_CNT;
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_Encoder( TIM_PERIPH_3, &encoderConfig ) );

    encoderConfig.EncoderMode = TIM_ENCODER_MODE_X2_TI1;
    encoderConfig.InputFilter = TIM_INPUT_FILTER_CNT;
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_Encoder( TIM_PERIPH_3, &encoderConfig ) );

    encoderConfig.InputFilter = TIM_INPUT_FILTER_INACTIVE;
    encoderConfig.Period      = 0x10000u;                       /* Over 16-bit resolution of TIM3 */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_Encoder( TIM_PERIPH_3, &encoderConfig ) );

    encoderConfig.Period = 100u;
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_Encoder( TIM_PERIPH_6, &encoderConfig ) );
#endif /* TIM6 */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_Encoder( TIM_PERIPH_10, &encoderConfig ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_Encoder( TIM_PERIPH_3, NULL ) );

    TIM3->CR1 = TIM_CR1_CEN;
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_Encoder( TIM_PERIPH_3, &encoderConfig ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SMCR );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCMR1 );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->PSC );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->ARR );
}


/**
 * \brief   Encoder position is read from the counter.
 *
 * \details CNT = 123, reads TIM3 encoder position, then position of TIM6 and with NULL.
 *
 * \par Expected results
 * - TIM_REQUEST_OK, position 123.
 * - TIM6, NULL pointer: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Get_EncoderPosition_ReadsCounter( void )
{
    tim_Counter_t position = 0u;

    TIM3->CNT = 123u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_EncoderPosition( TIM_PERIPH_3, &position ) );
    TEST_ASSERT_EQUAL_UINT32( 123u, position );

#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_EncoderPosition( TIM_PERIPH_6, &position ) );
#endif /* TIM6 */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_EncoderPosition( TIM_PERIPH_3, NULL ) );
}


/**
 * \brief   Encoder index is not available on STM32F7.
 *
 * \details Configures index of TIM3 with valid configuration and with NULL.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in both cases, SMCR stays 0.
 */
void Ut_Tim_Set_EncoderIndex_NotAvailable_ReturnsError( void )
{
    const tim_EncoderIndexConfig_t indexConfig =
    {
        .IndexState     = TIM_FUNCTION_ACTIVE,
        .Direction      = TIM_INDEX_DIR_UP,
        .Position       = TIM_INDEX_POS_UP_UP,
        .Blanking       = TIM_INDEX_BLANK_ALWAYS,
        .FirstIndexOnly = TIM_FUNCTION_ACTIVE
    };

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_EncoderIndex( TIM_PERIPH_3, &indexConfig ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_EncoderIndex( TIM_PERIPH_3, NULL ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SMCR );
}


/**
 * \brief   Hall sensor mode configures hall sensor interface.
 *
 * \details TIM3 hall sensor with normal polarity, filter fDTS N=4, prescaler 1,
 *          commutation delay 100.
 *
 * \par Expected results
 * - CR2.TI1S set (XOR of TI1 - TI3), channel 1 input TRC.
 * - SMCR = TS TI1F_ED | SMS reset.
 * - Channel 2 PWM mode 2, CCR2 = 100 (commutation delay), TRGO = OC2REF.
 */
void Ut_Tim_Set_Mode_HalSensor_ConfiguresHallInterface( void )
{
    const tim_HallSensorConfig_t hallConfig =
    {
        .InputPolarity    = TIM_INPUT_POLARITY_NORMAL,
        .InputFilter      = TIM_INPUT_FILTER_FDIV1_N4,
        .InputPrescaler   = TIM_INPUT_PRESCALER_DIV1,
        .CommutationDelay = 100u
    };

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Mode_HalSensor( TIM_PERIPH_3, &hallConfig ) );

    TEST_ASSERT_EQUAL_HEX32( TIM_CR2_TI1S, TIM3->CR2 & TIM_CR2_TI1S );                         /* XOR of TI1..TI3 */
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_ACTIVEINPUT_TRC, LL_TIM_IC_GetActiveInput( TIM3, LL_TIM_CHANNEL_CH1 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_IC_FILTER_FDIV1_N4, LL_TIM_IC_GetFilter( TIM3, LL_TIM_CHANNEL_CH1 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_TS_TI1F_ED | LL_TIM_SLAVEMODE_RESET, TIM3->SMCR );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_OCMODE_PWM2, LL_TIM_OC_GetMode( TIM3, LL_TIM_CHANNEL_CH2 ) );
    TEST_ASSERT_EQUAL_HEX32( 100u, TIM3->CCR2 );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_TRGO_OC2REF, TIM3->CR2 & TIM_CR2_MMS );
}


/**
 * \brief   Hall sensor mode rejects invalid arguments.
 *
 * \details Configures hall sensor with invalid polarity, filter and prescaler out of
 *          range, commutation delay over 16-bit resolution of TIM3, on TIM6 and TIM9
 *          (no hall interface), with NULL and on running TIM3.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, CR2, SMCR and CCR2 of TIM3 stay 0.
 */
void Ut_Tim_Set_Mode_HalSensor_InvalidArgs_ReturnsErrorWithoutWrite( void )
{
    tim_HallSensorConfig_t hallConfig =
    {
        .InputPolarity    = UT_TIM_INVALID_IC_POLARITY,
        .InputFilter      = TIM_INPUT_FILTER_INACTIVE,
        .InputPrescaler   = TIM_INPUT_PRESCALER_DIV1,
        .CommutationDelay = 10u
    };

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_HalSensor( TIM_PERIPH_3, &hallConfig ) );

    hallConfig.InputPolarity = TIM_INPUT_POLARITY_NORMAL;
    hallConfig.InputFilter   = TIM_INPUT_FILTER_CNT;
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_HalSensor( TIM_PERIPH_3, &hallConfig ) );

    hallConfig.InputFilter    = TIM_INPUT_FILTER_INACTIVE;
    hallConfig.InputPrescaler = TIM_INPUT_PRESCALER_CNT;
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_HalSensor( TIM_PERIPH_3, &hallConfig ) );

    hallConfig.InputPrescaler   = TIM_INPUT_PRESCALER_DIV1;
    hallConfig.CommutationDelay = 0x10000u;                     /* Over 16-bit resolution of TIM3 */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_HalSensor( TIM_PERIPH_3, &hallConfig ) );

    hallConfig.CommutationDelay = 10u;
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_HalSensor( TIM_PERIPH_6, &hallConfig ) );
#endif /* TIM6 */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_HalSensor( TIM_PERIPH_9, &hallConfig ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_HalSensor( TIM_PERIPH_3, NULL ) );

    TIM3->CR1 = TIM_CR1_CEN;
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Mode_HalSensor( TIM_PERIPH_3, &hallConfig ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CR2 );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SMCR );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CCR2 );
}

/* =========================== DEAD-TIME, BREAK ============================= */

/**
 * \brief   Dead-time in range 1 is written directly to DTG.
 *
 * \details Kernel clock 100 MHz, rising and falling dead-time 500 ns (50 fDTS periods).
 *
 * \par Expected results
 * - BDTR.DTG = 50.
 */
void Ut_Tim_Set_DeadTime_Range1_WritesDtgDirectly( void )
{
    /* 500 ns at 100 MHz = 50 fDTS periods */
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_DeadTime( TIM_PERIPH_1, 500u, 500u ) );

    TEST_ASSERT_EQUAL_HEX32( 50u, TIM1->BDTR );
}


/**
 * \brief   Dead-time in ranges 2 - 4 is encoded in DTG.
 *
 * \details Kernel clock 100 MHz, dead-time 2000 ns, 4000 ns and 8000 ns.
 *
 * \par Expected results
 * - 200 periods: range 2, DTG[5:0] = 36 ((64 + 36) * 2).
 * - 400 periods: range 3, DTG[4:0] = 18 ((32 + 18) * 8).
 * - 800 periods: range 4, DTG[4:0] = 18 ((32 + 18) * 16).
 */
void Ut_Tim_Set_DeadTime_HigherRanges_EncodesDtg( void )
{
    /* 200 periods: range 2 (64 + 36) * 2 */
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_DeadTime( TIM_PERIPH_1, 2000u, 2000u ) );
    TEST_ASSERT_EQUAL_HEX32( DT_RANGE_2 | 36u, TIM1->BDTR & TIM_BDTR_DTG );

    /* 400 periods: range 3 (32 + 18) * 8 */
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_DeadTime( TIM_PERIPH_1, 4000u, 4000u ) );
    TEST_ASSERT_EQUAL_HEX32( DT_RANGE_3 | 18u, TIM1->BDTR & TIM_BDTR_DTG );

    /* 800 periods: range 4 (32 + 18) * 16 */
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_DeadTime( TIM_PERIPH_1, 8000u, 8000u ) );
    TEST_ASSERT_EQUAL_HEX32( DT_RANGE_4 | 18u, TIM1->BDTR & TIM_BDTR_DTG );
}


/**
 * \brief   Dead-time is calculated from fDTS with clock division.
 *
 * \details Kernel clock 100 MHz, CKD /2 (fDTS 50 MHz), dead-time 500 ns.
 *
 * \par Expected results
 * - BDTR.DTG = 25.
 */
void Ut_Tim_Set_DeadTime_ClockDivision_UsesDtsClock( void )
{
    /* fDTS = 100 MHz / 2 -> 500 ns = 25 periods */
    TIM1->CR1 = LL_TIM_CLOCKDIVISION_DIV2;
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_DeadTime( TIM_PERIPH_1, 500u, 500u ) );

    TEST_ASSERT_EQUAL_HEX32( 25u, TIM1->BDTR );
}


/**
 * \brief   Asymmetric dead-time is rejected (single DTG field on STM32F7).
 *
 * \details Kernel clock 100 MHz, rising dead-time 100 ns, falling 200 ns.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, BDTR stays 0.
 */
void Ut_Tim_Set_DeadTime_Asymmetric_ReturnsErrorWithoutWrite( void )
{
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DeadTime( TIM_PERIPH_1, 100u, 200u ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->BDTR );
}


/**
 * \brief   Dead-time above maximum is rejected.
 *
 * \details Kernel clock 100 MHz, rising dead-time 10090 ns (1009 periods, maximum
 *          1008).
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, BDTR stays 0.
 */
void Ut_Tim_Set_DeadTime_TooLong_ReturnsErrorWithoutWrite( void )
{
    /* 1009 periods > maximum (32 + 31) * 16 = 1008 */
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DeadTime( TIM_PERIPH_1, 10090u, 100u ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->BDTR );
}


/**
 * \brief   Dead-time is rejected on timer without complementary outputs.
 *
 * \details Sets dead-time of TIM3 and of peripheral out of range.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in both cases, no RCC call (strict mock).
 */
void Ut_Tim_Set_DeadTime_GeneralTimer_ReturnsErrorWithoutRccAccess( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DeadTime( TIM_PERIPH_3, 100u, 100u ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DeadTime( TIM_PERIPH_CNT, 100u, 100u ) );
}


/**
 * \brief   Dead-time between DTG ranges 2 and 3 is rejected.
 *
 * \details Kernel clock 100 MHz, dead-time 2550 ns (255 periods, range 2 maximum 254,
 *          range 3 minimum 256).
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, BDTR stays 0.
 */
void Ut_Tim_Set_DeadTime_BetweenRanges2And3_ReturnsErrorWithoutWrite( void )
{
    /* 255 periods: above range 2 maximum (254), below range 3 minimum (256) */
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DeadTime( TIM_PERIPH_1, 2550u, 2550u ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->BDTR );
}


/**
 * \brief   Dead-time between DTG ranges 3 and 4 is rejected.
 *
 * \details Kernel clock 100 MHz, rising dead-time 5050 ns and 5110 ns (505 and 511
 *          periods, range 3 maximum 504, range 4 minimum 512).
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in both cases, BDTR stays 0.
 */
void Ut_Tim_Set_DeadTime_BetweenRanges3And4_ReturnsErrorWithoutWrite( void )
{
    /* 505 and 511 periods: above range 3 maximum (504), below range 4 minimum (512) */
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DeadTime( TIM_PERIPH_1, 5050u, 100u ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->BDTR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DeadTime( TIM_PERIPH_1, 5110u, 100u ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->BDTR );
}


/**
 * \brief   Not representable falling dead-time is rejected.
 *
 * \details Kernel clock 100 MHz, rising dead-time 100 ns (valid), falling 2550 ns
 *          (255 periods).
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR, BDTR stays 0 (rising dead-time not written either).
 */
void Ut_Tim_Set_DeadTime_FallingBetweenRanges_ReturnsErrorWithoutWrite( void )
{
    /* Valid rising edge (10 periods), falling edge 255 periods not representable */
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DeadTime( TIM_PERIPH_1, 100u, 2550u ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->BDTR );
}


/**
 * \brief   Dead-time at DTG range boundaries is encoded correctly.
 *
 * \details Kernel clock 100 MHz, dead-time 256, 504 and 512 periods.
 *
 * \par Expected results
 * - 256: range 3, DTG[4:0] = 0.
 * - 504: range 3, DTG[4:0] = 31.
 * - 512: range 4, DTG[4:0] = 0.
 */
void Ut_Tim_Set_DeadTime_RangeBoundaries_EncodedCorrectly( void )
{
    /* 256 periods: range 3 minimum (DTG[4:0] = 0) */
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_DeadTime( TIM_PERIPH_1, 2560u, 2560u ) );
    TEST_ASSERT_EQUAL_HEX32( DT_RANGE_3, TIM1->BDTR );

    /* 504 periods: range 3 maximum (DTG[4:0] = 31) */
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_DeadTime( TIM_PERIPH_1, 5040u, 5040u ) );
    TEST_ASSERT_EQUAL_HEX32( DT_RANGE_3 | DT_DELAY_3, TIM1->BDTR );

    /* 512 periods: range 4 minimum (DTG[4:0] = 0) */
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );
    Ut_Tim_Expect_PeriphClk( RCC_PERIPH_TIM1, UT_TIM_DT_CLK_HZ );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_DeadTime( TIM_PERIPH_1, 5120u, 5120u ) );
    TEST_ASSERT_EQUAL_HEX32( DT_RANGE_4, TIM1->BDTR );
}


/**
 * \brief   Break configuration is written to BDTR.
 *
 * \details TIM1 break active, polarity low, filter fDTS N=4, input mode, then break
 *          inactive with polarity high and no filter.
 *
 * \par Expected results
 * - BDTR = BKE | polarity low | filter fDTS N=4.
 * - BDTR = polarity high after de-activation.
 */
void Ut_Tim_Set_BreakConfig_WritesBreakFields( void )
{
    tim_BreakConfig_t breakConfig =
    {
        .BreakState    = TIM_FUNCTION_ACTIVE,
        .BreakPolarity = TIM_POLARITY_LOW,
        .BreakFilter   = TIM_INPUT_FILTER_FDIV1_N4,
        .BreakMode     = TIM_BREAK_MODE_INPUT
    };

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_BreakConfig( TIM_PERIPH_1, &breakConfig ) );

    TEST_ASSERT_EQUAL_HEX32( TIM_BDTR_BKE | LL_TIM_BREAK_POLARITY_LOW | LL_TIM_BREAK_FILTER_FDIV1_N4, TIM1->BDTR );

    breakConfig.BreakState    = TIM_FUNCTION_INACTIVE;
    breakConfig.BreakPolarity = TIM_POLARITY_HIGH;
    breakConfig.BreakFilter   = TIM_INPUT_FILTER_INACTIVE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_BreakConfig( TIM_PERIPH_1, &breakConfig ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_BREAK_POLARITY_HIGH, TIM1->BDTR );
}


/**
 * \brief   Break 2 configuration is written to BDTR.
 *
 * \details TIM1 break 2 active, polarity high, filter fDTS/32 N=8, input mode, then
 *          break 2 inactive with polarity low and no filter.
 *
 * \par Expected results
 * - BDTR = BK2E | polarity high | filter fDTS/32 N=8.
 * - BDTR = 0 after de-activation.
 */
void Ut_Tim_Set_Break2Config_WritesBreak2Fields( void )
{
    tim_BreakConfig_t breakConfig =
    {
        .BreakState    = TIM_FUNCTION_ACTIVE,
        .BreakPolarity = TIM_POLARITY_HIGH,
        .BreakFilter   = TIM_INPUT_FILTER_FDIV32_N8,
        .BreakMode     = TIM_BREAK_MODE_INPUT
    };

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Break2Config( TIM_PERIPH_1, &breakConfig ) );

    TEST_ASSERT_EQUAL_HEX32( TIM_BDTR_BK2E | LL_TIM_BREAK2_POLARITY_HIGH | LL_TIM_BREAK2_FILTER_FDIV32_N8, TIM1->BDTR );

    breakConfig.BreakState    = TIM_FUNCTION_INACTIVE;
    breakConfig.BreakPolarity = TIM_POLARITY_LOW;
    breakConfig.BreakFilter   = TIM_INPUT_FILTER_INACTIVE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Break2Config( TIM_PERIPH_1, &breakConfig ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->BDTR );
}


/**
 * \brief   Bidirectional break mode is not available on STM32F7.
 *
 * \details Configures TIM1 break and break 2 with bidirectional mode.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in both cases, BDTR stays 0.
 */
void Ut_Tim_Set_BreakConfig_BidirectionalMode_ReturnsErrorWithoutWrite( void )
{
    const tim_BreakConfig_t breakConfig =
    {
        .BreakState    = TIM_FUNCTION_ACTIVE,
        .BreakPolarity = TIM_POLARITY_HIGH,
        .BreakFilter   = TIM_INPUT_FILTER_INACTIVE,
        .BreakMode     = TIM_BREAK_MODE_BIDIRECTIONAL
    };

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_BreakConfig( TIM_PERIPH_1, &breakConfig ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Break2Config( TIM_PERIPH_1, &breakConfig ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->BDTR );
}


/**
 * \brief   Break configuration rejects invalid arguments.
 *
 * \details Configures break with filter out of range, mode out of range, break and
 *          break 2 of TIM3 and NULL configuration.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, BDTR of TIM1 and TIM3 stays 0.
 */
void Ut_Tim_Set_BreakConfig_InvalidArgs_ReturnsErrorWithoutWrite( void )
{
    tim_BreakConfig_t breakConfig =
    {
        .BreakState    = TIM_FUNCTION_ACTIVE,
        .BreakPolarity = TIM_POLARITY_HIGH,
        .BreakFilter   = TIM_INPUT_FILTER_CNT,
        .BreakMode     = TIM_BREAK_MODE_INPUT
    };

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_BreakConfig( TIM_PERIPH_1, &breakConfig ) );

    breakConfig.BreakFilter = TIM_INPUT_FILTER_INACTIVE;
    breakConfig.BreakMode   = TIM_BREAK_MODE_CNT;
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_BreakConfig( TIM_PERIPH_1, &breakConfig ) );

    breakConfig.BreakMode = TIM_BREAK_MODE_INPUT;
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_BreakConfig( TIM_PERIPH_3, &breakConfig ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Break2Config( TIM_PERIPH_3, &breakConfig ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_BreakConfig( TIM_PERIPH_1, NULL ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Break2Config( TIM_PERIPH_1, NULL ) );

    breakConfig.BreakFilter = TIM_INPUT_FILTER_CNT;
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_Break2Config( TIM_PERIPH_1, &breakConfig ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->BDTR );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->BDTR );
}


/**
 * \brief   Automatic output enable is set and cleared by AOE bit.
 *
 * \details Activates and deactivates automatic output of TIM1, then activates it on
 *          TIM3.
 *
 * \par Expected results
 * - BDTR = AOE, then 0.
 * - TIM3: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Set_AutomaticOutput_TogglesAoe( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_AutomaticOutputActive( TIM_PERIPH_1 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_BDTR_AOE, TIM1->BDTR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_AutomaticOutputInactive( TIM_PERIPH_1 ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->BDTR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_AutomaticOutputActive( TIM_PERIPH_3 ) );
}


/**
 * \brief   Automatic output enable is refused while the clock security system is enabled.
 *
 * \details RCC CR CSSON set, AOE of TIM1 activated and deactivated, then CSSON cleared.
 *
 * \note    Device errata bug AB#663 (ES0182 2.7.1 "PWM re-enabled in automatic output enable
 *          mode despite of system break"): the system break of the clock security system is
 *          not latched - with AOE the PWM outputs are re-armed on the next counter period after
 *          a clock failure. Break has to be used for fault protection with AOE inactive.
 *
 * \par Expected results
 * - CSSON set: activation TIM_REQUEST_ERROR (BDTR not written), deactivation accepted.
 * - CSSON cleared: activation accepted, BDTR = AOE.
 */
void Ut_Tim_Set_AutomaticOutput_CssEnabled_Refused( void )
{
    RCC->CR = RCC_CR_CSSON;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_AutomaticOutputActive( TIM_PERIPH_1 ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->BDTR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_AutomaticOutputInactive( TIM_PERIPH_1 ) );

    RCC->CR = 0u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_AutomaticOutputActive( TIM_PERIPH_1 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_BDTR_AOE, TIM1->BDTR );
}


/**
 * \brief   Off-state configuration is written to OSSI / OSSR bits.
 *
 * \details Sets TIM1 off-state idle active / run inactive, then idle inactive / run
 *          active, then invalid state and off-state of TIM3.
 *
 * \par Expected results
 * - BDTR = OSSI, then BDTR = OSSR.
 * - Invalid state, TIM3: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Set_OffStateConfig_WritesOssiOssr( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_OffStateConfig( TIM_PERIPH_1, TIM_FUNCTION_ACTIVE, TIM_FUNCTION_INACTIVE ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_BDTR_OSSI, TIM1->BDTR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_OffStateConfig( TIM_PERIPH_1, TIM_FUNCTION_INACTIVE, TIM_FUNCTION_ACTIVE ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_BDTR_OSSR, TIM1->BDTR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_OffStateConfig( TIM_PERIPH_1, (tim_FunctionState_t)( TIM_FUNCTION_ACTIVE + 1u ), TIM_FUNCTION_ACTIVE ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_OffStateConfig( TIM_PERIPH_3, TIM_FUNCTION_ACTIVE, TIM_FUNCTION_ACTIVE ) );
}


/**
 * \brief   Lock level is written to LOCK field.
 *
 * \details Sets TIM1 lock level 2, then level out of range and level of TIM3.
 *
 * \par Expected results
 * - BDTR = lock level 2.
 * - Level out of range, TIM3: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Set_LockLevel_WritesLock( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_LockLevel( TIM_PERIPH_1, TIM_LOCK_LEVEL_2 ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_LOCKLEVEL_2, TIM1->BDTR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_LockLevel( TIM_PERIPH_1, TIM_LOCK_LEVEL_CNT ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_LockLevel( TIM_PERIPH_3, TIM_LOCK_LEVEL_1 ) );
}


/**
 * \brief   Commutation preload is written to CCPC / CCUS bits.
 *
 * \details Activates TIM1 commutation preload with update by COMG and TRGI, then
 *          deactivates it. Calls the functions with update source out of range and
 *          for TIM3.
 *
 * \par Expected results
 * - CR2 = CCPC | CCUS, CCPC cleared after deactivation.
 * - Invalid arguments: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Set_CommutationPreload_WritesCcpcAndCcus( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CommutationPreloadActive( TIM_PERIPH_1, TIM_COMMUTATION_UPDATE_COMG_TRGI ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_CR2_CCPC | TIM_CR2_CCUS, TIM1->CR2 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CommutationPreloadInactive( TIM_PERIPH_1 ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM1->CR2 & TIM_CR2_CCPC );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_CommutationPreloadActive( TIM_PERIPH_1, TIM_COMMUTATION_UPDATE_CNT ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_CommutationPreloadActive( TIM_PERIPH_3, TIM_COMMUTATION_UPDATE_COMG ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_CommutationPreloadInactive( TIM_PERIPH_3 ) );
}

/* ======================== DMA AND EXTERNAL TRIGGER ======================== */

/**
 * \brief   DMA requests are enabled and disabled.
 *
 * \details Enables TIM3 update and CC2 DMA request, disables update request, enables
 *          TIM1 commutation DMA request.
 *
 * \par Expected results
 * - TIM3 DIER = UDE | CC2DE, then CC2DE only.
 * - TIM1 DIER = COMDE.
 */
void Ut_Tim_Set_DmaRequest_TogglesRequestEnable( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_DmaRequestActive( TIM_PERIPH_3, TIM_DMA_REQUEST_UPDATE ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_DmaRequestActive( TIM_PERIPH_3, TIM_DMA_REQUEST_CC2 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_DIER_UDE | TIM_DIER_CC2DE, TIM3->DIER );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_DmaRequestInactive( TIM_PERIPH_3, TIM_DMA_REQUEST_UPDATE ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_DIER_CC2DE, TIM3->DIER );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_DmaRequestActive( TIM_PERIPH_1, TIM_DMA_REQUEST_COMMUTATION ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_DIER_COMDE, TIM1->DIER );
}


/**
 * \brief   DMA request not available on the timer is rejected.
 *
 * \details Enables CC1 and trigger DMA request of TIM6, commutation and out of range
 *          request of TIM3, then update DMA request of TIM6.
 *
 * \par Expected results
 * - Not available requests: TIM_REQUEST_ERROR, DIER of TIM6 and TIM3 stays 0.
 * - TIM6 update DMA request: TIM_REQUEST_OK.
 */
void Ut_Tim_Set_DmaRequest_NotAvailable_ReturnsErrorWithoutWrite( void )
{
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DmaRequestActive( TIM_PERIPH_6, TIM_DMA_REQUEST_CC1 ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DmaRequestActive( TIM_PERIPH_6, TIM_DMA_REQUEST_TRIGGER ) );
#endif /* TIM6 */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DmaRequestActive( TIM_PERIPH_3, TIM_DMA_REQUEST_COMMUTATION ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DmaRequestActive( TIM_PERIPH_3, TIM_DMA_REQUEST_CNT ) );

#if defined(TIM6)
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM6->DIER );
#endif /* TIM6 */
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->DIER );

    /* Update DMA request is available on basic timer */
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_DmaRequestActive( TIM_PERIPH_6, TIM_DMA_REQUEST_UPDATE ) );
#endif /* TIM6 */
}


/**
 * \brief   DMA burst configuration is written to DCR.
 *
 * \details Sets TIM3 DMA burst on update request, base register CCR1, 4 transfers.
 *
 * \par Expected results
 * - DCR = base CCR1 | length 4 transfers (no burst source selection on STM32F7).
 */
void Ut_Tim_Set_DmaBurst_WritesDcr( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_DmaBurst( TIM_PERIPH_3, TIM_DMA_REQUEST_UPDATE, TIM_DMA_BURST_REG_CCR1, 4u ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_DMABURST_BASEADDR_CCR1 | LL_TIM_DMABURST_LENGTH_4TRANSFERS, TIM3->DCR );
}


/**
 * \brief   DMA burst base registers of STM32F7 timers are written to DCR.
 *
 * \details Sets TIM1 DMA burst on update request with base register CCR5 and 18
 *          transfers (maximum), then CCMR3 and 2 transfers, TIM2 burst with base
 *          register option register (OR1 maps to TIMx_OR).
 *
 * \par Expected results
 * - TIM1 DCR = base CCR5 | length 18 transfers, then base CCMR3 | length 2 transfers.
 * - TIM2 DCR = base OR | length 1 transfer.
 */
void Ut_Tim_Set_DmaBurst_Stm32F7Registers_WritesDcr( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_DmaBurst( TIM_PERIPH_1, TIM_DMA_REQUEST_UPDATE, TIM_DMA_BURST_REG_CCR5, 18u ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_DMABURST_BASEADDR_CCR5 | LL_TIM_DMABURST_LENGTH_18TRANSFERS, TIM1->DCR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_DmaBurst( TIM_PERIPH_1, TIM_DMA_REQUEST_UPDATE, TIM_DMA_BURST_REG_CCMR3, 2u ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_DMABURST_BASEADDR_CCMR3 | LL_TIM_DMABURST_LENGTH_2TRANSFERS, TIM1->DCR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_DmaBurst( TIM_PERIPH_2, TIM_DMA_REQUEST_UPDATE, TIM_DMA_BURST_REG_OR1, 1u ) );
    TEST_ASSERT_EQUAL_HEX32( LL_TIM_DMABURST_BASEADDR_OR | LL_TIM_DMABURST_LENGTH_1TRANSFER, TIM2->DCR );
}

/**
 * \brief   DMA burst rejects invalid arguments.
 *
 * \details Sets burst length 0 and 19 (maximum 18 on STM32F7), base register DTR2 (not
 *          available on STM32F7), commutation burst source of TIM3, base register out of
 *          range and burst of TIM6.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, DCR of TIM3 and TIM6 stays 0.
 */
void Ut_Tim_Set_DmaBurst_InvalidArgs_ReturnsErrorWithoutWrite( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DmaBurst( TIM_PERIPH_3, TIM_DMA_REQUEST_UPDATE, TIM_DMA_BURST_REG_CCR1, 0u ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DmaBurst( TIM_PERIPH_3, TIM_DMA_REQUEST_UPDATE, TIM_DMA_BURST_REG_CCR1, 19u ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DmaBurst( TIM_PERIPH_3, TIM_DMA_REQUEST_UPDATE, TIM_DMA_BURST_REG_DTR2, 1u ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DmaBurst( TIM_PERIPH_3, TIM_DMA_REQUEST_COMMUTATION, TIM_DMA_BURST_REG_CCR1, 1u ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DmaBurst( TIM_PERIPH_3, TIM_DMA_REQUEST_UPDATE, TIM_DMA_BURST_REG_CNT, 1u ) );
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DmaBurst( TIM_PERIPH_6, TIM_DMA_REQUEST_UPDATE, TIM_DMA_BURST_REG_ARR, 1u ) );
#endif /* TIM6 */

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->DCR );
#if defined(TIM6)
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM6->DCR );
#endif /* TIM6 */
}


/**
 * \brief   DMA register address is returned.
 *
 * \details Reads address of TIM3 CCR2 and TIM6 ARR, then of TIM6 CCR1 and DMAR,
 *          register out of range and with NULL pointer.
 *
 * \par Expected results
 * - Addresses of TIM3->CCR2 and TIM6->ARR.
 * - Not available registers, invalid arguments: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Get_DmaRegAddr_ReturnsRegisterAddress( void )
{
    tim_RegAddr_t regAddr = 0u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_DmaRegAddr( TIM_PERIPH_3, TIM_DMA_REG_CCR2, &regAddr ) );
    TEST_ASSERT_EQUAL_HEX32( (uint32_t)(uintptr_t)&TIM3->CCR2, regAddr );

#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_DmaRegAddr( TIM_PERIPH_6, TIM_DMA_REG_ARR, &regAddr ) );
    TEST_ASSERT_EQUAL_HEX32( (uint32_t)(uintptr_t)&TIM6->ARR, regAddr );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_DmaRegAddr( TIM_PERIPH_6, TIM_DMA_REG_CCR1, &regAddr ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_DmaRegAddr( TIM_PERIPH_6, TIM_DMA_REG_DMAR, &regAddr ) );
#endif /* TIM6 */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_DmaRegAddr( TIM_PERIPH_3, TIM_DMA_REG_CNT, &regAddr ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_DmaRegAddr( TIM_PERIPH_3, TIM_DMA_REG_ARR, NULL ) );
}


/**
 * \brief   ETR configuration is written to SMCR.
 *
 * \details Sets TIM3 ETR polarity low, prescaler 4, filter fDTS/2 N=6.
 *
 * \par Expected results
 * - SMCR = ETP inverted | ETPS /4 | ETF fDTS/2 N=6.
 */
void Ut_Tim_Set_EtrConfig_WritesSmcrEtrFields( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_EtrConfig( TIM_PERIPH_3, TIM_POLARITY_LOW, TIM_ETR_PRESCALER_DIV4, TIM_INPUT_FILTER_FDIV2_N6 ) );

    TEST_ASSERT_EQUAL_HEX32( LL_TIM_ETR_POLARITY_INVERTED | LL_TIM_ETR_PRESCALER_DIV4 | LL_TIM_ETR_FILTER_FDIV2_N6, TIM3->SMCR );
}


/**
 * \brief   ETR configuration rejects invalid arguments.
 *
 * \details Configures ETR of TIM6, prescaler out of range and filter out of range.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases, SMCR of TIM3 and TIM6 stays 0.
 */
void Ut_Tim_Set_EtrConfig_InvalidArgs_ReturnsErrorWithoutWrite( void )
{
#if defined(TIM6)
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_EtrConfig( TIM_PERIPH_6, TIM_POLARITY_HIGH, TIM_ETR_PRESCALER_DIV1, TIM_INPUT_FILTER_INACTIVE ) );
#endif /* TIM6 */
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_EtrConfig( TIM_PERIPH_3, TIM_POLARITY_HIGH, TIM_ETR_PRESCALER_CNT,  TIM_INPUT_FILTER_INACTIVE ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_EtrConfig( TIM_PERIPH_3, TIM_POLARITY_HIGH, TIM_ETR_PRESCALER_DIV1, TIM_INPUT_FILTER_CNT ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->SMCR );
#if defined(TIM6)
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM6->SMCR );
#endif /* TIM6 */
}


/**
 * \brief   ETR source selection is not available on STM32F7.
 *
 * \details Sets TIM2 ETR source pin and source 3.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in both cases, SMCR stays 0.
 */
void Ut_Tim_Set_EtrSource_NotAvailable_ReturnsError( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_EtrSource( TIM_PERIPH_2, TIM_ETR_SOURCE_PIN ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_EtrSource( TIM_PERIPH_2, TIM_ETR_SOURCE_3 ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM2->SMCR );
}

/* ============== DITHERING, UIF REMAPPING (NOT AVAILABLE ON F4) ============== */

/**
 * \brief   Dithering is not available on STM32F7.
 *
 * \details Activates / deactivates dithering of TIM3.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in both cases, CR1 stays 0.
 */
void Ut_Tim_Set_Dithering_NotAvailable_ReturnsError( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DitheringActive( TIM_PERIPH_3 ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_DitheringInactive( TIM_PERIPH_3 ) );

    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CR1 );
}


/**
 * \brief   UIF remapping is enabled and disabled by UIFREMAP bit.
 *
 * \details Activates and deactivates UIF remapping of TIM3, activates it for peripheral
 *          out of range.
 *
 * \par Expected results
 * - CR1 = UIFREMAP, then 0.
 * - Peripheral out of range: TIM_REQUEST_ERROR.
 */
void Ut_Tim_Set_UifRemap_TogglesUifremap( void )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_UifRemapActive( TIM_PERIPH_3 ) );
    TEST_ASSERT_EQUAL_HEX32( TIM_CR1_UIFREMAP, TIM3->CR1 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_UifRemapInactive( TIM_PERIPH_3 ) );
    TEST_ASSERT_EQUAL_HEX32( 0u, TIM3->CR1 );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Set_UifRemapActive( TIM_PERIPH_CNT ) );
}


/**
 * \brief   Counter with overflow splits UIF copy from counter value.
 *
 * \details UIF remap active. Reads TIM3 CNT = UIFCPY | 0x1234, then CNT = 0x1234,
 *          then TIM2 (32-bit timer, bit 31 is UIF copy) CNT = UIFCPY | 0x7FFFFFFF.
 *
 * \par Expected results
 * - Counter 0x1234 with overflow flag active.
 * - Overflow flag inactive without UIFCPY.
 * - TIM2: counter 0x7FFFFFFF with overflow flag active.
 */
void Ut_Tim_Get_CounterWithOverflow_SplitsUifCopy( void )
{
    tim_Counter_t   counterValue = 0u;
    tim_FlagState_t overflowFlag = TIM_FLAG_INACTIVE;

    TIM3->CR1 = TIM_CR1_UIFREMAP;
    TIM3->CNT = TIM_CNT_UIFCPY | 0x1234u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_CounterWithOverflow( TIM_PERIPH_3, &counterValue, &overflowFlag ) );
    TEST_ASSERT_EQUAL_HEX32( 0x1234u, counterValue );
    TEST_ASSERT_EQUAL( TIM_FLAG_ACTIVE, overflowFlag );

    TIM3->CNT = 0x1234u;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_CounterWithOverflow( TIM_PERIPH_3, &counterValue, &overflowFlag ) );
    TEST_ASSERT_EQUAL( TIM_FLAG_INACTIVE, overflowFlag );

    TIM2->CR1 = TIM_CR1_UIFREMAP;
    TIM2->CNT = TIM_CNT_UIFCPY | 0x7FFFFFFFu;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_CounterWithOverflow( TIM_PERIPH_2, &counterValue, &overflowFlag ) );
    TEST_ASSERT_EQUAL_HEX32( 0x7FFFFFFFu, counterValue );
    TEST_ASSERT_EQUAL( TIM_FLAG_ACTIVE, overflowFlag );
}


/**
 * \brief   Counter with overflow requires UIF remapping.
 *
 * \details UIF remap inactive, reads counter with overflow, then with NULL pointers and
 *          for peripheral out of range.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR in all cases.
 */
void Ut_Tim_Get_CounterWithOverflow_RemapInactive_ReturnsError( void )
{
    tim_Counter_t   counterValue = 0u;
    tim_FlagState_t overflowFlag = TIM_FLAG_INACTIVE;

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_CounterWithOverflow( TIM_PERIPH_3, &counterValue, &overflowFlag ) );

    TIM3->CR1 = TIM_CR1_UIFREMAP;
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_CounterWithOverflow( TIM_PERIPH_3, NULL, &overflowFlag ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_CounterWithOverflow( TIM_PERIPH_3, &counterValue, NULL ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Get_CounterWithOverflow( TIM_PERIPH_CNT, &counterValue, &overflowFlag ) );
}


/* ============================ GPIO CONFIGURATION ========================== */

/**
 * \brief   Channel pin is configured in alternate function mode.
 *
 * \details Initializes pin TIM1_CH1 PA8.
 *
 * \par Expected results
 * - Gpio_Init() called for PA8 with alternate function 1, TIM_REQUEST_OK.
 */
void Ut_Tim_InitIOPin_ConfiguresAlternateFunction( void )
{
    Ut_Tim_Expect_GpioInit( GPIO_PORT_A, GPIO_PIN_ID_8, GPIO_ALT_FUNC_1, GPIO_REQUEST_OK );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_InitIOPin( TIM_1_CH1_PA8 ) );
}


/**
 * \brief   GPIO error of complementary pin is reported.
 *
 * \details Initializes pin TIM1_CH1N PA7, GPIO mock returns error.
 *
 * \par Expected results
 * - TIM_REQUEST_ERROR.
 */
void Ut_Tim_InitIOComplPin_GpioError_ReturnsError( void )
{
    Ut_Tim_Expect_GpioInit( GPIO_PORT_A, GPIO_PIN_ID_7, GPIO_ALT_FUNC_1, GPIO_REQUEST_ERROR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_InitIOComplPin( TIM_1_CH1N_PA7 ) );
}


/**
 * \brief   ETR and break input pins are configured in alternate function mode.
 *
 * \details Initializes pins TIM1_ETR PA12 and TIM1_BKIN PB12.
 *
 * \par Expected results
 * - Gpio_Init() called for PA12 and PB12 with alternate function 1.
 * - TIM_REQUEST_OK in both cases.
 */
void Ut_Tim_InitTriggerAndBreakGpio_ConfiguresAlternateFunction( void )
{
    Ut_Tim_Expect_GpioInit( GPIO_PORT_A, GPIO_PIN_ID_12, GPIO_ALT_FUNC_1, GPIO_REQUEST_OK );
    Ut_Tim_Expect_GpioInit( GPIO_PORT_B, GPIO_PIN_ID_12, GPIO_ALT_FUNC_1, GPIO_REQUEST_OK );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_InitTriggerEventGpio( TIM_1_ETR_PA12 ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_InitBreakInputGpio( TIM_1_BKIN_PB12 ) );
}



/**
 * \brief   Pins of STM32F7 alternate function groups are encoded correctly.
 *
 * \details Initializes pins TIM8_CH1 PC6 (AF3, TIM9_CH1 PA2 on MCUs without TIM8), TIM12_CH2
 *          PH9 (AF9, MCUs with TIM12 only), TIM5_CH4 PA3 (AF2) and TIM2_ETR PA15 (AF1).
 *
 * \par Expected results
 * - Gpio_Init() called with port, pin and alternate function of the pin.
 * - TIM_REQUEST_OK in all cases.
 */
void Ut_Tim_InitIOPin_Stm32F7AlternateFunctions_EncodedCorrectly( void )
{
#if defined(TIM8)
    Ut_Tim_Expect_GpioInit( GPIO_PORT_C, GPIO_PIN_ID_6,  GPIO_ALT_FUNC_3, GPIO_REQUEST_OK );
#else
    Ut_Tim_Expect_GpioInit( GPIO_PORT_A, GPIO_PIN_ID_2,  GPIO_ALT_FUNC_3, GPIO_REQUEST_OK );
#endif /* TIM8 */
#if defined(TIM12)
    Ut_Tim_Expect_GpioInit( GPIO_PORT_H, GPIO_PIN_ID_9,  GPIO_ALT_FUNC_9, GPIO_REQUEST_OK );
#endif /* TIM12 */
    Ut_Tim_Expect_GpioInit( GPIO_PORT_A, GPIO_PIN_ID_3,  GPIO_ALT_FUNC_2, GPIO_REQUEST_OK );
    Ut_Tim_Expect_GpioInit( GPIO_PORT_A, GPIO_PIN_ID_15, GPIO_ALT_FUNC_1, GPIO_REQUEST_OK );

#if defined(TIM8)
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_InitIOPin( TIM_8_CH1_PC6 ) );
#else
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_InitIOPin( TIM_9_CH1_PA2 ) );
#endif /* TIM8 */
#if defined(TIM12)
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_InitIOPin( TIM_12_CH2_PH9 ) );
#endif /* TIM12 */
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_InitIOPin( TIM_5_CH4_PA3 ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_InitTriggerEventGpio( TIM_2_ETR_PA15 ) );
}


/**
 * \brief   Break input 2 pins are configured as alternate function.
 *
 * \details Initializes break input 2 pins TIM1_BKIN2 PE6 (AF1) and TIM8_BKIN2 PA8
 *          (AF3), then unused pin with GPIO error.
 *
 * \par Expected results
 * - Gpio_Init() called with port, pin and alternate function of the pin.
 * - TIM_REQUEST_OK for both pins, TIM_REQUEST_ERROR for GPIO error.
 */
void Ut_Tim_InitBreakInput2Gpio_ConfiguresAlternateFunction( void )
{
    Ut_Tim_Expect_GpioInit( GPIO_PORT_E, GPIO_PIN_ID_6, GPIO_ALT_FUNC_1, GPIO_REQUEST_OK );
    Ut_Tim_Expect_GpioInit( GPIO_PORT_A, GPIO_PIN_ID_8, GPIO_ALT_FUNC_3, GPIO_REQUEST_OK );
    Ut_Tim_Expect_GpioInit( GPIO_PORT_A, GPIO_PIN_ID_8, GPIO_ALT_FUNC_3, GPIO_REQUEST_ERROR );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_InitBreakInput2Gpio( TIM_1_BKIN2_PE6 ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_InitBreakInput2Gpio( TIM_8_BKIN2_PA8 ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_InitBreakInput2Gpio( TIM_8_BKIN2_PA8 ) );
}


/**
 * \brief   Interrupt handler of every interrupt line processes its own timer.
 *
 * \details For every interrupt line not covered by other tests (TIM1 CC, TIM2, TIM4, TIM5,
 *          TIM6, TIM7, TIM8 break 2 / update / trigger / CC - lines of the MCU only): the
 *          callback and the interrupt of the timer are enabled, the flag is set and the ISR
 *          captured from the NVIC line is called.
 *
 * \par Expected results
 * - ISR registered for the NVIC line of the timer interrupt.
 * - Callback of the interrupt called once, flag of the interrupt cleared.
 */
void Ut_Tim_Isr_EveryLine_OwnTimerProcessed( void )
{
    const struct
    {
        tim_PeriphId_t        PeriphId;
        TIM_TypeDef *         PeriphReg;
        tim_IrqId_t           IrqId;
        nvic_PeriphIrqList_t  NvicIrqId;
        uint32_t              SrFlag;
    }   lineLut[] =
    {
        { TIM_PERIPH_1, TIM1, TIM_IRQ_CAPTURE_COMPARE_CH1, NVIC_PERIPH_IRQ_TIM1_CC,           TIM_SR_CC1IF },
#ifdef TIM2
        { TIM_PERIPH_2, TIM2, TIM_IRQ_UPDATE,              NVIC_PERIPH_IRQ_TIM2,              TIM_SR_UIF   },
#endif /* TIM2 */
#ifdef TIM4
        { TIM_PERIPH_4, TIM4, TIM_IRQ_UPDATE,              NVIC_PERIPH_IRQ_TIM4,              TIM_SR_UIF   },
#endif /* TIM4 */
#ifdef TIM5
        { TIM_PERIPH_5, TIM5, TIM_IRQ_UPDATE,              NVIC_PERIPH_IRQ_TIM5,              TIM_SR_UIF   },
#endif /* TIM5 */
#ifdef TIM6
#if defined(DAC)
        { TIM_PERIPH_6, TIM6, TIM_IRQ_UPDATE,              NVIC_PERIPH_IRQ_TIM6_DAC,          TIM_SR_UIF   },
#else
        { TIM_PERIPH_6, TIM6, TIM_IRQ_UPDATE,              NVIC_PERIPH_IRQ_TIM6,              TIM_SR_UIF   },
#endif /* DAC */
#endif /* TIM6 */
#ifdef TIM7
        { TIM_PERIPH_7, TIM7, TIM_IRQ_UPDATE,              NVIC_PERIPH_IRQ_TIM7,              TIM_SR_UIF   },
#endif /* TIM7 */
#ifdef TIM8
        { TIM_PERIPH_8, TIM8, TIM_IRQ_BREAK2,              NVIC_PERIPH_IRQ_TIM8_BRK_TIM12,    TIM_SR_B2IF  },
        { TIM_PERIPH_8, TIM8, TIM_IRQ_UPDATE,              NVIC_PERIPH_IRQ_TIM8_UP_TIM13,     TIM_SR_UIF   },
        { TIM_PERIPH_8, TIM8, TIM_IRQ_TRIGGER,             NVIC_PERIPH_IRQ_TIM8_TRG_COM_TIM14, TIM_SR_TIF  },
        { TIM_PERIPH_8, TIM8, TIM_IRQ_CAPTURE_COMPARE_CH1, NVIC_PERIPH_IRQ_TIM8_CC,           TIM_SR_CC1IF },
#endif /* TIM8 */
    };

    for( uint32_t idx = 0u; ( sizeof( lineLut ) / sizeof( lineLut[ 0u ] ) ) > idx; idx++ )
    {
        const tim_PeriphId_t periphId = lineLut[ idx ].PeriphId;

        TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_UpdateCallback( periphId, Ut_Tim_UpdateCallback ) );
        TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_TriggerCallback( periphId, Ut_Tim_TriggerCallback ) );
        if( TIM_IRQ_CAPTURE_COMPARE_CH1 == lineLut[ idx ].IrqId )
        {
            TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CaptureCompareCallback( periphId, TIM_CHANNEL_1, Ut_Tim_CaptureCompareCallback ) );
        }
        else
        {
            /* No action required */
        }
        if( TIM_IRQ_BREAK2 == lineLut[ idx ].IrqId )
        {
            TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Break2Callback( periphId, Ut_Tim_Break2Callback ) );
        }
        else
        {
            /* No action required */
        }

        Ut_Tim_Enable_Irq( periphId, lineLut[ idx ].IrqId, lineLut[ idx ].NvicIrqId );

        utTim_UpdateCnt         = 0u;
        utTim_TriggerCnt        = 0u;
        utTim_Break2Cnt         = 0u;
        utTim_CaptureCompareCnt = 0u;
        lineLut[ idx ].PeriphReg->SR = lineLut[ idx ].SrFlag;

        utTim_Isr[ lineLut[ idx ].NvicIrqId ]();

        TEST_ASSERT_EQUAL_UINT32_MESSAGE( 1u, utTim_UpdateCnt + utTim_TriggerCnt + utTim_Break2Cnt + utTim_CaptureCompareCnt,
                                          "One callback of the interrupt" );
        TEST_ASSERT_EQUAL_HEX32( 0u, lineLut[ idx ].PeriphReg->SR & lineLut[ idx ].SrFlag );

        lineLut[ idx ].PeriphReg->DIER = 0u;
    }
}

/* =========================== LOCAL FUNCTIONS ============================== */

/** Stub of Nvic_Set_PeriphIrq_Handler - stores registered timer ISR */
static nvic_RequestState_t Ut_Tim_NvicSetHandlerStub( nvic_PeriphIrqList_t irqId, const nvic_IsrCallback_t irqHandler, int callCnt )
{
    (void)callCnt;

    TEST_ASSERT_TRUE( NVIC_PERIPH_IRQ_SIZE > irqId );
    TEST_ASSERT_NOT_NULL( irqHandler );

    utTim_Isr[ irqId ] = irqHandler;
    utTim_HandlerCallCnt++;

    return ( NVIC_REQUEST_OK );
}


/**
 * \brief Expects request of timer kernel clock frequency.
 *
 * \param rccId     [in]: RCC identification of the timer
 * \param periphClk [in]: Returned kernel clock frequency in Hz
 */
static void Ut_Tim_Expect_PeriphClk( rcc_PeriphId_t rccId, rcc_FreqHz_t periphClk )
{
    TEST_ASSERT_TRUE( UT_TIM_MOCK_VALUE_CNT > utTim_PeriphClkIdx );

    /* Value is copied by mock during the call - it has to stay valid until then */
    utTim_PeriphClk[ utTim_PeriphClkIdx ] = periphClk;

    Rcc_Get_PeriphClk_ExpectAndReturn( rccId, NULL, RCC_REQUEST_OK );
    Rcc_Get_PeriphClk_IgnoreArg_periphClk();
    Rcc_Get_PeriphClk_ReturnThruPtr_periphClk( &utTim_PeriphClk[ utTim_PeriphClkIdx ] );

    utTim_PeriphClkIdx++;
}


/**
 * \brief Expects activation of timer clock which is inactive.
 *
 * \param rccId [in]: RCC identification of the timer
 */
static void Ut_Tim_Expect_ClockActivation( rcc_PeriphId_t rccId )
{
    static rcc_FunctionState_t clockState = RCC_FUNCTION_INACTIVE;

    Rcc_Get_PeriphState_ExpectAndReturn( rccId, NULL, RCC_REQUEST_OK );
    Rcc_Get_PeriphState_IgnoreArg_funcState();
    Rcc_Get_PeriphState_ReturnThruPtr_funcState( &clockState );
    Rcc_Set_PeriphActive_ExpectAndReturn( rccId, RCC_REQUEST_OK );
}


/**
 * \brief Expects GPIO configuration of timer pin (alternate function, push-pull, medium speed).
 *
 * \param portId   [in]: Expected GPIO port
 * \param pinId    [in]: Expected GPIO pin
 * \param altFunc  [in]: Expected alternate function
 * \param retState [in]: Result returned by GPIO module
 */
static void Ut_Tim_Expect_GpioInit( gpio_PortId_t portId, gpio_PinId_t pinId, gpio_AltFunction_t altFunc, gpio_RequestState_t retState )
{
    TEST_ASSERT_TRUE( UT_TIM_GPIO_CFG_CNT > utTim_GpioCfgIdx );

    gpio_Config_t * const expectedCfg = &utTim_GpioCfg[ utTim_GpioCfgIdx ];

    *expectedCfg                = (gpio_Config_t){ 0u };
    expectedCfg->PortId         = portId;
    expectedCfg->PinId          = pinId;
    expectedCfg->PinMode        = GPIO_PIN_MODE_ALTERNATE;
    expectedCfg->PinPull        = GPIO_PIN_PULL_NONE;
    expectedCfg->PinSpeed       = GPIO_PIN_SPEED_MEDIUM;
    expectedCfg->PinOutType     = GPIO_PIN_OUTPUT_PUSHPULL;
    expectedCfg->PinAltFunction = altFunc;

    Gpio_Init_ExpectAndReturn( expectedCfg, retState );

    utTim_GpioCfgIdx++;
}


/**
 * \brief Expects successful de-initialization of timer (NVIC lines, reset, clock).
 *
 * \param rccId    [in]: RCC identification of the timer
 * \param irqLines [in]: NVIC lines of the timer in order of processing
 */
static void Ut_Tim_Expect_Deinit( rcc_PeriphId_t rccId, const nvic_PeriphIrqList_t * const irqLines )
{
    for( uint32_t lineIdx = 0u; UT_TIM_IRQ_LINE_CNT > lineIdx; lineIdx++ )
    {
        Nvic_Set_PeriphIrq_Inactive_ExpectAndReturn( irqLines[ lineIdx ], NVIC_REQUEST_OK );
    }

    Rcc_Set_ResetActive_ExpectAndReturn( rccId, RCC_REQUEST_OK );
    Rcc_Set_ResetInactive_ExpectAndReturn( rccId, RCC_REQUEST_OK );
    Rcc_Set_PeriphInactive_ExpectAndReturn( rccId, RCC_REQUEST_OK );
}


/**
 * \brief Activates timer interrupt (ISR is captured by NVIC stub).
 *
 * \param periphId  [in]: Timer peripheral
 * \param irqId     [in]: Timer interrupt
 * \param nvicIrqId [in]: Expected NVIC line of the interrupt
 */
static void Ut_Tim_Enable_Irq( tim_PeriphId_t periphId, tim_IrqId_t irqId, nvic_PeriphIrqList_t nvicIrqId )
{
    Nvic_Set_PeriphIrq_Active_ExpectAndReturn( nvicIrqId, NVIC_REQUEST_OK );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_IrqActive( periphId, irqId ) );
    TEST_ASSERT_NOT_NULL( utTim_Isr[ nvicIrqId ] );
}


/** De-registers all user callbacks of all timers (module keeps them between tests) */
static void Ut_Tim_Reset_Callbacks( void )
{
    for( uint32_t periphId = 0u; TIM_PERIPH_CNT > periphId; periphId++ )
    {
        const tim_PeriphId_t timId = (tim_PeriphId_t)periphId;

        TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_UpdateCallback( timId, NULL ) );
        TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_TriggerCallback( timId, NULL ) );
        TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CommutationCallback( timId, NULL ) );
        TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_BreakCallback( timId, NULL ) );
        TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Break2Callback( timId, NULL ) );

        for( uint32_t channelId = 0u; UT_TIM_CC_CALLBACK_CNT > channelId; channelId++ )
        {
            /* Fails for channels not available on the timer - no callback can be registered there */
            (void)Tim_Set_CaptureCompareCallback( timId, (tim_ChannelId_t)channelId, NULL );
        }
    }
}


/**
 * \brief Returns default configuration of timer.
 *
 * \param periphId [in]: Timer peripheral
 *
 * \return Default configuration (10 MHz counter, 1 MHz refresh, no channel)
 */
static tim_PeriphConfig_t Ut_Tim_Get_Config( tim_PeriphId_t periphId )
{
    tim_PeriphConfig_t config;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_DefaultConfig( &config ) );

    config.PeriphId = periphId;

    return ( config );
}


/**
 * \brief Returns default configuration of timer channel.
 *
 * \param channelId   [in]: Channel identification
 * \param channelMode [in]: Channel mode
 *
 * \return Channel configuration (active, no pins, default polarities)
 */
static tim_ChannelConfig_t Ut_Tim_Get_ChannelConfig( tim_ChannelId_t channelId, tim_ChannelMode_t channelMode )
{
    tim_PeriphConfig_t  config        = Ut_Tim_Get_Config( TIM_PERIPH_CNT );
    tim_ChannelConfig_t channelConfig = config.ChannelConfig[ TIM_CHANNEL_1 ];

    channelConfig.ChannelId    = channelId;
    channelConfig.ChannelState = TIM_FUNCTION_ACTIVE;
    channelConfig.ChannelMode  = channelMode;

    return ( channelConfig );
}


/*------------------------- User callback recorders -------------------------*/

static void Ut_Tim_UpdateCallback( void )
{
    utTim_UpdateCnt++;
}


static void Ut_Tim_CaptureCompareCallback( tim_OvercaptureFlag_t overcaptureFlag )
{
    utTim_CaptureCompareCnt++;
    utTim_LastOvercapture = overcaptureFlag;
}


static void Ut_Tim_TriggerCallback( void )
{
    utTim_TriggerCnt++;
}


static void Ut_Tim_CommutationCallback( void )
{
    utTim_CommutationCnt++;
}


static void Ut_Tim_BreakCallback( void )
{
    utTim_BreakCnt++;
}


/** Break 2 interrupt user callback */
static void Ut_Tim_Break2Callback( void )
{
    utTim_Break2Cnt++;
}





