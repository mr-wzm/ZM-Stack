/*****************************************************************
* Copyright (C) 2017 60Plus Technology Co.,Ltd.*
******************************************************************
* transmit.h
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
#ifndef TRANSMIT_H
#define TRANSMIT_H
#ifdef __cplusplus
extern "C"
{
#endif
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/    
/* BE */
#define MAC_MIN_BE                  3
#define MAC_MAX_BE                  6
/* NB */
#define MAC_MIN_NB                  0
#define MAC_MAX_NB                  5
/* CW */
#define MAC_VALUE_CW                1//2
/* Retransmit number */
#define MAC_RETRANSMIT_NUM          10
/* Num of broadcast times */
#define BROADCAST_MAX_NUM           5

/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/
/*
 * Add mode
 */
typedef enum
{
    pointAddr16Bit = 1,
    pointAddr64Bit,
    broadcastAddr,
}E_addrMode;

/*
 * cmd type
 */
typedef enum
{
    ERR_ORDER = 0U,
    DATA_ORDER,
    ACK_ORDER,
    BEACON_ORDER,
    JOIN_REQUEST_ORDER,
    JOIN_RESPONSE_ORDER,
    ANNONCE_ORDER,
    LEAVE_ORDER,
    //RTS_ORDER,
    //CTS_ORDER,
}E_cmdType;

/*
 * Transmit type.
 */
typedef enum
{
    NO_TRANS = 0,
    T_TRANSMIT,
    T_RETRANSMIT,
}E_transmitType;

/*
 * Addr type
 */
typedef struct
{
  union
  {
    uint16_t        m_dstShortAddr;
    ZLongAddr_t     m_dstMacAddr;
  } addr;
  E_addrMode        addrMode;
}t_addrType;

/*
 * CSMA parameters.
 */
typedef struct
{
    uint8_t     NB;
    uint8_t     CW;
    uint8_t     BE;
    //uint8_t     retransmit;
}t_macCsmaParm;

/*
 * RTS and CTS.
 */
typedef struct
{
    uint16_t        m_panId;
    E_addrMode      m_addrMode;
    E_cmdType       m_cmdType;
    uint16_t        m_dstAddr;
    uint16_t        m_srcAddr;
    uint16_t        m_duration;
}t_shakeHandsPacket;

/*
 * Ack packet.
 */
typedef struct
{
    uint16_t        m_panId;
    t_addrType      m_dstAddr;
    E_cmdType       m_cmdType;
    uint16_t        m_srcAddr;
    uint8_t         m_transmitID;
}t_ackPacket;

/*
 * Beacon packet.
 */
typedef struct
{
    uint16_t        m_panId;
    t_addrType      m_dstAddr;
    E_cmdType       m_cmdType;
}t_beaconPacket;


/*
 * Join request packet.
 */
typedef struct
{
    uint16_t        m_panId;
    t_addrType      m_dstAddr;
    E_cmdType       m_cmdType;
    t_addrType      m_srcAddr;
    uint8_t         m_keyNum;
    uint8_t         m_securityKey[SECURITY_KEY_LEN];
}t_joinRequestPacket;

/*
 * Join Response packet.
 */
typedef struct
{
    uint16_t        m_panId;
    t_addrType      m_dstAddr;
    E_cmdType       m_cmdType;
    uint16_t        m_srcAddr;
    bool            m_joinSuccess;
#ifdef SELF_ORGANIZING_NETWORK
    uint16_t        m_shortAddr;
#endif
}t_joinResponsePacket;
    
/*
 * Device annonce packet.
 */
typedef struct
{
    uint16_t        m_panId;
    t_addrType      m_dstAddr;
    E_cmdType       m_cmdType;
    t_addrType      m_srcAddr;
    uint16_t        m_shortAddr;
}t_annoncePacket;

/*
 * Ask the device to leave the network packet.
 */
typedef struct
{
    uint16_t        m_panId;
    t_addrType      m_dstAddr;
    E_cmdType       m_cmdType;
    uint16_t        m_srcAddr;
}t_leavePacket;

/*
 * Transmit command queue.
 */
typedef struct T_transmitCommandQueue
{
    struct T_transmitCommandQueue       *m_next;
    void                                *m_packet;
    uint8_t                              m_size;
    E_cmdType                            m_cmdType;
}t_transmitCommandQueue;

/*
 * Transmit packet.
 */
typedef struct
{
    uint16_t        m_panId;
    t_addrType      m_dstAddr;
    E_cmdType       m_cmdType;
    uint16_t        m_srcAddr;
    uint8_t         m_transmitID;
    uint8_t         m_keyNum;
    uint8_t         m_size;
    uint8_t         m_data[0];
}t_transmitPacket;

/*
 * Transmit queue.
 */
typedef struct T_transmitQueue
{
    struct T_transmitQueue      *m_next;
    uint8_t                      m_retransmit;
    t_transmitPacket             m_transmitPacket;
}t_transmitQueue;
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
extern t_macCsmaParm                macPIB;
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/

/*****************************************************************
* DESCRIPTION: transmitTx
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
E_typeErr transmitTx( t_addrType *a_dstAddr, uint8_t a_size, uint8_t *a_data );
/*****************************************************************
* DESCRIPTION: transmitRx
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
E_cmdType transmitRx( t_transmitPacket *a_packet );
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
t_transmitQueue * getTransmitHeadPacket( void );
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
t_transmitQueue * getRetransmitCurrentPacket( void );
/*****************************************************************
* DESCRIPTION: transmitSendData
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
E_transmitType transmitSendData( void );
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
void setTransmitType( E_transmitType a_type );
/*****************************************************************
* DESCRIPTION: transmitRetransmit
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void transmitRetransmit( E_transmitType a_transmitType );
/*****************************************************************
* DESCRIPTION: transmitSendCommand
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
bool transmitSendCommand( void );
/*****************************************************************
* DESCRIPTION: transmitFreeHeadData
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void transmitFreeHeadData( void );
/*****************************************************************
* DESCRIPTION: retransmitFreeCurrentPacket
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void retransmitFreePacket( t_transmitQueue *a_transmitFree );
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
void scanBeaconMessage( TaskHandle_t a_notifyTask );
/*****************************************************************
* DESCRIPTION: sensingBeaconMessage
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void scanBeaconMessage( TaskHandle_t a_notifyTask );
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
uint8_t getAvoidtime( void );
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
void checkTransmitQueue( void );
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
void transmitBeacon( void );
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
void transmitJoinRequest( void );
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
void loraDeleteDevice( t_addrType *a_dstAddr );

#ifdef __cplusplus
}
#endif
#endif /* transmit.h */
