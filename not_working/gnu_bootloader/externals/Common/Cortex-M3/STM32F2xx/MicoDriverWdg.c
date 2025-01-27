/**
******************************************************************************
* @file    MicoDriverWdg.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide WDG driver functions.
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2014 MXCHIP Inc.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy 
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights 
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is furnished
*  to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
*  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************
*/ 


#include "MicoPlatform.h"
#include "MicoRtos.h"
#include "common.h"
//#include "debug.h"

#include "platform.h"
#include "platform_common_config.h"
#include "stm32f2xx_platform.h"
#include "stm32f2xx.h"
#include "stm32f2xx_iwdg.h"
/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/
#ifndef MICO_DISABLE_WATCHDOG
static __IO uint32_t LsiFreq = 0;
static __IO uint32_t CaptureNumber = 0, PeriodValue = 0;
static mico_semaphore_t  _measureLSIComplete_SEM = NULL;
uint16_t tmpCC4[2] = {0, 0};
#endif

/******************************************************
 *               Function Declarations
 ******************************************************/
#ifndef MICO_DISABLE_WATCHDOG
static uint32_t GetLSIFrequency(void);
#endif

/******************************************************
 *               Function Definitions
 ******************************************************/

OSStatus MicoWdgInitialize( uint32_t timeout_ms )
{
// PLATFORM_TO_DO
#ifndef MICO_DISABLE_WATCHDOG
  OSStatus err = kNoErr;
  uint16_t reloadTick;
  /* Get the LSI frequency:  TIM5 is used to measure the LSI frequency */
  LsiFreq = GetLSIFrequency();
  
  /* Set counter reload value to obtain 250ms IWDG TimeOut.
     Counter Reload Value = timeout_ms /IWDG counter clock period
                          = timeout_ms * (LSI/256) / 1000
   */
  reloadTick = LsiFreq*timeout_ms/256000;
  require_action( reloadTick <= 0xFFF, exit, err = kParamErr );

  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

  /* IWDG counter clock: LSI/256 */
  IWDG_SetPrescaler(IWDG_Prescaler_256);

  IWDG_SetReload(reloadTick);

  /* Reload IWDG counter */
  IWDG_ReloadCounter();

  /* Enable IWDG (the LSI oscillator will be enabled by hardware) */
  IWDG_Enable();

exit:
  return err;
#else
  UNUSED_PARAMETER( timeout_ms );
  return kUnsupportedErr;
#endif
}

OSStatus MicoWdgFinalize( void )
{
    // PLATFORM_TO_DO
    return kNoErr;
}

void MicoWdgReload( void )
{
#ifndef MICO_DISABLE_WATCHDOG
  IWDG_ReloadCounter();
#else
  return;
#endif
}


#ifndef MICO_DISABLE_WATCHDOG
/**
  * @brief  Configures TIM5 to measure the LSI oscillator frequency. 
  * @param  None
  * @retval LSI Frequency
  */
uint32_t GetLSIFrequency(void)
{
  NVIC_InitTypeDef   NVIC_InitStructure;
  TIM_ICInitTypeDef  TIM_ICInitStructure;
  RCC_ClocksTypeDef  RCC_ClockFreq;

  mico_rtos_init_semaphore(&_measureLSIComplete_SEM, 1);


  /* Enable the LSI oscillator ************************************************/
  RCC_LSICmd(ENABLE);
  
  /* Wait till LSI is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
  {}

  /* TIM5 configuration *******************************************************/ 
  /* Enable TIM5 clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
  
  /* Connect internally the TIM5_CH4 Input Capture to the LSI clock output */
  TIM_RemapConfig(TIM5, TIM5_LSI);

  /* Configure TIM5 presclaer */
  TIM_PrescalerConfig(TIM5, 0, TIM_PSCReloadMode_Immediate);
  
  /* TIM5 configuration: Input Capture mode ---------------------
     The LSI oscillator is connected to TIM5 CH4
     The Rising edge is used as active edge,
     The TIM5 CCR4 is used to compute the frequency value 
  ------------------------------------------------------------ */
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV8;
  TIM_ICInitStructure.TIM_ICFilter = 0;
  TIM_ICInit(TIM5, &TIM_ICInitStructure);
  
  /* Enable TIM5 Interrupt channel */
  NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 8;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable TIM5 counter */
  TIM_Cmd(TIM5, ENABLE);

  /* Reset the flags */
  TIM5->SR = 0;
    
  /* Enable the CC4 Interrupt Request */  
  TIM_ITConfig(TIM5, TIM_IT_CC4, ENABLE);

  /* Wait until the TIM5 get 2 LSI edges (refer to TIM5_IRQHandler()) *********/
  mico_rtos_get_semaphore(&_measureLSIComplete_SEM, MICO_WAIT_FOREVER);

  /* Deinitialize the TIM5 peripheral registers to their default reset values */
  TIM_ITConfig(TIM5, TIM_IT_CC4, DISABLE);
  TIM_DeInit(TIM5);
  NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 8;
  NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Compute the LSI frequency, depending on TIM5 input clock frequency (PCLK1)*/
  /* Get SYSCLK, HCLK and PCLKx frequency */
  RCC_GetClocksFreq(&RCC_ClockFreq);

  /* Get PCLK1 prescaler */
  if ((RCC->CFGR & RCC_CFGR_PPRE1) == 0)
  { 
    /* PCLK1 prescaler equal to 1 => TIMCLK = PCLK1 */
    return ((RCC_ClockFreq.PCLK1_Frequency / PeriodValue) * 8);
  }
  else
  { /* PCLK1 prescaler different from 1 => TIMCLK = 2 * PCLK1 */
    return (((2 * RCC_ClockFreq.PCLK1_Frequency) / PeriodValue) * 8) ;
  }
}

/**
  * @brief  This function handles TIM5 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM5_IRQHandler(void)
{
  if (TIM_GetITStatus(TIM5, TIM_IT_CC4) != RESET)
  {  
    /* Clear CC4 Interrupt pending bit */
    TIM_ClearITPendingBit(TIM5, TIM_IT_CC4);
    if (CaptureNumber > 2)
        return;
   
    /* Get the Input Capture value */
    tmpCC4[CaptureNumber++] = TIM_GetCapture4(TIM5);
 
    if (CaptureNumber == 2)
    {
      /* Compute the period length */
      PeriodValue = (uint16_t)(0xFFFF - tmpCC4[0] + tmpCC4[1] + 1);
      if(_measureLSIComplete_SEM != NULL){
        mico_rtos_set_semaphore(&_measureLSIComplete_SEM);
      }
    }
  }
}
#endif
