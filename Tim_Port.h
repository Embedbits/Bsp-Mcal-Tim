/**
 * \author Mr.Nobody
 * \file Tim_Port.h
 * \ingroup Tim
 * \brief Timer module public functionality
 *
 * This file contains all available public functionality, any other files shall 
 * not be used outside of the module.
 *
 */

#ifndef TIM_TIM_PORT_H
#define TIM_TIM_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================== INCLUDES ================================== */
#include "Tim_Types.h"                      /* Module types definition        */
/* ============================== TYPEDEFS ================================== */

/* ========================== SYMBOLIC CONSTANTS ============================ */

/* ========================== EXPORTED MACROS =============================== */

/* ========================== EXPORTED VARIABLES ============================ */

/* ========================= EXPORTED FUNCTIONS ============================= */

tim_ModuleVersion_t     Tim_Get_ModuleVersion           ( void );

tim_RequestState_t      Tim_Init                        ( tim_PeriphConfig_t * const timConfig );
tim_RequestState_t      Tim_Deinit                      ( tim_PeriphConfig_t * const timConfig );
void                    Tim_Task                        ( void );

tim_RequestState_t      Tim_Get_DefaultConfig           ( tim_PeriphConfig_t * const timConfig );

tim_RequestState_t      Tim_Start                       ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Stop                        ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Pause                       ( tim_PeriphId_t periphId );

tim_RequestState_t      Tim_InitBase                    ( tim_PeriphConfig_t * const timConfig );
tim_RequestState_t      Tim_InitChannel                 ( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig );

tim_RequestState_t      Tim_Set_ClkInternal             ( tim_PeriphId_t periphId, tim_FreqHz_t reqFreq, tim_FreqHz_t * const trueFreq );
tim_RequestState_t      Tim_Get_ClkInternal             ( tim_PeriphId_t periphId, tim_FreqHz_t * const timFreq );

//tim_RequestState_t      Tim_Set_ChannelMode             ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_Mode_t channelMode );
//tim_RequestState_t      Tim_Get_ChannelMode             ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_Mode_t * const channelMode );

tim_RequestState_t      Tim_Set_TriggerSource           ( tim_PeriphId_t periphId, tim_ChannelId_t channelId );

tim_RequestState_t      Tim_Set_Mode_InputCapture       ( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig );
tim_RequestState_t      Tim_Set_Mode_Encoder            ( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig );
tim_RequestState_t      Tim_Set_Mode_HalSensor          ( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig );
tim_RequestState_t      Tim_Set_Mode_Pwm                ( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig );
tim_RequestState_t      Tim_Set_Mode_ForcedOutput       ( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig );
tim_RequestState_t      Tim_Set_Mode_OutputCompare      ( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig );

tim_RequestState_t      Tim_Get_TimStepTimeMin          ( tim_PeriphId_t periphId, tim_Time_ns_t * const stepTime );
tim_RequestState_t      Tim_Get_TimStepTimeMax          ( tim_PeriphId_t periphId, tim_Time_ns_t * const stepTime );


tim_RequestState_t      Tim_Set_PwmMode_DutyCycle       ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_CentiPercent_t dutyCycle );
tim_RequestState_t      Tim_Get_PwmMode_DutyCycle       ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_CentiPercent_t * const dutyCycle );

/*-------------- Channel input mode configuration functionality --------------*/

tim_RequestState_t      Tim_Set_InputCaptureFilter      ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_InputFilter_t inputFilter );

/*--------------------- GPIO configuration functionality ---------------------*/

tim_RequestState_t      Tim_InitIOPin                   ( tim_IoPin_t pinId );
tim_RequestState_t      Tim_InitIOComplPin              ( tim_IOComplPin_t pinId );
tim_RequestState_t      Tim_InitTriggerEventGpio        ( tim_EtrPin_t pinId );
tim_RequestState_t      Tim_InitBreakInputGpio          ( tim_BkinPin_t pinId );
tim_RequestState_t      Tim_InitBreakInput2Gpio         ( tim_Bkin2Pin_t pinId );

/*-------------------- Primitive function's functionality --------------------*/

tim_RequestState_t      Tim_Set_PeriphActive            ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Set_PeriphInactive          ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Get_PeriphState             ( tim_PeriphId_t periphId, tim_FunctionState_t * const periphState );

tim_RequestState_t      Tim_Set_ClockSource             ( tim_PeriphId_t periphId, tim_ClockSource_t clockSource, tim_ExtClkSource_t triggerSource );
tim_RequestState_t      Tim_Get_ClockSource             ( tim_PeriphId_t periphId, tim_ClockSource_t * const clockSource );

tim_RequestState_t      Tim_Set_AutoreloadActive        ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Set_AutoreloadInactive      ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Get_AutoreloadState         ( tim_PeriphId_t periphId, tim_FunctionState_t * const modeState );

tim_RequestState_t      Tim_Set_OnePulseModeActive      ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Set_OnePulseModeInactive    ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Get_OnePulseMode            ( tim_PeriphId_t periphId, tim_FunctionState_t * const modeState );


tim_RequestState_t      Tim_Set_OutputActive            ( tim_PeriphId_t periphId, tim_OutputId_t outputId );
tim_RequestState_t      Tim_Set_OutputInactive          ( tim_PeriphId_t periphId, tim_OutputId_t outputId );
tim_RequestState_t      Tim_Get_OutputState             ( tim_PeriphId_t periphId, tim_OutputId_t outputId, tim_FunctionState_t * const modeState );

#ifdef __cplusplus
}
#endif

#endif /* TIM_TIM_PORT_H */

