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
#define DEVICE_TYPE     LOW_CONTROL
     
typedef enum
{
    LIGHT_DRIVER,
    LOW_CONTROL,
    HIGH_CONTROL,
    LOW_CONTROL_1,
    HIGH_CONTROL_1,
}E_type;

typedef struct
{
    uint16_t        m_head;
    E_type          m_type;
    uint32_t        m_count;
    
}t_testSendPacket;

typedef struct
{
    uint16_t        m_head;
    E_type          m_type;
    uint32_t        m_count;
    uint8_t         m_lostNum;
}t_testRcvPacket;
static uint16_t sendAddr = 0x0000;
static t_testRcvPacket  rcvPacket[5] = {0};
static t_testSendPacket  sendPacket = 
{
    0xAADD,
    DEVICE_TYPE,
    0,
};
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
    //for( uint8_t num = 0; num <= 6; num++)
    //{
    //    TOGGLE_GPIO_PIN(LED_GPIO_Port, LED_Pin);
    //    for( uint32_t count = 0; count < 655350; count++ );
    //}
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
        
        vTaskDelay(10);
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
    while(1)
    {
        if( g_networkStatus == NETWORK_COOR || 
            g_networkStatus == NETWORK_DEVICE )
        {
            //loraAllowJoinNetwork( 1200000 );
            tempState = true;
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
            //nwkAttributeSave();
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
static uint32_t counts = 0;
static void networkRun( void )
{
    
    uint32_t rcvData;
    uint8_t *rcvPoint;
 
    while(1)
    {
        loraReceiveData();
        if( g_networkStatus == NETWORK_DEVICE && (HAL_GetTick()%500) == 0 )
        {
            t_addrType dstAddr;
            dstAddr.addrMode = pointAddr16Bit;
            dstAddr.addr.m_dstShortAddr = 0x0000;
            if( transmitTx( &dstAddr, sizeof(t_testSendPacket), (uint8_t *)&sendPacket ) == E_SUCCESS )
            {
                sendPacket.m_count++;
            }
        }
        else if( g_networkStatus == NETWORK_COOR && xTaskNotifyWait( (uint32_t)0, ULONG_MAX, &rcvData, 1 ) == pdTRUE )
        {
            rcvPoint = (void *)rcvData;
            if( ((t_testRcvPacket *)rcvPoint)->m_head == 0xAADD )
            {
                rcvPacket[((t_testRcvPacket *)rcvPoint)->m_type].m_type = ((t_testRcvPacket *)rcvPoint)->m_type;
                rcvPacket[((t_testRcvPacket *)rcvPoint)->m_type].m_head = (uint16_t)((rcvPoint[sizeof(t_testSendPacket)+1] << 8) | ((rcvPoint[sizeof(t_testSendPacket)])));
                if( rcvPacket[((t_testRcvPacket *)rcvPoint)->m_type].m_count == ((t_testRcvPacket *)rcvPoint)->m_count )
                {
                    rcvPacket[((t_testRcvPacket *)rcvPoint)->m_type].m_count++;
                }
                else if( rcvPacket[((t_testRcvPacket *)rcvPoint)->m_type].m_count < ((t_testRcvPacket *)rcvPoint)->m_count )
                {
                    rcvPacket[((t_testRcvPacket *)rcvPoint)->m_type].m_lostNum += ((t_testRcvPacket *)rcvPoint)->m_count - rcvPacket[((t_testRcvPacket *)rcvPoint)->m_type].m_count;
                    rcvPacket[((t_testRcvPacket *)rcvPoint)->m_type].m_count = ((t_testRcvPacket *)rcvPoint)->m_count + 1;
                }
                else if( rcvPacket[((t_testRcvPacket *)rcvPoint)->m_type].m_count > ( ((t_testRcvPacket *)rcvPoint)->m_count + 20) )
                {
                    rcvPacket[((t_testRcvPacket *)rcvPoint)->m_type].m_lostNum = 0;
                    rcvPacket[((t_testRcvPacket *)rcvPoint)->m_type].m_count = 0;
                }
            }
            
        }
        if( g_networkStatus == NETWORK_COOR && (HAL_GetTick()%10000) == 0 )
        {
            if( sendAddr != 0x0000)
            {
                t_addrType dstAddr;
                dstAddr.addrMode = pointAddr16Bit;
                dstAddr.addr.m_dstShortAddr = sendAddr;
                transmitTx( &dstAddr, sizeof(t_testSendPacket), (uint8_t *)&sendPacket );
            }
        }
        if( counts == 5 )
        {
            t_addrType dstAddr;
            dstAddr.addrMode = broadcastAddr;
            loraDeleteDevice( &dstAddr );
            for( uint8_t count = 0; count < 5; count++ )
            {
                rcvPacket[count].m_lostNum = 0;
                rcvPacket[count].m_count = 0;
            }
            
        }
        else if( counts == 6 )
        {
            loraAllowJoinNetwork( 120000 );
        }
        else if( counts == 7 )
        {
            loraCloseBeacon();
        }
        else if( counts == 10 && g_networkStatus != NETWORK_COOR )
        {
            leaveNetwork();
        }
        //if( sendPacket.m_count >= 30)
        //{
        //    configASSERT( 0 );
        //}
        //nwkAttributeSave();
        //taskYIELD();
        vTaskDelay(1);
    }
}


/****************************************************** END OF FILE ******************************************************/
