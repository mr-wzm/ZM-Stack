/*****************************************************************
* Copyright (C) 2017 60Plus Technology Co.,Ltd.*
******************************************************************
* Network.c
*
* DESCRIPTION:
*     Lora network process
* AUTHOR:
*     Ziming
* CREATED DATE:
*     2018/11/27
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
#include "gpio.h"
#include "attribute.h"
#include "OS_timers.h"
#include "NwkConfig.h"
#include "transmit.h"
#include "lora.h"
#include "zigbee.h"
#include "lora_driver.h"
#include "Network.h"
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/

/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/

/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/

/*************************************************************************************************************************
 *                                                   GLOBAL VARIABLES                                                    *
 *************************************************************************************************************************/
static E_nwkStatus          g_networkStatus = NETWORK_HOLD;
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/

/* 
 * network process task handle.
 */
TaskHandle_t                    networkTaskHandle;
/*************************************************************************************************************************
 *                                                    LOCAL VARIABLES                                                    *
 *************************************************************************************************************************/
     
/*************************************************************************************************************************
 *                                                 FUNCTION DECLARATIONS                                                 *
 *************************************************************************************************************************/
static void     networkStart( void );
static void     networkRun( void );
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/

/*************************************************************************************************************************
 *                                                    LOCAL FUNCTIONS                                                    *
 *************************************************************************************************************************/

/*****************************************************************
* DESCRIPTION: networkInit
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void networkInit( void )
{
    nwkAttributeRead();
    
#ifdef SELF_ORGANIZING_NETWORK
#ifdef DEVICE_TYPE_COOR
    g_networkStatus = NETWORK_INIT;
#endif
#endif
    if( nwkAttribute.m_nwkStatus == true &&
        nwkAttribute.m_panId != 0x0000 )
    {
        if( nwkAttribute.m_shortAddr != 0x0000 )
        {
            g_networkStatus = NETWORK_DEVICE;
        }
        else
        {
            g_networkStatus = NETWORK_COOR;
        }
        loRaSetFrequency( LORA_FREQUENCY_MIN + LORA_FREQUENCY_STEP*nwkAttribute.m_channelNum );
    }
}

/*****************************************************************
* DESCRIPTION: networkProcess
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void networkProcess( void *parm )
{
   
    while(1)
    {
        networkStart();

        networkRun();
        /* Network process is error,It should never get here */
        resetSystem();
    }
}


/*****************************************************************
* DESCRIPTION: getNetworkStatus
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
E_nwkStatus getNetworkStatus( void )
{
    return g_networkStatus;
}

/*****************************************************************
* DESCRIPTION: setNetworkStatus
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void setNetworkStatus( E_nwkStatus a_status )
{
    g_networkStatus = a_status;
}

/*****************************************************************
* DESCRIPTION: networkStart
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void networkStart( void )
{
    bool tempState = false;
    
    zigbeeUartInit();
    
    while(1)
    {
        if( g_networkStatus == NETWORK_COOR || 
            g_networkStatus == NETWORK_DEVICE )
        {
            tempState = true;
            break;
        }
        else if( g_networkStatus == NETWORK_HOLD )
        {
            uint32_t eventId = 0;
            while(1)
            {
                eventId = 0;
                /* wait task notify */
                xTaskNotifyWait( (uint32_t)0, ULONG_MAX, &eventId, portMAX_DELAY );
                /* Uart receive done */
                if( (eventId & NETWORK_NOFITY_UART_RX_DONE) == NETWORK_NOFITY_UART_RX_DONE )
                {
                    uartReceiveDone();
                    eventId ^= NETWORK_NOFITY_UART_RX_DONE;
                }
                /* Lora join network */
                else if( (eventId & NETWORK_NOFITY_JOIN_START) == NETWORK_NOFITY_JOIN_START )
                {
                    break;
                }
                /* Returns the untreated event */
                if( eventId )
                {
                    xTaskNotify( networkTaskHandle, eventId, eSetBits );
                }
                taskYIELD();
            }
        }
        if( !tempState )
        {
#ifdef SELF_ORGANIZING_NETWORK
#ifdef DEVICE_TYPE_COOR            
            g_networkStatus = NETWORK_FIND_CHANNEL;
            tempState = findChannel();
#else
            g_networkStatus = NETWORK_JOIN_SCAN;
            tempState = joinNetwork();
#endif
#else
            if( nwkAttribute.m_shortAddr == 0x0000 )
            {
                g_networkStatus = NETWORK_FIND_CHANNEL;
                tempState = findChannel();
            }
            else
            {
                g_networkStatus = NETWORK_JOIN_SCAN;
                tempState = joinNetwork();
            }
#endif
        }
        else
        {
            break;
        }
        taskYIELD();
    }
}

/*****************************************************************
* DESCRIPTION: networkRun
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void networkRun( void )
{
    uint32_t eventId = 0;
    while(1)
    {
        eventId = 0;
        /* wait task notify */
        xTaskNotifyWait( (uint32_t)0, ULONG_MAX, &eventId, portMAX_DELAY );
        /* Uart receive done */
        if( (eventId & NETWORK_NOFITY_UART_RX_DONE) == NETWORK_NOFITY_UART_RX_DONE )
        {
            uartReceiveDone();
            eventId ^= NETWORK_NOFITY_UART_RX_DONE;
        }
        /* Returns the untreated event */
        if( eventId )
        {
            xTaskNotify( networkTaskHandle, eventId, eSetBits );
        }
        taskYIELD();
    }
}


/****************************************************** END OF FILE ******************************************************/
