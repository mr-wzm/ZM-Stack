/*****************************************************************
* Copyright (C) 2017 60Plus Technology Co.,Ltd.*
******************************************************************
* NwkConfig.h
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
#ifndef NWKCONFIG_H
#define NWKCONFIG_H
#ifdef __cplusplus
extern "C"
{
#endif
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
/* nwkConfig task priority. */
#define NWKCONFIG_TASK_PRIORITY           4
/* nwkConfig task depth. */
#define NWKCONFIG_TASK_DEPTH              64
/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/

/*
 * Network identity.
 */
typedef enum
{
    nwkIdentityNone = 0U,
    nwkIdentityCoor,
    nwkIdentityDevice,
}E_nwkIdentity;
/**
 *
 */
typedef struct
{
    double      m_RSSI;
    uint16_t    m_panId;
}t_rssiValue;
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
extern TaskHandle_t                    nwkConfigTaskHandle;
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
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
void nwkConfigProcess( void *parm );
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
bool findChannel( void );
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
bool joinNetwork( void );
/*****************************************************************
* DESCRIPTION: getNetworkIdentity
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
E_nwkIdentity getNetworkIdentity( void );
/*****************************************************************
* DESCRIPTION: setNetworkIdentity
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void setNetworkIdentity( E_nwkIdentity a_nwkIdentity );
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
void networConfigkStart( void );
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
void allowJoinNetwork( uint32_t a_time );
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
void closeAllowJoinNetwork( void );
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
void leaveNetwork( void );

 
#ifdef __cplusplus
}
#endif
#endif /* NwkConfig.h */
