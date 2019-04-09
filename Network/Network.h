/*****************************************************************
* Copyright (C) 2017 60Plus Technology Co.,Ltd.*
******************************************************************
* Network.h
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
#ifndef NETWORK_H
#define NETWORK_H
#ifdef __cplusplus
extern "C"
{
#endif
    
#include <stdbool.h>
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
/* network task priority. */
#define NETWORK_TASK_PRIORITY           5
/* network task depth. */
#define NETWORK_TASK_DEPTH              256

    
    
    
/******************* Lora Task Notify Events **********************/
/**
 * Min :BV(0)
 * Max :BV(31)
 */
#define NETWORK_NOFITY_INIT_START                           BV(0)
#define NETWORK_NOFITY_INIT_SUCCESS                         BV(1)
#define NETWORK_NOFITY_UART_RX_DONE                         BV(2)
/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/

/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/
typedef enum
{
    NETWORK_HOLD,
    NETWORK_INIT,
    NETWORK_FIND_CHANNEL,
    NETWORK_BUILD,
    NETWORK_COOR,
    NETWORK_JOIN_SCAN,
    NETWORK_JOIN_REQUEST,
    NETWORK_DEVICE,
    NETWORK_EXIT,
}E_nwkStatus;


//#pragma pack(1) //One byte alignment




//#pragma pack() //The end of the alignment
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
extern TaskHandle_t                    networkTaskHandle;
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
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
void networkInit( void );
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
void networkProcess( void *parm );
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
E_nwkStatus getNetworkStatus( void );
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
void setNetworkStatus( E_nwkStatus a_status );

#ifdef __cplusplus
}
#endif
#endif /* Network.h */
