/*****************************************************************
* Copyright (C) 2017 60Plus Technology Co.,Ltd.*
******************************************************************
* lora.h
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
#ifndef LORA_H
#define LORA_H
#ifdef __cplusplus
extern "C"
{
#endif
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/

/* Lora task priority. */
#define LORA_TASK_PRIORITY          5
/* Lora task depth. */
#define LORA_TASK_DEPTH             512
    


/******************* Lora Task Notify Events **********************/
/**
 * Min :BV(0)
 * Max :BV(31)
 */
#define LORA_NOTIFY_TRANSMIT_START                  BV(0)
#define LORA_NOTIFY_TRANSMIT_DONE                   BV(1)
#define LORA_NOTIFY_TRANSMIT_TIMEOUT                BV(2)
#define LORA_NOTIFY_TRANSMIT_COMMAND                BV(3)
#define LORA_NOTIFY_TRANSMIT_JOIN_REQUEST           BV(4)
#define LORA_NOTIFY_TRANSMIT_BEACON                 BV(5)
#define LORA_NOTIFY_SET_PANID                       BV(6)
/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/

/*
 * Channel status.
 */
typedef struct
{
    bool        m_cts;
    bool        m_rts;
    uint16_t    m_linkAddr;
}t_channelStatus;
/*
typedef struct
{
    uint8_t     *m_data;
    uint8_t      m_size;
}t_dataNotifyPacket;
*/

/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
extern TaskHandle_t                 loraTaskHandle;
extern t_channelStatus              channelStatus;
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
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
void loraInit( void );
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
void loraProcess( void *parm );
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
void loraEnterLowPower( void );
/*****************************************************************
* DESCRIPTION: loraAllowJoinNetwork
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void loraAllowJoinNetwork( uint32_t a_time );
/*****************************************************************
* DESCRIPTION: loraCloseBeacon
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void loraCloseBeacon( void );
/*****************************************************************
* DESCRIPTION: sensingChannel
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void detectionChannel( TaskHandle_t a_notifyTask );

#ifdef __cplusplus
}
#endif
#endif /* lora.h */
