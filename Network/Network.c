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
    initEEP();
    
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
#if configUSE_TICKLESS_IDLE == 1
        loraSetFrequency( LORA_FREQUENCY_MAX );
        loraSetPreambleLength(LORA_PREAMBLE_LENGTH_LP);
#else
        loraSetFrequency( LORA_FREQUENCY_MIN + LORA_FREQUENCY_STEP*nwkAttribute.m_channelNum );
        loraSetPreambleLength(LORA_PREAMBLE_LENGTH);
#endif
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
   uint32_t eventId = 0;
   
   //networConfigkStart();
   
   //zigbeeUartInit();
#ifdef DEVICE_TYPE_COOR
   //t_addrType dstaddr;
   //dstaddr.addrMode = broadcastAddr;
   //dstaddr.addr.m_dstShortAddr = 0x51CF;
   //loraDeleteDevice( &dstaddr );
   //allowJoinNetwork(120000);
#endif
    while(1)
    {
        eventId = 0;
        /* wait task notify */
        xTaskNotifyWait( (uint32_t)0, ULONG_MAX, &eventId, portMAX_DELAY );
        /* Lora init network start */
        if( (eventId & NETWORK_NOFITY_INIT_START) == NETWORK_NOFITY_INIT_START )
        {
            static bool createLock = false;
            
            if( createLock == false && nwkAttribute.m_nwkStatus == false )
            {
                taskENTER_CRITICAL();           //Enter the critical area
                /* Create network task */
                if(xTaskCreate( nwkConfigProcess,
                               "nwkConfigProcess",
                                NWKCONFIG_TASK_DEPTH,
                                NULL, NWKCONFIG_TASK_PRIORITY,
                                &nwkConfigTaskHandle ) == pdPASS)
                {
                    /* Create task success */
                    createLock = true;
                }
                else
                {
                    /* Create task faild */
                    xTaskNotify( networkTaskHandle, NETWORK_NOFITY_INIT_START, eSetBits );
                }
                taskEXIT_CRITICAL();            //Exit the critical area
            }
            /* Clear the event flag */
            eventId ^= NETWORK_NOFITY_INIT_START;
        }
        /* Lora init network success */
        else if( (eventId & NETWORK_NOFITY_INIT_SUCCESS) == NETWORK_NOFITY_INIT_SUCCESS )
        {
            taskENTER_CRITICAL();           //Enter the critical area
            /* Delete the task */
            vTaskDelete(nwkConfigTaskHandle);
            taskEXIT_CRITICAL();            //Exit the critical area
            /* Clear the event flag */
            eventId ^= NETWORK_NOFITY_INIT_SUCCESS;
        }
        /* Uart receive done */
        else if( (eventId & NETWORK_NOFITY_UART_RX_DONE) == NETWORK_NOFITY_UART_RX_DONE )
        {
            uartReceiveDone();
            /* Clear the event flag */
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

/****************************************************** END OF FILE ******************************************************/
