/*****************************************************************
* Copyright (C) 2017 60Plus Technology Co.,Ltd.*
******************************************************************
* attribute.c
*
* DESCRIPTION:
*     Network attribute1
* AUTHOR:
*     Ziming
* CREATED DATE:
*     2018/12/21
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
#include <stdlib.h>
#include <string.h>
#include "loraConfig.h"
#include "OS_flash.h"
#include "iwdg.h"
#include "attribute.h"
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
#ifdef SELF_ORGANIZING_NETWORK
#define MAC_BASE_ADDRESS_1            0x1ff80050
#define MAC_BASE_ADDRESS_2            0x1ff80054
#define MAC_BASE_ADDRESS_3            0x1ff80064
#endif
/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                   GLOBAL VARIABLES                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
t_nwkAtt            nwkAttribute;
t_deviceList        *deviceList;
/*************************************************************************************************************************
 *                                                    LOCAL VARIABLES                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                 FUNCTION DECLARATIONS                                                 *
 *************************************************************************************************************************/
static void     readMacAddr( void );
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                    LOCAL FUNCTIONS                                                    *
 *************************************************************************************************************************/

/*****************************************************************
* DESCRIPTION: EEP_Init
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void initEEP( void )
{
    /* Read network attribute. */
    eepSysNwkAttRead();
    /* Read mac address */
    readMacAddr();
    
#ifdef DEVICE_TYPE_COOR
    /* Read device list if it is coor */
    eepSysDeviceListRead();
#endif
}

/*****************************************************************
* DESCRIPTION: readMacAddr
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void readMacAddr( void )
{
#ifdef SELF_ORGANIZING_NETWORK
    uint8_t temp = 0;
    for( uint8_t offset = 0; offset < 3; offset++ )
    {
        for( uint8_t count = 0; count < 4; count++ )
        {
            if( offset < 2 )
            {
                nwkAttribute.m_mac[temp++] = (uint8_t)(*(uint32_t *)(MAC_BASE_ADDRESS_1 + offset*4) >> (24 - count*8));
            }
            else
            {
                nwkAttribute.m_mac[temp++] = (uint8_t)(*(uint32_t *)(MAC_BASE_ADDRESS_3) >> (24 - count*8));
            }
        }
    }
#endif
}

/*****************************************************************
* DESCRIPTION: eepSysNwkAttRead
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void eepSysNwkAttRead( void )
{
    memset( &nwkAttribute, 0, sizeof(t_nwkAtt) );
    flashReadData( NETWORK_ATTRIBUTE_ADDR, (uint8_t *)&nwkAttribute, sizeof(t_nwkAtt)/sizeof(uint8_t) );
}

/*****************************************************************
* DESCRIPTION: eepSysDeviceListRead
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void eepSysDeviceListRead( void )
{
#ifdef DEVICE_TYPE_COOR
    if( nwkAttribute.m_nwkStatus == true )
    {
        if( nwkAttribute.m_panId != 0x0000 && nwkAttribute.m_shortAddr == 0x0000 && nwkAttribute.m_deviceNum)
        {
            uint32_t pAddr = DEVICE_LIST_ADDR;
            uint8_t deviceNum = nwkAttribute.m_deviceNum;
            deviceList = (t_deviceList *)pvPortMalloc(sizeof(t_deviceList));
            if( deviceList )
            {
                t_deviceList *deviceListNode = deviceList;
                flashReadData( DEVICE_LIST_ADDR, (uint8_t *)deviceListNode, sizeof(t_deviceList)/sizeof(uint8_t) );
                while( --deviceNum )
                {
                    pAddr += sizeof(t_deviceList);
                    t_deviceList *deviceListNew = (t_deviceList *)pvPortMalloc(sizeof(t_deviceList));
                    flashReadData( pAddr, (uint8_t *)deviceListNew, sizeof(t_deviceList)/sizeof(uint8_t) );
                    deviceListNode->m_next = deviceListNew;
                    deviceListNode = deviceListNew;
                }
            }
        }
    }
#endif
}
/*****************************************************************
* DESCRIPTION: eepSysNwkAttSave
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void eepSysNwkAttSave( void )
{
    t_nwkAtt nwkAttributeOld;
    flashReadData( NETWORK_ATTRIBUTE_ADDR, (uint8_t *)&nwkAttributeOld, sizeof(t_nwkAtt)/sizeof(uint8_t) );
    if( memcmp(&nwkAttributeOld, &nwkAttribute, sizeof(t_nwkAtt)) != 0 )
    {
        flashWriteData( NETWORK_ATTRIBUTE_ADDR, (uint8_t *)&nwkAttribute, sizeof(t_nwkAtt)/sizeof(uint8_t) );
    }
}
/*****************************************************************
* DESCRIPTION: eepSysDeviceListSave
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void eepSysDeviceListSave( void )
{
#ifdef DEVICE_TYPE_COOR
    if( deviceList )
    {
        uint32_t pAddr = DEVICE_LIST_ADDR;
        uint8_t deviceNum = nwkAttribute.m_deviceNum;
        t_deviceList *deviceListNode = deviceList;
        t_deviceList deviceNodeInfo;
        while( deviceNum-- )
        {
            //memset( &deviceNodeInfo, 0, sizeof(t_deviceList) );
            flashReadData( pAddr, (uint8_t *)&deviceNodeInfo, sizeof(t_deviceList)/sizeof(uint8_t) );
            if( memcmp(&deviceNodeInfo, deviceListNode, sizeof(t_deviceList)) != 0 )
            {
                flashWriteData( pAddr, (uint8_t *)deviceListNode, sizeof(t_deviceList)/sizeof(uint8_t) );
            }
            deviceListNode = deviceListNode->m_next;
            pAddr += sizeof(t_deviceList);
        }
    }
#endif
}
/*****************************************************************
* DESCRIPTION: increaseNewDevice
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
uint16_t increaseNewDevice( uint8_t *a_macAddr, bool a_lowPower )
{
    uint16_t shortAddr = 0xFFFF;
#ifdef DEVICE_TYPE_COOR
    t_deviceList *deviceListNode = deviceList;
    while( deviceListNode )
    {
        if( memcmp( deviceListNode->m_mac, a_macAddr, MAC_ADDR_LEN) == 0 )
        {
            if( deviceListNode->m_lowPowerDevice != a_lowPower )
            {
                deviceListNode->m_lowPowerDevice = a_lowPower;
                eepSysDeviceListSave();
            }
            return deviceListNode->m_shortAddr;
        }
        if( deviceListNode->m_next )
        {
            deviceListNode = deviceListNode->m_next;
        }
        else
        {
            break;
        }
    }
    if( deviceList == NULL || deviceListNode->m_next == NULL )
    {
        t_deviceList *deviceListNew = (t_deviceList *)pvPortMalloc(sizeof(t_deviceList));
        if( deviceListNew )
        {
            memcpy( deviceListNew->m_mac, a_macAddr, MAC_ADDR_LEN);
            deviceListNew->m_shortAddr = generatedRand();
            deviceListNew->m_lowPowerDevice = a_lowPower;
            deviceListNew->m_next = NULL;
            if( deviceList == NULL )
            {
                deviceList = deviceListNew;
            }
            else
            {
                deviceListNode->m_next = deviceListNew;
            }
            nwkAttribute.m_deviceNum++;
            eepSysNwkAttSave();
            eepSysDeviceListSave();
            shortAddr = deviceListNew->m_shortAddr;
        }
    }
#endif
    return shortAddr;
}
/*****************************************************************
* DESCRIPTION: deleteDevice
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void deleteDevice( t_deviceList *a_deleteNode )
{
#ifdef DEVICE_TYPE_COOR
    if( a_deleteNode )
    {
        t_deviceList *deleteNode = a_deleteNode;
        t_deviceList *deviceListNode = deviceList;
        
        while( deviceListNode )
        {
            if( deviceListNode->m_next == deleteNode )
            {
                deviceListNode = deleteNode->m_next;
                nwkAttribute.m_deviceNum--;
                vPortFree(deleteNode);
                eepSysNwkAttSave();
                eepSysDeviceListSave();
            }
            deviceListNode = deviceListNode->m_next;
        }
    }
#endif
}


/*****************************************************************
* DESCRIPTION: getDeviceAttWithShortAddr
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
t_deviceList * getDeviceAttWithShortAddr( uint16_t a_shortAddr )
{
    t_deviceList *deviceListNode = deviceList;
    
    while( deviceListNode )
    {
        if( a_shortAddr == deviceListNode->m_shortAddr )
        {
            return deviceListNode;
        }
        deviceListNode = deviceListNode->m_next;
    }
    return deviceListNode;
}

/*****************************************************************
* DESCRIPTION: getDeviceAttWithMac
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
t_deviceList * getDeviceAttWithMac( ZLongAddr_t a_mac )
{
    t_deviceList *deviceListNode = deviceList;
    
    while( deviceListNode )
    {
        if( memcmp( deviceListNode->m_mac, a_mac, MAC_ADDR_LEN) == 0 )
        {
            return deviceListNode;
        }
        deviceListNode = deviceListNode->m_next;
    }
    return deviceListNode;
}


/*****************************************************************
* DESCRIPTION: nwkAttributeErase
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void nwkAttributeErase( void )
{
    uint8_t size = 0;
    if( sizeof(t_nwkAtt)%sizeof(uint32_t) )
    {
        size = 1;
    }
    flashEraseData( NETWORK_ATTRIBUTE_ADDR, sizeof(t_nwkAtt)/sizeof(uint32_t) + size );
}


/*****************************************************************
* DESCRIPTION: resetAttribute
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void resetAttribute( void )
{
     memset( &nwkAttribute, 0, sizeof(t_nwkAtt) );
     nwkAttributeErase();
}

/*****************************************************************
* DESCRIPTION: generatedRand
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
int generatedRand( void )
{
    static uint16_t basicIndex = 0;
    
    if( basicIndex == 0 )
    {
        for( uint8_t count = 0; count < MAC_ADDR_LEN; count++ )
        {
            basicIndex += nwkAttribute.m_mac[count];
        }
    }
    srand(HAL_GetTick()+basicIndex);
    
    return rand();
}
/****************************************************** END OF FILE ******************************************************/
