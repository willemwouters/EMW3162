/**
******************************************************************************
* @file    MicoDriverI2C.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide I2C driver functions.
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


#include "MicoRtos.h"
#include "MicoPlatform.h"

#include "platform.h"
#include "platform_common_config.h"
#include "stm32f2xx_platform.h"
#include "stm32f2xx.h"
#include "stm32f2xx_i2c.h"
/******************************************************
*                    Constants
******************************************************/

#define I2C_FLAG_CHECK_TIMEOUT      ( 1000 )
#define I2C_FLAG_CHECK_LONG_TIMEOUT ( 1000 )

//#define I2C_USE_DMA           

#define DMA_FLAG_TC(stream_id) dma_flag_tc(stream_id)
#define DMA_TIMEOUT_LOOPS      (10000000)

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

#ifdef I2C_USE_DMA
static DMA_InitTypeDef  i2c_dma_init; /* Should investigate why this is global */
#endif

/******************************************************
*               Function Declarations
******************************************************/


/******************************************************
*               Function Definitions
******************************************************/

OSStatus MicoI2cInitialize( mico_i2c_device_t* device  )
{
  GPIO_InitTypeDef GPIO_InitStructure;
  I2C_InitTypeDef  I2C_InitStructure;
  
  MicoMcuPowerSaveConfig(false);
  
  // Init I2C GPIO clocks
  RCC_APB1PeriphClockCmd( i2c_mapping[device->port].peripheral_clock_reg, ENABLE );
  RCC_AHB1PeriphClockCmd( i2c_mapping[device->port].pin_scl->peripheral_clock | i2c_mapping[device->port].pin_sda->peripheral_clock, ENABLE );
  RCC_APB2PeriphClockCmd( RCC_APB2Periph_SYSCFG, ENABLE );
  
  // Reset the I2C clock
  RCC_APB1PeriphResetCmd( i2c_mapping[device->port].peripheral_clock_reg, ENABLE );
  RCC_APB1PeriphResetCmd( i2c_mapping[device->port].peripheral_clock_reg, DISABLE );
  
  // GPIO Configuration
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  
  GPIO_InitStructure.GPIO_Pin = (uint32_t)( 1 << i2c_mapping[device->port].pin_scl->number );
  GPIO_Init( i2c_mapping[device->port].pin_scl->bank, &GPIO_InitStructure );
  GPIO_InitStructure.GPIO_Pin = (uint32_t)( 1 << i2c_mapping[device->port].pin_sda->number );
  GPIO_Init( i2c_mapping[device->port].pin_sda->bank, &GPIO_InitStructure );
  
  // Configure SDA and SCL as I2C pins
  GPIO_PinAFConfig( i2c_mapping[device->port].pin_scl->bank, (uint16_t)i2c_mapping[device->port].pin_scl->number, (uint8_t)i2c_mapping[device->port].gpio_af );
  GPIO_PinAFConfig( i2c_mapping[device->port].pin_sda->bank, (uint16_t)i2c_mapping[device->port].pin_sda->number, (uint8_t)i2c_mapping[device->port].gpio_af );
  
#ifdef I2C_USE_DMA
  // Enable the DMA clock
  RCC_AHB1PeriphClockCmd( i2c_mapping[device->port].tx_dma_peripheral_clock, ENABLE );
  
  // Configure the DMA streams for operation with the CP
  i2c_dma_init.DMA_Channel            = i2c_mapping[device->port].tx_dma_channel;
  i2c_dma_init.DMA_PeripheralBaseAddr = (uint32_t)&i2c_mapping[device->port].i2c->DR;
  i2c_dma_init.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
  i2c_dma_init.DMA_MemoryInc          = DMA_MemoryInc_Enable;
  i2c_dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  i2c_dma_init.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
  i2c_dma_init.DMA_Mode               = DMA_Mode_Normal;
  i2c_dma_init.DMA_Priority           = DMA_Priority_VeryHigh;
  //dma_init.DMA_FIFOMode           = DMA_FIFOMode_Enable;
  //dma_init.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full;
  i2c_dma_init.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
  i2c_dma_init.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;
  i2c_dma_init.DMA_Memory0BaseAddr    = (uint32_t) 0;               // This parameter will be configured during communication
  i2c_dma_init.DMA_DIR                = DMA_DIR_MemoryToPeripheral; // This parameter will be configured during communication
  i2c_dma_init.DMA_BufferSize         = 0xFFFF;                     // This parameter will be configured during communication
  
  DMA_DeInit( i2c_mapping[device->port].rx_dma_stream );
  DMA_DeInit( i2c_mapping[device->port].tx_dma_stream );
  
  // Clear any pending flags, disable, and clear the Tx DMA channel
  //DMA_ClearFlag( i2c_mapping[device->port].tx_dma_stream, CP_TX_DMA_FLAG_FEIF | CP_TX_DMA_FLAG_DMEIF | CP_TX_DMA_FLAG_TEIF | CP_TX_DMA_FLAG_HTIF | CP_TX_DMA_FLAG_TCIF );
  DMA_Cmd( i2c_mapping[device->port].tx_dma_stream, DISABLE );
  DMA_Cmd( i2c_mapping[device->port].rx_dma_stream, DISABLE );
  
  // Clear any pending flags, disable, and clear the Rx DMA channel
  //DMA_ClearFlag( i2c_mapping[device->port].rx_dma_stream, CP_RX_DMA_FLAG_FEIF | CP_RX_DMA_FLAG_DMEIF | CP_RX_DMA_FLAG_TEIF | CP_RX_DMA_FLAG_HTIF | CP_RX_DMA_FLAG_TCIF );
#endif
  
  // Initialize the InitStruct for the CP
  I2C_InitStructure.I2C_Mode                = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle           = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1         = 0xA0;
  I2C_InitStructure.I2C_Ack                 = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  
  if ( device->speed_mode == I2C_LOW_SPEED_MODE )
  {
    I2C_InitStructure.I2C_ClockSpeed = 10000;
  }
  else if ( device->speed_mode == I2C_STANDARD_SPEED_MODE )
  {
    I2C_InitStructure.I2C_ClockSpeed = 100000;
  }
  else if ( device->speed_mode == I2C_HIGH_SPEED_MODE )
  {
    I2C_InitStructure.I2C_ClockSpeed = 400000;
  }
  
  // Enable and initialize the I2C bus
  I2C_Cmd ( i2c_mapping[device->port].i2c, ENABLE );
  I2C_Init( i2c_mapping[device->port].i2c, &I2C_InitStructure );
  
#ifdef I2C_USE_DMA
  // Enable DMA on the I2C bus
  I2C_DMACmd( i2c_mapping[device->port].i2c, ENABLE );
#endif
  
  MicoMcuPowerSaveConfig(true);
  
  return kNoErr;
}

static OSStatus i2c_wait_for_event( I2C_TypeDef* i2c, uint32_t event_id, uint32_t number_of_waits )
{
  msleep(2);
  while ( I2C_CheckEvent( i2c, event_id ) != SUCCESS )
  {
    number_of_waits--;
    if ( number_of_waits == 0 )
    {
      return kTimeoutErr;
    }
  }
  
  return kNoErr;
}

#ifdef I2C_USE_DMA
static uint32_t dma_flag_tc( int stream_id )
{
  const uint32_t transfer_complete_flags[]=
  {
    /* for every stream get a transfer complete flag */
    [0] =  DMA_FLAG_TCIF0,
    [1] =  DMA_FLAG_TCIF1,
    [2] =  DMA_FLAG_TCIF2,
    [3] =  DMA_FLAG_TCIF3,
    [4] =  DMA_FLAG_TCIF4,
    [5] =  DMA_FLAG_TCIF5,
    [6] =  DMA_FLAG_TCIF6,
    [7] =  DMA_FLAG_TCIF7,
  };
  
  return transfer_complete_flags[stream_id];
}
#endif

bool MicoI2cProbeDevice( mico_i2c_device_t* device, int retries )
{
  int            i;
  OSStatus       result;
  
  MicoMcuPowerSaveConfig(false);
  
  for ( i = 0; i < retries; i++ )
  {
    /* generate a start condition and address a device in write mode */
    I2C_GenerateSTART( i2c_mapping[device->port].i2c, ENABLE );
    
    /* wait till start condition is generated and the bus becomes free */
    result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_MODE_SELECT, I2C_FLAG_CHECK_TIMEOUT );
    if ( result != kNoErr )
    {
      MicoMcuPowerSaveConfig(true);
      return false;
    }
    
    if ( device->address_width == I2C_ADDRESS_WIDTH_7BIT )
    {
      /* send the address and R/W bit set to write of the requested device, wait for an acknowledge */
      I2C_Send7bitAddress( i2c_mapping[device->port].i2c, (uint8_t) ( device->address << 1 ), I2C_Direction_Transmitter );
      
      /* wait till address gets sent and the direction bit is sent and */
      result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
      if ( result != kNoErr )
      {
        /* keep on pinging */
        continue;
      }
      else
      {
        break;
      }
    }
    else
    {
      // TODO
      /* send 10 bits of the address and wait for an acknowledge */
    }
  }
  
  /* generate a stop condition */
  I2C_GenerateSTOP( i2c_mapping[device->port].i2c, ENABLE );
  
  /* Check if the device didn't respond */
  if ( i == retries )
  {
    MicoMcuPowerSaveConfig(true);
    return false;
  }
  else
  {
    MicoMcuPowerSaveConfig(false);
    return true;
  }
}

#ifdef I2C_USE_DMA
static OSStatus i2c_dma_config_and_execute( mico_i2c_device_t* device, mico_i2c_message_t* message, bool tx_dma  )
{
  uint32_t counter;
  
  /* Initialize the DMA with the new parameters */
  if ( tx_dma == true )
  {
    /* Enable DMA channel for I2C */
    I2C_DMACmd( i2c_mapping[device->port].i2c, ENABLE );
    
    /* TX DMA configuration */
    DMA_DeInit( i2c_mapping[device->port].tx_dma_stream );
    
    /* Configure the DMA TX Stream with the buffer address and the buffer size */
    i2c_dma_init.DMA_Memory0BaseAddr = (uint32_t)message->tx_buffer;
    i2c_dma_init.DMA_DIR             = DMA_DIR_MemoryToPeripheral;
    i2c_dma_init.DMA_BufferSize      = (uint32_t)message->tx_length;
    DMA_Init(i2c_mapping[device->port].tx_dma_stream, &i2c_dma_init);
    
    /* Enable DMA channel */
    DMA_Cmd( i2c_mapping[device->port].tx_dma_stream, ENABLE );
    
    /* wait until transfer is completed */
    /* TODO: change flag!!!!,wait on a semaphore */
    counter = DMA_TIMEOUT_LOOPS;
    while ( DMA_GetFlagStatus( i2c_mapping[device->port].tx_dma_stream, DMA_FLAG_TC(i2c_mapping[device->port].tx_dma_stream_id) ) == RESET )
    {
      --counter;
      if ( counter == 0 )
      {
        return kGeneralErr;
      }
    }
    
    /* Disable DMA and channel */
    I2C_DMACmd( i2c_mapping[device->port].i2c, DISABLE );
    DMA_Cmd( i2c_mapping[device->port].tx_dma_stream, DISABLE );
  }
  else
  {
    /* Enable dma channel for I2C */
    I2C_DMACmd( i2c_mapping[device->port].i2c, ENABLE );
    
    /* RX DMA configuration */
    DMA_DeInit( i2c_mapping[device->port].rx_dma_stream );
    
    /* Configure the DMA Rx Stream with the buffer address and the buffer size */
    i2c_dma_init.DMA_Memory0BaseAddr = (uint32_t)message->rx_buffer;
    i2c_dma_init.DMA_DIR             = DMA_DIR_PeripheralToMemory;
    i2c_dma_init.DMA_BufferSize      = (uint32_t)message->rx_length;
    DMA_Init(i2c_mapping[device->port].rx_dma_stream, &i2c_dma_init);
    
    /* Enable DMA channel */
    DMA_Cmd( i2c_mapping[device->port].rx_dma_stream, ENABLE );
    
    /* wait until transfer is completed */
    counter = DMA_TIMEOUT_LOOPS;
    while ( DMA_GetFlagStatus( i2c_mapping[device->port].rx_dma_stream, DMA_FLAG_TC(i2c_mapping[device->port].rx_dma_stream_id) ) == RESET )
    {
      --counter;
      if ( counter == 0 )
      {
        return kGeneralErr;
      }
    }
    
    /* disable DMA and channel */
    I2C_DMACmd( i2c_mapping[device->port].i2c, DISABLE );
    DMA_Cmd( i2c_mapping[device->port].rx_dma_stream, DISABLE );
  }
  
  return kNoErr;
}

static OSStatus i2c_dma_transfer( mico_i2c_device_t* device, mico_i2c_message_t* message )
{
  OSStatus       result;
  uint32_t       counter;
  int            i = 0;
  
  if ( message->combined == true )
  {
    /* combined transaction case, please refer to Philips I2C document to have an understanding of a combined fragment */
    
    /* some chips( authentication and security related chips ) has to be addressed several times before they acknowledge their address */
    for ( i = 0; i < message->retries; i++ )
    {
      /* generate a start condition and address a device in write mode */
      I2C_GenerateSTART( i2c_mapping[device->port].i2c, ENABLE );
      
      /* wait till start condition is generated and the bus becomes free */
      result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_MODE_SELECT, I2C_FLAG_CHECK_TIMEOUT );
      if ( result != kNoErr )
      {
        return kTimeoutErr;
      }
      
      if ( device->address_width == I2C_ADDRESS_WIDTH_7BIT )
      {
        /* send the address and R/W bit set to write of the requested device, wait for an acknowledge */
        I2C_Send7bitAddress( i2c_mapping[device->port].i2c, (uint8_t) ( device->address << 1 ), I2C_Direction_Transmitter );
        
        /* wait till address gets sent and the direction bit is sent and */
        result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
        if ( result == kNoErr )
        {
          break;
        }
      }
      else
      {
        //TODO
        /* send 10 bits of the address and wait for an acknowledge */
      }
    }
    
    if ( i == message->retries )
    {
      return kTimeoutErr;
    }
    
    /* configure dma tx channel for i2c */
    i2c_dma_config_and_execute( device, message, true );
    
    /* wait till the byte is actually sent from the i2c peripheral */
    counter = 1000;
    while ( I2C_GetFlagStatus( i2c_mapping[device->port].i2c, I2C_FLAG_BTF ) == RESET )
    {
      --counter;
      if ( counter == 0 )
      {
        return kGeneralErr;
      }
    }
    
    /* generate start condition again and address a device in read mode */
    /* some chips( authentication and security related chips ) has to be addressed several times before they acknowledge their address */
    for ( i = 0; i < message->retries; i++ )
    {
      /* generate a start condition */
      I2C_GenerateSTART( i2c_mapping[device->port].i2c, ENABLE );
      
      /* wait till start condition is generated and the bus becomes free */
      result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_MODE_SELECT, I2C_FLAG_CHECK_TIMEOUT );
      if ( result != kNoErr )
      {
        return kTimeoutErr;
      }
      
      if ( device->address_width == I2C_ADDRESS_WIDTH_7BIT )
      {
        /* send the address and R/W bit set to write of the requested device, wait for an acknowledge */
        I2C_Send7bitAddress( i2c_mapping[device->port].i2c, (uint8_t) ( device->address << 1 ), I2C_Direction_Receiver );
        
        /* wait till address gets sent and the direction bit is sent and */
        result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
        if ( result == kNoErr )
        {
          break;
        }
      }
      else
      {
        //TODO
        /* send 10 bits of the address and wait for an acknowledge */
      }
    }
    
    if ( i == message->retries )
    {
      return kTimeoutErr;
    }
    
    /* receive data from the slave device */
    if ( message->rx_length == 1 )
    {
      /* disable acknowledgement before we start receiving bytes, this is a single byte transmission */
      I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, DISABLE );
    }
    else
    {
      /* enable acknowledgement before we start receiving bytes, this is a single byte transmission */
      I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, ENABLE );
    }
    
    /* start dma which will read bytes */
    i2c_dma_config_and_execute( device, message, false );
    /* maybe we will have to wait on the BTF flag!!! */
  }
  else
  {
    
    /* read or write transaction */
    
    /* some chips( authentication and security related chips ) has to be addressed several times before they acknowledge their address */
    for ( i = 0; i < message->retries; i++ )
    {
      /* generate a start condition */
      I2C_GenerateSTART( i2c_mapping[device->port].i2c, ENABLE );
      
      /* wait till start condition is generated and the bus becomes free */
      result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_MODE_SELECT, I2C_FLAG_CHECK_TIMEOUT );
      if ( result != kNoErr )
      {
        return kTimeoutErr;
      }
      
      if ( device->address_width == I2C_ADDRESS_WIDTH_7BIT )
      {
        /* send the address of the requested device, wait for an acknowledge */
        I2C_Send7bitAddress( i2c_mapping[device->port].i2c, (uint8_t) ( device->address << 1 ), ( ( message->tx_buffer ) ? I2C_Direction_Transmitter : I2C_Direction_Receiver ) );
        
        /* wait till address gets sent and the direction bit is sent */
        if ( message->tx_buffer )
        {
          result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
        }
        else
        {
          result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
        }
        
        if ( result == kNoErr )
        {
          break;
        }
      }
      else
      {
        //TODO
        /* send 10 bits of the address and wait for an acknowledge */
      }
    }
    if ( i == message->retries )
    {
      return kTimeoutErr;
    }
    
    if ( message->tx_buffer )
    {
      /* write transaction */
      /* configure dma tx channel for i2c */
      i2c_dma_config_and_execute( device, message, true );
      
      /* wait till the byte is actually sent from the i2c peripheral */
      counter = 1000;
      while ( I2C_GetFlagStatus( i2c_mapping[device->port].i2c, I2C_FLAG_BTF ) == RESET )
      {
        --counter;
        if ( counter == 0 )
        {
          return kGeneralErr;
        }
      }
    }
    else
    {
      /* read transaction */
      if ( message->rx_length == 1 )
      {
        /* disable acknowledgement before we are going to receive a single byte */
        I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, DISABLE );
      }
      else
      {
        /* enable acknowledgement before we start receiving multiple bytes */
        I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, ENABLE );
      }
      
      /* start dma which will read bytes */
      i2c_dma_config_and_execute( device, message, false );
      
      /* wait til the last byte is received */
      counter = 1000;
      while ( I2C_GetFlagStatus( i2c_mapping[device->port].i2c, I2C_FLAG_BTF ) == RESET )
      {
        --counter;
        if ( counter == 0 )
        {
          return kGeneralErr;
        }
      }
    }
  }
  
  /* generate a stop condition */
  I2C_GenerateSTOP( i2c_mapping[device->port].i2c, ENABLE );
  return kNoErr;
}
#else

static OSStatus i2c_transfer_message_no_dma( mico_i2c_device_t* device, mico_i2c_message_t* message )
{
  OSStatus       result;
  int            i = 0;
  
  if ( message->combined == true )
  {
    const char* tmp_ptr;
    uint8_t*    tmp_rd_ptr;
    
    /* combined transaction case, please refer to Philips I2C document to have an understanding of a combined fragment */
    
    /* some chips( authentication and security related chips ) has to be addressed several times before they acknowledge their address */
    for ( i = 0; i < message->retries; i++ )
    {
      /* generate a start condition and address a device in write mode */
      I2C_GenerateSTART( i2c_mapping[device->port].i2c, ENABLE );
      
      /* wait till start condition is generated and the bus becomes free */
      result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_MODE_SELECT, I2C_FLAG_CHECK_TIMEOUT );
      if ( result != kNoErr )
      {
        return kTimeoutErr;
      }
      
      if ( device->address_width == I2C_ADDRESS_WIDTH_7BIT )
      {
        /* send the address and R/W bit set to write of the requested device, wait for an acknowledge */
        I2C_Send7bitAddress( i2c_mapping[device->port].i2c, (uint8_t) ( device->address << 1 ), I2C_Direction_Transmitter );
        
        /* wait till address gets sent and the direction bit is sent and */
        result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
        if ( result == kNoErr )
        {
          break;
        }
      }
      else
      {
        //TODO
        /* send 10 bits of the address and wait for an acknowledge */
      }
    }
    
    if ( i == message->retries )
    {
      return kTimeoutErr;
    }
    
    tmp_ptr = (const char*) message->tx_buffer;
    
    /* send data to the i2c device */
    for ( i = 0; i < message->tx_length; i++ )
    {
      I2C_SendData( i2c_mapping[device->port].i2c, (uint8_t) tmp_ptr[i] );
      
      /* wait till it actually gets transferred and acknowledged */
      result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED, I2C_FLAG_CHECK_TIMEOUT );
      if ( result != kNoErr )
      {
        return result;
      }
    }
    
    /* generate start condition again and address a device in read mode */
    /* some chips( authentication and security related chips ) has to be addressed several times before they acknowledge their address */
    for ( i = 0; i < message->retries; i++ )
    {
      /* generate a start condition */
      I2C_GenerateSTART( i2c_mapping[device->port].i2c, ENABLE );
      
      /* wait till start condition is generated and the bus becomes free */
      result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_MODE_SELECT, I2C_FLAG_CHECK_TIMEOUT );
      if ( result != kNoErr )
      {
        return kTimeoutErr;
      }
      if ( device->address_width == I2C_ADDRESS_WIDTH_7BIT )
      {
        /* send the address and R/W bit set to write of the requested device, wait for an acknowledge */
        I2C_Send7bitAddress( i2c_mapping[device->port].i2c, (uint8_t) ( device->address << 1 ), I2C_Direction_Receiver );
        
        /* wait till address gets sent and the direction bit is sent and */
        result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
        if ( result != kNoErr )
        {
          /* keep on pinging, if a device doesnt respond */
          continue;
        }
        else
        {
          break;
        }
      }
      else
      {
        //TODO
        /* send 10 bits of the address and wait for an acknowledge */
      }
    }
    if ( i == message->retries )
    {
      return kTimeoutErr;
    }
    
    /* receive data from the slave device */
    if ( message->rx_length == 1 )
    {
      /* disable acknowledgement before we start receiving bytes, this is a single byte transmission */
      I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, DISABLE );
    }
    else
    {
      /* enable acknowledgement before we start receiving bytes, this is a single byte transmission */
      I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, ENABLE );
    }
    tmp_rd_ptr = (uint8_t*) message->rx_buffer;
    /* start reading bytes */
    for ( i = 0; i < message->rx_length; i++ )
    {
      /* wait till something is in the i2c data register */
      result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_BYTE_RECEIVED, I2C_FLAG_CHECK_TIMEOUT );
      if ( result != kNoErr )
      {
        return result;
      }
      
      /* get data */
      tmp_rd_ptr[i] = I2C_ReceiveData( i2c_mapping[device->port].i2c );
      if ( i == ( message->rx_length - 1 ) )
      {
        /* setup NACK for the last byte to be received */
        I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, DISABLE );
      }
      else
      {
        /* setup an acknowledgement beforehand for every byte that is to be received */
        I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, ENABLE );
      }
    }
  }
  else
  {
    
    /* read or write transaction */
    
    /* some chips( authentication and security related chips ) has to be addressed several times before they acknowledge their address */
    for ( i = 0; i < message->retries; i++ )
    {
      /* generate a start condition */
      I2C_GenerateSTART( i2c_mapping[device->port].i2c, ENABLE );
      
      /* wait till start condition is generated and the bus becomes free */
      result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_MODE_SELECT, I2C_FLAG_CHECK_TIMEOUT );
      if ( result != kNoErr )
      {
        return kTimeoutErr;
      }
      if ( device->address_width == I2C_ADDRESS_WIDTH_7BIT )
      {
        /* send the address of the requested device, wait for an acknowledge */
        I2C_Send7bitAddress( i2c_mapping[device->port].i2c, (uint8_t) ( device->address << 1 ), ( ( message->tx_buffer ) ? I2C_Direction_Transmitter : I2C_Direction_Receiver ) );
        
        /* wait till address gets sent and the direction bit is sent */
        if ( message->tx_buffer )
        {
          result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
        }
        else
        {
          result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
        }
        if ( result != kNoErr )
        {
          continue;
        }
        else
        {
          break;
        }
      }
      else
      {
        //TODO
        /* send 10 bits of the address and wait for an acknowledge */
      }
    }
    if ( i == message->retries )
    {
      return kTimeoutErr;
    }
    
    if ( message->tx_buffer )
    {
      /* write transaction */
      
      const char* temp_ptr = (const char*) message->tx_buffer;
      /* send data to the i2c device */
      for ( i = 0; i < message->tx_length; i++ )
      {
        I2C_SendData( i2c_mapping[device->port].i2c, (uint8_t) temp_ptr[i] );
        
        /* wait till it actually gets transferred and acknowledged */
        result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED, I2C_FLAG_CHECK_TIMEOUT );
        if ( result != kNoErr )
        {
          return result;
        }
      }
    }
    else
    {
      /* read transaction */
      
      uint8_t* tmp_ptr = (uint8_t*) message->rx_buffer;
      if ( message->rx_length == 1 )
      {
        /* disable acknowledgement before we are going to receive a single byte */
        I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, DISABLE );
      }
      else
      {
        /* enable acknowledgement before we start receiving multiple bytes */
        I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, ENABLE );
      }
      /* receive data from the i2c device */
      for ( i = 0; i < message->rx_length; i++ )
      {
        /* wait till something is in the i2c data register */
        result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_BYTE_RECEIVED, I2C_FLAG_CHECK_TIMEOUT );
        if ( result != kNoErr )
        {
          return result;
        }
        
        /* get data */
        tmp_ptr[i] = I2C_ReceiveData( i2c_mapping[device->port].i2c );
        if ( i == ( message->rx_length - 1 ) )
        {
          /* setup NACK for the last byte to be received */
          I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, DISABLE );
        }
        else
        {
          /* setup an acknowledgement beforehand for every byte that is to be received */
          I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, ENABLE );
        }
      }
    }
  }
  
  /* generate a stop condition */
  I2C_GenerateSTOP( i2c_mapping[device->port].i2c, ENABLE );
  return kNoErr;
}
#endif

OSStatus MicoI2cBuildTxMessage(mico_i2c_message_t* message, const void* tx_buffer, uint16_t  tx_buffer_length, uint16_t retries)
{
  if( ( message == 0 ) || ( tx_buffer == 0 ) || ( tx_buffer_length == 0 ) )
  {
    return kParamErr;
  }
  memset(message, 0x00, sizeof(mico_i2c_message_t));
  message->tx_buffer = tx_buffer;
  message->combined = false;
  message->retries = retries;
  message->tx_length = tx_buffer_length;
  
  return kNoErr;
}

OSStatus MicoI2cBuildRxMessage(mico_i2c_message_t* message, void* rx_buffer, uint16_t  rx_buffer_length, uint16_t retries)
{
  if( ( message == 0 ) || ( rx_buffer == 0 ) || ( rx_buffer_length == 0 ) )
  {
    return kParamErr;
  }
  memset(message, 0x00, sizeof(mico_i2c_message_t));
  message->rx_buffer = rx_buffer;
  message->combined = false;
  message->retries = retries;
  message->rx_length = rx_buffer_length;
  
  return kNoErr;
}

OSStatus MicoI2cBuildCombinedMessage(mico_i2c_message_t* message, const void* tx_buffer, void* rx_buffer, uint16_t tx_buffer_length, uint16_t rx_buffer_length, uint16_t retries)
{
  if( ( message == 0 ) || ( rx_buffer == 0 ) || ( tx_buffer == 0 ) || ( tx_buffer_length == 0 ) || ( rx_buffer_length == 0 ) )
  {
    return kParamErr;
  }
  memset(message, 0x00, sizeof(mico_i2c_message_t));
  message->rx_buffer = rx_buffer;
  message->tx_buffer = tx_buffer;
  message->combined = true;
  message->retries = retries;
  message->tx_length = tx_buffer_length;
  message->rx_length = rx_buffer_length;
  
  return kNoErr;
}

OSStatus MicoI2cTransfer( mico_i2c_device_t* device, mico_i2c_message_t* messages, uint16_t number_of_messages )
{
  OSStatus result;
  int i = 0;
  
  MicoMcuPowerSaveConfig(false);
  
  for( i=0; i < number_of_messages; i++ )
  {
#ifdef I2C_USE_DMA
    result = i2c_dma_transfer(device, &messages[i]);
    if( result != kNoErr )
    {
      MicoMcuPowerSaveConfig(true);
      return kGeneralErr;
    }
#else
    
    result = i2c_transfer_message_no_dma( device, &messages[i] );
    if( result != kNoErr )
    {
      MicoMcuPowerSaveConfig(true);
      return kGeneralErr;
    }
#endif
  }
  
  MicoMcuPowerSaveConfig(true);
  return kNoErr;
}

OSStatus MicoI2cFinalize( mico_i2c_device_t* device )
{
  MicoMcuPowerSaveConfig(false);
  
  /* Disable I2C peripheral clocks */
  RCC_APB1PeriphClockCmd( i2c_mapping[device->port].peripheral_clock_reg, DISABLE );
  
  /* Disable sda, scl gpio clocks */
  RCC_AHB1PeriphClockCmd( i2c_mapping[device->port].pin_scl->peripheral_clock | i2c_mapping[device->port].pin_sda->peripheral_clock, DISABLE );
  
  /* Disable DMA */
#ifdef I2C_USE_DMA
  
  DMA_DeInit( i2c_mapping[device->port].rx_dma_stream );
  DMA_DeInit( i2c_mapping[device->port].tx_dma_stream );
  RCC_AHB1PeriphClockCmd( i2c_mapping[device->port].tx_dma_peripheral_clock, DISABLE );
#endif
  
  MicoMcuPowerSaveConfig(true);
  
  return kNoErr;
}


