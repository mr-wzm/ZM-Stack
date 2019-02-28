/*****************************************************************
* Copyright (C) 2017 60Plus Technology Co.,Ltd.*
******************************************************************
* flash.c
*
* DESCRIPTION:
*     Flash resd write
* AUTHOR:
*     Ziming
* CREATED DATE:
*     2018/12/13
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
#include "OS_flash.h"
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
 
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
 
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
* DESCRIPTION: flashReadData
*     
* INPUTS:
*     
* OUTPUTS:
*     E_typeErr
* NOTE:
*     null
*****************************************************************/
E_typeErr flashReadData( uint16_t a_addr, uint8_t *a_data, uint8_t a_size )
{
    if( !a_data || !a_size || a_addr > 0x0FFF )
    {
        return E_ERR;
    }
    uint8_t *pData;
    /* Load start addr */
    pData = (uint8_t *)( DATA_EEPROM_BASE + a_addr );
    
    while(a_size--)
    {
        *a_data++ = *pData++;
    }
    return E_SUCCESS;
}

/*****************************************************************
* DESCRIPTION: flashWriteData
*     
* INPUTS:
*     
* OUTPUTS:
*     E_typeErr
* NOTE:
*     null
*****************************************************************/
E_typeErr flashWriteData( uint16_t a_addr, uint8_t *a_data, uint8_t a_size )
{
    if( !a_data || !a_size || a_addr > 0x0FFF )
    {
        return E_ERR;
    }
    __disable_interrupt();
    // Unlock eeprom
    HAL_FLASHEx_DATAEEPROM_Unlock();
    
    while( a_size-- )
    {
        /* Write data in eeprom */
        HAL_FLASHEx_DATAEEPROM_Program( FLASH_TYPEPROGRAMDATA_BYTE, DATA_EEPROM_BASE + a_addr++, *a_data++ );
    }
    // lock eeprom
    HAL_FLASHEx_DATAEEPROM_Lock();
    
    __enable_interrupt();
    
    return E_SUCCESS;
}



/*****************************************************************
* DESCRIPTION: flashEraseData
*     
* INPUTS:
*     
* OUTPUTS:
*     E_typeErr
* NOTE:
*     擦除的地址必须为4的倍数，一次擦除4个字节
*****************************************************************/
E_typeErr flashEraseData( uint16_t a_addr, uint8_t a_size )
{
    if( a_addr > 0x0FFF || (a_addr%4) || !a_size )
    {
        return E_ERR;
    }
    
    uint16_t addrStep = 0;
    HAL_FLASHEx_DATAEEPROM_Unlock();
    while( a_size-- )
    {
        HAL_FLASHEx_DATAEEPROM_Erase( DATA_EEPROM_BASE + a_addr + addrStep );
        addrStep += EEPROM_ADDR_STEP;
    }
    HAL_FLASHEx_DATAEEPROM_Lock();
    return E_SUCCESS;
}

 
/****************************************************** END OF FILE ******************************************************/
