/*****************************************************************
* Copyright (C) 2017 60Plus Technology Co.,Ltd.*
******************************************************************
* lora.c
*
* DESCRIPTION:
*     Lora task
* AUTHOR:
*     Ziming
* CREATED DATE:
*     2018/12/20
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
#include "loraConfig.h"
#include <stdlib.h>
#include <string.h>
#include "iwdg.h"
#include "lora_driver.h"
#include "radio.h"
#include "transmit.h"
#include "attribute.h"
#include "zigbee.h"
#include "OS_timers.h"
#include "Network.h"
#include "gpio.h"
#include "lora.h"
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
/* Panid build time(ms) */
#define PAN_ID_BUILD_TIME                        5000
/* Join request packet send interval time(ms) */
#define JOIN_REQUEST_INTERVAL_TIME               500
/* Beacon packet send interval time(ms) */
#define BEACON_INTERCAL_TIME                     500
/* wait ack max time(ms) */
#define TRANSMIT_ACK_WAIT_TIME                   200
/* Cad poll time(ms) */
#define CAD_POLL_TIME                            1000
/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                   GLOBAL VARIABLES                                                    *
 *************************************************************************************************************************/
static bool             transFlag = false;
static TaskHandle_t     notifyTask = NULL;
static E_transmitType   transmitType = NO_TRANS;
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/

/*
 * Lora porcess task handle.
 */
TaskHandle_t                loraTaskHandle;

/*************************************************************************************************************************
 *                                                    LOCAL VARIABLES                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                 FUNCTION DECLARATIONS                                                 *
 *************************************************************************************************************************/
static void     loraReceiveDone( uint8_t *a_data, uint16_t a_size );
static void     loraReceiveError( void );
static void     loraReceiveTimeout( void );
static void     loraSendDone( void );
static void     loraSendTimeout( void );
static void     loraCadDone( uint8_t a_detected );
static void     loraCadTimeout( void );
/*
 * Full callback function.
 */
static t_radioCallBack loraCallBack =
{
    .RxDone     = loraReceiveDone,
    .RxError    = loraReceiveError,
    .RxTimeout  = loraReceiveTimeout,
    .TxDone     = loraSendDone,
    .TxTimeout  = loraSendTimeout,
    .CadDone    = loraCadDone,
    .CadTimeout = loraCadTimeout,
};
static void     networkBuildSuccess( void );
static void     getChannelStarus( void );
static void     transmitNoAck( void );
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/

/*************************************************************************************************************************
 *                                                    LOCAL FUNCTIONS                                                    *
 *************************************************************************************************************************/

/*****************************************************************
* DESCRIPTION: loraInit
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     Initialize lora
*****************************************************************/
void loraInit( void )
{
    /* Open lora mcu */
    SET_GPIO_PIN_HIGH( Lora_Reset_GPIO_Port, Lora_Reset_Pin );
    /* Initialize lora config */
    loraDriverInit();
    /* Register call back */
    loraRegisterCallback( &loraCallBack );
    
}

/*****************************************************************
* DESCRIPTION: loraProcess
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void loraProcess( void *parm )
{
    uint32_t eventId;
    
#if configUSE_TICKLESS_IDLE == 1
    //loraEnterSleep();
    loraEnterLowPower();
#else
    if(nwkAttribute.m_nwkStatus == true)
    {
        loraReceiveData();
    }
#endif
        
    while(1)
    {
        eventId = 0;
        /* wait task notify */
        xTaskNotifyWait( (uint32_t)0, ULONG_MAX, &eventId, portMAX_DELAY );
        /* Transmit data */
        if( (eventId & LORA_NOTIFY_TRANSMIT_START) == LORA_NOTIFY_TRANSMIT_START )
        {
            if( !transFlag && (getTransmitHeadPacket() || getRetransmitCurrentPacket()) )
            {
                transFlag = true;
                macPIB.BE = MAC_MIN_BE;
                macPIB.NB = MAC_MIN_NB;
                macPIB.CW = MAC_VALUE_CW;
            }
            startSingleTimer( TRANSMIT_NB_TIME_EVENT, getAvoidtime(), getChannelStarus );

            eventId ^= (uint32_t)LORA_NOTIFY_TRANSMIT_START;
        }
        /* Transmit done */
        else if( (eventId & LORA_NOTIFY_TRANSMIT_DONE) == LORA_NOTIFY_TRANSMIT_DONE )
        {
            /* Stop timeout timer */
            stopTimer( LORA_TIMEOUT_EVENT, SINGLE_TIMER );
            loraDoneHandler();
            
            eventId ^= (uint32_t)LORA_NOTIFY_TRANSMIT_DONE;
        }
        /* Transmit timeout */
        else if( (eventId & LORA_NOTIFY_TRANSMIT_TIMEOUT) == LORA_NOTIFY_TRANSMIT_TIMEOUT )
        {
            loraTimeoutHandler();
            
            eventId ^= (uint32_t)LORA_NOTIFY_TRANSMIT_TIMEOUT;
        }
        /* transmit command message */
        else if( (eventId & LORA_NOTIFY_TRANSMIT_COMMAND) == LORA_NOTIFY_TRANSMIT_COMMAND )
        {
            startSingleTimer( TRANSMIT_NB_TIME_EVENT, getAvoidtime(), getChannelStarus );
            
            eventId ^= (uint32_t)LORA_NOTIFY_TRANSMIT_COMMAND;
        }
        /* Join request */
        else if( (eventId & LORA_NOTIFY_TRANSMIT_JOIN_REQUEST) == LORA_NOTIFY_TRANSMIT_JOIN_REQUEST )
        { 
            /* Set the network status to join request */
            setNetworkStatus( NETWORK_JOIN_REQUEST );
            startReloadTimer( NETWORK_JOIN_NWK_EVENT, JOIN_REQUEST_INTERVAL_TIME, transmitJoinRequest );
            
            eventId ^= (uint32_t)LORA_NOTIFY_TRANSMIT_JOIN_REQUEST;
        }
        /* Transmit beacon */
        else if( (eventId & LORA_NOTIFY_TRANSMIT_BEACON) == LORA_NOTIFY_TRANSMIT_BEACON )
        {
            startReloadTimer( NETWORK_BEACON_EVENT, BEACON_INTERCAL_TIME, transmitBeacon );
            
            eventId ^= (uint32_t)LORA_NOTIFY_TRANSMIT_BEACON;
        }
#ifdef SELF_ORGANIZING_NETWORK
        /* Set pan network */
        else if( (eventId & LORA_NOTIFY_SET_PANID) == LORA_NOTIFY_SET_PANID )
        {
            setNetworkStatus( NETWORK_BUILD );
            nwkAttribute.m_panId = (uint16_t)generatedRand();
            startSingleTimer( NETWORK_BUILD_EVENT, PAN_ID_BUILD_TIME, networkBuildSuccess );
            eventId ^= (uint32_t)LORA_NOTIFY_SET_PANID;
        }
#endif
        /* Returns the untreated event */
        if( eventId )
        {
            xTaskNotify( loraTaskHandle, eventId, eSetBits );
        }
        taskYIELD();
    }
}

/*****************************************************************
* DESCRIPTION: loraEnterLowPower
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void loraEnterLowPower( void )
{
#if configUSE_TICKLESS_IDLE == 1
    t_timerActiveList timerList = whichTimerIsActive();
    /* No timer task is working */
    if( timerList.m_activeNum < 3 && timerList.m_activeNum )
    {
        for( int count = timerList.m_activeNum; count > 0; count-- )
        {
            if( timerList.m_activeList[count] == SYSTEM_FEED_DOG_EVENT )
            {
                continue;
            }
            stopTimer( (E_timerEvent)timerList.m_activeList[count], ALL_TYPE_TIMER );
        }
        loraEnterSleep();
        if( nwkAttribute.m_nwkStatus )
        {
            systemFeedDog();
            resetTimer( SYSTEM_FEED_DOG_EVENT, RELOAD_TIMER );  //Sync feed dog timer.
            startSingleTimer( LOW_POWER_CAD_POLL_EVENT, CAD_POLL_TIME, getChannelStarus );
        }
    }
    vPortFree(timerList.m_activeList);
#endif
}

/*****************************************************************
* DESCRIPTION: setTransmitType
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void setTransmitType( E_transmitType a_type )
{
    transmitType = a_type;
}


/*****************************************************************
* DESCRIPTION: detectionChannel
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void detectionChannel( TaskHandle_t a_notifyTask )
{
    notifyTask = a_notifyTask;
    getChannelStarus();
}

/*****************************************************************
* DESCRIPTION: setPanId
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void networkBuildSuccess( void )
{
    setNetworkStatus( NETWORK_COOR );
    nwkAttribute.m_nwkStatus = true;
    eepSysNwkAttSave();
}
/*****************************************************************
* DESCRIPTION: getChannelStarus
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void getChannelStarus( void )
{
    if( getLoraStatus() == RFLR_STATE_RX_RUNNING )
    {
        loraEnterStandby();
    }
    if( loraEnterCAD() != E_SUCCESS )
    {
        startSingleTimer( TRANSMIT_NB_TIME_EVENT, 20, getChannelStarus );
    }
}

/*****************************************************************
* DESCRIPTION: transmitNoAck
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void transmitNoAck( void )
{
    if( transmitType == T_RETRANSMIT )
    {
        if( ++getRetransmitCurrentPacket()->m_retransmit < MAC_RETRANSMIT_NUM )
        {
            transFlag = false;
            xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_START, eSetBits );
        }
        else
        {
            retransmitFreePacket(getRetransmitCurrentPacket());
            return;
        }
        transmitRetransmit(transmitType);
    }
#ifdef DEVICE_TYPE_COOR
    if( LORA_FREQUENCY_MAX == loraGetFrequency() )
    {
        loraSetFrequency( LORA_FREQUENCY_MIN + LORA_FREQUENCY_STEP*nwkAttribute.m_channelNum );
        loraSetPreambleLength(LORA_PREAMBLE_LENGTH);
        checkTransmitQueue();
    }
#endif
}
/*****************************************************************
* DESCRIPTION: loraReceiveDone
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void loraReceiveDone( uint8_t *a_data, uint16_t a_size )
{
#ifdef SELF_ORGANIZING_NETWORK
    if( getNetworkStatus() == NETWORK_BUILD && 
        nwkAttribute.m_panId == ((t_transmitPacket *)a_data)->m_panId )
    {
        xTaskNotify( loraTaskHandle, LORA_NOTIFY_SET_PANID, eSetBits );
    }
#endif
    
    switch( transmitRx( (t_transmitPacket *)a_data ) )
    {
    case DATA_ORDER:
        zigbeeUartSend( ((t_transmitPacket *)a_data)->m_data, ((t_transmitPacket *)a_data)->m_size );
        break;
    default:
        break;
    }
    
    checkTransmitQueue();
#if configUSE_TICKLESS_IDLE == 0
    startSingleTimer( LORA_TIMEOUT_EVENT, LORA_TIMEOUT_VALUE, NULL );
#endif
}
/*****************************************************************
* DESCRIPTION: loraReceiveError
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void loraReceiveError( void )
{
    //startSingleTimer( TRANSMIT_NB_TIME_EVENT, getAvoidtime(), getChannelStarus );
}
/*****************************************************************
* DESCRIPTION: loraReceiveTimeout
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void loraReceiveTimeout( void )
{
    //TOGGLE_GPIO_PIN(LED_GPIO_Port, LED_Pin);
    checkTransmitQueue();
#if configUSE_TICKLESS_IDLE == 0
    startSingleTimer( LORA_TIMEOUT_EVENT, LORA_TIMEOUT_VALUE, NULL );
#endif
}
/*****************************************************************
* DESCRIPTION: loraSendDone
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void loraSendDone( void )
{
    static uint8_t broadcastCount = 0;
   // if( transFlag && macPIB.CW == 0 )
    if( transmitType != NO_TRANS )
    {
        transFlag = false;
        if( transmitType == T_TRANSMIT )
        {
            if( getTransmitHeadPacket()->m_transmitPacket.m_dstAddr.addrMode != broadcastAddr )
            {
                if( ++getTransmitHeadPacket()->m_retransmit < PRE_TRANSMIT_NUM )
                {
                    transmitType = transmitSendData();
                }
                else
                {
                    transmitRetransmit(T_TRANSMIT);
                    if( getTransmitHeadPacket() )
                    {
                        transmitType = transmitSendData();
                    }
                    else
                    {
                        startSingleTimer( TRANSMIT_WAIT_ACK_EVENT, TRANSMIT_ACK_WAIT_TIME, transmitNoAck );
                    }
                }
            }
            else
            {
                /* Num of broadcast times is BROADCAST_MAX_NUM */
                if( ++broadcastCount < BROADCAST_MAX_NUM )
                {
                    xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_START, eSetBits );
                }
                else
                {
                    broadcastCount = 0;
                    transmitFreeHeadData();
                }
            }
        }
        else if( transmitType == T_RETRANSMIT )
        {
            startSingleTimer( TRANSMIT_WAIT_ACK_EVENT, TRANSMIT_ACK_WAIT_TIME, transmitNoAck );
        }
    }
    loraReceiveData();
}
/*****************************************************************
* DESCRIPTION: loraSendTimeout
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void loraSendTimeout( void )
{
    if( transFlag && macPIB.CW == 0 )
    {
        transFlag = false;
        xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_START, eSetBits );
    }
    else
    {
        xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_COMMAND, eSetBits );
    }
}
/*****************************************************************
* DESCRIPTION: loraCadDone
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void loraCadDone( uint8_t a_detected )
{
    TOGGLE_GPIO_PIN(LED_GPIO_Port, LED_Pin);
    switch( a_detected )
    {
    case RF_CHANNEL_EMPTY:
        
        if( transmitSendCommand() )
        {
            transmitType = NO_TRANS;
            startSingleTimer( TRANSMIT_NB_TIME_EVENT, getAvoidtime(), getChannelStarus );
        }
        else if( transFlag )
        {
            if( --macPIB.CW )
            {
                startSingleTimer( TRANSMIT_NB_TIME_EVENT, getAvoidtime(), getChannelStarus );
            }
            else
            {
                transmitType = transmitSendData();
            }
        }
        else
        {
#if configUSE_TICKLESS_IDLE == 0
            loraReceiveData();
#else
            checkTransmitQueue();
#endif
        }
        break;
    case RF_CHANNEL_ACTIVITY_DETECTED:
        if( transFlag )
        {
            if( ++macPIB.NB < MAC_MAX_NB )
            {
                macPIB.CW = MAC_VALUE_CW;
                macPIB.BE = minValue(macPIB.BE++, MAC_MAX_BE);
            }
            else
            {
                transFlag = false;
                xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_START, eSetBits );
            }
        }
        loraReceiveData();
        //startSingleTimer( TRANSMIT_NB_TIME_EVENT, getAvoidtime()+200, getChannelStarus );
        
        break;
    default:
        startSingleTimer( TRANSMIT_NB_TIME_EVENT, getAvoidtime(), getChannelStarus );
        return;
    }
    /* Send notify to Task which need */
    if( notifyTask )
    {
        xTaskNotify( notifyTask, a_detected, eSetValueWithOverwrite );
        notifyTask = NULL;
    }
}
/*****************************************************************
* DESCRIPTION: loraCadTimeout
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void loraCadTimeout( void )
{
    startSingleTimer( TRANSMIT_NB_TIME_EVENT, getAvoidtime(), getChannelStarus );
}




/****************************************************** END OF FILE ******************************************************/
