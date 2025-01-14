/**
******************************************************************************
* @file    platform_common_config.h
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provides common configuration for current platform.
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

#pragma once
#include "stm32f2xx_flash.h"
#include "misc.h"
/******************************************************
*                      Macros
******************************************************/

/******************************************************
*                    Constants
******************************************************/

/* MICO RTOS tick rate in Hz */
#define MICO_DEFAULT_TICK_RATE_HZ                   (1000) 

/************************************************************************
 * Uncomment to disable watchdog. For debugging only */
//#define MICO_DISABLE_WATCHDOG

/************************************************************************
 * Uncomment to disable standard IO, i.e. printf(), etc. */
//#define MICO_DISABLE_STDIO

/************************************************************************
 * Uncomment to disable MCU powersave API functions */
//#define MICO_DISABLE_MCU_POWERSAVE

/************************************************************************
 * Uncomment to enable MCU real time clock */
#define MICO_ENABLE_MCU_RTC



#define HSE_SOURCE              RCC_HSE_ON               /* Use external crystal                 */
#define AHB_CLOCK_DIVIDER       RCC_SYSCLK_Div1          /* AHB clock = System clock             */
#define APB1_CLOCK_DIVIDER      RCC_HCLK_Div4            /* APB1 clock = AHB clock / 4           */
#define APB2_CLOCK_DIVIDER      RCC_HCLK_Div2            /* APB2 clock = AHB clock / 2           */
#define PLL_SOURCE              RCC_PLLSource_HSE        /* PLL source = external crystal        */
#define PLL_M_CONSTANT          26                       /* PLLM = 26                            */
#define PLL_N_CONSTANT          240                      /* PLLN = 240                           */
#define PLL_P_CONSTANT          2                        /* PLLP = 2                             */
#define PPL_Q_CONSTANT          5                        /* PLLQ = 5                             */
#define SYSTEM_CLOCK_SOURCE     RCC_SYSCLKSource_PLLCLK  /* System clock source = PLL clock      */
#define SYSTICK_CLOCK_SOURCE    SysTick_CLKSource_HCLK   /* SysTick clock source = AHB clock     */
#define INT_FLASH_WAIT_STATE    FLASH_Latency_3          /* Internal flash wait state = 3 cycles */

#define SDIO_OOB_IRQ_BANK       GPIOB
#define SDIO_CLK_BANK           GPIOC
#define SDIO_CMD_BANK           GPIOD
#define SDIO_D0_BANK            GPIOC
#define SDIO_D1_BANK            GPIOC
#define SDIO_D2_BANK            GPIOC
#define SDIO_D3_BANK            GPIOC
#define SDIO_OOB_IRQ_BANK_CLK   RCC_AHB1Periph_GPIOB
#define SDIO_CLK_BANK_CLK       RCC_AHB1Periph_GPIOC
#define SDIO_CMD_BANK_CLK       RCC_AHB1Periph_GPIOD
#define SDIO_D0_BANK_CLK        RCC_AHB1Periph_GPIOC
#define SDIO_D1_BANK_CLK        RCC_AHB1Periph_GPIOC
#define SDIO_D2_BANK_CLK        RCC_AHB1Periph_GPIOC
#define SDIO_D3_BANK_CLK        RCC_AHB1Periph_GPIOC
#define SDIO_OOB_IRQ_PIN        13
#define SDIO_CLK_PIN            12
#define SDIO_CMD_PIN            2
#define SDIO_D0_PIN             8
#define SDIO_D1_PIN             9
#define SDIO_D2_PIN             10
#define SDIO_D3_PIN             11

/* These are internal platform connections only */
typedef enum
{
  MICO_GPIO_UNUSED = -1,
  MICO_GPIO_WLAN_POWERSAVE_CLOCK = 0,
  WL_GPIO0,
  WL_GPIO1,
  WL_REG,
  WL_RESET,
  MICO_SYS_LED,
  MICO_RF_LED,
  BOOT_SEL,
  MFG_SEL,
  EasyLink_BUTTON,
  MICO_COMMON_GPIO_MAX,
} mico_common_gpio_t;

/* How the wlan's powersave clock is connected */
typedef enum
{
  MICO_PWM_WLAN_POWERSAVE_CLOCK,
  MICO_COMMON_PWM_MAX,
} mico_common_pwm_t;

/* WLAN Powersave Clock Source
 * The WLAN sleep clock can be driven from one of two sources:
 * 1. Timer/PWM (default)
 *    - With the PWM selected, the STM32 can *NOT* be put into MCU powersave mode or the PWM output will be disabled
 * 2. MCO (MCU Clock Output). 
 *    - Change the following directive to MICO_WLAN_POWERSAVE_CLOCK_IS_MCO
 */
#define MICO_WLAN_POWERSAVE_CLOCK_SOURCE MICO_WLAN_POWERSAVE_CLOCK_IS_MCO

#define MICO_WLAN_POWERSAVE_CLOCK_IS_NOT_EXIST  0
#define MICO_WLAN_POWERSAVE_CLOCK_IS_PWM        1
#define MICO_WLAN_POWERSAVE_CLOCK_IS_MCO        2

#define WLAN_POWERSAVE_CLOCK_FREQUENCY 32768 /* 32768Hz        */
#define WLAN_POWERSAVE_CLOCK_DUTY_CYCLE   50 /* 50% duty-cycle */

#define WL_32K_OUT_BANK         GPIOA
#define WL_32K_OUT_PIN          8
#define WL_32K_OUT_BANK_CLK     RCC_AHB1Periph_GPIOA

/* The number of UART interfaces this hardware platform has */
#define NUMBER_OF_UART_INTERFACES  2

#define STDIO_UART       MICO_UART_1

/* Define the address from where user application will be loaded.
Note: the 1st sector 0x08000000-0x08003FFF is reserved for the IAP code */
#define INTERNAL_FLASH_START_ADDRESS   (uint32_t)0x08000000
#define INTERNAL_FLASH_END_ADDRESS     (uint32_t)0x080FFFFF
#define INTERNAL_FLASH_SIZE            (INTERNAL_FLASH_END_ADDRESS - INTERNAL_FLASH_START_ADDRESS + 1)

#define MICO_FLASH_FOR_APPLICATION  MICO_INTERNAL_FLASH
#define APPLICATION_START_ADDRESS   (uint32_t)0x0800C000
#define APPLICATION_END_ADDRESS     (uint32_t)0x0805FFFF
#define APPLICATION_FLASH_SIZE      (APPLICATION_END_ADDRESS - APPLICATION_START_ADDRESS + 1)

#define MICO_FLASH_FOR_UPDATE       MICO_INTERNAL_FLASH /* Optional */
#define UPDATE_START_ADDRESS        (uint32_t)0x08060000  /* Optional */
#define UPDATE_END_ADDRESS          (uint32_t)0x080BFFFF  /* Optional */
#define UPDATE_FLASH_SIZE           (UPDATE_END_ADDRESS - UPDATE_START_ADDRESS + 1) /* 384k bytes, optional*/

#define MICO_FLASH_FOR_BOOT         MICO_INTERNAL_FLASH
#define BOOT_START_ADDRESS          (uint32_t)0x08000000 
#define BOOT_END_ADDRESS            (uint32_t)0x08003FFF 
#define BOOT_FLASH_SIZE             (BOOT_END_ADDRESS - BOOT_START_ADDRESS + 1)

#define MICO_FLASH_FOR_DRIVER       MICO_INTERNAL_FLASH
#define DRIVER_START_ADDRESS        (uint32_t)0x080C0000 
#define DRIVER_END_ADDRESS          (uint32_t)0x080FFFFF 
#define DRIVER_FLASH_SIZE           (DRIVER_END_ADDRESS - DRIVER_START_ADDRESS + 1)

#define MICO_FLASH_FOR_PARA         MICO_INTERNAL_FLASH
#define PARA_START_ADDRESS          (uint32_t)0x08004000 
#define PARA_END_ADDRESS            (uint32_t)0x08007FFF
#define PARA_FLASH_SIZE             (PARA_END_ADDRESS - PARA_START_ADDRESS + 1)  

#define MICO_FLASH_FOR_EX_PARA      MICO_INTERNAL_FLASH
#define EX_PARA_START_ADDRESS       (uint32_t)0x08008000 
#define EX_PARA_END_ADDRESS         (uint32_t)0x0800BFFF
#define EX_PARA_FLASH_SIZE          (EX_PARA_END_ADDRESS - EX_PARA_START_ADDRESS + 1)  

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
*                 Global Variables
******************************************************/

/******************************************************
*               Function Declarations
******************************************************/
