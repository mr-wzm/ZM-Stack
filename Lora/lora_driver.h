/*****************************************************************
* Copyright (C) 2017 60Plus Technology Co.,Ltd.*
******************************************************************
* lora_driver.h
*
* DESCRIPTION:
*     Lora driver
* AUTHOR:
*     Ziming
* CREATED DATE:
*     2018/11/26
* REVISION:
*     v0.1
*
* MODIFICATION HISTORY
* --------------------
* $Log:$
*
*****************************************************************/
#ifndef LORA_DRIVER_H
#define LORA_DRIVER_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "sx1276-LoRa.h"
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
/* Buffer size max */
#define BUFFER_SIZE_MAX                 256
/* Frequency range  */
#define LORA_FREQUENCY_MIN              730000000
#define LORA_FREQUENCY_MAX              860000000
#define LORA_FREQUENCY_STEP              20000000
/* Channel max number */
#define LORA_CHANNEL_MAX                6
     
/* Lora timeout value(ms) */
#define LORA_TIMEOUT_VALUE              200
     
/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/
    
/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/

    
/**
* @brief  Radio driver callback functions
* @note  Insure callback functions are SMALL, FAST and SAFE that would been invoked by ISR. 
*/
typedef struct
{
    void (*TxDone )(void); /*!< Tx Done callback prototype. */
    void (*TxTimeout)(void); /*!< Tx Timeout callback prototype. */
    /*!
     * \brief Rx Done callback prototype.
     * \param [IN] size    Received buffer size
     * \param [IN] rssi    RSSI value computed while receiving the frame [dBm]
     * \param [IN] snr    Raw SNR value given by the radio hardware, LoRa: SNR value in dB
     */
    void (*RxDone)(uint8_t *a_data, uint16_t a_size);
    void (*RxTimeout )(void); /*!< Rx Timeout callback prototype. */
    void (*RxError )(void); /*!< Rx Error callback prototype. */
    /*!
     * \brief CAD Done callback prototype.
     * \param [IN] detected    TRUE=channel activity, FALSE=channel empty. 
     */
    void (*CadDone)(uint8_t detected);
    void (*CadTimeout)(void); /*!< CAD Timeout callback prototype. */
} t_radioCallBack;

/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/

     
     
/*****************************************************************
* DESCRIPTION: loraDriverInit
*     
* INPUTS:
*     null
* OUTPUTS:
*     null
* RETURNS:
*     null
* NOTE:
*     Initialize lora driver setting
*****************************************************************/
void loraDriverInit( void );
/*****************************************************************
* DESCRIPTION: loraRegisterCallback
*     
* INPUTS:
*     a_radioCallback
* OUTPUTS:
*     null
* RETURNS:
*     null
* NOTE:
*     Register callback function
*****************************************************************/
void loraRegisterCallback( t_radioCallBack  *a_radioCallback );
/*****************************************************************
* DESCRIPTION: getLoraStatus
*     
* INPUTS:
*     null
* OUTPUTS:
*     tRFLRStates
* NOTE:
*     null
*****************************************************************/
tRFLRStates getLoraStatus( void );
/*****************************************************************
* DESCRIPTION: getLoraSnr
*     
* INPUTS:
*     null
* OUTPUTS:
*     snr
* RETURNS:
*     null
* NOTE:
*     null
*****************************************************************/
int8_t getLoraSnr( void );
/*****************************************************************
* DESCRIPTION: getLoraRssi
*     
* INPUTS:
*     null
* OUTPUTS:
*     lora rssi
* RETURNS:
*     null
* NOTE:
*     null
*****************************************************************/
double getLoraRssi( void );
/*****************************************************************
* DESCRIPTION: loRaSetFrequency
*     
* INPUTS:
*     
* OUTPUTS:
*     
* NOTE:
*     null
*****************************************************************/
E_typeErr loRaSetFrequency( uint32_t a_freq );
/*****************************************************************
* DESCRIPTION: loRaGetFrequency
*     
* INPUTS:
*     null
* OUTPUTS:
*     frequency
* NOTE:
*     null
*****************************************************************/
uint32_t loRaGetFrequency( void );
/*****************************************************************
* DESCRIPTION: loraEnterStandby
*     
* INPUTS:
*     null
* OUTPUTS:
*     null
* NOTE:
*     null
*****************************************************************/
void loraEnterStandby( void );
/*****************************************************************
* DESCRIPTION: loraEnterSleep
*     
* INPUTS:
*     null
* OUTPUTS:
*     null
* NOTE:
*     null
*****************************************************************/
void loraEnterSleep( void );
/*****************************************************************
* DESCRIPTION: loraSendData
*     
* INPUTS:
*     a_data:
*     a_size: 
* OUTPUTS:
*     null
* RETURNS:
*     null
* NOTE:
*     null
*****************************************************************/
E_typeErr loraSendData( uint8_t *a_data, uint8_t a_size );
/*****************************************************************
* DESCRIPTION: loraReceiveData
*     
* INPUTS:
*     null
* OUTPUTS:
*     null
* RETURNS:
*     E_typeErr
* NOTE:
*     Lora receive data
*****************************************************************/
E_typeErr loraReceiveData( void );
/*****************************************************************
* DESCRIPTION: loraEnterCAD
*     
* INPUTS:
*     null
* OUTPUTS:
*     null
* RETURNS:
*     null
* NOTE:
*     Enter CAD mode
*****************************************************************/
E_typeErr loraEnterCAD( void );
/*****************************************************************
* DESCRIPTION: sx1276Dio0Handler
*     
* INPUTS:
*     null
* OUTPUTS:
*     null
* RETURNS:
*     null
* NOTE:
*     Dio0 interrupt
*****************************************************************/
void loraDoneHandler( void );
/*****************************************************************
* DESCRIPTION: loraTimeoutHandler
*     
* INPUTS:
*     null
* OUTPUTS:
*     nul
* NOTE:
*     null
*****************************************************************/
void loraTimeoutHandler( void );


#ifdef __cplusplus
}
#endif
#endif /* lora_driver.h */
