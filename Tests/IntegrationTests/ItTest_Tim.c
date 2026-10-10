/**
 * \author Mr.Nobody
 * \file ItTest_Tim.c
 * \ingroup Tim
 * \brief Integration tests of Timer (TIM) module on target.
 *
 * Tim module runs on the MCU together with real RCC, NVIC and GPIO modules and
 * hardware. Tests verify behavior which cannot be verified by unit tests
 * (emulated registers): counter clocking from the RCC kernel clock (prescaler),
 * counting, update / capture / compare events, interrupts with user callbacks,
 * repetition counter, one pulse mode, master / slave synchronization of timers
 * and timer output signal on the pin (forced output, polarity, main output and
 * idle state, PWM).
 *
 * Used resources:
 * - TIM2 - 32-bit timer, reference time base 1 MHz (\ref It_Tim_Wait_RefTime)
 * - TIM1 - advanced timer, channel 1 output on IT_TIM_PWM_* pin
 * - TIM3 - slave of TIM2 (internal trigger ITR1 = TIM2_TRGO)
 * - TIM6 - basic timer
 * - TIM9 - general purpose timer with interrupt on NVIC line shared with TIM1
 *          break interrupt (TIM1_BRK_TIM9)
 * - IT_TIM_PWM_* - TIM1_CH1 pin not connected on the board (no LED, pull
 *                  resistor or solder bridge) and on connectors. Timer output
 *                  is read back by GPIO input data register (input stage is
 *                  active in alternate function mode), no external wiring.
 *
 * \note Expected frequencies are exact if the timer kernel clock is an integer
 *       multiple of 1 MHz (default clock configuration of Rcc_Init: APB1 timers
 *       108 MHz, APB2 timers 216 MHz).
 *
 * Boards (named by the MCU as the detection of the connected boards does):
 * - STM32F745xG / STM32F746xG - 32F746GDISCOVERY (STM32F746NG); the detection
 *   names every board with ID 0x449 and 1 MB flash STM32F745xG
 * - STM32F722xE, STM32F756xG, STM32F765xI / STM32F767xI - NUCLEO-F722ZE,
 *   NUCLEO-F756ZG (board file override with the name STM32F756xG), NUCLEO-F767ZI
 *   (Nucleo-144 boards, common pinout)
 */

/* ============================= INCLUDES =================================== */
#include "unity.h"                          /* Unity testing framework        */
#include "IntegrationTesting.h"             /* Integration testing on target  */
#include "Tim_Port.h"                       /* Module under test              */
#include "Gpio_Port.h"                      /* Pin level of timer output      */
/* ============================= TYPEDEFS =================================== */

/* ======================= FORWARD DECLARATIONS ============================= */

static void It_Tim_Get_TimeBaseConfig   ( tim_PeriphConfig_t * const timConfig, tim_PeriphId_t periphId, tim_FreqHz_t timerFreq, tim_FreqHz_t refreshFreq );
static void It_Tim_Init_TimeBase        ( tim_PeriphId_t periphId, tim_FreqHz_t timerFreq, tim_FreqHz_t refreshFreq );
static void It_Tim_Init_PinChannel      ( tim_ChannelMode_t channelMode, tim_Polarity_t outputPolarity, tim_Polarity_t idleState );
static void It_Tim_Apply_UpdateEvent    ( tim_PeriphId_t periphId );
static void It_Tim_Wait_RefTime         ( tim_Counter_t waitTime_us );
static void It_Tim_Wait_CallbackCount   ( volatile const uint32_t * const callbackCnt, uint32_t expectedCnt );
static void It_Tim_WaitPinSettled       ( void );
static void It_Tim_Check_PinLevel       ( gpio_PinLevel_t expectedLevel );

static void It_Tim_UpdateCallback       ( void );
static void It_Tim_CaptureCompareCallback( tim_OvercaptureFlag_t overcaptureFlag );

/* ========================= SYMBOLIC CONSTANTS ============================= */

/*----------------------------- Board configuration --------------------------*/
/* Boards are named by their MCU (IT_BOARD_<MCU>, name of the board from the detection) */
#if defined(IT_BOARD_STM32F745xG) || \
    defined(IT_BOARD_STM32F746xG)

    /* 32F746GDISCOVERY */

    /** Arduino D10 (PA8, TIM1_CH1) - not connected on the board */
    #define IT_TIM_PWM_PIN                  ( TIM_1_CH1_PA8 )
    #define IT_TIM_PWM_PORT                 ( GPIO_PORT_A )
    #define IT_TIM_PWM_PIN_ID               ( GPIO_PIN_ID_8 )

#elif defined(IT_BOARD_STM32F722xE) || \
      defined(IT_BOARD_STM32F756xG) || \
      defined(IT_BOARD_STM32F765xI) || \
      defined(IT_BOARD_STM32F767xI)

    /* NUCLEO-F722ZE / NUCLEO-F756ZG / NUCLEO-F767ZI (Nucleo-144) */

    /** PA8 (TIM1_CH1, USB SOF test point TP1) - not connected on the board */
    #define IT_TIM_PWM_PIN                  ( TIM_1_CH1_PA8 )
    #define IT_TIM_PWM_PORT                 ( GPIO_PORT_A )
    #define IT_TIM_PWM_PIN_ID               ( GPIO_PIN_ID_8 )

#else
    #error "Board of Tim integration tests is not defined (INTEGRATION_TEST_BOARD)."
#endif

/*------------------------------ Timers --------------------------------------*/

/** Reference timer (32-bit), counts microseconds */
#define IT_TIM_REF                          ( TIM_PERIPH_2 )

/** Advanced timer with channel 1 output on IT_TIM_PWM_PIN */
#define IT_TIM_ADV                          ( TIM_PERIPH_1 )

/** General purpose timer, slave of IT_TIM_REF */
#define IT_TIM_SLAVE                        ( TIM_PERIPH_3 )

/** Trigger input of IT_TIM_SLAVE connected to TRGO of IT_TIM_REF (TIM3 ITR1 = TIM2_TRGO) */
#define IT_TIM_SLAVE_ITR                    ( TIM_TRIGGER_INPUT_TIM3_ITR1_TIM2_TRGO )

/** Basic timer (time base only) - TIM6 on every STM32F7 (TIM11 on MCUs without TIM6) */
#if defined(TIM6)
    #define IT_TIM_BASIC                    ( TIM_PERIPH_6 )
#else
    #define IT_TIM_BASIC                    ( TIM_PERIPH_11 )
#endif /* TIM6 */

/** General purpose timer with interrupt on NVIC line shared with TIM1 break */
#define IT_TIM_SHARED_IRQ                   ( TIM_PERIPH_9 )

/*------------------------------ Timing --------------------------------------*/

/** Counter frequency of the reference timer [Hz] (1 count = 1 us) */
#define IT_TIM_REF_FREQ_HZ                  ( 1000000u )

/** Refresh frequency of the reference timer [Hz] (period 1 s, no overflow during the tests) */
#define IT_TIM_REF_REFRESH_HZ               ( 1u )

/** Counter frequency of timers under test [Hz] */
#define IT_TIM_FREQ_HZ                      ( 1000000u )

/** Refresh frequency of timers under test [Hz] (period 1000 counts) */
#define IT_TIM_REFRESH_HZ                   ( 1000u )

/** Auto-reload value for IT_TIM_FREQ_HZ / IT_TIM_REFRESH_HZ */
#define IT_TIM_PERIOD                       ( ( IT_TIM_FREQ_HZ / IT_TIM_REFRESH_HZ ) - 1u )

/** Maximal count of wait loop iterations (timeout of waiting for hardware event, ~0.8 s at 216 MHz) */
#define IT_TIM_WAIT_LOOPS                   ( 4000000u )

/** Count of wait loop iterations until pin level is settled */
#define IT_TIM_SETTLE_LOOPS                 ( 2000u )

/** Count of samples of PWM pin level */
#define IT_TIM_PWM_SAMPLES                  ( 20000u )

/** Counter margin around PWM edges where pin level is not evaluated [counts] */
#define IT_TIM_PWM_EDGE_MARGIN              ( 20u )

/** Minimal count of evaluated samples in each PWM level */
#define IT_TIM_PWM_MIN_SAMPLES              ( 100u )

/** Duty cycle 100 % [c%] */
#define IT_TIM_DUTY_100                     ( 10000u )

/* ============================== MACROS ==================================== */

/* ========================== LOCAL VARIABLES =============================== */

/** Count of update callback calls */
static volatile uint32_t                itTim_UpdateCallbackCnt     = 0u;

/** Count of capture / compare callback calls */
static volatile uint32_t                itTim_CcCallbackCnt         = 0u;

/** Over-capture flag of the last capture / compare callback call */
static volatile tim_OvercaptureFlag_t   itTim_CcOvercapture         = TIM_OVERCAPTURE_ACTIVE;

/* ============================= TEST SETUP ================================= */

void setUp( void )
{
    itTim_UpdateCallbackCnt = 0u;
    itTim_CcCallbackCnt     = 0u;
    itTim_CcOvercapture     = TIM_OVERCAPTURE_ACTIVE;
}


void tearDown( void )
{
    /* Every test case runs after system reset, timers are in reset state */
}

/* =============================== TESTS ==================================== */

/*----------------------------- Initialization -------------------------------*/

/**
 * \brief   Tim_Init() on target configures time base.
 *
 * \details Initializes TIM2 with counter frequency 1 MHz and refresh frequency 1 kHz,
 *          reads the configuration back.
 *
 * \par Expected results
 * - Counter frequency 1 MHz, period 999, refresh frequency 1 kHz.
 * - Counter is not started by initialization (state inactive).
 */
void It_Tim_Init_TimeBase_FrequencyAndPeriodReadBack( void )
{
    tim_FreqHz_t        timerFreq   = 0u;
    tim_FreqHz_t        refreshFreq = 0u;
    tim_Counter_t       period      = 0u;
    tim_FunctionState_t periphState = TIM_FUNCTION_ACTIVE;

    It_Tim_Init_TimeBase( IT_TIM_REF, IT_TIM_FREQ_HZ, IT_TIM_REFRESH_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_ClkInternal( IT_TIM_REF, &timerFreq ) );
    TEST_ASSERT_EQUAL_UINT32( IT_TIM_FREQ_HZ, timerFreq );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Period( IT_TIM_REF, &period ) );
    TEST_ASSERT_EQUAL_UINT32( IT_TIM_PERIOD, period );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_RefreshFreq( IT_TIM_REF, &refreshFreq ) );
    TEST_ASSERT_EQUAL_UINT32( IT_TIM_REFRESH_HZ, refreshFreq );

    /* Counter is not started by initialization */
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_PeriphState( IT_TIM_REF, &periphState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_INACTIVE, periphState );
}


/**
 * \brief   Tim_Init() rejects running timer.
 *
 * \details Initializes and starts TIM2, then initializes it again.
 *
 * \par Expected results
 * - Second Tim_Init(): TIM_REQUEST_ERROR.
 */
void It_Tim_Init_RunningTimer_ReturnsError( void )
{
    tim_PeriphConfig_t timConfig;

    It_Tim_Get_TimeBaseConfig( &timConfig, IT_TIM_REF, IT_TIM_FREQ_HZ, IT_TIM_REFRESH_HZ );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Init( &timConfig ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_REF ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_ERROR, Tim_Init( &timConfig ) );
}


/**
 * \brief   Prescaler is effective from the first counter period.
 *
 * \details TIM3 initialized with 10 kHz counter and 1 s period. TIM3 and reference
 *          TIM2 (1 MHz) are started, TIM3 is paused after 10 ms.
 *
 * \par Expected results
 * - No update flag of TIM3 (prescaler loaded by Tim_Init, counter did not run at
 *   kernel clock).
 * - TIM3 counter = 100 +- 2.
 */
void It_Tim_Init_FirstPeriod_RunsAtTimerFrequency( void )
{
    tim_PeriphConfig_t timConfig;
    tim_FlagState_t    updateFlag = TIM_FLAG_ACTIVE;
    tim_Counter_t      counter    = 0u;

    It_Tim_Init_TimeBase( IT_TIM_REF, IT_TIM_REF_FREQ_HZ, IT_TIM_REF_REFRESH_HZ );

    /* Timer 10 kHz, period 1 s - Tim_Init loads prescaler and clears update flag */
    It_Tim_Get_TimeBaseConfig( &timConfig, IT_TIM_SLAVE, 10000u, 1u );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Init( &timConfig ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_SLAVE ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_REF   ) );
    It_Tim_Wait_RefTime( 10000u );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Pause( IT_TIM_SLAVE ) );

    /* Prescaler must be effective from the start - no overflow within 10 ms */
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Flag( IT_TIM_SLAVE, TIM_IRQ_UPDATE, &updateFlag ) );
    TEST_ASSERT_EQUAL_MESSAGE( TIM_FLAG_INACTIVE, updateFlag, "Prescaler not loaded by Tim_Init (counter ran at kernel clock)" );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Counter( IT_TIM_SLAVE, &counter ) );
    TEST_ASSERT_UINT32_WITHIN( 2u, 100u, counter );
}


/**
 * \brief   Tim_Deinit() on target resets the timer.
 *
 * \details Initializes TIM2, enables update interrupt, starts the timer, deinitializes
 *          it and initializes it again with refresh frequency 100 Hz.
 *
 * \par Expected results
 * - Update interrupt is disabled (peripheral reset state).
 * - Period = 9999 (new configuration applied).
 */
void It_Tim_Deinit_InitializedTimer_PeripheralReset( void )
{
    tim_PeriphConfig_t  timConfig;
    tim_FunctionState_t irqState = TIM_FUNCTION_ACTIVE;
    tim_Counter_t       period   = 0u;

    It_Tim_Get_TimeBaseConfig( &timConfig, IT_TIM_REF, IT_TIM_FREQ_HZ, IT_TIM_REFRESH_HZ );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Init( &timConfig ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_IrqActive( IT_TIM_REF, TIM_IRQ_UPDATE ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_REF ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Deinit( &timConfig ) );

    /* Re-initialization with other period - interrupt enable is in reset state */
    It_Tim_Get_TimeBaseConfig( &timConfig, IT_TIM_REF, IT_TIM_FREQ_HZ, 100u );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Init( &timConfig ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_IrqState( IT_TIM_REF, TIM_IRQ_UPDATE, &irqState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_INACTIVE, irqState );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Period( IT_TIM_REF, &period ) );
    TEST_ASSERT_EQUAL_UINT32( ( IT_TIM_FREQ_HZ / 100u ) - 1u, period );
}

/*----------------------------- Counter control ------------------------------*/

/**
 * \brief   Started timer counts from internal clock.
 *
 * \details Initializes TIM2 (1 MHz), starts it and waits 1 ms.
 *
 * \par Expected results
 * - State active, counter >= 1000.
 */
void It_Tim_Start_InternalClock_CounterIncrements( void )
{
    tim_Counter_t       counter     = 0u;
    tim_FunctionState_t periphState = TIM_FUNCTION_INACTIVE;

    It_Tim_Init_TimeBase( IT_TIM_REF, IT_TIM_REF_FREQ_HZ, IT_TIM_REF_REFRESH_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_REF ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_PeriphState( IT_TIM_REF, &periphState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_ACTIVE, periphState );

    It_Tim_Wait_RefTime( 1000u );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Counter( IT_TIM_REF, &counter ) );
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32( 1000u, counter );
}


/**
 * \brief   Tim_Pause() stops counting and keeps the counter value.
 *
 * \details
 * 1. Starts TIM2 (1 MHz), waits 1 ms, pauses it, reads the counter twice with delay.
 * 2. Starts TIM2 again and waits next 1 ms.
 *
 * \par Expected results
 * 1. Counter >= 1000 and does not change while paused.
 * 2. Counting resumes from the paused value (counter >= paused value + 1000).
 */
void It_Tim_Pause_RunningCounter_CounterValueKept( void )
{
    tim_Counter_t counterPaused = 0u;
    tim_Counter_t counterLater  = 0u;

    It_Tim_Init_TimeBase( IT_TIM_REF, IT_TIM_REF_FREQ_HZ, IT_TIM_REF_REFRESH_HZ );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_REF ) );
    It_Tim_Wait_RefTime( 1000u );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Pause( IT_TIM_REF ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Counter( IT_TIM_REF, &counterPaused ) );
    It_Tim_WaitPinSettled();
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Counter( IT_TIM_REF, &counterLater ) );

    TEST_ASSERT_GREATER_OR_EQUAL_UINT32( 1000u, counterPaused );
    TEST_ASSERT_EQUAL_UINT32( counterPaused, counterLater );

    /* Start resumes counting from the paused value */
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_REF ) );
    It_Tim_Wait_RefTime( counterPaused + 1000u );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Counter( IT_TIM_REF, &counterLater ) );
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32( counterPaused + 1000u, counterLater );
}


/**
 * \brief   Tim_Stop() stops counting and resets the counter.
 *
 * \details Starts TIM2 (1 MHz), waits 1 ms and stops it.
 *
 * \par Expected results
 * - State inactive, counter 0 (also after delay).
 */
void It_Tim_Stop_RunningCounter_CounterResetAndStopped( void )
{
    tim_Counter_t       counter     = 1u;
    tim_FunctionState_t periphState = TIM_FUNCTION_ACTIVE;

    It_Tim_Init_TimeBase( IT_TIM_REF, IT_TIM_REF_FREQ_HZ, IT_TIM_REF_REFRESH_HZ );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_REF ) );
    It_Tim_Wait_RefTime( 1000u );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Stop( IT_TIM_REF ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_PeriphState( IT_TIM_REF, &periphState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_INACTIVE, periphState );

    It_Tim_WaitPinSettled();
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Counter( IT_TIM_REF, &counter ) );
    TEST_ASSERT_EQUAL_UINT32( 0u, counter );
}


/**
 * \brief   Down-counting timer decrements the counter.
 *
 * \details TIM3 (1 MHz, period 50000) set to down-counting, counter preset to 40000.
 *          TIM3 and reference TIM2 are started, TIM3 is paused after 10 ms.
 *
 * \par Expected results
 * - Direction reads back down.
 * - TIM3 counter = 30000 +- 200.
 */
void It_Tim_Set_CounterDirection_Down_CounterDecrements( void )
{
    tim_Counter_t    counter    = 0u;
    tim_CounterDir_t counterDir = TIM_COUNTER_DIR_UP;

    It_Tim_Init_TimeBase( IT_TIM_REF, IT_TIM_REF_FREQ_HZ, IT_TIM_REF_REFRESH_HZ );
    It_Tim_Init_TimeBase( IT_TIM_SLAVE, IT_TIM_FREQ_HZ, 20u );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CounterDirection( IT_TIM_SLAVE, TIM_COUNTER_DIR_DOWN ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_CounterDirection( IT_TIM_SLAVE, &counterDir ) );
    TEST_ASSERT_EQUAL( TIM_COUNTER_DIR_DOWN, counterDir );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Counter( IT_TIM_SLAVE, 40000u ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_SLAVE ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_REF   ) );
    It_Tim_Wait_RefTime( 10000u );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Pause( IT_TIM_SLAVE ) );

    /* 10 ms at 1 MHz: 40000 -> ~30000 */
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Counter( IT_TIM_SLAVE, &counter ) );
    TEST_ASSERT_UINT32_WITHIN( 200u, 30000u, counter );
}


/**
 * \brief   Two timers count with ratio of their counter frequencies.
 *
 * \details TIM2 (1 MHz) and basic timer (10 kHz) are started together and paused after
 *          500 ms.
 *
 * \par Expected results
 * - Basic timer counter = TIM2 counter / 100 +- 2.
 */
void It_Tim_Set_ClkInternal_TwoTimers_CountRatioMatchesFrequencies( void )
{
    tim_Counter_t refCounter   = 0u;
    tim_Counter_t basicCounter = 0u;

    /* 1 MHz reference and 10 kHz basic timer, both without overflow for 1 s */
    It_Tim_Init_TimeBase( IT_TIM_REF,   IT_TIM_REF_FREQ_HZ, IT_TIM_REF_REFRESH_HZ );
    It_Tim_Init_TimeBase( IT_TIM_BASIC, 10000u,             1u                    );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_BASIC ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_REF   ) );
    It_Tim_Wait_RefTime( 500000u );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Pause( IT_TIM_REF   ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Pause( IT_TIM_BASIC ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Counter( IT_TIM_REF,   &refCounter   ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Counter( IT_TIM_BASIC, &basicCounter ) );

    TEST_ASSERT_UINT32_WITHIN( 2u, refCounter / 100u, basicCounter );
}


/**
 * \brief   One pulse mode stops the counter at update event.
 *
 * \details TIM3 (1 MHz, period 1 ms) in one pulse mode is started, the test waits
 *          until it stops.
 *
 * \par Expected results
 * - State inactive before timeout, update flag active, counter 0.
 */
void It_Tim_Set_OnePulseModeActive_CounterStopsAfterUpdate( void )
{
    tim_FunctionState_t periphState = TIM_FUNCTION_ACTIVE;
    tim_FlagState_t     updateFlag  = TIM_FLAG_INACTIVE;
    tim_Counter_t       counter     = 1u;

    It_Tim_Init_TimeBase( IT_TIM_SLAVE, IT_TIM_FREQ_HZ, IT_TIM_REFRESH_HZ );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_OnePulseModeActive( IT_TIM_SLAVE ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_SLAVE ) );

    for( uint32_t loopIdx = 0u;
         ( IT_TIM_WAIT_LOOPS > loopIdx ) &&
         ( TIM_FUNCTION_INACTIVE != periphState );
         loopIdx++ )
    {
        TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_PeriphState( IT_TIM_SLAVE, &periphState ) );
    }

    TEST_ASSERT_EQUAL( TIM_FUNCTION_INACTIVE, periphState );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Flag( IT_TIM_SLAVE, TIM_IRQ_UPDATE, &updateFlag ) );
    TEST_ASSERT_EQUAL( TIM_FLAG_ACTIVE, updateFlag );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Counter( IT_TIM_SLAVE, &counter ) );
    TEST_ASSERT_EQUAL_UINT32( 0u, counter );
}

/*--------------------------- Update event and flags -------------------------*/

/**
 * \brief   Software update event resets the counter and sets update flag.
 *
 * \details TIM2 counter preset to 500, update event generated, update flag cleared.
 *
 * \par Expected results
 * - Counter 0, update flag active after the event.
 * - Update flag inactive after clearing.
 */
void It_Tim_Generate_Event_Update_CounterResetAndFlagSet( void )
{
    tim_FlagState_t updateFlag = TIM_FLAG_INACTIVE;
    tim_Counter_t   counter    = 1u;

    It_Tim_Init_TimeBase( IT_TIM_REF, IT_TIM_FREQ_HZ, IT_TIM_REFRESH_HZ );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Counter( IT_TIM_REF, 500u ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Generate_Event( IT_TIM_REF, TIM_EVENT_UPDATE ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Counter( IT_TIM_REF, &counter ) );
    TEST_ASSERT_EQUAL_UINT32( 0u, counter );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Flag( IT_TIM_REF, TIM_IRQ_UPDATE, &updateFlag ) );
    TEST_ASSERT_EQUAL( TIM_FLAG_ACTIVE, updateFlag );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Clear_Flag( IT_TIM_REF, TIM_IRQ_UPDATE ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Flag( IT_TIM_REF, TIM_IRQ_UPDATE, &updateFlag ) );
    TEST_ASSERT_EQUAL( TIM_FLAG_INACTIVE, updateFlag );
}


/**
 * \brief   Counter overflow sets update flag.
 *
 * \details TIM6 (1 MHz, period 1 ms) is started, the test waits for update flag.
 *
 * \par Expected results
 * - Update flag inactive before start, active after overflow (before timeout).
 */
void It_Tim_Get_Flag_CounterOverflow_UpdateFlagActive( void )
{
    tim_FlagState_t updateFlag = TIM_FLAG_INACTIVE;

    It_Tim_Init_TimeBase( IT_TIM_BASIC, IT_TIM_FREQ_HZ, IT_TIM_REFRESH_HZ );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Flag( IT_TIM_BASIC, TIM_IRQ_UPDATE, &updateFlag ) );
    TEST_ASSERT_EQUAL( TIM_FLAG_INACTIVE, updateFlag );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_BASIC ) );

    for( uint32_t loopIdx = 0u;
         ( IT_TIM_WAIT_LOOPS > loopIdx ) &&
         ( TIM_FLAG_ACTIVE != updateFlag );
         loopIdx++ )
    {
        TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Flag( IT_TIM_BASIC, TIM_IRQ_UPDATE, &updateFlag ) );
    }

    TEST_ASSERT_EQUAL( TIM_FLAG_ACTIVE, updateFlag );
}


/**
 * \brief   Interrupt on NVIC line shared by two timers calls the callback.
 *
 * \details TIM9 (1 MHz, period 1 ms, interrupt on line TIM1_BRK_TIM9 shared with TIM1
 *          break) with update callback and update interrupt enabled is started, the
 *          test waits for 3 callbacks.
 *
 * \par Expected results
 * - Update callback called at least 3x, update flag cleared by interrupt handler.
 */
void It_Tim_Set_IrqActive_SharedNvicLine_CallbackCalled( void )
{
    tim_FlagState_t updateFlag = TIM_FLAG_ACTIVE;

    It_Tim_Init_TimeBase( IT_TIM_SHARED_IRQ, IT_TIM_FREQ_HZ, IT_TIM_REFRESH_HZ );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_UpdateCallback( IT_TIM_SHARED_IRQ, It_Tim_UpdateCallback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_IrqActive( IT_TIM_SHARED_IRQ, TIM_IRQ_UPDATE ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_SHARED_IRQ ) );
    It_Tim_Wait_CallbackCount( &itTim_UpdateCallbackCnt, 3u );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Pause( IT_TIM_SHARED_IRQ ) );

    TEST_ASSERT_GREATER_OR_EQUAL_UINT32( 3u, itTim_UpdateCallbackCnt );

    /* Flag is cleared by the module interrupt handler */
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Flag( IT_TIM_SHARED_IRQ, TIM_IRQ_UPDATE, &updateFlag ) );
    TEST_ASSERT_EQUAL( TIM_FLAG_INACTIVE, updateFlag );
}

/*---------------------------- Update interrupt ------------------------------*/

/**
 * \brief   Update interrupt calls the update callback.
 *
 * \details TIM6 (1 MHz, period 1 ms) with update callback and update interrupt
 *          enabled is started, the test waits for 3 callbacks.
 *
 * \par Expected results
 * - Update interrupt state active.
 * - Update callback called at least 3x.
 */
void It_Tim_Set_IrqActive_UpdateInterrupt_CallbackCalled( void )
{
    tim_FunctionState_t irqState = TIM_FUNCTION_INACTIVE;

    It_Tim_Init_TimeBase( IT_TIM_BASIC, IT_TIM_FREQ_HZ, IT_TIM_REFRESH_HZ );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_UpdateCallback( IT_TIM_BASIC, It_Tim_UpdateCallback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_IrqActive( IT_TIM_BASIC, TIM_IRQ_UPDATE ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_IrqState( IT_TIM_BASIC, TIM_IRQ_UPDATE, &irqState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_ACTIVE, irqState );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_BASIC ) );
    It_Tim_Wait_CallbackCount( &itTim_UpdateCallbackCnt, 3u );

    TEST_ASSERT_GREATER_OR_EQUAL_UINT32( 3u, itTim_UpdateCallbackCnt );
}


/**
 * \brief   Disabled update interrupt does not call the callback.
 *
 * \details TIM6 with update callback, update interrupt enabled and disabled. The
 *          timer is started, the test waits for update flag.
 *
 * \par Expected results
 * - Update flag active (overflow occurred, flag not cleared by interrupt handler).
 * - Update callback is not called.
 */
void It_Tim_Set_IrqInactive_UpdateInterrupt_CallbackNotCalled( void )
{
    tim_FlagState_t updateFlag = TIM_FLAG_INACTIVE;

    It_Tim_Init_TimeBase( IT_TIM_BASIC, IT_TIM_FREQ_HZ, IT_TIM_REFRESH_HZ );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_UpdateCallback( IT_TIM_BASIC, It_Tim_UpdateCallback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_IrqActive  ( IT_TIM_BASIC, TIM_IRQ_UPDATE ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_IrqInactive( IT_TIM_BASIC, TIM_IRQ_UPDATE ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_BASIC ) );

    /* Overflow occurred (flag stays active - not cleared by interrupt handler) */
    for( uint32_t loopIdx = 0u;
         ( IT_TIM_WAIT_LOOPS > loopIdx ) &&
         ( TIM_FLAG_ACTIVE != updateFlag );
         loopIdx++ )
    {
        TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Flag( IT_TIM_BASIC, TIM_IRQ_UPDATE, &updateFlag ) );
    }

    TEST_ASSERT_EQUAL( TIM_FLAG_ACTIVE, updateFlag );
    TEST_ASSERT_EQUAL_UINT32( 0u, itTim_UpdateCallbackCnt );
}


/**
 * \brief   Update callback rate matches refresh frequency.
 *
 * \details TIM6 (refresh 1 kHz) with update interrupt and reference TIM2 are started,
 *          TIM6 is paused after 100 ms.
 *
 * \par Expected results
 * - Update callback called 100x +- 2.
 */
void It_Tim_Set_UpdateCallback_UpdateRateMatchesRefreshFreq( void )
{
    It_Tim_Init_TimeBase( IT_TIM_REF,   IT_TIM_REF_FREQ_HZ, IT_TIM_REF_REFRESH_HZ );
    It_Tim_Init_TimeBase( IT_TIM_BASIC, IT_TIM_FREQ_HZ,     IT_TIM_REFRESH_HZ     );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_UpdateCallback( IT_TIM_BASIC, It_Tim_UpdateCallback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_IrqActive( IT_TIM_BASIC, TIM_IRQ_UPDATE ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_BASIC ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_REF   ) );
    It_Tim_Wait_RefTime( 100000u );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Pause( IT_TIM_BASIC ) );

    /* 100 ms at 1 kHz refresh frequency */
    TEST_ASSERT_UINT32_WITHIN( 2u, 100u, itTim_UpdateCallbackCnt );
}


/**
 * \brief   Repetition counter divides update event rate.
 *
 * \details TIM1 (1 MHz, period 100 us) with repetition counter 9 (update event every
 *          10th overflow = 1 ms) and update interrupt. TIM1 and reference TIM2 are
 *          started, TIM1 is paused after 100 ms.
 *
 * \par Expected results
 * - Repetition counter reads back 9.
 * - Update callback called 100x +- 2.
 */
void It_Tim_Set_RepetitionCounter_Rep9_UpdateRateDividedBy10( void )
{
    tim_RepCnt_t repetitionCnt = 0u;

    It_Tim_Init_TimeBase( IT_TIM_REF, IT_TIM_REF_FREQ_HZ, IT_TIM_REF_REFRESH_HZ );

    /* Counter period 100 us, update event every 10th overflow (1 ms) */
    It_Tim_Init_TimeBase( IT_TIM_ADV, IT_TIM_FREQ_HZ, 10000u );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_RepetitionCounter( IT_TIM_ADV, 9u ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_RepetitionCounter( IT_TIM_ADV, &repetitionCnt ) );
    TEST_ASSERT_EQUAL_UINT16( 9u, repetitionCnt );
    It_Tim_Apply_UpdateEvent( IT_TIM_ADV );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_UpdateCallback( IT_TIM_ADV, It_Tim_UpdateCallback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_IrqActive( IT_TIM_ADV, TIM_IRQ_UPDATE ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_ADV ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_REF ) );
    It_Tim_Wait_RefTime( 100000u );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Pause( IT_TIM_ADV ) );

    TEST_ASSERT_UINT32_WITHIN( 2u, 100u, itTim_UpdateCallbackCnt );
}

/*------------------------ Output compare and capture ------------------------*/

/**
 * \brief   Compare match calls capture / compare callback.
 *
 * \details TIM2 (1 MHz, period 1 ms) channel 1 in output frozen mode, compare value
 *          500, CC1 interrupt with callback enabled. The timer is started, the test
 *          waits for 2 callbacks.
 *
 * \par Expected results
 * - Compare value reads back 500.
 * - Callback called at least 2x, over-capture inactive.
 */
void It_Tim_Set_CompareValue_CompareMatch_CaptureCompareCallbackCalled( void )
{
    tim_PeriphConfig_t timConfig;
    tim_Counter_t      compareValue = 0u;

    It_Tim_Get_TimeBaseConfig( &timConfig, IT_TIM_REF, IT_TIM_FREQ_HZ, IT_TIM_REFRESH_HZ );
    timConfig.ChannelConfig[ TIM_CHANNEL_1 ].ChannelState = TIM_FUNCTION_ACTIVE;
    timConfig.ChannelConfig[ TIM_CHANNEL_1 ].ChannelMode  = TIM_CHANNEL_MODE_OUTPUT_FROZEN;
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Init( &timConfig ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CompareValue( IT_TIM_REF, TIM_CHANNEL_1, 500u ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_CompareValue( IT_TIM_REF, TIM_CHANNEL_1, &compareValue ) );
    TEST_ASSERT_EQUAL_UINT32( 500u, compareValue );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CaptureCompareCallback( IT_TIM_REF, TIM_CHANNEL_1, It_Tim_CaptureCompareCallback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_IrqActive( IT_TIM_REF, TIM_IRQ_CAPTURE_COMPARE_CH1 ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_REF ) );
    It_Tim_Wait_CallbackCount( &itTim_CcCallbackCnt, 2u );

    TEST_ASSERT_GREATER_OR_EQUAL_UINT32( 2u, itTim_CcCallbackCnt );
    TEST_ASSERT_EQUAL( TIM_OVERCAPTURE_INACTIVE, itTim_CcOvercapture );
}


/**
 * \brief   Software capture event captures the counter.
 *
 * \details TIM2 channel 1 in input capture mode, counter preset to 432, CC1 flag
 *          cleared, CC1 event generated by software. Captured value is read.
 *
 * \par Expected results
 * - CC1 flag active after the event, captured value 432.
 * - CC1 flag cleared by reading of the captured value.
 */
void It_Tim_Get_CaptureValue_SoftwareCaptureEvent_CounterCaptured( void )
{
    tim_PeriphConfig_t timConfig;
    tim_Counter_t      captureValue = 0u;
    tim_FlagState_t    captureFlag  = TIM_FLAG_INACTIVE;

    It_Tim_Get_TimeBaseConfig( &timConfig, IT_TIM_REF, IT_TIM_FREQ_HZ, IT_TIM_REFRESH_HZ );
    timConfig.ChannelConfig[ TIM_CHANNEL_1 ].ChannelState = TIM_FUNCTION_ACTIVE;
    timConfig.ChannelConfig[ TIM_CHANNEL_1 ].ChannelMode  = TIM_CHANNEL_MODE_INPUT_CAPTURE;
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Init( &timConfig ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_Counter( IT_TIM_REF, 432u ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Clear_Flag( IT_TIM_REF, TIM_IRQ_CAPTURE_COMPARE_CH1 ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Generate_Event( IT_TIM_REF, TIM_EVENT_CC1 ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Flag( IT_TIM_REF, TIM_IRQ_CAPTURE_COMPARE_CH1, &captureFlag ) );
    TEST_ASSERT_EQUAL( TIM_FLAG_ACTIVE, captureFlag );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_CaptureValue( IT_TIM_REF, TIM_CHANNEL_1, &captureValue ) );
    TEST_ASSERT_EQUAL_UINT32( 432u, captureValue );

    /* Reading of the captured value clears capture flag (input capture mode) */
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Flag( IT_TIM_REF, TIM_IRQ_CAPTURE_COMPARE_CH1, &captureFlag ) );
    TEST_ASSERT_EQUAL( TIM_FLAG_INACTIVE, captureFlag );
}


/**
 * \brief   Capture interrupt calls capture / compare callback.
 *
 * \details TIM2 channel 1 in input capture mode, CC1 interrupt with callback enabled,
 *          CC1 event generated by software.
 *
 * \par Expected results
 * - Callback called 1x, over-capture inactive (flag cleared by interrupt handler).
 */
void It_Tim_Set_CaptureCompareCallback_SoftwareCapture_CallbackCalled( void )
{
    tim_PeriphConfig_t timConfig;

    It_Tim_Get_TimeBaseConfig( &timConfig, IT_TIM_REF, IT_TIM_FREQ_HZ, IT_TIM_REFRESH_HZ );
    timConfig.ChannelConfig[ TIM_CHANNEL_1 ].ChannelState = TIM_FUNCTION_ACTIVE;
    timConfig.ChannelConfig[ TIM_CHANNEL_1 ].ChannelMode  = TIM_CHANNEL_MODE_INPUT_CAPTURE;
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Init( &timConfig ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_CaptureCompareCallback( IT_TIM_REF, TIM_CHANNEL_1, It_Tim_CaptureCompareCallback ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_IrqActive( IT_TIM_REF, TIM_IRQ_CAPTURE_COMPARE_CH1 ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Generate_Event( IT_TIM_REF, TIM_EVENT_CC1 ) );
    It_Tim_Wait_CallbackCount( &itTim_CcCallbackCnt, 1u );

    /* Flag is cleared by the interrupt handler, no over-capture */
    TEST_ASSERT_EQUAL_UINT32( 1u, itTim_CcCallbackCnt );
    TEST_ASSERT_EQUAL( TIM_OVERCAPTURE_INACTIVE, itTim_CcOvercapture );
}

/*----------------------- Master / slave synchronization ---------------------*/

/**
 * \brief   Slave timer clocked by master trigger output.
 *
 * \details Master TIM2 with trigger output on UG bit, slave TIM3 clocked by ITR1
 *          (TIM2 TRGO, external clock mode 1) is started. 5 update events of TIM2
 *          are generated by software.
 *
 * \par Expected results
 * - TIM3 counter 5.
 */
void It_Tim_Set_ClockSource_ItrFromMasterUpdate_SlaveCountsMasterEvents( void )
{
    tim_PeriphConfig_t timConfig;
    tim_Counter_t      counter = 0u;

    /* Master: UG bit is used as trigger output */
    It_Tim_Get_TimeBaseConfig( &timConfig, IT_TIM_REF, IT_TIM_FREQ_HZ, IT_TIM_REFRESH_HZ );
    timConfig.MasterTrigger = TIM_MASTER_TRIGGER_RESET;
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Init( &timConfig ) );

    /* Slave: clocked by master trigger output (external clock mode 1) */
    It_Tim_Get_TimeBaseConfig( &timConfig, IT_TIM_SLAVE, IT_TIM_FREQ_HZ, IT_TIM_REFRESH_HZ );
    timConfig.ClockSource    = TIM_CLOCKSOURCE_EXTERNAL_CH_IN;
    timConfig.ExtClockSource = IT_TIM_SLAVE_ITR;
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Init( &timConfig ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_SLAVE ) );

    for( uint32_t eventIdx = 0u; 5u > eventIdx; eventIdx++ )
    {
        TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Generate_Event( IT_TIM_REF, TIM_EVENT_UPDATE ) );
        It_Tim_WaitPinSettled();
    }

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Counter( IT_TIM_SLAVE, &counter ) );
    TEST_ASSERT_EQUAL_UINT32( 5u, counter );
}


/**
 * \brief   Slave timer in trigger mode starts with master.
 *
 * \details Master TIM2 (1 MHz) with trigger output on counter enable, slave TIM3
 *          (1 MHz) in trigger mode with input ITR1. Reads slave configuration and
 *          state, starts TIM2 and waits 1 ms.
 *
 * \par Expected results
 * - Slave mode reads back trigger mode with ITR1, TIM3 inactive before TIM2 start.
 * - TIM3 active after TIM2 start, TIM3 counter = 1000 +- 50.
 */
void It_Tim_Set_SlaveMode_TriggerFromMasterEnable_SlaveStartsWithMaster( void )
{
    tim_PeriphConfig_t  timConfig;
    tim_FunctionState_t periphState = TIM_FUNCTION_ACTIVE;
    tim_SlaveMode_t     slaveMode   = TIM_SLAVE_MODE_DISABLE;
    tim_TriggerInput_t  triggerIn   = TIM_TRIGGER_INPUT_UNUSED;
    tim_Counter_t       counter     = 0u;

    /* Master: counter enable is used as trigger output */
    It_Tim_Get_TimeBaseConfig( &timConfig, IT_TIM_REF, IT_TIM_REF_FREQ_HZ, IT_TIM_REF_REFRESH_HZ );
    timConfig.MasterTrigger = TIM_MASTER_TRIGGER_ENABLE;
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Init( &timConfig ) );

    /* Slave: counter started by rising edge of trigger input */
    It_Tim_Get_TimeBaseConfig( &timConfig, IT_TIM_SLAVE, IT_TIM_FREQ_HZ, 20u );
    timConfig.SlaveMode         = TIM_SLAVE_MODE_TRIGGER;
    timConfig.SlaveTriggerInput = IT_TIM_SLAVE_ITR;
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Init( &timConfig ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_SlaveMode( IT_TIM_SLAVE, &slaveMode, &triggerIn ) );
    TEST_ASSERT_EQUAL( TIM_SLAVE_MODE_TRIGGER, slaveMode );
    TEST_ASSERT_EQUAL( IT_TIM_SLAVE_ITR, triggerIn );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_PeriphState( IT_TIM_SLAVE, &periphState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_INACTIVE, periphState );

    /* Start of master starts the slave */
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_REF ) );
    It_Tim_Wait_RefTime( 1000u );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_PeriphState( IT_TIM_SLAVE, &periphState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_ACTIVE, periphState );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Counter( IT_TIM_SLAVE, &counter ) );
    TEST_ASSERT_UINT32_WITHIN( 50u, 1000u, counter );
}

/*------------------------------ Output pin ----------------------------------*/

/**
 * \brief   Forced active output drives the pin high.
 *
 * \details TIM1 channel 1 in forced active mode, polarity high.
 *
 * \par Expected results
 * - Pin reads high.
 */
void It_Tim_Set_Mode_ForcedOutput_ForcedActive_PinReadsHigh( void )
{
    It_Tim_Init_PinChannel( TIM_CHANNEL_MODE_OUTPUT_FORCED_ACTIVE, TIM_POLARITY_HIGH, TIM_POLARITY_LOW );

    It_Tim_Check_PinLevel( GPIO_PIN_LEVEL_HIGH );
}


/**
 * \brief   Forced inactive output drives the pin low.
 *
 * \details TIM1 channel 1 in forced inactive mode, polarity high.
 *
 * \par Expected results
 * - Pin reads low.
 */
void It_Tim_Set_Mode_ForcedOutput_ForcedInactive_PinReadsLow( void )
{
    It_Tim_Init_PinChannel( TIM_CHANNEL_MODE_OUTPUT_FORCED_INACTIVE, TIM_POLARITY_HIGH, TIM_POLARITY_LOW );

    It_Tim_Check_PinLevel( GPIO_PIN_LEVEL_LOW );
}


/**
 * \brief   Output polarity low inverts the pin level.
 *
 * \details TIM1 channel 1 in forced active mode with polarity high, then polarity is
 *          changed to low.
 *
 * \par Expected results
 * - Polarity high: pin reads high.
 * - Polarity reads back low, pin reads low.
 */
void It_Tim_Set_OutputPolarity_LowForcedActive_PinReadsLow( void )
{
    tim_Polarity_t outputPolarity = TIM_POLARITY_HIGH;

    It_Tim_Init_PinChannel( TIM_CHANNEL_MODE_OUTPUT_FORCED_ACTIVE, TIM_POLARITY_HIGH, TIM_POLARITY_LOW );
    It_Tim_Check_PinLevel( GPIO_PIN_LEVEL_HIGH );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_OutputPolarity( IT_TIM_ADV, TIM_OUTPUT_1, TIM_POLARITY_LOW ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_OutputPolarity( IT_TIM_ADV, TIM_OUTPUT_1, &outputPolarity ) );
    TEST_ASSERT_EQUAL( TIM_POLARITY_LOW, outputPolarity );

    It_Tim_Check_PinLevel( GPIO_PIN_LEVEL_LOW );
}


/**
 * \brief   Disabled main output drives idle level of the pin.
 *
 * \details TIM1 channel 1 in forced inactive mode, idle state high. Off-state in idle
 *          mode is enabled, main output is disabled and enabled again.
 *
 * \par Expected results
 * - Main output enabled: pin reads low.
 * - Main output disabled: state inactive, pin reads high (idle level).
 * - Main output enabled again: pin reads low.
 */
void It_Tim_Set_MainOutputInactive_OffStateIdleHigh_PinReadsIdleLevel( void )
{
    tim_FunctionState_t outputState = TIM_FUNCTION_ACTIVE;

    It_Tim_Init_PinChannel( TIM_CHANNEL_MODE_OUTPUT_FORCED_INACTIVE, TIM_POLARITY_HIGH, TIM_POLARITY_HIGH );
    It_Tim_Check_PinLevel( GPIO_PIN_LEVEL_LOW );

    /* Off-state in idle mode: output drives idle level when main output is disabled */
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_OffStateConfig( IT_TIM_ADV, TIM_FUNCTION_ACTIVE, TIM_FUNCTION_INACTIVE ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_MainOutputInactive( IT_TIM_ADV ) );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_MainOutputState( IT_TIM_ADV, &outputState ) );
    TEST_ASSERT_EQUAL( TIM_FUNCTION_INACTIVE, outputState );
    It_Tim_Check_PinLevel( GPIO_PIN_LEVEL_HIGH );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_MainOutputActive( IT_TIM_ADV ) );
    It_Tim_Check_PinLevel( GPIO_PIN_LEVEL_LOW );
}


/**
 * \brief   PWM with 0 % and 100 % duty cycle drives constant pin level.
 *
 * \details TIM1 channel 1 in PWM mode is started (initial duty cycle 0 %), then duty
 *          cycle 100 % is set and applied by update event. Pin level is sampled 1000x
 *          in both cases.
 *
 * \par Expected results
 * - 0 %: pin always low.
 * - 100 %: pin always high.
 */
void It_Tim_Set_PwmMode_DutyCycle_0And100Percent_PinConstantLevel( void )
{
    It_Tim_Init_PinChannel( TIM_CHANNEL_MODE_OUTPUT_PWM, TIM_POLARITY_HIGH, TIM_POLARITY_LOW );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_ADV ) );

    /* PWM starts with 0 % duty cycle */
    for( uint32_t sampleIdx = 0u; 1000u > sampleIdx; sampleIdx++ )
    {
        It_Tim_Check_PinLevel( GPIO_PIN_LEVEL_LOW );
    }

    /* 100 % - compare value above period, applied at next update event */
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_PwmMode_DutyCycle( IT_TIM_ADV, TIM_CHANNEL_1, IT_TIM_DUTY_100 ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Generate_Event( IT_TIM_ADV, TIM_EVENT_UPDATE ) );

    for( uint32_t sampleIdx = 0u; 1000u > sampleIdx; sampleIdx++ )
    {
        It_Tim_Check_PinLevel( GPIO_PIN_LEVEL_HIGH );
    }
}


/**
 * \brief   PWM with 25 % duty cycle drives the pin according to the counter.
 *
 * \details TIM1 channel 1 in PWM mode 1 (1 MHz, period 1 ms), duty cycle 25 %. The
 *          timer is started and pin level is sampled 20000x together with the counter
 *          (samples near PWM edges and over counter overflow are not evaluated).
 *
 * \par Expected results
 * - Duty cycle reads back 25 %, compare value 250.
 * - Pin high while counter < compare value, low while counter >= compare value.
 * - At least 100 evaluated samples in both levels.
 */
void It_Tim_Set_PwmMode_DutyCycle_25Percent_PinFollowsCounter( void )
{
    const tim_Counter_t compareValue = ( ( IT_TIM_PERIOD + 1u ) * 2500u ) / IT_TIM_DUTY_100;
    tim_CentiPercent_t  dutyCycle    = 0u;
    tim_Counter_t       compareRead  = 0u;
    uint32_t            highSamples  = 0u;
    uint32_t            lowSamples   = 0u;

    It_Tim_Init_PinChannel( TIM_CHANNEL_MODE_OUTPUT_PWM, TIM_POLARITY_HIGH, TIM_POLARITY_LOW );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Set_PwmMode_DutyCycle( IT_TIM_ADV, TIM_CHANNEL_1, 2500u ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_PwmMode_DutyCycle( IT_TIM_ADV, TIM_CHANNEL_1, &dutyCycle ) );
    TEST_ASSERT_EQUAL_UINT16( 2500u, dutyCycle );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_CompareValue( IT_TIM_ADV, TIM_CHANNEL_1, &compareRead ) );
    TEST_ASSERT_EQUAL_UINT32( compareValue, compareRead );

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Generate_Event( IT_TIM_ADV, TIM_EVENT_UPDATE ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Start( IT_TIM_ADV ) );

    /* PWM mode 1, up-counting: output high while counter < compare value. Pin
     * level is evaluated only if counter read before and after the pin read is
     * in the same PWM phase (not near edges, no counter overflow between). */
    for( uint32_t sampleIdx = 0u; IT_TIM_PWM_SAMPLES > sampleIdx; sampleIdx++ )
    {
        tim_Counter_t   counterBefore = 0u;
        tim_Counter_t   counterAfter  = 0u;
        gpio_PinLevel_t pinLevel      = GPIO_PIN_LEVEL_LOW;

        TEST_ASSERT_EQUAL( TIM_REQUEST_OK,  Tim_Get_Counter  ( IT_TIM_ADV, &counterBefore ) );
        TEST_ASSERT_EQUAL( GPIO_REQUEST_OK, Gpio_Get_PinLevel( IT_TIM_PWM_PORT, IT_TIM_PWM_PIN_ID, &pinLevel ) );
        TEST_ASSERT_EQUAL( TIM_REQUEST_OK,  Tim_Get_Counter  ( IT_TIM_ADV, &counterAfter  ) );

        if( counterAfter < counterBefore )
        {
            /* Counter overflow during the sample */
        }
        else if( ( IT_TIM_PWM_EDGE_MARGIN <= counterBefore                         ) &&
                 ( ( compareValue - IT_TIM_PWM_EDGE_MARGIN ) > counterAfter        )    )
        {
            TEST_ASSERT_EQUAL_MESSAGE( GPIO_PIN_LEVEL_HIGH, pinLevel, "PWM pin low while counter < compare value" );
            highSamples++;
        }
        else if( ( ( compareValue + IT_TIM_PWM_EDGE_MARGIN ) <= counterBefore       ) &&
                 ( ( IT_TIM_PERIOD - IT_TIM_PWM_EDGE_MARGIN ) > counterAfter       )    )
        {
            TEST_ASSERT_EQUAL_MESSAGE( GPIO_PIN_LEVEL_LOW, pinLevel, "PWM pin high while counter >= compare value" );
            lowSamples++;
        }
        else
        {
            /* Sample near PWM edge, not evaluated */
        }
    }

    TEST_ASSERT_GREATER_OR_EQUAL_UINT32( IT_TIM_PWM_MIN_SAMPLES, highSamples );
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32( IT_TIM_PWM_MIN_SAMPLES, lowSamples  );
}

/* ========================== LOCAL FUNCTIONS =============================== */

/**
 * \brief Fills time base configuration of timer (internal clock, up-counting, no channels).
 *
 * \param timConfig   [out]: Timer configuration
 * \param periphId     [in]: Timer peripheral identification
 * \param timerFreq    [in]: Counter frequency [Hz]
 * \param refreshFreq  [in]: Refresh (counter period) frequency [Hz]
 */
static void It_Tim_Get_TimeBaseConfig( tim_PeriphConfig_t * const timConfig, tim_PeriphId_t periphId, tim_FreqHz_t timerFreq, tim_FreqHz_t refreshFreq )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_DefaultConfig( timConfig ) );

    timConfig->PeriphId         = periphId;
    timConfig->TimerFrequency   = timerFreq;
    timConfig->RefreshFrequency = refreshFreq;
}


/**
 * \brief Initializes time base of timer (see \ref It_Tim_Get_TimeBaseConfig, \ref Tim_Init).
 *
 * \param periphId    [in]: Timer peripheral identification
 * \param timerFreq   [in]: Counter frequency [Hz]
 * \param refreshFreq [in]: Refresh (counter period) frequency [Hz]
 */
static void It_Tim_Init_TimeBase( tim_PeriphId_t periphId, tim_FreqHz_t timerFreq, tim_FreqHz_t refreshFreq )
{
    tim_PeriphConfig_t timConfig;

    It_Tim_Get_TimeBaseConfig( &timConfig, periphId, timerFreq, refreshFreq );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Init( &timConfig ) );
}


/**
 * \brief Initializes advanced timer with channel 1 output on IT_TIM_PWM_PIN.
 *
 * \param channelMode    [in]: Output channel mode
 * \param outputPolarity [in]: Output polarity
 * \param idleState      [in]: Output level when main output is disabled
 */
static void It_Tim_Init_PinChannel( tim_ChannelMode_t channelMode, tim_Polarity_t outputPolarity, tim_Polarity_t idleState )
{
    tim_PeriphConfig_t timConfig;

    It_Tim_Get_TimeBaseConfig( &timConfig, IT_TIM_ADV, IT_TIM_FREQ_HZ, IT_TIM_REFRESH_HZ );

    timConfig.ChannelConfig[ TIM_CHANNEL_1 ].ChannelState   = TIM_FUNCTION_ACTIVE;
    timConfig.ChannelConfig[ TIM_CHANNEL_1 ].ChannelMode    = channelMode;
    timConfig.ChannelConfig[ TIM_CHANNEL_1 ].IoPin          = IT_TIM_PWM_PIN;
    timConfig.ChannelConfig[ TIM_CHANNEL_1 ].OutputPolarity = outputPolarity;
    timConfig.ChannelConfig[ TIM_CHANNEL_1 ].IdleState      = idleState;

    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Init( &timConfig ) );
}


/**
 * \brief Generates update event (loads preloaded registers) and clears update flag.
 *
 * \param periphId [in]: Timer peripheral identification
 */
static void It_Tim_Apply_UpdateEvent( tim_PeriphId_t periphId )
{
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Generate_Event( periphId, TIM_EVENT_UPDATE ) );
    TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Clear_Flag( periphId, TIM_IRQ_UPDATE ) );
}


/**
 * \brief Waits until counter of the running reference timer reaches required time.
 *
 * \param waitTime_us [in]: Counter value of reference timer [us]
 */
static void It_Tim_Wait_RefTime( tim_Counter_t waitTime_us )
{
    tim_Counter_t refCounter = 0u;

    for( uint32_t loopIdx = 0u;
         ( IT_TIM_WAIT_LOOPS > loopIdx ) &&
         ( waitTime_us > refCounter );
         loopIdx++ )
    {
        TEST_ASSERT_EQUAL( TIM_REQUEST_OK, Tim_Get_Counter( IT_TIM_REF, &refCounter ) );
    }

    TEST_ASSERT_GREATER_OR_EQUAL_UINT32_MESSAGE( waitTime_us, refCounter, "Reference timer does not count" );
}


/**
 * \brief Waits until callback is called required count of times (or timeout).
 *
 * \param callbackCnt [in]: Callback call counter
 * \param expectedCnt [in]: Required count of callback calls
 */
static void It_Tim_Wait_CallbackCount( volatile const uint32_t * const callbackCnt, uint32_t expectedCnt )
{
    for( volatile uint32_t loopIdx = 0u;
         ( IT_TIM_WAIT_LOOPS > loopIdx ) &&
         ( expectedCnt > *callbackCnt );
         loopIdx++ )
    {
        /* Busy wait */
    }
}


/**
 * \brief Waits until pin level (and timer synchronization logic) is settled.
 */
static void It_Tim_WaitPinSettled( void )
{
    for( volatile uint32_t loopIdx = 0u; IT_TIM_SETTLE_LOOPS > loopIdx; loopIdx++ )
    {
        /* Busy wait */
    }
}


/**
 * \brief Checks actual level of timer output pin (input data register).
 *
 * \param expectedLevel [in]: Expected pin level
 */
static void It_Tim_Check_PinLevel( gpio_PinLevel_t expectedLevel )
{
    gpio_PinLevel_t pinLevel = GPIO_PIN_LEVEL_LOW;

    It_Tim_WaitPinSettled();

    TEST_ASSERT_EQUAL( GPIO_REQUEST_OK, Gpio_Get_PinLevel( IT_TIM_PWM_PORT, IT_TIM_PWM_PIN_ID, &pinLevel ) );
    TEST_ASSERT_EQUAL( expectedLevel, pinLevel );
}


/**
 * \brief Update interrupt callback - counts calls.
 */
static void It_Tim_UpdateCallback( void )
{
    itTim_UpdateCallbackCnt++;
}


/**
 * \brief Capture / compare interrupt callback - counts calls, stores over-capture flag.
 *
 * \param overcaptureFlag [in]: Over-capture state
 */
static void It_Tim_CaptureCompareCallback( tim_OvercaptureFlag_t overcaptureFlag )
{
    itTim_CcOvercapture = overcaptureFlag;
    itTim_CcCallbackCnt++;
}
