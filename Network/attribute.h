/*****************************************************************
* Copyright (C) 2017 60Plus Technology Co.,Ltd.*
******************************************************************
* attribute.h
*
* DESCRIPTION:
*     Network attribute
* AUTHOR:
*     ziming
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
#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H
#ifdef __cplusplus
extern "C"
{
#endif
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
/* 
 * eeprom save addr 0x0000--0x0FFF 
 */
#define NETWORK_ATTRIBUTE_ADDR      0x0000
#define DEVICE_LIST_ADDR            NETWORK_ATTRIBUTE_ADDR + sizeof(t_nwkAtt)
/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/

     
typedef struct
{
    uint16_t        m_panId;
    
    uint16_t        m_shortAddr;
    
    ZLongAddr_t     m_mac;
    
    uint8_t         m_channelNum;  //0--6;
    
    bool            m_nwkStatus;   //Whether in the network
    
#ifdef DEVICE_TYPE_COOR
    uint8_t         m_deviceNum;
#endif
    
}t_nwkAtt;


typedef struct T_deviceList
{
    uint16_t                m_shortAddr;
    
    ZLongAddr_t             m_mac;
    
    bool                    m_lowPowerDevice;
    
    //bool                    m_isActive;
    
    struct T_deviceList     *m_next;
    
}t_deviceList;

/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
extern t_nwkAtt             nwkAttribute;
extern t_deviceList        *deviceList;
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
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
void initEEP( void );
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
void eepSysNwkAttRead( void );
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
void eepSysDeviceListRead( void );
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
void eepSysNwkAttSave( void );
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
void eepSysDeviceListSave( void );
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
uint16_t increaseNewDevice( uint8_t *a_macAddr, bool a_lowPower );
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
void deleteDevice( t_deviceList *a_deleteNode );
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
t_deviceList * getDeviceAttWithShortAddr( uint16_t a_shortAddr );
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
t_deviceList * getDeviceAttWithMac( ZLongAddr_t a_mac );
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
void nwkAttributeErase( void );
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
void resetAttribute( void );
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
int generatedRand( void );

#ifdef __cplusplus
}
#endif
#endif /* attribute.h */
