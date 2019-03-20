/*****************************************************************
* Copyright (C) 2017 60Plus Technology Co.,Ltd.*
******************************************************************
* transmit.c
*
* DESCRIPTION:
*     Transmit
* AUTHOR:
*     Ziming
* CREATED DATE:
*     2018/12/27
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
#include "gpio.h"
#include "attribute.h"
#include "security.h"
#include "OS_timers.h"
#include "Network.h"
#include "NwkConfig.h"
#include "lora.h"
#include "lora_driver.h"
#include "transmit.h"
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
/* Number of data message queues */
static uint8_t                  g_queueCount  = 0;
/* Number of data message retransmit queues */
static uint8_t                  g_retransmitQueueCount = 0;
/* Number of command message queues */
static uint8_t                  g_commandQueueCount = 0;
/* Message transmit ID */
static uint8_t                  g_transmitID  = 0;
/* Reveiced beacon packet notify task which need */
static TaskHandle_t             notifyTask = NULL;
/* Data message queues head */
static t_transmitQueue          *transmitHead = NULL;
/* Data message queues end */
static t_transmitQueue          *transmitEnd  = NULL;
/* Data message queue retransmit head */
static t_transmitQueue          *retransmitHead = NULL;
/* Data message queue retransmit send current node */
static t_transmitQueue          *retransmitCurrent = NULL;
/* Command message queues head */
static t_transmitCommandQueue   *transmitCommandHead = NULL;
/* Command message queues end */
static t_transmitCommandQueue   *transmitCommandEnd  = NULL;
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
/*
 * Channel csma parameters.
 */
t_macCsmaParm                macPIB;

/*************************************************************************************************************************
 *                                                    LOCAL VARIABLES                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                 FUNCTION DECLARATIONS                                                 *
 *************************************************************************************************************************/
static int      getRand( void );
static void     retransmitCurrentPointNext( void );
static void     checkTransmitQueue( void );
static void     addTransmitCommandQueue( t_transmitCommandQueue *transmitCommandNode );
static void     transmitAck( uint16_t a_shortAddr, uint8_t a_transmitId );
static void     transmitLeave( t_addrType *a_dstAddr );
static void     transmitJoinResponse( E_addrMode a_addrMode, uint8_t *a_dstAddr, bool a_joinSuccess );
static void     packetAck( t_ackPacket *a_packet );
static void     packetBeacon( t_beaconPacket *a_packet );
static void     packetJoinRequest( t_joinRequestPacket *a_packet );
static void     packetJoinResponse( t_joinResponsePacket *a_packet );
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                    LOCAL FUNCTIONS                                                    *
 *************************************************************************************************************************/


/*****************************************************************
* DESCRIPTION: transmitTx
*     
* INPUTS:
*      a_dstAddr : Full ZB destination address: Nwk Addr.
*      a_size : The size of the data.
*      a_data : Data to be sent.
* OUTPUTS:
*     Add queue success or failure.
* NOTE:
*     Add data in transmit queue.
*****************************************************************/
E_typeErr transmitTx( t_addrType *a_dstAddr, uint8_t a_size, uint8_t *a_data )
{
    if( a_size == 0 || a_data == NULL )
    {
        return E_ERR;
    }
    t_transmitQueue *packetQueue = (t_transmitQueue *)pvPortMalloc(sizeof(t_transmitQueue) + a_size);
    /* malloc success */
    if( packetQueue )
    {
        /* pointAddr16Bit : use short addr mode */
        if( a_dstAddr->addrMode == pointAddr16Bit )
        {
            packetQueue->m_transmitPacket.m_dstAddr.addr.m_dstShortAddr = a_dstAddr->addr.m_dstShortAddr;
        }
        /* pointAddr64Bit : use long addr mode */
        else if( a_dstAddr->addrMode == pointAddr64Bit )
        {
            memcpy( packetQueue->m_transmitPacket.m_dstAddr.addr.m_dstMacAddr, a_dstAddr->addr.m_dstMacAddr, MAC_ADDR_LEN );
        }
        /* broadcast mode */
        else
        {
            packetQueue->m_transmitPacket.m_dstAddr.addr.m_dstShortAddr = 0xFFFF;
        }
        /* Full message packet in data message queues */
        packetQueue->m_transmitPacket.m_dstAddr.addrMode = a_dstAddr->addrMode;
        packetQueue->m_transmitPacket.m_panId = nwkAttribute.m_panId;
        packetQueue->m_transmitPacket.m_cmdType = DATA_ORDER;
        packetQueue->m_transmitPacket.m_srcAddr = nwkAttribute.m_shortAddr;
        packetQueue->m_transmitPacket.m_transmitID = g_transmitID++;
        packetQueue->m_transmitPacket.m_size = a_size;
        memcpy( packetQueue->m_transmitPacket.m_data, a_data, a_size );
        packetQueue->m_transmitPacket.m_keyNum = dataEncrypt( packetQueue->m_transmitPacket.m_data, a_size );
        packetQueue->m_retransmit = 0;
        packetQueue->m_next = NULL;
        /* Is data message queues empty */
        if( transmitHead == NULL && transmitEnd == NULL )
        {
            //macPIB.retransmit = 0;
            transmitHead = packetQueue;
            /* Send notify to lora process */
            xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_START, eSetBits );
        }
        else
        {
            /* Add packet in the queues last */
            transmitEnd->m_next = packetQueue;
        }
        transmitEnd = packetQueue;
        /* Record queue number */
        g_queueCount++;

        return E_SUCCESS;
    }
    else
    {
        /* Failed to malloc, check if the queue is full */
        checkTransmitQueue();
    }
    return E_ERR;
}

/*****************************************************************
* DESCRIPTION: transmitRx
*     
* INPUTS:
*     a_packet : Received packet.
* OUTPUTS:
*     The cmd type of the message.
* NOTE:
*     Parse the received packet.
*****************************************************************/
E_cmdType transmitRx( t_transmitPacket *a_packet )
{
    /* Check the pandId, or mac addr */
    if( nwkAttribute.m_nwkStatus && a_packet->m_panId == nwkAttribute.m_panId || 
       (a_packet->m_dstAddr.addrMode == pointAddr64Bit &&
        memcmp(a_packet->m_dstAddr.addr.m_dstMacAddr, nwkAttribute.m_mac, MAC_ADDR_LEN) == 0) )
    {
        /* Is the destination is yourself */
        if( (a_packet->m_dstAddr.addrMode == pointAddr16Bit && 
             a_packet->m_dstAddr.addr.m_dstShortAddr != nwkAttribute.m_shortAddr) ||
            (a_packet->m_dstAddr.addrMode == pointAddr64Bit &&
             memcmp(a_packet->m_dstAddr.addr.m_dstMacAddr, nwkAttribute.m_mac, MAC_ADDR_LEN) != 0) )
        {
           return ERR_ORDER;
        }
        
        TOGGLE_GPIO_PIN(LED_GPIO_Port, LED_Pin);
        /* The packet type */
        switch( a_packet->m_cmdType )
        {
        case DATA_ORDER:
            dataDecode( a_packet->m_keyNum, a_packet->m_data, a_packet->m_size );
            if( a_packet->m_dstAddr.addrMode != broadcastAddr )
            {
                transmitAck( a_packet->m_srcAddr, a_packet->m_transmitID );
            }
            break;
        case ACK_ORDER:
            packetAck( (t_ackPacket *)a_packet );
            break;
        case BEACON_ORDER:
           // if(nwkAttribute.m_nwkStatus == false)
           // {
           //     packetBeacon( (t_beaconPacket *)a_packet );
           // }
            break;
        case JOIN_REQUEST_ORDER:
            packetJoinRequest( (t_joinRequestPacket *)a_packet );
            break;
        case JOIN_RESPONSE_ORDER:
            packetJoinResponse( (t_joinResponsePacket *)a_packet );
            break;
        case LEAVE_ORDER:
            leaveNetwork();
            break;
        default:
            break;
        }
    }
    else if( !nwkAttribute.m_nwkStatus && 
             getNetworkStatus() == NETWORK_JOIN_SCAN && 
             a_packet->m_cmdType == BEACON_ORDER )
    {
        packetBeacon( (t_beaconPacket *)a_packet );
    }
    return a_packet->m_cmdType;
}

/*****************************************************************
* DESCRIPTION: transmitSendData
*     
* INPUTS:
*     NULL
* OUTPUTS:
*     Current send type(transmit or retransmit),the value is in
*       E_transmitType enum.
* NOTE:
*     Lora send message package,and return send type.
*     If the transmit queue has packet,the first send is in transmit
*     queue.
*     Else send retransmitCurrent pointer point packet.
*****************************************************************/
E_transmitType transmitSendData( void )
{
    loraEnterStandby();
    if( transmitHead )
    {
        /* Send message */
        if( loraSendData( (uint8_t *)&transmitHead->m_transmitPacket, 
                         sizeof(t_transmitPacket) + transmitHead->m_transmitPacket.m_size ) != E_SUCCESS )
        {
            /* Send notify to lora process */
            xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_START, eSetBits );
        }
        else
        {
            return T_TRANSMIT;
        }
    }
    else if( retransmitHead )
    {
        if( retransmitCurrent == NULL )
        {
            retransmitCurrent = retransmitHead;
        }
        /* Send message */
        if( loraSendData( (uint8_t *)&retransmitCurrent->m_transmitPacket, 
                         sizeof(t_transmitPacket) + retransmitCurrent->m_transmitPacket.m_size ) != E_SUCCESS )
        {
            /* Send notify to lora process */
            xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_START, eSetBits );
        }
        else
        {
            return T_RETRANSMIT;
        }
    }
    return NO_TRANS;
}

/*****************************************************************
* DESCRIPTION: transmitRetransmit
*     
* INPUTS:
*     a_transmitType : Current send type(transmit or retransmit),
*                      the value is in E_transmitType enum.
* OUTPUTS:
*     NULL
* NOTE:
*     If the input parameter is T_TRANSMIT, it is indicated as 
*       transmit,and put it in retransmit queue( broadcast packet
*       do not put in).
*     If the input parameter is T_RETRANSMIT, it means the current
*       transmission is a retransmission,and make the 
*       retransmitCurrent pointer point next packet.
*****************************************************************/
void transmitRetransmit( E_transmitType a_transmitType )
{
    if( a_transmitType == T_TRANSMIT )
    {
        /* Do not retransmit broadcast packet */
        if( transmitHead->m_transmitPacket.m_dstAddr.addrMode == broadcastAddr )
        {
            return;
        }
        if( retransmitHead == NULL )
        {
            retransmitHead = transmitHead;
            transmitHead = transmitHead->m_next;
            retransmitHead->m_next = NULL;
            retransmitCurrent = retransmitHead;
        }
        else
        {
            t_transmitQueue *retransmitNode = retransmitHead;
            while( retransmitNode->m_next )
            {
                retransmitNode = retransmitNode->m_next;
            }
            retransmitNode->m_next = transmitHead;
            transmitHead = transmitHead->m_next;
            retransmitNode->m_next->m_next = NULL;
        }
        g_queueCount--;
        g_retransmitQueueCount++;
    }
    else if( a_transmitType == T_RETRANSMIT )
    {
        retransmitCurrentPointNext();
    }
}

/*****************************************************************
* DESCRIPTION: transmitSendCommand
*     
* INPUTS:
*     NULL
* OUTPUTS:
*     true : There are command packet need send and sended success.
*     false: No have command packet need send.
* NOTE:
*     Check if there are any command packages to send.
*     And send the command package which in the transmitCommand 
*     queue head.
*****************************************************************/
bool transmitSendCommand( void )
{
    /* Is ther command packet that needs to be sent */
    if( transmitCommandHead != NULL )
    {
        t_transmitCommandQueue *transmitCommandDel = transmitCommandHead;
        /* If the network status is true（device joined network success）,
           never send join request */
        if( transmitCommandHead->m_cmdType == JOIN_REQUEST_ORDER && nwkAttribute.m_nwkStatus )
        {
            goto SKIP_SEND;
        }
        /* Send message unitl it return success */
        while( loraSendData( (uint8_t *)transmitCommandHead->m_packet, transmitCommandHead->m_size ) != E_SUCCESS )
        {
            loraEnterStandby();
        }
    SKIP_SEND:
        /* Is it last in the queues */
        if( transmitCommandEnd == transmitCommandHead )
        {
            if( transmitCommandEnd->m_next == NULL )
            {
                transmitCommandEnd = NULL;
                transmitCommandHead = NULL;
            }
            else  //The queue is error.
            {
                while( transmitCommandEnd->m_next )
                {
                    transmitCommandEnd = transmitCommandEnd->m_next;
                }
                transmitCommandHead = transmitCommandHead->m_next;
            }
        }
        else
        {
            /* Point next packet */
            transmitCommandHead = transmitCommandHead->m_next;
            /* Send notify to lora process */
            xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_COMMAND, eSetBits );
        }
        /* Number of queues minus one */
        g_commandQueueCount--;
        /* Free sended success packet */
        vPortFree(transmitCommandDel->m_packet);
        vPortFree(transmitCommandDel);
    }
    else
    {
        return false;
    }
    return true;
}

/*****************************************************************
* DESCRIPTION: transmitFreeHeadData
*     
* INPUTS:
*     NULL
* OUTPUTS:
*     NULL
* NOTE:
*     Delete transmit head packet.
*****************************************************************/
void transmitFreeHeadData( void )
{
    if( transmitHead == NULL )
    {
        return;
    }
    t_transmitQueue *transmitFree = transmitHead;
    
    if( transmitHead == transmitEnd )
    {
        if( transmitEnd->m_next == NULL )
        {
            transmitHead = NULL;
            transmitEnd  = NULL;
        }
        else    //Queue is error
        {
            transmitHead = transmitEnd->m_next;
            while( transmitEnd->m_next )
            {
                transmitEnd = transmitEnd->m_next;
            }
            /* Send notify to lora process */
            xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_START, eSetBits );
        }
    }
    else
    {
        /* Point next packet */
        transmitHead = transmitHead->m_next;
        /* Send notify to lora process */
        xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_START, eSetBits );
    }
    //macPIB.retransmit = 0;
    /* Number of queues minus one */
    g_queueCount--;
    /* Free sended success packet */
    vPortFree(transmitFree);
}

/*****************************************************************
* DESCRIPTION: retransmitFreeCurrentPacket
*     
* INPUTS:
*     a_transmitFree : Which packet needed delete.
* OUTPUTS:
*     NULL
* NOTE:
*     Delete packet in retransmit queue.
*****************************************************************/
void retransmitFreePacket( t_transmitQueue *a_transmitFree )
{
    t_transmitQueue *retransmitFree = a_transmitFree;
    if( retransmitFree == NULL )
    {
        return;
    }
    if( retransmitFree == retransmitHead )
    {
        retransmitHead = retransmitHead->m_next;
        if( retransmitCurrent == retransmitFree )
        {
            retransmitCurrentPointNext();
        }
        /* Free packet */
        vPortFree(retransmitFree);
    }
    else
    {
        t_transmitQueue *retransmitNode = retransmitHead;
        while( retransmitNode->m_next != retransmitFree && 
              retransmitNode != NULL )
        {
            retransmitNode = retransmitNode->m_next;
        }
        if( retransmitNode )
        {
            retransmitNode->m_next = retransmitFree->m_next;
            if( retransmitCurrent == retransmitFree )
            {
                retransmitCurrentPointNext();
            }
            /* Free packet */
            vPortFree(retransmitFree);
        }
    }
    if( transmitHead || retransmitHead )
    {
        /* Send notify to lora process */
        xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_START, eSetBits );
    }
}

/*****************************************************************
* DESCRIPTION: scanBeaconMessage
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void scanBeaconMessage( TaskHandle_t a_notifyTask )
{
    notifyTask = a_notifyTask;
    loraEnterStandby();
    loraReceiveData();
}

/*****************************************************************
* DESCRIPTION: getTransmitPacket
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
t_transmitQueue * getTransmitPacket( void )
{
    return transmitHead;
}
/*****************************************************************
* DESCRIPTION: getRetransmitCurrentPacket
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
t_transmitQueue * getRetransmitCurrentPacket( void )
{
    return retransmitCurrent;
}
/*****************************************************************
* DESCRIPTION: getAvoidtime
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
uint8_t getAvoidtime( void )
{
    uint8_t index = 1;
    srand(HAL_GetTick());
    for( uint8_t count = 0; count < maxValue(MAC_MIN_BE, macPIB.BE); count++)
    {
        index *= 2;
    }
    return (rand()%index + 1);
}

/*****************************************************************
* DESCRIPTION: getRand
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static int getRand( void )
{
    srand(HAL_GetTick());
    return rand();
}

/*****************************************************************
* DESCRIPTION: retransmitCurrentPointNext
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void retransmitCurrentPointNext( void )
{
    if( retransmitCurrent->m_next )
    {
        retransmitCurrent = retransmitCurrent->m_next;
    }
    else
    {
        retransmitCurrent = retransmitHead;
    }
}
/*****************************************************************
* DESCRIPTION: checkTransmitQueue
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void checkTransmitQueue( void )
{
    if( transmitCommandHead != NULL || retransmitHead != NULL )
    {
        /* Send notify to lora process */
        xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_COMMAND, eSetBits );
    }
    if( transmitHead != NULL )
    {
        /* Send notify to lora process */
        xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_START, eSetBits );
    }
}
/*****************************************************************
* DESCRIPTION: addTransmitCommandQueue
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void addTransmitCommandQueue( t_transmitCommandQueue *transmitCommandNode )
{
    if( transmitCommandNode == NULL )
    {
        return;
    }
    if( transmitCommandHead == NULL && transmitCommandEnd == NULL )
    {
        transmitCommandHead = transmitCommandNode;
        /* Send notify to lora process */
        xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_COMMAND, eSetBits );
    }
    else
    {
        transmitCommandEnd->m_next = transmitCommandNode;
    }
    g_commandQueueCount++;
    transmitCommandEnd = transmitCommandNode;
}

/*****************************************************************
* DESCRIPTION: transmitAck
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void transmitAck( uint16_t a_dstAddr, uint8_t a_transmitId )
{
    if( transmitCommandHead )
    {
        t_transmitCommandQueue *transmitCommandFind = transmitCommandHead;
        while( transmitCommandFind )
        {
            if( transmitCommandFind->m_cmdType == ACK_ORDER && 
                ((t_ackPacket *)transmitCommandFind->m_packet)->m_dstAddr.addr.m_dstShortAddr == a_dstAddr &&
                ((t_ackPacket *)transmitCommandFind->m_packet)->m_transmitID == a_transmitId )
            {
                //checkTransmitQueue();
                return;
            }
            transmitCommandFind = transmitCommandFind->m_next;
        }
    }
    t_transmitCommandQueue *transmitCommandNode = (t_transmitCommandQueue *)pvPortMalloc(sizeof(t_transmitCommandQueue));
    if(transmitCommandNode == NULL)
    {
        checkTransmitQueue();
        return;
    }
    t_ackPacket *ackPacket = (t_ackPacket *)pvPortMalloc(sizeof(t_ackPacket));
    if(ackPacket)
    {
        ackPacket->m_panId = nwkAttribute.m_panId;
        ackPacket->m_cmdType = ACK_ORDER;
        ackPacket->m_dstAddr.addrMode = pointAddr16Bit;
        ackPacket->m_dstAddr.addr.m_dstShortAddr = a_dstAddr;
        ackPacket->m_transmitID = a_transmitId;
        transmitCommandNode->m_size = sizeof(t_ackPacket);
        transmitCommandNode->m_packet = ackPacket;
        transmitCommandNode->m_cmdType = ACK_ORDER;
        transmitCommandNode->m_next = NULL;
        addTransmitCommandQueue(transmitCommandNode);
    }
    else
    {
        vPortFree(transmitCommandNode);
        checkTransmitQueue();
    }
}

/*****************************************************************
* DESCRIPTION: transmitBeacon
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void transmitBeacon( void )
{
    t_transmitCommandQueue *transmitCommandNode = (t_transmitCommandQueue *)pvPortMalloc(sizeof(t_transmitCommandQueue));
    if(transmitCommandNode == NULL)
    {
        checkTransmitQueue();
        return;
    }
    t_beaconPacket *beaconPacket = (t_beaconPacket *)pvPortMalloc(sizeof(t_beaconPacket));
    if( beaconPacket )
    {
        beaconPacket->m_panId = nwkAttribute.m_panId;
        beaconPacket->m_cmdType = BEACON_ORDER;
        beaconPacket->m_dstAddr.addrMode = broadcastAddr;
        beaconPacket->m_dstAddr.addr.m_dstShortAddr = 0xFFFF;
        transmitCommandNode->m_size = sizeof(t_beaconPacket);
        transmitCommandNode->m_packet = beaconPacket;
        transmitCommandNode->m_cmdType = BEACON_ORDER;
        transmitCommandNode->m_next = NULL;
        addTransmitCommandQueue(transmitCommandNode);
    }
    else
    {
        vPortFree(transmitCommandNode);
        checkTransmitQueue();
    }
}

/*****************************************************************
* DESCRIPTION: transmitJoinRequest
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void transmitJoinRequest( void )
{
    t_transmitCommandQueue *transmitCommandNode = (t_transmitCommandQueue *)pvPortMalloc(sizeof(t_transmitCommandQueue));
    if(transmitCommandNode == NULL)
    {
        checkTransmitQueue();
        return;
    }
    t_joinRequestPacket *joinRequestPacket = (t_joinRequestPacket *)pvPortMalloc(sizeof(t_joinRequestPacket));
    if( joinRequestPacket )
    {
        joinRequestPacket->m_panId = nwkAttribute.m_panId;
        joinRequestPacket->m_cmdType = JOIN_REQUEST_ORDER;
        joinRequestPacket->m_dstAddr.addrMode = pointAddr16Bit;
        joinRequestPacket->m_dstAddr.addr.m_dstShortAddr = 0x0000;
#ifdef SELF_ORGANIZING_NETWORK
        joinRequestPacket->m_srcAddr.addrMode = pointAddr64Bit;
        memcpy(joinRequestPacket->m_srcAddr.addr.m_dstMacAddr, nwkAttribute.m_mac, MAC_ADDR_LEN);
#else
        joinRequestPacket->m_srcAddr.addrMode = pointAddr16Bit;
        joinRequestPacket->m_srcAddr.addr.m_dstShortAddr = nwkAttribute.m_shortAddr;
#endif        
        memcpy(joinRequestPacket->m_securityKey, securityKey, SECURITY_KEY_LEN );
        joinRequestPacket->m_keyNum = dataEncrypt( joinRequestPacket->m_securityKey, SECURITY_KEY_LEN );
        transmitCommandNode->m_size = sizeof(t_joinRequestPacket);
        transmitCommandNode->m_packet = joinRequestPacket;
        transmitCommandNode->m_cmdType = JOIN_REQUEST_ORDER;
        transmitCommandNode->m_next = NULL;
        addTransmitCommandQueue(transmitCommandNode);
    }
    else
    {
        vPortFree(transmitCommandNode);
        checkTransmitQueue();
    }
}

/*****************************************************************
* DESCRIPTION: loraDeleteDevice
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void loraDeleteDevice( t_addrType *a_dstAddr )
{
    for( uint8_t count = 0; count < BROADCAST_MAX_NUM; count++ )
    {
        transmitLeave(a_dstAddr);
    }
}

/*****************************************************************
* DESCRIPTION: transmitLeave
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void transmitLeave( t_addrType *a_dstAddr )
{
    t_transmitCommandQueue *transmitCommandNode = (t_transmitCommandQueue *)pvPortMalloc(sizeof(t_transmitCommandQueue));
    if( transmitCommandNode == NULL || getNetworkStatus() != NETWORK_COOR )
    {
        checkTransmitQueue();
        return;
    }
    t_leavePacket *leavePacket = (t_leavePacket *)pvPortMalloc(sizeof(t_leavePacket));
    if( leavePacket )
    {
        leavePacket->m_dstAddr.addrMode = a_dstAddr->addrMode;
        /* pointAddr16Bit : use short addr mode */
        if( a_dstAddr->addrMode == pointAddr16Bit )
        {
            leavePacket->m_dstAddr.addr.m_dstShortAddr = a_dstAddr->addr.m_dstShortAddr;
        }
        /* pointAddr64Bit : use long addr mode */
        else if( a_dstAddr->addrMode == pointAddr64Bit )
        {
            memcpy( leavePacket->m_dstAddr.addr.m_dstMacAddr, a_dstAddr->addr.m_dstMacAddr, MAC_ADDR_LEN );
        }
        /* broadcast mode */
        else
        {
            leavePacket->m_dstAddr.addr.m_dstShortAddr = 0xFFFF;
        }
        leavePacket->m_panId = nwkAttribute.m_panId;
        leavePacket->m_cmdType = LEAVE_ORDER;
        leavePacket->m_srcAddr = nwkAttribute.m_shortAddr;
        transmitCommandNode->m_size = sizeof(t_leavePacket);
        transmitCommandNode->m_packet = leavePacket;
        transmitCommandNode->m_cmdType = leavePacket->m_cmdType;
        transmitCommandNode->m_next = NULL;
        addTransmitCommandQueue(transmitCommandNode);
    }
    else
    {
        vPortFree(transmitCommandNode);
        checkTransmitQueue();
    }
    
}
/*****************************************************************
* DESCRIPTION: transmitJoinResponse
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void transmitJoinResponse( E_addrMode a_addrMode, uint8_t *a_dstAddr, bool a_joinSuccess )
{
    if( a_addrMode == broadcastAddr )
    {
        return;
    }
    t_transmitCommandQueue *transmitCommandNode = (t_transmitCommandQueue *)pvPortMalloc(sizeof(t_transmitCommandQueue));
    if(transmitCommandNode == NULL)
    {
        return;
    }
    t_joinResponsePacket *joinResponsePacket = (t_joinResponsePacket *)pvPortMalloc(sizeof(t_joinResponsePacket));
    if( joinResponsePacket )
    {
        joinResponsePacket->m_panId = nwkAttribute.m_panId;
        joinResponsePacket->m_cmdType = JOIN_RESPONSE_ORDER;
        joinResponsePacket->m_dstAddr.addrMode = a_addrMode;
        if( a_addrMode == pointAddr16Bit )
        {
            joinResponsePacket->m_dstAddr.addr.m_dstShortAddr = (uint16_t)(a_dstAddr[0] << 8 | a_dstAddr[1]);
        }
        else if( a_addrMode == pointAddr64Bit )
        {
            memcpy(joinResponsePacket->m_dstAddr.addr.m_dstMacAddr, a_dstAddr, MAC_ADDR_LEN);
        }
        joinResponsePacket->m_joinSuccess = a_joinSuccess;
        joinResponsePacket->m_srcAddr = nwkAttribute.m_shortAddr;
        joinResponsePacket->m_shortAddr = getRand();
        transmitCommandNode->m_size = sizeof(t_joinResponsePacket);
        transmitCommandNode->m_packet = joinResponsePacket;
        transmitCommandNode->m_cmdType = JOIN_RESPONSE_ORDER;
        transmitCommandNode->m_next = NULL;
        addTransmitCommandQueue(transmitCommandNode);
    }
    else
    {
        vPortFree(transmitCommandNode);
        checkTransmitQueue();
    }
}

/*****************************************************************
* DESCRIPTION: packetAck
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void packetAck( t_ackPacket *a_packet )
{
    if( a_packet->m_transmitID == transmitHead->m_transmitPacket.m_transmitID )
    {
        stopTimer( TRANSMIT_WAIT_ACK_EVENT, SINGLE_TIMER );
        transmitFreeHeadData();
    }
    else
    {
        t_transmitQueue *retransmitNode = retransmitHead;
        while( retransmitNode )
        {
            if( a_packet->m_transmitID == retransmitNode->m_transmitPacket.m_transmitID )
            {
                if( retransmitNode == retransmitCurrent )
                {
                    stopTimer( TRANSMIT_WAIT_ACK_EVENT, SINGLE_TIMER );
                }
                retransmitFreePacket(retransmitNode);
            }
            retransmitNode = retransmitNode->m_next;
        }
    }
}

/*****************************************************************
* DESCRIPTION: packetBeacon
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void packetBeacon( t_beaconPacket *a_packet )
{
    if( notifyTask )
    {
        loraEnterStandby();
        xTaskNotify( notifyTask, a_packet->m_panId, eSetValueWithOverwrite );
        notifyTask = NULL;
    }
}

/*****************************************************************
* DESCRIPTION: packetJoinRequest
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void packetJoinRequest( t_joinRequestPacket *a_packet )
{
    dataDecode( a_packet->m_keyNum, a_packet->m_securityKey,  SECURITY_KEY_LEN);
    if( memcmp( a_packet->m_securityKey, securityKey, SECURITY_KEY_LEN ) == 0 )
    {
        transmitJoinResponse( a_packet->m_srcAddr.addrMode, a_packet->m_srcAddr.addr.m_dstMacAddr, true );
    }
    else
    {
        transmitJoinResponse( a_packet->m_srcAddr.addrMode, a_packet->m_srcAddr.addr.m_dstMacAddr, false );
    }
}

/*****************************************************************
* DESCRIPTION: packetJoinResponse
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void packetJoinResponse( t_joinResponsePacket *a_packet )
{
    if( nwkAttribute.m_nwkStatus == true )
    {
        return;
    }
    if( a_packet->m_joinSuccess == true )
    {
#ifdef SELF_ORGANIZING_NETWORK
        nwkAttribute.m_shortAddr = a_packet->m_shortAddr;
#endif
        nwkAttribute.m_nwkStatus = true;
        setNetworkStatus( NETWORK_DEVICE );
        nwkAttributeSave();
    }
    else
    {
        leaveNetwork();
    }
    clearTimer( NETWORK_JOIN_NWK_EVENT, RELOAD_TIMER );
}

/****************************************************** END OF FILE ******************************************************/
