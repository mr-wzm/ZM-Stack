/*****************************************************************
* Copyright (C) 2017 60Plus Technology Co.,Ltd.*
******************************************************************
* OS_timers.c
*
* DESCRIPTION:
*     OS timers
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
#include "loraConfig.h"
#include "stdlib.h"
#include "lora.h"
#include "OS_timers.h"
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
/*
 * Timer link list head.
 */
static t_timerList                     *timerListHead; 
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
     
/* 
 * Timer process task handle.
 */
TaskHandle_t                    timerTaskHandle;
/*
 * Timer queue handle.
 */
QueueHandle_t                   timerQueueHandle;
/*************************************************************************************************************************
 *                                                    LOCAL VARIABLES                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                 FUNCTION DECLARATIONS                                                 *
 *************************************************************************************************************************/
static void             timerCallback( TimerHandle_t xTimer );
static E_typeErr        addTimer( E_timerEvent a_timerEvent, uint32_t a_time, bool a_reloadTimer, timerCallback_t a_timerCallback );
static E_typeErr        deleteTimer( t_timerList *a_timerList );
static t_timerList *    findTimer( E_timerEvent a_timerEvent, bool a_reloadTimer );
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                    LOCAL FUNCTIONS                                                    *
 *************************************************************************************************************************/

/*****************************************************************
* DESCRIPTION: osTimerInit
*     
* INPUTS:
*     NULL
* OUTPUTS:
*     NULL
* NOTE:
*     Initialize os timer.
*     Create timer link list head.
*****************************************************************/
void osTimerInit( void )
{
    timerListHead = (t_timerList *)pvPortMalloc( sizeof(t_timerList) );
    if( timerListHead == NULL)
    {
        /* malloc faild, flash is bad */
        resetSystem();
    }
    timerListHead->m_timerEvent = LORA_TIMEOUT_EVENT;
    timerListHead->m_timerId = 1;
    timerListHead->m_time = 100;
    timerListHead->m_next = NULL;
    timerListHead->m_timerCallback = NULL;
    timerListHead->m_reloadTimer = false;
    timerListHead->m_timerHandle = xTimerCreate("timerProcess", 
                                                timerListHead->m_time,
                                                ( BaseType_t )false,               //mode
                                                (void *)timerListHead->m_timerId, 
                                                timerCallback);

}

/*****************************************************************
* DESCRIPTION: osTimerProcess
*     
* INPUTS:
*     parm : no used
* OUTPUTS:
*     NULL
* NOTE:
*     Deal with
*****************************************************************/
void osTimerProcess( void *parm )
{
    
    while(1)
    {
        uint8_t timerId;
        bool delFlag = true;
            
        t_timerList *timerListNode = timerListHead;
        /* Wait timer notify */
        xQueueReceive( timerQueueHandle, &timerId, portMAX_DELAY );
        /* Find the time with ID */
        while( timerListNode )
        {
            if( timerListNode->m_timerId == timerId )
            {
                break;
            }
            timerListNode = timerListNode->m_next;
        }
        if( timerListNode )
        {
            switch( timerListNode->m_timerEvent )
            {
            case NETWORK_BUILD_EVENT:
                
                break;
            case LORA_TIMEOUT_EVENT:
                xTaskNotify( loraTaskHandle, LORA_NOTIFY_TRANSMIT_TIMEOUT, eSetBits );
                delFlag = false;
                break;
            case TRANSMIT_NB_TIME_EVENT:
                delFlag = false;
                break;
            case LORA_ALLOW_JOIN_TIME_EVENT:
                clearTimer( NETWORK_BEACON_EVENT, ALL_TYPE_TIMER );
                break;
            case TRANSMIT_WAIT_ACK_EVENT:
                delFlag = false;
                break;
            default:
                break;
            }
            /* Call the callback function if it was registered */
            if( timerListNode->m_timerCallback )
            {
                timerListNode->m_timerCallback();
            }
            /* Is it a single timer */
            if( delFlag && timerListNode->m_reloadTimer == false )
            {
                /* Delete timer */
                deleteTimer( timerListNode );
            }
            taskYIELD();
        }
    }
}


/*****************************************************************
* DESCRIPTION: startSingleTimer
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
E_typeErr startSingleTimer( E_timerEvent a_timerEvent, uint32_t a_time, timerCallback_t a_timerCallback )
{
    if( a_time == 0 )
    {
        return E_ERR;
    }
    /* Add a single timer */
    return addTimer( a_timerEvent, a_time, false, a_timerCallback );
}


/*****************************************************************
* DESCRIPTION: startReloadTimer
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
E_typeErr startReloadTimer( E_timerEvent a_timerEvent, uint32_t a_time, timerCallback_t a_timerCallback )
{
    if( a_time == 0)
    {
        return E_ERR;
    }
    /* Add a period timer */
    return addTimer( a_timerEvent, a_time, true, a_timerCallback );
    
}

/*****************************************************************
* DESCRIPTION: getTimerType
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
E_timerType getTimerType( E_timerEvent a_timerEvent )
{
    if( a_timerEvent == NO_EVENT )
    {
        return NO_TIMER;
    }
    
    E_timerType timerType = NO_TIMER;
    t_timerList *timerListNode = timerListHead;
    
    while( timerListNode )
    {
        /* Find timer with timer event */
        if( timerListNode->m_timerEvent == a_timerEvent )
        {
            if( timerType != NO_TIMER)
            {
                return ALL_TYPE_TIMER;
            }
            timerType = (E_timerType)timerListNode->m_reloadTimer;
        }
        timerListNode = timerListNode->m_next;
    }
    return timerType;
}

/*****************************************************************
* DESCRIPTION: getTimerIsActive
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
bool getTimerIsActive( E_timerEvent a_timerEvent, E_timerType a_timerType )
{
    t_timerList *timerListNode = NULL;
    
    if( a_timerType == SINGLE_TIMER )
    {
        timerListNode = findTimer(a_timerEvent, false);
    }
    else if( a_timerType == RELOAD_TIMER )
    {
        timerListNode = findTimer(a_timerEvent, true);
    }
    if( timerListNode )
    {
        return xTimerIsTimerActive( timerListNode->m_timerHandle );
    }
    /* No have the timer */
    return false;
}
/*****************************************************************
* DESCRIPTION: whichTimerIsActive
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
t_timerActiveList whichTimerIsActive( void )
{
    uint8_t count = 0;
    uint32_t events = 0;
    t_timerList *timerListNode = timerListHead;
    t_timerActiveList activeTimer;
    
    while( timerListNode )
    {
        if( xTimerIsTimerActive(timerListNode->m_timerHandle) )
        {
            count++;
            events |= BV(timerListNode->m_timerEvent);
        }
        timerListNode = timerListNode->m_next;
    }
    if( count )
    {
        activeTimer.m_activeNum = count;
        activeTimer.m_activeList = (uint8_t *)pvPortMalloc(activeTimer.m_activeNum);
        if( activeTimer.m_activeList )
        {
            uint8_t *tempPoint = activeTimer.m_activeList;
            for( uint8_t cnt = 0; count; cnt++ )
            {
                if( events & BV(cnt) )
                {
                    count--;
                    *tempPoint++ = (E_timerEvent)cnt;
                }
            }
        }
    }
    return activeTimer;
}

/*****************************************************************
* DESCRIPTION: startTimer
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
E_typeErr startTimer( E_timerEvent a_timerEvent, E_timerType a_timerType )
{
    E_typeErr   errStatus = E_ERR;
    t_timerList *timerListNode;
    /* Start single timer */
    if( a_timerType == SINGLE_TIMER || a_timerType == ALL_TYPE_TIMER )
    {
        /* Find the timer */
        timerListNode = findTimer(a_timerEvent, false);
        if( timerListNode )
        {
            /* Start the timer */
            xTimerStart( timerListNode->m_timerHandle, 10 );
            errStatus = E_SUCCESS;
        }
    }
    /* Start reload timer */
    if( a_timerType == RELOAD_TIMER || a_timerType == ALL_TYPE_TIMER )
    {
        /* Find the timer */
        timerListNode = findTimer(a_timerEvent, true);
        if( timerListNode )
        {
            /* Start the timer */
            xTimerStart( timerListNode->m_timerHandle, 10 );
            errStatus = E_SUCCESS;
        }
    }
    return errStatus;
}

/*****************************************************************
* DESCRIPTION: resetTimer
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
E_typeErr resetTimer( E_timerEvent a_timerEvent, E_timerType a_timerType )
{
    E_typeErr   errStatus = E_ERR;
    t_timerList *timerListNode;
    /* Reset single timer */
    if( a_timerType == SINGLE_TIMER || a_timerType == ALL_TYPE_TIMER )
    {
        /* Find the timer */
        timerListNode = findTimer(a_timerEvent, false);
        if( timerListNode )
        {
            /* Reset the timer */
            xTimerReset( timerListNode->m_timerHandle, 10 );
            xTimerStart( timerListNode->m_timerHandle, 10 );
            errStatus = E_SUCCESS;
        }
    }
    /* Reset reload timer */
    if( a_timerType == RELOAD_TIMER || a_timerType == ALL_TYPE_TIMER )
    {
        /* Find the timer */
        timerListNode = findTimer(a_timerEvent, true);
        if( timerListNode )
        {
            /* Reset the timer */
            xTimerReset( timerListNode->m_timerHandle, 10 );
            xTimerStart( timerListNode->m_timerHandle, 10 );
            errStatus = E_SUCCESS;
        }
    }
    return errStatus;
}


/*****************************************************************
* DESCRIPTION: stopTimer
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
t_timerList *stopTimer( E_timerEvent a_timerEvent, E_timerType a_timerType )
{
    t_timerList *timerListNode;
    /* Stop single timer */
    if( a_timerType == SINGLE_TIMER || a_timerType == ALL_TYPE_TIMER )
    {
        timerListNode = findTimer(a_timerEvent, false);
        if( timerListNode )
        {
            xTimerStop( timerListNode->m_timerHandle, 10 );
            if( a_timerType == SINGLE_TIMER )
            {
                return timerListNode;
            }
        }
    }
    /* Stop reload timer */
    if( a_timerType == RELOAD_TIMER || a_timerType == ALL_TYPE_TIMER )
    {
        timerListNode = findTimer(a_timerEvent, true);
        if( timerListNode )
        {
            xTimerStop( timerListNode->m_timerHandle, 100 );
            if( a_timerType == RELOAD_TIMER )
            {
                return timerListNode;
            }
        }
    }
    return NULL;
}

/*****************************************************************
* DESCRIPTION: clearTimer
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void clearTimer( E_timerEvent a_timerEvent, E_timerType a_timerType )
{
    t_timerList *timerListNode;
    /* Clear single timer */
    if( a_timerType == SINGLE_TIMER || a_timerType == ALL_TYPE_TIMER )
    {
        timerListNode = stopTimer(a_timerEvent, SINGLE_TIMER);
        deleteTimer( timerListNode );
    }
    /* Clear reload timer */
    if( a_timerType == RELOAD_TIMER || a_timerType == ALL_TYPE_TIMER )
    {
        timerListNode = stopTimer(a_timerEvent, RELOAD_TIMER);
        deleteTimer( timerListNode );
    }
}

/*****************************************************************
* DESCRIPTION: timerProcess
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static void timerCallback( TimerHandle_t xTimer )
{
    uint32_t timerId = (uint32_t)pvTimerGetTimerID( xTimer );
    
    /* Send timer id to timer process */
    xQueueSend( timerQueueHandle, &timerId, 50 );
    
}

/*****************************************************************
* DESCRIPTION: addTimer
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static E_typeErr addTimer( E_timerEvent a_timerEvent, uint32_t a_time, bool a_reloadTimer, timerCallback_t a_timerCallback )
{
    t_timerList *timerListNode;
    /* Find timer */
    timerListNode = findTimer(a_timerEvent, a_reloadTimer);
    /* Whether or not exist the timer */
    if( timerListNode )
    {
        taskENTER_CRITICAL();           //Enter the critical area
        /* Stop the timer */
        xTimerStop( timerListNode->m_timerHandle, 10 );
        /* whether change time */
        if( timerListNode->m_time != a_time )
        {
            timerListNode->m_time = a_time;
            xTimerChangePeriod( timerListNode->m_timerHandle, a_time, 10 );
        }
        /* whether change callback function */
        if( timerListNode->m_timerCallback != a_timerCallback )
        {
            timerListNode->m_timerCallback = a_timerCallback;
        }
        /* Reset and start the timer */
        xTimerReset( timerListNode->m_timerHandle, 10 );
        xTimerStart( timerListNode->m_timerHandle, 10 );
        
        taskEXIT_CRITICAL();            //Exit the critical area
        return E_SUCCESS;
    }
    else //This timer doesn't exist
    {
        timerListNode = timerListHead;
        /* Find the timer at the end */
        while( timerListNode->m_next )
        {
            timerListNode = timerListNode->m_next;
        }
        /* Mallo a new memory */
        t_timerList *timerListNew = (t_timerList *)pvPortMalloc(sizeof(t_timerList));
        /* Whether or not malloc success */
        if( timerListNew )
        {
            taskENTER_CRITICAL();           //Enter the critical area
            
            timerListNew->m_reloadTimer = a_reloadTimer;
            timerListNew->m_time = a_time;
            timerListNew->m_timerEvent = a_timerEvent;
            timerListNew->m_timerCallback = a_timerCallback;
            timerListNew->m_next = NULL;
            timerListNew->m_timerId = timerListNode->m_timerId + 1;
            timerListNew->m_timerHandle = xTimerCreate("timerProcess", 
                                                       timerListNew->m_time,
                                                       ( BaseType_t )a_reloadTimer,               //mode
                                                       (void *)timerListNew->m_timerId, 
                                                       timerCallback);
            
            taskEXIT_CRITICAL();            //Exit the critical area
             /* Create timer success */
            if( timerListNew->m_timerHandle )
            {
                timerListNode->m_next = timerListNew;
                xTimerStart( timerListNew->m_timerHandle, 10 );
                return E_SUCCESS;
            }
            else  //Create timer faild
            {
                vPortFree(timerListNew);
            }
        }
    }
    return E_ERR;
}

/*****************************************************************
* DESCRIPTION: deleteTimer
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static E_typeErr deleteTimer( t_timerList *a_timerList )
{
    if( a_timerList == NULL || a_timerList == timerListHead )
    {
        return E_ERR;
    }
    
    t_timerList *timerListNode = a_timerList;
    t_timerList *timerListEnd = timerListHead;
    /* Delete Timer */
    xTimerDelete( timerListNode->m_timerHandle, 100 );
    while( timerListEnd->m_next != timerListNode )
    {
        timerListEnd = timerListEnd->m_next;
    }
    /* Delete list */
    timerListEnd->m_next = timerListNode->m_next;
    /* Free the link list */
    vPortFree(timerListNode);
    return E_SUCCESS;
}


/*****************************************************************
* DESCRIPTION: findTimer
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
static t_timerList * findTimer( E_timerEvent a_timerEvent, bool a_reloadTimer )
{
    t_timerList *timerListNode = timerListHead;
    /* Find timer */
    while( timerListNode )
    {
        if( timerListNode->m_timerEvent == a_timerEvent && 
            timerListNode->m_reloadTimer == a_reloadTimer )
        {
            break;
        }
        timerListNode = timerListNode->m_next;
    }
    return timerListNode;
}

     
/****************************************************** END OF FILE ******************************************************/
