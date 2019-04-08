/*****************************************************************
* Copyright (C) 2017 60Plus Technology Co.,Ltd.*
******************************************************************
* NwkConfig.c
*
* DESCRIPTION:
*     network config
* AUTHOR:
*     Ziming
* CREATED DATE:
*     2019/1/10
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
#include "attribute.h"
#include "OS_timers.h"
#include "lora.h"
#include "lora_driver.h"
#include "radio.h"
#include "transmit.h"
#include "NwkConfig.h"
#include "Network.h"
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
/* Detection channel time(ms) */
#define DETECTION_CHANNEL_TIME            500
/* Wait beacon time(ms) */
#define WAIT_BEACON_TIME                  2000
/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                   GLOBAL VARIABLES                                                    *
 *************************************************************************************************************************/
static uint8_t g_channelNum = 0; 
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
/* 
 * network process task handle.
 */
TaskHandle_t                    nwkConfigTaskHandle;
/*************************************************************************************************************************
 *                                                    LOCAL VARIABLES                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                 FUNCTION DECLARATIONS                                                 *
 *************************************************************************************************************************/
static void     detectionChannelTimeout( void );
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                    LOCAL FUNCTIONS                                                    *
 *************************************************************************************************************************/

/*****************************************************************
* DESCRIPTION: nwkConfigProcess
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void nwkConfigProcess( void *parm )
{
    bool tempState = false;
    
    while(1)
    {
        if( !tempState )
        {
#ifdef SELF_ORGANIZING_NETWORK
#ifdef DEVICE_TYPE_COOR
            setNetworkStatus(NETWORK_FIND_CHANNEL);
            tempState = findChannel();
#else
            setNetworkStatus(NETWORK_JOIN_SCAN);
            tempState = joinNetwork();
#endif
#else
            if( nwkAttribute.m_shortAddr == 0x0000 )
            {
                setNetworkStatus(NETWORK_FIND_CHANNEL);
                tempState = findChannel();
            }
            else
            {
                setNetworkStatus(NETWORK_JOIN_SCAN);
                tempState = joinNetwork();
            }
#endif
        }
        else
        {
            xTaskNotify( networkTaskHandle, NETWORK_NOFITY_INIT_SUCCESS, eSetBits );
            vTaskDelay(100);
        }
        taskYIELD();
    }
}
     
     
/*****************************************************************
* DESCRIPTION: findChannel
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
bool findChannel( void )
{
    uint8_t channelStatus;
    static uint8_t channelNum = 0xFF;
    static bool taskLock = false;
    static uint8_t channelValue[7] = {0};

    if( !taskLock && g_channelNum <= LORA_CHANNEL_MAX )
    {
        taskLock = true;
        
        if( channelNum != g_channelNum )
        {
            channelNum = g_channelNum;
            loRaSetFrequency( LORA_FREQUENCY_MIN + LORA_FREQUENCY_STEP*g_channelNum );
            startSingleTimer( NETWORK_DETECTION_CHANNEL_EVENT, DETECTION_CHANNEL_TIME, detectionChannelTimeout );
        }
        /* Detection channel */
        detectionChannel( xTaskGetCurrentTaskHandle() );
        /* Wait for channel detection notification */
        if( xTaskNotifyWait( (uint32_t)0, ULONG_MAX, (uint32_t *)&channelStatus, 100 ) == pdTRUE )
        {
            taskLock = false;
            if(channelStatus == RF_CHANNEL_ACTIVITY_DETECTED)
            {
                channelValue[g_channelNum]++;
            }
        }
    }
    else if( g_channelNum > LORA_CHANNEL_MAX )
    {
        uint8_t minActivity = channelValue[0];
        nwkAttribute.m_channelNum = 0;
        for( uint8_t count = 0; count < g_channelNum; count++ )
        {
            if( minActivity > channelValue[count] )
            {
                minActivity = channelValue[count];
                nwkAttribute.m_channelNum = count;
            }
            channelValue[count] = 0;
        }
        g_channelNum = 0;
        loRaSetFrequency( LORA_FREQUENCY_MIN + LORA_FREQUENCY_STEP*nwkAttribute.m_channelNum );
#ifdef SELF_ORGANIZING_NETWORK
        xTaskNotify( loraTaskHandle, LORA_NOTIFY_SET_PANID, eSetBits );
#endif
        return true;
    }
    return false;
}

/*****************************************************************
* DESCRIPTION: joinNetwork
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
bool joinNetwork( void )
{
#ifdef SELF_ORGANIZING_NETWORK
    uint16_t panId;
    static t_rssiValue rssiValue[7] = {0};
#endif
    static uint8_t channelNum = 0xFF;
    
    if( g_channelNum <= LORA_CHANNEL_MAX )
    {
        if( channelNum != g_channelNum )
        {
            channelNum = g_channelNum;
            loRaSetFrequency( LORA_FREQUENCY_MIN + LORA_FREQUENCY_STEP*g_channelNum );
            startSingleTimer( NETWORK_WAIT_BEACON_EVENT, WAIT_BEACON_TIME, detectionChannelTimeout );
#ifndef SELF_ORGANIZING_NETWORK
            xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_JOIN_REQUEST, eSetBits );
            if( nwkAttribute.m_nwkStatus == true )
            {
                return true;
            }
        }
    }
#else
            /* Scan beacon message */
            scanBeaconMessage( xTaskGetCurrentTaskHandle() );
        }
        
        /* Wait notify */
        if( xTaskNotifyWait( (uint32_t)0, ULONG_MAX, (uint32_t *)&panId, 100 ) == pdTRUE )
        {
            if( rssiValue[g_channelNum].m_panId != 0x0000 && 
                rssiValue[g_channelNum].m_panId != panId )
            {
                if( rssiValue[g_channelNum].m_RSSI < getLoraRssi() )
                {
                    rssiValue[g_channelNum].m_panId = panId;
                    rssiValue[g_channelNum].m_RSSI = getLoraRssi();
                }
            }
            else
            {
                rssiValue[g_channelNum].m_panId = panId;
                rssiValue[g_channelNum].m_RSSI = getLoraRssi();
            }
        }
    }
    else if( g_channelNum > LORA_CHANNEL_MAX )
    {
        double rssiTemp = -10000.0;
        uint16_t dstPanId = 0x0000;
        
        for( uint8_t count = 0; count < g_channelNum; count++ )
        {
            if( rssiValue[count].m_panId != 0x0000 && rssiTemp < rssiValue[count].m_RSSI )
            {
                rssiTemp = rssiValue[count].m_RSSI;
                dstPanId = rssiValue[count].m_panId;
                nwkAttribute.m_channelNum = count;
            }
            rssiValue[count].m_panId = 0;
            rssiValue[count].m_RSSI = 0;
        }
        g_channelNum = 0;
        if( dstPanId != 0x0000 )
        {
            nwkAttribute.m_panId = dstPanId;
            loRaSetFrequency( LORA_FREQUENCY_MIN + LORA_FREQUENCY_STEP*nwkAttribute.m_channelNum );
            xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_JOIN_REQUEST, eSetBits );
            return true;
        }
    }
#endif
    return false;
}

/*****************************************************************
* DESCRIPTION: networConfigkStart
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void networConfigkStart( void )
{
    if( nwkAttribute.m_nwkStatus == false )
    {
        xTaskNotify( networkTaskHandle, NETWORK_NOFITY_INIT_START, eSetBits );
    }
}

/*****************************************************************
* DESCRIPTION: allowJoinNetwork
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void allowJoinNetwork( uint32_t a_time )
{
    if( getNetworkStatus() != NETWORK_COOR || a_time == 0 )
    {
        return;
    }
    /* Notify task send beacon packet start */
    xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_BEACON, eSetBits );
    /* Duration time */
    startSingleTimer( LORA_ALLOW_JOIN_TIME_EVENT, a_time, NULL );
}

/*****************************************************************
* DESCRIPTION: closeAllowJoinNetwork
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void closeAllowJoinNetwork( void )
{
    clearTimer( NETWORK_BEACON_EVENT, ALL_TYPE_TIMER );
    clearTimer( LORA_ALLOW_JOIN_TIME_EVENT, ALL_TYPE_TIMER );
}

/*****************************************************************
* DESCRIPTION: leaveNetwork
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void leaveNetwork( void )
{
    if( getNetworkStatus() != NETWORK_DEVICE )
    {
        return;
    }
    /* Clean network data */
    resetAttribute();
    /* Reset */
    resetSystem();
}



                

/*****************************************************************
* DESCRIPTION: detectionChannelTimeout
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void detectionChannelTimeout( void )
{    
    g_channelNum++;
}


/****************************************************** END OF FILE ******************************************************/
