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

/*------------------- Counter and time base functionality --------------------*/

tim_RequestState_t      Tim_Set_Counter                 ( tim_PeriphId_t periphId, tim_Counter_t counterValue );
tim_RequestState_t      Tim_Get_Counter                 ( tim_PeriphId_t periphId, tim_Counter_t * const counterValue );

tim_RequestState_t      Tim_Set_Period                  ( tim_PeriphId_t periphId, tim_Counter_t autoreloadValue );
tim_RequestState_t      Tim_Get_Period                  ( tim_PeriphId_t periphId, tim_Counter_t * const autoreloadValue );

tim_RequestState_t      Tim_Set_RefreshFreq             ( tim_PeriphId_t periphId, tim_FreqHz_t reqFreq, tim_FreqHz_t * const trueFreq );
tim_RequestState_t      Tim_Get_RefreshFreq             ( tim_PeriphId_t periphId, tim_FreqHz_t * const refreshFreq );

tim_RequestState_t      Tim_Set_CounterDirection        ( tim_PeriphId_t periphId, tim_CounterDir_t counterDir );
tim_RequestState_t      Tim_Get_CounterDirection        ( tim_PeriphId_t periphId, tim_CounterDir_t * const counterDir );

tim_RequestState_t      Tim_Set_RepetitionCounter       ( tim_PeriphId_t periphId, tim_RepCnt_t repetitionCnt );
tim_RequestState_t      Tim_Get_RepetitionCounter       ( tim_PeriphId_t periphId, tim_RepCnt_t * const repetitionCnt );

tim_RequestState_t      Tim_Set_UpdateEventActive       ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Set_UpdateEventInactive     ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Get_UpdateEventState        ( tim_PeriphId_t periphId, tim_FunctionState_t * const eventState );

tim_RequestState_t      Tim_Set_UpdateSource            ( tim_PeriphId_t periphId, tim_UpdateSource_t updateSource );
tim_RequestState_t      Tim_Get_UpdateSource            ( tim_PeriphId_t periphId, tim_UpdateSource_t * const updateSource );

tim_RequestState_t      Tim_Generate_Event              ( tim_PeriphId_t periphId, tim_EventId_t eventId );

/*---------------------- Interrupt handling functionality --------------------*/

tim_RequestState_t      Tim_Set_IrqActive               ( tim_PeriphId_t periphId, tim_IrqId_t irqId );
tim_RequestState_t      Tim_Set_IrqInactive             ( tim_PeriphId_t periphId, tim_IrqId_t irqId );
tim_RequestState_t      Tim_Get_IrqState                ( tim_PeriphId_t periphId, tim_IrqId_t irqId, tim_FunctionState_t * const irqState );

tim_RequestState_t      Tim_Set_IrqPriority             ( tim_PeriphId_t periphId, tim_IrqPrio_t irqPrio );
tim_RequestState_t      Tim_Get_IrqPriority             ( tim_PeriphId_t periphId, tim_IrqPrio_t * const irqPrio );

tim_RequestState_t      Tim_Get_Flag                    ( tim_PeriphId_t periphId, tim_IrqId_t flagId, tim_FlagState_t * const flagState );
tim_RequestState_t      Tim_Clear_Flag                  ( tim_PeriphId_t periphId, tim_IrqId_t flagId );

tim_RequestState_t      Tim_Set_UpdateCallback          ( tim_PeriphId_t periphId, tim_UpdateIsrCallback_t * const callback );
tim_RequestState_t      Tim_Set_CaptureCompareCallback  ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_CaptureComapreIsrCallback_t * const callback );
tim_RequestState_t      Tim_Set_TriggerCallback         ( tim_PeriphId_t periphId, tim_TriggerIsrCallback_t * const callback );
tim_RequestState_t      Tim_Set_CommutationCallback     ( tim_PeriphId_t periphId, tim_CommutationIsrCallback_t * const callback );
tim_RequestState_t      Tim_Set_BreakCallback           ( tim_PeriphId_t periphId, tim_BreakIsrCallback_t * const callback );
tim_RequestState_t      Tim_Set_Break2Callback          ( tim_PeriphId_t periphId, tim_Break2IsrCallback_t * const callback );
tim_RequestState_t      Tim_Set_SystemBreakCallback     ( tim_PeriphId_t periphId, tim_SystemBreakIsrCallback_t * const callback );
tim_RequestState_t      Tim_Set_DirectionCallback       ( tim_PeriphId_t periphId, tim_DirectionIsrCallback_t * const callback );
tim_RequestState_t      Tim_Set_IndexCallback           ( tim_PeriphId_t periphId, tim_IndexIsrCallback_t * const callback );
tim_RequestState_t      Tim_Set_ErrorCallback           ( tim_PeriphId_t periphId, tim_ErrIsrCallback_t * const callback );

/*----------------- Master / slave synchronization functionality -------------*/

tim_RequestState_t      Tim_Set_MasterTrigger           ( tim_PeriphId_t periphId, tim_MasterTrigger_t masterTrigger );
tim_RequestState_t      Tim_Get_MasterTrigger           ( tim_PeriphId_t periphId, tim_MasterTrigger_t * const masterTrigger );

tim_RequestState_t      Tim_Set_MasterTrigger2          ( tim_PeriphId_t periphId, tim_MasterTrigger2_t masterTrigger2 );
tim_RequestState_t      Tim_Get_MasterTrigger2          ( tim_PeriphId_t periphId, tim_MasterTrigger2_t * const masterTrigger2 );

tim_RequestState_t      Tim_Set_MasterSlaveModeActive   ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Set_MasterSlaveModeInactive ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Get_MasterSlaveMode         ( tim_PeriphId_t periphId, tim_FunctionState_t * const modeState );

tim_RequestState_t      Tim_Set_SlaveMode               ( tim_PeriphId_t periphId, tim_SlaveMode_t slaveMode, tim_TriggerInput_t triggerInput );
tim_RequestState_t      Tim_Get_SlaveMode               ( tim_PeriphId_t periphId, tim_SlaveMode_t * const slaveMode, tim_TriggerInput_t * const triggerInput );

tim_RequestState_t      Tim_Set_ChannelMode             ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_ChannelMode_t channelMode );
tim_RequestState_t      Tim_Get_ChannelMode             ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_ChannelMode_t * const channelMode );

tim_RequestState_t      Tim_Set_Mode_InputCapture       ( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig );
tim_RequestState_t      Tim_Set_Mode_Encoder            ( tim_PeriphId_t periphId, const tim_EncoderConfig_t * const encoderConfig );
tim_RequestState_t      Tim_Set_Mode_HalSensor          ( tim_PeriphId_t periphId, const tim_HallSensorConfig_t * const hallConfig );
tim_RequestState_t      Tim_Set_Mode_Pwm                ( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig );
tim_RequestState_t      Tim_Set_Mode_ForcedOutput       ( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig );
tim_RequestState_t      Tim_Set_Mode_OutputCompare      ( tim_PeriphId_t periphId, tim_ChannelConfig_t * const channelConfig );
tim_RequestState_t      Tim_Set_Mode_InputPwm           ( tim_PeriphId_t periphId, tim_ChannelId_t inputChannel, tim_InputPolarity_t inputPolarity, tim_InputFilter_t inputFilter );

tim_RequestState_t      Tim_Get_TimStepTimeMin          ( tim_PeriphId_t periphId, tim_Time_ns_t * const stepTime );
tim_RequestState_t      Tim_Get_TimStepTimeMax          ( tim_PeriphId_t periphId, tim_Time_ns_t * const stepTime );

tim_RequestState_t      Tim_Set_PwmMode_DutyCycle       ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_CentiPercent_t dutyCycle );
tim_RequestState_t      Tim_Get_PwmMode_DutyCycle       ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_CentiPercent_t * const dutyCycle );
tim_RequestState_t      Tim_Set_PwmMode_PulseWidth      ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_Time_ns_t pulseWidth );

/*------------------ Output compare and PWM functionality --------------------*/

tim_RequestState_t      Tim_Set_CompareValue            ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_Counter_t compareValue );
tim_RequestState_t      Tim_Get_CompareValue            ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_Counter_t * const compareValue );

tim_RequestState_t      Tim_Set_ComparePreloadActive    ( tim_PeriphId_t periphId, tim_ChannelId_t channelId );
tim_RequestState_t      Tim_Set_ComparePreloadInactive  ( tim_PeriphId_t periphId, tim_ChannelId_t channelId );

tim_RequestState_t      Tim_Set_OutputPolarity          ( tim_PeriphId_t periphId, tim_OutputId_t outputId, tim_Polarity_t outputPolarity );
tim_RequestState_t      Tim_Get_OutputPolarity          ( tim_PeriphId_t periphId, tim_OutputId_t outputId, tim_Polarity_t * const outputPolarity );

tim_RequestState_t      Tim_Set_MainOutputActive        ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Set_MainOutputInactive      ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Get_MainOutputState         ( tim_PeriphId_t periphId, tim_FunctionState_t * const outputState );

/*------------------------ Input capture functionality -----------------------*/

tim_RequestState_t      Tim_Get_CaptureValue            ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_Counter_t * const captureValue );

tim_RequestState_t      Tim_Set_InputFilter             ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_InputFilter_t inputFilter );
tim_RequestState_t      Tim_Set_InputPrescaler          ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_InputPrescaler_t inputPrescaler );
tim_RequestState_t      Tim_Set_InputPolarity           ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_InputPolarity_t inputPolarity );

tim_RequestState_t      Tim_Get_InputPwm                ( tim_PeriphId_t periphId, tim_ChannelId_t inputChannel, tim_FreqHz_t * const frequency, tim_CentiPercent_t * const dutyCycle );

tim_RequestState_t      Tim_Set_InputSource             ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_InputSource_t inputSource );
tim_RequestState_t      Tim_Get_InputSource             ( tim_PeriphId_t periphId, tim_ChannelId_t channelId, tim_InputSource_t * const inputSource );

tim_RequestState_t      Tim_Set_ClockDivision           ( tim_PeriphId_t periphId, tim_ClockDiv_t clockDiv );
tim_RequestState_t      Tim_Get_ClockDivision           ( tim_PeriphId_t periphId, tim_ClockDiv_t * const clockDiv );

/*--------------------- Encoder interface functionality ----------------------*/

tim_RequestState_t      Tim_Get_EncoderPosition         ( tim_PeriphId_t periphId, tim_Counter_t * const position );
tim_RequestState_t      Tim_Set_EncoderIndex            ( tim_PeriphId_t periphId, const tim_EncoderIndexConfig_t * const indexConfig );

/*----------------------- Dead-time and break functionality ------------------*/

tim_RequestState_t      Tim_Set_DeadTime                ( tim_PeriphId_t periphId, tim_Time_ns_t risingDeadTime, tim_Time_ns_t fallingDeadTime );

tim_RequestState_t      Tim_Set_BreakConfig             ( tim_PeriphId_t periphId, const tim_BreakConfig_t * const breakConfig );
tim_RequestState_t      Tim_Set_Break2Config            ( tim_PeriphId_t periphId, const tim_BreakConfig_t * const breakConfig );

tim_RequestState_t      Tim_Set_AutomaticOutputActive   ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Set_AutomaticOutputInactive ( tim_PeriphId_t periphId );

tim_RequestState_t      Tim_Set_OffStateConfig          ( tim_PeriphId_t periphId, tim_FunctionState_t offStateIdle, tim_FunctionState_t offStateRun );
tim_RequestState_t      Tim_Set_LockLevel               ( tim_PeriphId_t periphId, tim_LockLevel_t lockLevel );

tim_RequestState_t      Tim_Set_CommutationPreloadActive  ( tim_PeriphId_t periphId, tim_CommutationUpdate_t updateSource );
tim_RequestState_t      Tim_Set_CommutationPreloadInactive( tim_PeriphId_t periphId );

/*--------------------- DMA and external trigger functionality ---------------*/

tim_RequestState_t      Tim_Set_DmaRequestActive        ( tim_PeriphId_t periphId, tim_DmaRequest_t dmaRequest );
tim_RequestState_t      Tim_Set_DmaRequestInactive      ( tim_PeriphId_t periphId, tim_DmaRequest_t dmaRequest );
tim_RequestState_t      Tim_Set_DmaBurst                ( tim_PeriphId_t periphId, tim_DmaRequest_t burstSource, tim_DmaBurstReg_t baseReg, tim_DmaBurstLen_t burstLength );
tim_RequestState_t      Tim_Get_DmaRegAddr              ( tim_PeriphId_t periphId, tim_DmaReg_t dmaReg, tim_RegAddr_t * const regAddr );

tim_RequestState_t      Tim_Set_EtrConfig               ( tim_PeriphId_t periphId, tim_Polarity_t etrPolarity, tim_EtrPrescaler_t etrPrescaler, tim_InputFilter_t etrFilter );
tim_RequestState_t      Tim_Set_EtrSource               ( tim_PeriphId_t periphId, tim_EtrSource_t etrSource );

/*------- STM32H5 functionality (STM32H7: UIF remapping, no dithering) -------*/

tim_RequestState_t      Tim_Set_DitheringActive         ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Set_DitheringInactive       ( tim_PeriphId_t periphId );

tim_RequestState_t      Tim_Set_UifRemapActive          ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Set_UifRemapInactive        ( tim_PeriphId_t periphId );
tim_RequestState_t      Tim_Get_CounterWithOverflow     ( tim_PeriphId_t periphId, tim_Counter_t * const counterValue, tim_FlagState_t * const overflowFlag );

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

