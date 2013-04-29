/*
***********************************************
 * NAME	    : Elan668.h
***********************************************
*/
#ifndef __ELAN668_H__
#define __ELAN668_H__

#include <mach/gpio.h>
#include "ioc_f668.h"

#define Elan_Set_VDD					gpio_set_value(IOC_POWER_GPIO, 1)
#define Elan_Clr_VDD					gpio_set_value(IOC_POWER_GPIO, 0)
#define Elan_Read_VDD_Status	gpio_get_value(IOC_POWER_GPIO)

#define Elan_Set_RST				  gpio_set_value(IOC_RST_GPIO, 0)
#define Elan_Clr_RST					gpio_set_value(IOC_RST_GPIO, 1)
#define Elan_Read_RST_Status  gpio_get_value(IOC_RST_GPIO)

#define Elan_Set_DAT					gpio_set_value(UP_IOC_DATA_GPIO, 1)
#define Elan_Clr_DAT					gpio_set_value(UP_IOC_DATA_GPIO, 0)
#define Elan_Read_DAT_Status  gpio_get_value(UP_IOC_DATA_GPIO)

#define Elan_Set_CLK					gpio_set_value(UP_IOC_CLK_GPIO, 1)
#define Elan_Clr_CLK					gpio_set_value(UP_IOC_CLK_GPIO, 0)
#define Elan_Read_CLK_Status	gpio_get_value(UP_IOC_CLK_GPIO)

#endif


