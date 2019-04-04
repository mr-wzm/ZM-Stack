/*****************************************************************
* Copyright (C) 2019 60Plus Technology Co.,Ltd.*
******************************************************************
* zigbee.c
*
* DESCRIPTION:
*     Communication with zigbee
* AUTHOR:
*     Ziming
* CREATED DATE:
*     2019/4/2
* REVISION:
*     v0.1
*
* MODIFICATION HISTORY
* --------------------
* $Log:$
* <author>  <time>      <version >  <desc>
* Ziming      2019/4/2   v0.1        Created this file.
*
*****************************************************************/
 
/*************************************************************************************************************************
 *                                                       INCLUDES                                                        *
 *************************************************************************************************************************/
#include "loraConfig.h"
#include <string.h>
#include "network.h"
#include "transmit.h"
#include "attribute.h"
#include "zigbee.h"
#include "usart.h"
#include "dma.h"
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
static t_zigbeeUartStruct       g_uartWork;
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
extern DMA_HandleTypeDef hdma_usart4_rx;
extern DMA_HandleTypeDef hdma_usart4_tx;
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
* DESCRIPTION: zigbeeUartInit
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void zigbeeUartInit( void )
{
    g_uartWork.m_uartBusy = false;
    /* Clear IDLE interrupt flag, Prevents an interrupt 
       from entering after initialization */
    __HAL_UART_CLEAR_IT(&huart4, UART_FLAG_IDLE);
    /* Open uart rx */
    zigbeeUartStartReceive();
}

/*****************************************************************
* DESCRIPTION: zigbeeUartStartReceive
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
E_typeErr zigbeeUartStartReceive( void )
{
    if( g_uartWork.m_uartBusy == true )
    {
        /* Uart dma is busy */
        return E_ERR;
    }
    /* Initialization data buffer */
    memset(g_uartWork.m_data, 0, LORA_BUFFER_SIZE_MAX);
    /* Open uart dma receive */
    HAL_UART_Receive_DMA(&huart4, g_uartWork.m_data, LORA_BUFFER_SIZE_MAX);
    /* Enable uart IDLE interrupt */
    __HAL_UART_ENABLE_IT(&huart4, UART_IT_IDLE);
    /* Disable dma receive channel TC interrupt */
    __HAL_DMA_DISABLE_IT(&hdma_usart4_rx, DMA_IT_TC);
    /* Disable dma receive channel HT interrupt */
    __HAL_DMA_DISABLE_IT(&hdma_usart4_rx, DMA_IT_HT);
    
    return E_SUCCESS;
}

/*****************************************************************
* DESCRIPTION: zigbeeUartSend
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
E_typeErr zigbeeUartSend( uint8_t *a_data, uint16_t a_size )
{
    if( g_uartWork.m_uartBusy || a_data == NULL || a_size == 0 || a_size > LORA_BUFFER_SIZE_MAX )
    {
        return E_ERR;
    }
    g_uartWork.m_uartBusy = true;
    g_uartWork.m_dataSize = a_size;
    memcpy( g_uartWork.m_data, a_data, g_uartWork.m_dataSize );
    HAL_UART_Transmit_DMA(&huart4, g_uartWork.m_data, g_uartWork.m_dataSize);
    return E_SUCCESS;
}

/*****************************************************************
* DESCRIPTION: uartDmaSendDone
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void uartDmaSendDone( void )
{
    if(__HAL_DMA_GET_FLAG(&hdma_usart4_tx, __HAL_DMA_GET_TC_FLAG_INDEX(&hdma_usart4_tx)))
    {
        g_uartWork.m_uartBusy = false;
        /* Stop dma transmit */
        HAL_UART_DMAStop(&huart4);

        zigbeeUartStartReceive();
    }
}

/*****************************************************************
* DESCRIPTION: uartReceiveDone
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void uartReceiveDone( void )
{
    t_addrType dstAddr;
    
    /* Get data size */
    g_uartWork.m_dataSize = (LORA_BUFFER_SIZE_MAX - __HAL_DMA_GET_COUNTER(&hdma_usart4_rx));
    
    dstAddr.addrMode = pointAddr16Bit;
    dstAddr.addr.m_dstShortAddr = 0x0000;
    transmitTx( &dstAddr, g_uartWork.m_dataSize, g_uartWork.m_data );
    
    zigbeeUartStartReceive();
}

/****************************************************** END OF FILE ******************************************************/
