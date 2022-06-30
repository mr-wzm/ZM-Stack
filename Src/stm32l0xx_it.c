/**
  ******************************************************************************
  * @file    stm32l0xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2019 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"
#include "stm32l0xx.h"
#include "stm32l0xx_it.h"
#include "cmsis_os.h"

/* USER CODE BEGIN 0 */
/*************************************************************************************************************************
 *                                                       INCLUDES                                                        *
 *************************************************************************************************************************/
#include "loraConfig.h"
#include "network.h"
#include "OStask.h"
#include "zigbee.h"
#include "lora.h"
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
#ifdef SYSTEM_LOW_POWER_STOP
extern bool rtcWakeUpActive;

extern void sysCompleteTick( void );
#endif
/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern RTC_HandleTypeDef hrtc;
extern DMA_HandleTypeDef hdma_usart4_rx;
extern DMA_HandleTypeDef hdma_usart4_tx;
extern UART_HandleTypeDef huart4;

/******************************************************************************/
/*            Cortex-M0+ Processor Interruption and Exception Handlers         */ 
/******************************************************************************/

/**
* @brief This function handles Non maskable Interrupt.
*/
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */

  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
* @brief This function handles Hard fault interrupt.
*/
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
      /* Reset */
      resetSystem();
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
  /* USER CODE BEGIN HardFault_IRQn 1 */

  /* USER CODE END HardFault_IRQn 1 */
}

/**
* @brief This function handles System tick timer.
*/
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */
    
  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  osSystickHandler();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32L0xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32l0xx.s).                    */
/******************************************************************************/

/**
* @brief This function handles RTC global interrupt through EXTI lines 17, 19 and 20 and LSE CSS interrupt through EXTI line 19.
*/
void RTC_IRQHandler(void)
{
  /* USER CODE BEGIN RTC_IRQn 0 */
    if(__HAL_RTC_WAKEUPTIMER_GET_FLAG(&hrtc, RTC_FLAG_WUTF) != RESET)
    {
        __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
    }
    if( __HAL_RTC_WAKEUPTIMER_EXTI_GET_FLAG() )
    {
        __HAL_RTC_WAKEUPTIMER_EXTI_CLEAR_FLAG();

        HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
#ifdef SYSTEM_LOW_POWER_STOP
        if(rtcWakeUpActive == true)
        {
            rtcWakeUpActive = false;
            sysCompleteTick();
        }
        else
        {
            rtcWakeUpActive = true;
        }
#endif
    }
  /* USER CODE END RTC_IRQn 0 */
  HAL_RTC_AlarmIRQHandler(&hrtc);
  /* USER CODE BEGIN RTC_IRQn 1 */

  /* USER CODE END RTC_IRQn 1 */
}

/**
* @brief This function handles EXTI line 4 to 15 interrupts.
*/
void EXTI4_15_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI4_15_IRQn 0 */
   
  /* USER CODE END EXTI4_15_IRQn 0 */
  if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_12) != RESET)
  {
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_12);
    /* USER CODE BEGIN LL_EXTI_LINE_12 */
    xTaskNotifyFromISR( loraTaskHandle, (uint32_t)LORA_NOTIFY_TRANSMIT_DONE, eSetBits, NULL );
    taskYIELD();
    /* USER CODE END LL_EXTI_LINE_12 */
  }
  /* USER CODE BEGIN EXTI4_15_IRQn 1 */
   
  /* USER CODE END EXTI4_15_IRQn 1 */
}

/**
* @brief This function handles DMA1 channel 2 and channel 3 interrupts.
*/
void DMA1_Channel2_3_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel2_3_IRQn 0 */
  if(__HAL_DMA_GET_FLAG(&hdma_usart4_tx, __HAL_DMA_GET_TC_FLAG_INDEX(&hdma_usart4_tx)))
  {
      uartDmaSendDone();
  }
  /* USER CODE END DMA1_Channel2_3_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart4_rx);
  HAL_DMA_IRQHandler(&hdma_usart4_tx);
  /* USER CODE BEGIN DMA1_Channel2_3_IRQn 1 */

  /* USER CODE END DMA1_Channel2_3_IRQn 1 */
}

/**
* @brief This function handles USART4 and USART5 interrupt.
*/
void USART4_5_IRQHandler(void)
{
  /* USER CODE BEGIN USART4_5_IRQn 0 */
  if( __HAL_UART_GET_IT(&huart4, UART_IT_IDLE) )
  {
      /* Stop dma transmit */
      HAL_UART_DMAStop(&huart4);
      /* Send notify to task */
      xTaskNotify( networkTaskHandle, NETWORK_NOFITY_UART_RX_DONE, eSetBits );
      /* Clear IDLE interrupt flag */
      __HAL_UART_CLEAR_IT(&huart4, UART_FLAG_IDLE);
  }
  /* USER CODE END USART4_5_IRQn 0 */
  HAL_UART_IRQHandler(&huart4);
  /* USER CODE BEGIN USART4_5_IRQn 1 */

  /* USER CODE END USART4_5_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
