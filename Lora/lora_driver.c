/*****************************************************************
* Copyright (C) 2017 60Plus Technology Co.,Ltd.*
******************************************************************
* lora_driver.c
*
* DESCRIPTION:
*     Lora driver
* AUTHOR:
*     Ziming
* CREATED DATE:
*     2018/11/26
* REVISION:
*     v0.1
*
* MODIFICATION HISTORY
* --------------------
* $Log:$
*
*****************************************************************/
/*************************************************************************************************************************
 *                                                       INCLUDES                                                        *
 *************************************************************************************************************************/
#include <string.h>
#include "loraConfig.h"
#include "platform.h"
#include "radio.h"
#include "sx1276-Hal.h"
#include "sx1276.h"
#include "sx1276-LoRaMisc.h"
#include "sx1276-LoRa.h"
#include "OStask.h"
#include "OS_timers.h"
#include "lora_driver.h"
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
#define RSSI_OFFSET_LF                              -164.0
#define RSSI_OFFSET_HF                              -157.0
/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/

/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/

/*************************************************************************************************************************
 *                                                   GLOBAL VARIABLES                                                    *
 *************************************************************************************************************************/
static tRadioDriver             *Radio = NULL;
//radio status
volatile static tRFLRStates     g_radioStatus = RFLR_STATE_IDLE;
//radio rx buffer
static uint8_t                  g_loraRxBuffer[BUFFER_SIZE_MAX];
//callback function struct
static t_radioCallBack          *g_radioCallback = NULL;
//lora snr
static int8_t                   g_rxPacketSnrEstimate = 0;
//lora rssi
static double                   g_rxPacketRssiValue = 0;
//lora rssi offset
static double                   g_rssiOffset = RSSI_OFFSET_LF;
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/

/*************************************************************************************************************************
 *                                                    LOCAL VARIABLES                                                    *
 *************************************************************************************************************************/

/*************************************************************************************************************************
 *                                                 FUNCTION DECLARATIONS                                                 *
 *************************************************************************************************************************/
static E_typeErr        sx1276FullTxFIFO( const uint8_t *a_data, uint8_t a_size );
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/

/*************************************************************************************************************************
 *                                                    LOCAL FUNCTIONS                                                    *
 *************************************************************************************************************************/


/*****************************************************************
* DESCRIPTION: sx1276FullTxFIFO
*     
* INPUTS:
*     a_data : point to buffer that desired to be sended.
*     a_size : size of the desired transmitted data.
* OUTPUTS:
*     Full buffer success or faild.
*     E_typeErr( E_SUCCESS or E_ERR )
* NOTE:
*     Full sx1276 Tx FIFO
*****************************************************************/
static E_typeErr sx1276FullTxFIFO( const uint8_t *a_data, uint8_t a_size )
{
    if( g_radioStatus != RFLR_STATE_IDLE )
    {
        return E_ERR; //Radio is busy!
    }
   
    /* The LoRa FIFO can only be filled in Stand-by Mode. */
    SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY ); 
    
    SX1276LR->RegHopPeriod = 0;
    
    SX1276Write( REG_LR_HOPPERIOD, SX1276LR->RegHopPeriod );
    
    // Initializes the payload size
    SX1276LR->RegPayloadLength = a_size;
    SX1276Write( REG_LR_PAYLOADLENGTH, SX1276LR->RegPayloadLength );
    
    SX1276LR->RegFifoTxBaseAddr = 0x00; // Full buffer used for Tx
    SX1276Write( REG_LR_FIFOTXBASEADDR, SX1276LR->RegFifoTxBaseAddr );
    
    SX1276LR->RegFifoAddrPtr = SX1276LR->RegFifoTxBaseAddr;
    SX1276Write( REG_LR_FIFOADDRPTR, SX1276LR->RegFifoAddrPtr );
    
    // Write payload buffer to LORA modem
    SX1276WriteFifo( (uint8_t *)a_data, SX1276LR->RegPayloadLength );

    return E_SUCCESS;
}

/*****************************************************************
* DESCRIPTION: loraDriverInit
*     
* INPUTS:
*     null
* OUTPUTS:
*     null
* RETURNS:
*     null
* NOTE:
*     Initialize lora driver setting
*****************************************************************/
void loraDriverInit( void )
{
    Radio = RadioDriverInit( );
    
    Radio->Init( );
}


/*****************************************************************
* DESCRIPTION: loraRegisterCallback
*     
* INPUTS:
*     a_radioCallback : 
* OUTPUTS:
*     null
* RETURNS:
*     null
* NOTE:
*     Register callback function
*****************************************************************/
void loraRegisterCallback( t_radioCallBack  *a_radioCallback )
{
    g_radioCallback = a_radioCallback;
}

/*****************************************************************
* DESCRIPTION: getLoraStatus
*     
* INPUTS:
*     null
* OUTPUTS:
*     tRFLRStates
* NOTE:
*     null
*****************************************************************/
tRFLRStates getLoraStatus( void )
{
    return g_radioStatus;
}

/*****************************************************************
* DESCRIPTION: getLoraSnr
*     
* INPUTS:
*     null
* OUTPUTS:
*     snr
* RETURNS:
*     null
* NOTE:
*     null
*****************************************************************/
int8_t getLoraSnr( void )
{
    return g_rxPacketSnrEstimate;
}

/*****************************************************************
* DESCRIPTION: getLoraRssi
*     
* INPUTS:
*     null
* OUTPUTS:
*     lora rssi
* RETURNS:
*     null
* NOTE:
*     null
*****************************************************************/
double getLoraRssi( void )
{
    return g_rxPacketRssiValue;
}

/*****************************************************************
* DESCRIPTION: loRaSetFrequency
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
E_typeErr loRaSetFrequency( uint32_t a_freq )
{
    uint32_t freq;
    
    if( a_freq < LORA_FREQUENCY_MIN || a_freq > LORA_FREQUENCY_MAX )
    {
        return E_ERR;
    }
    loraEnterStandby();
    /* Setting frequency */
    SX1276LoRaSetRFFrequency( a_freq );
    /* check frequency */
    freq = SX1276LoRaGetRFFrequency();
    
    return (E_typeErr)( freq == a_freq );
}

/*****************************************************************
* DESCRIPTION: loRaGetFrequency
*     
* INPUTS:
*     null
* OUTPUTS:
*     frequency
* NOTE:
*     null
*****************************************************************/
uint32_t loRaGetFrequency( void )
{
    return SX1276LoRaGetRFFrequency();
}

/*****************************************************************
* DESCRIPTION: loraEnterStandby
*     
* INPUTS:
*     null
* OUTPUTS:
*     null
* NOTE:
*     null
*****************************************************************/
void loraEnterStandby( void )
{
    g_radioStatus = RFLR_STATE_IDLE;
    
    SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
}

/*****************************************************************
* DESCRIPTION: loraEnterSleep
*     
* INPUTS:
*     null
* OUTPUTS:
*     null
* NOTE:
*     null
*****************************************************************/
void loraEnterSleep( void )
{
    g_radioStatus = RFLR_STATE_IDLE;
    
    SX1276LoRaSetOpMode( RFLR_OPMODE_SLEEP );
}
/*****************************************************************
* DESCRIPTION: loraSendData
*     
* INPUTS:
*     a_data:
*     a_size: 
* OUTPUTS:
*     null
* RETURNS:
*     null
* NOTE:
*     null
*****************************************************************/
E_typeErr loraSendData( uint8_t *a_data, uint8_t a_size )
{
    uint8_t dioMapping;
    
    if( g_radioStatus != RFLR_STATE_IDLE )
    {
        return E_ERR; //Radio is busy!
    }
    /* The LoRa FIFO can only be filled in Stand-by Mode. */
    SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY ); 
    /* Enable interrupt = TxDone */
    SX1276LR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                RFLR_IRQFLAGS_RXDONE |
                                RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                RFLR_IRQFLAGS_VALIDHEADER |
                                //RFLR_IRQFLAGS_TXDONE |
                                RFLR_IRQFLAGS_CADDONE |
                                RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                RFLR_IRQFLAGS_CADDETECTED;
    
    SX1276Write( REG_LR_IRQFLAGSMASK, SX1276LR->RegIrqFlagsMask );
    sx1276FullTxFIFO(a_data, a_size);        
    /* Read dio mapping */
    SX1276Read(REG_LR_DIOMAPPING1, &dioMapping);           
    /* DIO0<01> = TxDone */
    SX1276Write( REG_LR_DIOMAPPING1, 
               ( dioMapping & RFLR_DIOMAPPING1_DIO0_MASK ) |
                 RFLR_DIOMAPPING1_DIO0_01 );
    /* Enter tx mode */
    SX1276LoRaSetOpMode( RFLR_OPMODE_TRANSMITTER );
    /* Set radio status to tx */
    g_radioStatus = RFLR_STATE_TX_RUNNING;
    /* Open send timer */
    if( startSingleTimer( LORA_TIMEOUT_EVENT, LORA_TIMEOUT_VALUE+100, NULL ) == E_ERR )
    {
        loraEnterStandby();
        return E_ERR;
    }
    
    return E_SUCCESS;
}

/*****************************************************************
* DESCRIPTION: loraReceiveData
*     
* INPUTS:
*     null
* OUTPUTS:
*     null
* RETURNS:
*     E_typeErr
* NOTE:
*     Lora receive data
*****************************************************************/
E_typeErr loraReceiveData( void )
{
    uint8_t dioMapping;
    
    if( g_radioStatus != RFLR_STATE_IDLE )
    {
        return E_ERR;   //Radio is busy!
    }
    
    if( LoRaSettings.RFFrequency < 860000000 )  // LF
    {
        g_rssiOffset = RSSI_OFFSET_LF;
    }
    else
    {
        g_rssiOffset = RSSI_OFFSET_HF;
    }
    
    SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
    /* Enable interrupt = RxDone + PayloadCrcError */
    SX1276LR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                //RFLR_IRQFLAGS_RXDONE |
                                //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                RFLR_IRQFLAGS_VALIDHEADER |
                                RFLR_IRQFLAGS_TXDONE |
                                RFLR_IRQFLAGS_CADDONE |
                                //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                RFLR_IRQFLAGS_CADDETECTED;
    SX1276Write( REG_LR_IRQFLAGSMASK, SX1276LR->RegIrqFlagsMask );
    
    if( LoRaSettings.FreqHopOn == true )
    {
        SX1276LR->RegHopPeriod = LoRaSettings.HopPeriod;
        
        SX1276Read( REG_LR_HOPCHANNEL, &SX1276LR->RegHopChannel );
        SX1276LoRaSetRFFrequency( HoppingFrequencies[SX1276LR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK] );
    }
    else
    {
        SX1276LR->RegHopPeriod = 255;
    }
    
    SX1276Write( REG_LR_HOPPERIOD, SX1276LR->RegHopPeriod );
     /* Read dio mapping */
    SX1276Read(REG_LR_DIOMAPPING1, &dioMapping);
    /* DIO0<00> = RxDone */
    SX1276Write( REG_LR_DIOMAPPING1, 
               ( dioMapping & RFLR_DIOMAPPING1_DIO0_MASK) | 
                 RFLR_DIOMAPPING1_DIO0_00 );
    
    if( LoRaSettings.RxSingleOn == true ) // Rx single mode
    {
        /* Enter rx single mode */
        SX1276LoRaSetOpMode( RFLR_OPMODE_RECEIVER_SINGLE );
        
       // if( startSingleTimer( LORA_TIMEOUT_EVENT, LORA_TIMEOUT_VALUE, NULL ) == E_ERR )
       // {
       //     loraEnterStandby();
       //     return E_ERR;
       // }
    }
    else // Rx continuous mode
    {
        SX1276LR->RegFifoAddrPtr = SX1276LR->RegFifoRxBaseAddr;
        SX1276Write( REG_LR_FIFOADDRPTR, SX1276LR->RegFifoAddrPtr );
        /* Enter rx continuous mode */
        SX1276LoRaSetOpMode( RFLR_OPMODE_RECEIVER );
    }
    
    memset( g_loraRxBuffer, 0, ( size_t )BUFFER_SIZE_MAX );
    /* Set radio status to rx */
    g_radioStatus = RFLR_STATE_RX_RUNNING;
    /* Open receive timer */
    if( startSingleTimer( LORA_TIMEOUT_EVENT, LORA_TIMEOUT_VALUE, NULL ) == E_ERR )
    {
        loraEnterStandby();
        return E_ERR;
    }
    
    return E_SUCCESS;
}



/*****************************************************************
* DESCRIPTION: loraEnterCAD
*     
* INPUTS:
*     null
* OUTPUTS:
*     null
* RETURNS:
*     null
* NOTE:
*     Enter CAD mode
*****************************************************************/
E_typeErr loraEnterCAD( void )
{
    uint8_t dioMapping;
    
    if( g_radioStatus != RFLR_STATE_IDLE )
    {
        return E_ERR;   //Radio is busy!
    }
    SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
    /* Enable interrupt */
    SX1276LR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                RFLR_IRQFLAGS_RXDONE |
                                RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                RFLR_IRQFLAGS_VALIDHEADER |
                                RFLR_IRQFLAGS_TXDONE |
                                //RFLR_IRQFLAGS_CADDONE |
                                RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL; // |
                                //RFLR_IRQFLAGS_CADDETECTED;
    SX1276Write( REG_LR_IRQFLAGSMASK, SX1276LR->RegIrqFlagsMask );
    /* Read dio mapping */
    SX1276Read(REG_LR_DIOMAPPING1, &dioMapping);
    /* DIO0<10> = CadDone */
    SX1276Write( REG_LR_DIOMAPPING1, 
               ( dioMapping & RFLR_DIOMAPPING1_DIO0_MASK) | 
                 RFLR_DIOMAPPING1_DIO0_10 );
    /* Enter cad mode */
    SX1276LoRaSetOpMode( RFLR_OPMODE_CAD );
    /* Set radio status to cad */
    g_radioStatus = RFLR_STATE_CAD_RUNNING;
    /* Open cad scan timer */
    if( startSingleTimer( LORA_TIMEOUT_EVENT, LORA_TIMEOUT_VALUE, NULL ) == E_ERR )
    {
        loraEnterStandby();
        return E_ERR;
    }
    
    return E_SUCCESS;
}


/*****************************************************************
* DESCRIPTION: loraDoneHandler
*     
* INPUTS:
*     null
* OUTPUTS:
*     null
* RETURNS:
*     null
* NOTE:
*     Dio0 interrupt
*****************************************************************/
void loraDoneHandler( void )
{
    /* Tx Done */
    if( RFLR_STATE_TX_RUNNING == g_radioStatus )
    {
        // Clear Irq
        SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE  );
        
        g_radioStatus = RFLR_STATE_IDLE;
        // optimize the power consumption by switching off the transmitter as soon as the packet has been sent
        SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
        /* Callback function */
        if( g_radioCallback && g_radioCallback->TxDone )
        {
            g_radioCallback->TxDone();
        }
    }
    /* Rx Done */
    else if( RFLR_STATE_RX_RUNNING == g_radioStatus )
    {
        if( LoRaSettings.FreqHopOn == true )
        {
            SX1276Read( REG_LR_HOPCHANNEL, &SX1276LR->RegHopChannel );
            SX1276LoRaSetRFFrequency( HoppingFrequencies[SX1276LR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK] );
        }
        // Clear Irq
        SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE  );
        /* Check crc */
        SX1276Read( REG_LR_IRQFLAGS, &SX1276LR->RegIrqFlags );
        if( ( SX1276LR->RegIrqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR ) == RFLR_IRQFLAGS_PAYLOADCRCERROR )
        {
            // Clear Irq
            SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR  );
            /* Callback function */
            if( g_radioCallback && g_radioCallback->RxError ) // Rx crc error
            {
                g_radioCallback->RxError();
            }
        }
        else
        {
            uint8_t rxSnrEstimate;
            /* SNR estimation of the final received packet */
            SX1276Read( REG_LR_PKTSNRVALUE, &rxSnrEstimate );
            if( rxSnrEstimate & 0x80 ) // The SNR sign bit is 1
            {
                // Invert and divide by 4
                g_rxPacketSnrEstimate = ( ( ~rxSnrEstimate + 1 ) & 0xFF ) >> 2;
                g_rxPacketSnrEstimate = -g_rxPacketSnrEstimate;
            }
            else
            {
                // Divide by 4
                g_rxPacketSnrEstimate = ( rxSnrEstimate & 0xFF ) >> 2;
            }
             /* Get RSSI */
            SX1276Read( REG_LR_PKTRSSIVALUE, &SX1276LR->RegPktRssiValue );
            if( g_rxPacketSnrEstimate < 0 )
            {
                g_rxPacketRssiValue = g_rssiOffset + ( ( double )SX1276LR->RegPktRssiValue ) + g_rxPacketSnrEstimate;
            }
            else
            {
                g_rxPacketRssiValue = g_rssiOffset + ( 1.0666 * ( ( double )SX1276LR->RegPktRssiValue ) );
            }
            /* Get data */
            if( LoRaSettings.RxSingleOn == true ) // Rx single mode
            {
                SX1276LR->RegFifoAddrPtr = SX1276LR->RegFifoRxBaseAddr;
                SX1276Write( REG_LR_FIFOADDRPTR, SX1276LR->RegFifoAddrPtr );
                
                SX1276Read( REG_LR_NBRXBYTES, &SX1276LR->RegNbRxBytes );
                SX1276ReadFifo( g_loraRxBuffer, SX1276LR->RegNbRxBytes );
                
                g_radioStatus = RFLR_STATE_IDLE;
                SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
            }
            else // Rx continuous mode
            {
                SX1276Read( REG_LR_FIFORXCURRENTADDR, &SX1276LR->RegFifoRxCurrentAddr );
                
                SX1276Read( REG_LR_NBRXBYTES, &SX1276LR->RegNbRxBytes );
                SX1276LR->RegFifoAddrPtr = SX1276LR->RegFifoRxCurrentAddr;
                SX1276Write( REG_LR_FIFOADDRPTR, SX1276LR->RegFifoAddrPtr );
                SX1276ReadFifo( g_loraRxBuffer, SX1276LR->RegNbRxBytes );
            }
            /* Callback function */
            if( g_radioCallback && g_radioCallback->RxDone )
            {
                g_radioCallback->RxDone( g_loraRxBuffer, SX1276LR->RegNbRxBytes);
            }
        }
    }
    /* CAD Done */
    else if( RFLR_STATE_CAD_RUNNING == g_radioStatus )
    {
        uint8_t cadStatus;
        // Clear Irq
        SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDONE  );
        SX1276Read(REG_LR_IRQFLAGS, &cadStatus);
        // CAD detected, we have a LoRa preamble
        if( (cadStatus & RFLR_IRQFLAGS_CADDETECTED_MASK) == RFLR_IRQFLAGS_CADDETECTED )
        {
            // Clear Irq
            SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDETECTED  );
            cadStatus = RF_CHANNEL_ACTIVITY_DETECTED;
        }
        else //Empty
        {
            cadStatus = RF_CHANNEL_EMPTY;
        }
        g_radioStatus = RFLR_STATE_IDLE;
        //Enter standby
        SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
        
        if( g_radioCallback && g_radioCallback->CadDone )
        {
            g_radioCallback->CadDone( cadStatus );
        }
    }
}

/*****************************************************************
* DESCRIPTION: loraTimeoutHandler
*     
* INPUTS:
*     null
* OUTPUTS:
*     nul
* NOTE:
*     null
*****************************************************************/
void loraTimeoutHandler( void )
{
    /* Tx timeout */
    if( RFLR_STATE_TX_RUNNING == g_radioStatus )
    {
        g_radioStatus = RFLR_STATE_IDLE;
        //Enter standby
        SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
        if( g_radioCallback && g_radioCallback->TxTimeout )
        {
            g_radioCallback->TxTimeout();
        }
    }
    /* Rx timeout */
    else if( RFLR_STATE_RX_RUNNING == g_radioStatus )
    {
         if( LoRaSettings.RxSingleOn == true ) // Rx single mode
         {
             g_radioStatus = RFLR_STATE_IDLE;
             //Enter standby
             SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
         }
         if( g_radioCallback && g_radioCallback->RxTimeout )
         {
             g_radioCallback->RxTimeout();
         }
    }
    /* CAD timeout */
    else if( RFLR_STATE_CAD_RUNNING == g_radioStatus )
    {
        g_radioStatus = RFLR_STATE_IDLE;
        //Enter standby
        SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
        if( g_radioCallback && g_radioCallback->CadTimeout )
        {
            g_radioCallback->CadTimeout();
        }
    }
}

/****************************************************** END OF FILE ******************************************************/
