#ifndef __ELAN_INITIAL_F668_H__
#define __ELAN_INITIAL_F668_H__

#include "ioc_f668.h"

extern void Elan_Initial_f668(unsigned short *);

#define Elan_Define_668_DAT_AS_OUTPUT_PIN()		gpio_direction_output(UP_IOC_DATA_GPIO, 1);
#define Elan_Define_668_DAT_AS_INPUT_PIN()		gpio_direction_input(UP_IOC_DATA_GPIO);
#define Elan_Define_668_CLK_AS_OUTPUT_PIN()		gpio_direction_output(UP_IOC_CLK_GPIO, 0);
#define Elan_Define_668_CLK_AS_INPUT_PIN()		gpio_direction_input(UP_IOC_CLK_GPIO);

#define Elan_Define_Delay1us()		udelay(1);
#define Elan_Define_Delay5us()		udelay(5);
#define Elan_Define_Delay10us()		udelay(10);
#define Elan_Define_Delay50us()		udelay(50);
#define Elan_Define_Delay100us()	udelay(100);
#define Elan_Define_Delay500us()	udelay(500);
#define Elan_Define_Delay1ms()		mdelay(1);
#define Elan_Define_Delay5ms()		mdelay(5);
#define Elan_Define_Delay100ms()	mdelay(100);
#define Elan_Define_Delay500ms()	mdelay(500);

#endif /* __ELAN_INITIAL_F668_H__ */
