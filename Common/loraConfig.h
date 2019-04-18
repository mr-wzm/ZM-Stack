/*****************************************************************
* Copyright (C) 2017 60Plus Technology Co.,Ltd.*
******************************************************************
* loraConfig.h
*
* DESCRIPTION:
*     Lora config
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
#ifndef LORACONFIG_H
#define LORACONFIG_H
#ifdef __cplusplus
extern "C"
{
#endif
/*************************************************************************************************************************
 *                                                        INCLUDES                                                         *
 *************************************************************************************************************************/
#include <stdbool.h>
#include "stm32l0xx_hal.h"
#include "main.h"
#include "cmsis_os.h"
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/

#ifdef DEVICE_TYPE_COOR
#ifndef SELF_ORGANIZING_NETWORK
#error "If you need use DEVICE_TYPE_COOR please define SELF_ORGANIZING_NETWORK!"
#endif
#endif

#if defined(DEVICE_TYPE_COOR) && defined(DEVICE_TYPE_DEVICE)
#error "DEVICE_TYPE_COOR and DEVICE_TYPE_DEVICE can only choose one!"
#endif
    
#define __FUNCTION_POSSIBLE_UNUSED    __attribute__ ((unused))

#ifndef BV
#define BV(x)       (1 << (x))
#endif
    
/* MAC addr len */
#ifdef SELF_ORGANIZING_NETWORK
#define MAC_ADDR_LEN        12
#else
#define MAC_ADDR_LEN        8
#endif

/* Security key len */
#define SECURITY_KEY_LEN            16
/* Lora max buffer size */
#define LORA_BUFFER_SIZE_MAX        255
/* Lora preamble length */
#define LORA_PREAMBLE_LENGTH        10
/* Low power mode preamble length */
#define LORA_PREAMBLE_LENGTH_LP     1000


/*
 * ULONG_MAX
 */
#ifndef ULONG_MAX
#define ULONG_MAX          0xFFFFFFFF
#endif

/**
 * Get max value.
 */
#define maxValue( a, b )        ( (a) > (b)?(a):(b) )
/**
 * Get min value.
 */
#define minValue( a, b )        ( (a) < (b)?(a):(b) )
/**
 * Reset system.
 */
#define resetSystem()           do{ portDISABLE_INTERRUPTS();\
                                    NVIC_SystemReset();}while(-1)

/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/
/*
 * Addr type.
 */
typedef uint8_t ZLongAddr_t[MAC_ADDR_LEN];

/*
 * success or faild
 */ 
typedef enum
{
    E_ERR = 0,
    E_SUCCESS,
}E_typeErr;
    
    
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/

#ifdef __cplusplus
}
#endif
#endif /* loraConfig.h */
