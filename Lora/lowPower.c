/*****************************************************************
* Copyright (C) 2019 60Plus Technology Co.,Ltd.*
******************************************************************
* lowPower.c
*
* DESCRIPTION:
*     Lora low power
* AUTHOR:
*     Ziming
* CREATED DATE:
*     2019/4/13
* REVISION:
*     v0.1
*
* MODIFICATION HISTORY
* --------------------
* $Log:$
* <author>  <time>      <version >  <desc>
* Ziming      2019/4/13   v0.1        Created this file.
*
*****************************************************************/
 
/*************************************************************************************************************************
 *                                                       INCLUDES                                                        *
 *************************************************************************************************************************/
#include "loraConfig.h"
#include "loraConfigLP.h"
#include "rtc.h"
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
#ifdef SYSTEM_LOW_POWER_STOP
#ifndef configSYSTICK_CLOCK_HZ
	#define configSYSTICK_CLOCK_HZ configCPU_CLOCK_HZ
#endif
/* Constants required to manipulate the NVIC. */
#define portNVIC_SYSTICK_CTRL			( * ( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD			( * ( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE	        ( * ( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SYSTICK_ENABLE		    0x00000001
#define portMISSED_COUNTS_FACTOR			( 45UL )

#define SYSTEM_CLOCK_HZ                 32768
#define SYSTEM_WAKEUP_TIME_BASIC        SYSTEM_CLOCK_HZ/16/1000


#define ulTimerCountsForOneTick         configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ
#define ulStoppedTimerCompensation      portMISSED_COUNTS_FACTOR / ( configCPU_CLOCK_HZ / configSYSTICK_CLOCK_HZ )  

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
#ifdef SYSTEM_LOW_POWER_STOP
static TickType_t g_xModifiableIdleTime;
#endif
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
bool rtcWakeUpActive = false;
/*************************************************************************************************************************
 *                                                    LOCAL VARIABLES                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                 FUNCTION DECLARATIONS                                                 *
 *************************************************************************************************************************/
extern void SystemClock_Config(void);
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                    LOCAL FUNCTIONS                                                    *
 *************************************************************************************************************************/
/*****************************************************************
* DESCRIPTION: sysEnterLowPower
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void sysEnterLowPower( uint32_t *a_xModifiableIdleTime )
{
#ifdef SYSTEM_LOW_POWER_STOP
    __HAL_RCC_DMA1_CLK_DISABLE();
    LL_IOP_GRP1_DisableClock(LL_IOP_GRP1_PERIPH_GPIOC);
    LL_IOP_GRP1_DisableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    LL_IOP_GRP1_DisableClock(LL_IOP_GRP1_PERIPH_GPIOB);
    LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_SPI1);
    __HAL_RCC_USART4_CLK_DISABLE();
    *a_xModifiableIdleTime = 0;
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
#else
    (void)*a_xModifiableIdleTime;
#endif
}

/*****************************************************************
* DESCRIPTION: sysExitLowPower
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void sysExitLowPower( uint32_t *a_xExpectedIdleTime )
{
#ifdef SYSTEM_LOW_POWER_STOP
    SystemClock_Config();
    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();
    /* GPIO Ports Clock Enable */
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOC);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
    /* Peripheral clock enable */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);
    /* USART4 clock enable */
    __HAL_RCC_USART4_CLK_ENABLE();
    
    g_xModifiableIdleTime = *a_xExpectedIdleTime;
#else
    (void)*a_xExpectedIdleTime;
#endif
}




#ifdef SYSTEM_LOW_POWER_STOP
/*****************************************************************
* DESCRIPTION: vPortSuppressTicksAndSleep
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
{
	uint32_t ulReloadValue, ulCompleteTickPeriods, ulSysTickCTRL;
	TickType_t xModifiableIdleTime;
    
    /* Stop the SysTick momentarily.  The time the SysTick is stopped for
    is accounted for as best it can be, but using the tickless mode will
    inevitably result in some tiny drift of the time maintained by the
    kernel with respect to calendar time. */
    portNVIC_SYSTICK_CTRL &= ~portNVIC_SYSTICK_ENABLE;
    
    /* Calculate the reload value required to wait xExpectedIdleTime
    tick periods.  -1 is used because this code will execute part way
    through one of the tick periods. */
    ulReloadValue = xExpectedIdleTime - 1UL;
    if( ulReloadValue > ulStoppedTimerCompensation )
    {
        ulReloadValue -= ulStoppedTimerCompensation;
    }
    
    /* Enter a critical section but don't use the taskENTER_CRITICAL()
    method as that will mask interrupts that should exit sleep mode. */
    __disable_interrupt();
    
    /* If a context switch is pending or a task is waiting for the scheduler
    to be unsuspended then abandon the low power entry. */
    if( eTaskConfirmSleepModeStatus() == eAbortSleep )
    {
        /* Restart from whatever is left in the count register to complete
        this tick period. */
        portNVIC_SYSTICK_LOAD = portNVIC_SYSTICK_CURRENT_VALUE;
        
        /* Restart SysTick. */
        portNVIC_SYSTICK_CTRL |= portNVIC_SYSTICK_ENABLE;
        
        /* Reset the reload register to the value required for normal tick
        periods. */
        portNVIC_SYSTICK_LOAD = ulTimerCountsForOneTick - 1UL;
        
        /* Re-enable interrupts - see comments above __disable_interrupt()
        call above. */
        __enable_interrupt();
    }
    else
    {
        /* Disable the write protection for RTC registers */
        __HAL_RTC_WRITEPROTECTION_DISABLE(&hrtc);
        __HAL_RTC_WAKEUPTIMER_ENABLE(&hrtc);
        __HAL_RTC_ALARMA_DISABLE(&hrtc);
        __HAL_RTC_ALARM_DISABLE_IT(&hrtc, RTC_FLAG_ALRAF);
        __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_IT();
        __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_RISING_FALLING_EDGE();
        /* Load next wake up time */
        HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, ulReloadValue*2, RTC_WAKEUPCLOCK_RTCCLK_DIV16);
        /* Enable the write protection for RTC registers */
        __HAL_RTC_WRITEPROTECTION_ENABLE(&hrtc);
        
        /* Sleep until something happens.  configPRE_SLEEP_PROCESSING() can
        set its parameter to 0 to indicate that its implementation contains
        its own wait for interrupt or wait for event instruction, and so wfi
        should not be executed again.  However, the original expected idle
        time variable must remain unmodified, so a copy is taken. */
        xModifiableIdleTime = xExpectedIdleTime;
        configPRE_SLEEP_PROCESSING( &xModifiableIdleTime );
        if( xModifiableIdleTime > 0 )
        {
            __DSB();
            __WFI();
            __ISB();
        }
        configPOST_SLEEP_PROCESSING( &xExpectedIdleTime );
        
        /* Stop SysTick.  Again, the time the SysTick is stopped for is
        accounted for as best it can be, but using the tickless mode will
        inevitably result in some tiny drift of the time maintained by the
        kernel with respect to calendar time. */
        ulSysTickCTRL = portNVIC_SYSTICK_CTRL;
        portNVIC_SYSTICK_CTRL = ( ulSysTickCTRL & ~portNVIC_SYSTICK_ENABLE );
        /* Re-enable interrupts - see comments above __disable_interrupt()
        call above. */
        __enable_interrupt();
        
        if( rtcWakeUpActive == true || __HAL_RTC_WAKEUPTIMER_EXTI_GET_FLAG() != 0 )
        {
            uint32_t ulCalculatedLoadValue;
            
            rtcWakeUpActive = false;
            /* The tick interrupt has already executed, and the SysTick
            count reloaded with ulReloadValue.  Reset the
            portNVIC_SYSTICK_LOAD_REG with whatever remains of this tick
            period. */
            ulCalculatedLoadValue = ulTimerCountsForOneTick - 1UL;
            
            /* Reset the reload register to the value required for normal tick
            periods. */
            portNVIC_SYSTICK_LOAD = ulCalculatedLoadValue;
            
            /* The tick interrupt handler will already have pended the tick
            processing in the kernel.  As the pending tick will be
            processed as soon as this function exits, the tick value
            maintained by the tick is stepped forward by one less than the
            time spent waiting. */
            ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
            /* Restart SysTick so it runs from portNVIC_SYSTICK_LOAD_REG
            again, then set portNVIC_SYSTICK_LOAD_REG back to its standard
            value.  The critical section is used to ensure the tick interrupt
            can only execute once in the case that the reload register is near
            zero. */
            portNVIC_SYSTICK_CURRENT_VALUE = 0UL;
            portENTER_CRITICAL();
            {
                portNVIC_SYSTICK_CTRL |= portNVIC_SYSTICK_ENABLE;
                vTaskStepTick( ulCompleteTickPeriods );
                portNVIC_SYSTICK_LOAD = ulTimerCountsForOneTick - 1UL;
            }
            portEXIT_CRITICAL();
        }
        else
        {
            /*Reset the
            portNVIC_SYSTICK_LOAD_REG with whatever remains of this tick
            period. */
            portNVIC_SYSTICK_LOAD = ulTimerCountsForOneTick - 1UL;
            /* Clear the SysTick count flag and set the count value back to
            zero. */
            portNVIC_SYSTICK_CURRENT_VALUE = 0UL;
            /* Restart SysTick. */
            portNVIC_SYSTICK_CTRL |= portNVIC_SYSTICK_ENABLE;
            
            rtcWakeUpActive = true;
        }
        /* Start systick */
        SysTick_Config( SystemCoreClock / 1000 );
    }
}

/*****************************************************************
* DESCRIPTION: sysCompleteTick
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
void sysCompleteTick( void )
{
    uint32_t ulCompleteTickPeriods, ulCompletedSysTickDecrements;

    /* Something other than the tick interrupt ended the sleep.
    Work out how long the sleep lasted rounded to complete tick
    periods (not the ulReload value which accounted for part
    ticks). */
    ulCompletedSysTickDecrements = ( g_xModifiableIdleTime * ulTimerCountsForOneTick ) - portNVIC_SYSTICK_CURRENT_VALUE;
    
    /* How many complete tick periods passed while the processor
    was waiting? */
    ulCompleteTickPeriods = ulCompletedSysTickDecrements / ulTimerCountsForOneTick;
    
    /* The reload value is set to whatever fraction of a single tick
    period remains. */
    portNVIC_SYSTICK_LOAD = ( ( ulCompleteTickPeriods + 1 ) * ulTimerCountsForOneTick ) - ulCompletedSysTickDecrements;
    
    portENTER_CRITICAL();
    {
        vTaskStepTick( ulCompleteTickPeriods );
    }
    portEXIT_CRITICAL();
}

#endif
/****************************************************** END OF FILE ******************************************************/
