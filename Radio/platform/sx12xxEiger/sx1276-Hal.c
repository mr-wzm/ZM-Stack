/*
 * THE FOLLOWING FIRMWARE IS PROVIDED: (1) "AS IS" WITH NO WARRANTY; AND 
 * (2)TO ENABLE ACCESS TO CODING INFORMATION TO GUIDE AND FACILITATE CUSTOMER.
 * CONSEQUENTLY, SEMTECH SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR
 * CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT
 * OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION
 * CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 * 
 * Copyright (C) SEMTECH S.A.
 */
/*! 
 * \file       sx1276-Hal.c
 * \brief      SX1276 Hardware Abstraction Layer
 *
 * \version    2.0.B2 
 * \date       Nov 21 2012
 * \author     Miguel Luis
 *
 * Last modified by Miguel Luis on Jun 19 2013
 */
#include <stdint.h>
#include <stdbool.h> 

#include "platform.h"

#if defined( USE_SX1276_RADIO )

#include "ioe.h"
#include "gpio.h"
#include "spi.h"
#include "../../sx127x/sx1276-Hal.h"



void SX1276InitIo( void )
{

}

void SX1276SetReset( uint8_t state )
{

    if( state == RADIO_RESET_ON )
    {
        // Set RESET pin to 0
        SET_GPIO_PIN_LOW( Lora_Reset_GPIO_Port, Lora_Reset_Pin );
    }
    else
    {
        // Set RESET pin to 1
        SET_GPIO_PIN_HIGH( Lora_Reset_GPIO_Port, Lora_Reset_Pin );
    }
}

void SX1276Write( uint8_t addr, uint8_t data )
{
    SX1276WriteBuffer( addr, &data, 1 );
}

void SX1276Read( uint8_t addr, uint8_t *data )
{
    SX1276ReadBuffer( addr, data, 1 );
}

void SX1276WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
    SET_GPIO_PIN_LOW( NSS_GPIO_Port, NSS_Pin );

    SpiInOut( addr | 0x80 );
    for( i = 0; i < size; i++ )
    {
        SpiInOut( buffer[i] );
    }

    //NSS = 1;
    SET_GPIO_PIN_HIGH( NSS_GPIO_Port, NSS_Pin );
}

void SX1276ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
    SET_GPIO_PIN_LOW( NSS_GPIO_Port, NSS_Pin );

    SpiInOut( addr & 0x7F );

    for( i = 0; i < size; i++ )
    {
        buffer[i] = SpiInOut( 0 );
    }

    //NSS = 1;
    SET_GPIO_PIN_HIGH( NSS_GPIO_Port, NSS_Pin );
}

void SX1276WriteFifo( uint8_t *buffer, uint8_t size )
{
    SX1276WriteBuffer( 0, buffer, size );
}

void SX1276ReadFifo( uint8_t *buffer, uint8_t size )
{
    SX1276ReadBuffer( 0, buffer, size );
}

inline uint8_t SX1276ReadDio0( void )
{
    return READ_GPIO_PIN(DIO0_GPIO_Port, DIO0_Pin);
}

inline uint8_t SX1276ReadDio1( void )
{
    return READ_GPIO_PIN(DIO1_GPIO_Port, DIO1_Pin);
}

inline uint8_t SX1276ReadDio2( void )
{
    return READ_GPIO_PIN(DIO2_GPIO_Port, DIO2_Pin);
}

inline uint8_t SX1276ReadDio3( void )
{
    return READ_GPIO_PIN(DIO3_GPIO_Port, DIO3_Pin);
}

inline uint8_t SX1276ReadDio4( void )
{
    return READ_GPIO_PIN(DIO4_GPIO_Port, DIO4_Pin);
}

inline uint8_t SX1276ReadDio5( void )
{
    return READ_GPIO_PIN(DIO5_GPIO_Port, DIO5_Pin);
}

inline void SX1276WriteRxTx( uint8_t txEnable )
{
    if( txEnable != 0 )
    {
        SET_GPIO_PIN_HIGH( LORA_TXRX_CTRL_GPIO_Port, LORA_TXRX_CTRL_Pin );
        SET_GPIO_PIN_LOW( LORA_TXRX_POWER_GPIO_Port, LORA_TXRX_POWER_Pin );
    }
    else
    {
        SET_GPIO_PIN_LOW( LORA_TXRX_CTRL_GPIO_Port, LORA_TXRX_CTRL_Pin );
        SET_GPIO_PIN_HIGH( LORA_TXRX_POWER_GPIO_Port, LORA_TXRX_POWER_Pin );
    }
}

#endif // USE_SX1276_RADIO
