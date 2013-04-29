#include <common.h>
#include <command.h>
#include <i2c.h>
#include <twl4030.h>
#include <off_mode.h>
#include <hardware_version.h>

#define mdelay(n) ({ unsigned long msec = (n); while (msec--) udelay(1000); })

// handle the special case, press power key 8s to shutdon device 
#define BOOTUP_8S_OFF	(BOOTUP_BATTERY_DETECT|BOOTUP_POWER_KEY)
#define BOOTUP_8S_OFF_CABLE	(BOOTUP_BATTERY_DETECT|BOOTUP_POWER_KEY|BOOTUP_CABLE_IN)
#define BOOTUP_8S_OFF_RTC	(BOOTUP_BATTERY_DETECT|BOOTUP_POWER_KEY|BOOTUP_RTC)

#if CONFIG_MAX17043_GAUGE
#define POWER_ON_VOLTAGE_NO_CABLE 350000
#define POWER_ON_VOLTAGE_WITH_AC_CABLE 330000
#define POWER_ON_VOLTAGE_WITH_USB_CABLE 350000
#elif CONFIG_TWL4030_MADC_VBAT
#define POWER_ON_VOLTAGE 3600
#endif

static struct off_mode_info off_mode_charging_info = {
	.bootup_reason = BOOTUP_UNKNOWN,
	.vcell = 0,
	.soc = 0,
	.is_reach_100_percentage = 0,
	.is_charging_complete = 0,
	.CableInType = 0, 
	.cable_exist = 1,
	.write_model_false = 0,
	.charging_fault = 1,
	.battery_info = BATTERY_ID_UNKNOWN,
	.enter_reduce_frequencce = 0,
};

//&*&*&*HC1_20110902, Add key light notification
#if CONFIG_KEY_LIGHT_NOTIFICATION
extern int zoom_pwm_init(void);
extern void zoom_pwm_enable(int enable);
extern void zoom_pwm_config(u8 brightness);

//
// times: always on -> 0, valid range -> 1-10, off -> -1
//
void key_light_flash(int times)
{
	static int init = 0;
	int i;
	gpio_t *gpio3_base = (gpio_t *)OMAP34XX_GPIO3_BASE;
	
	if (init == 0)
	{
		zoom_pwm_init();

		if (times == 0)
			zoom_pwm_config(100);			
		else
			zoom_pwm_config(40);

		init = 1;
	}

	if ((times < -1) || (times > 10))
		return;

	/** Enable KEY_LIGHT_EN GPIO_91 **/
	sr32((u32)&gpio3_base->oe, 27, 1, 0);

	zoom_pwm_enable(1);

	if (times == 0)
	{
		sr32((u32)&gpio3_base->setdataout, 27, 1, 1);	
		return;
	}
	else if (times == -1)
	{
		sr32((u32)&gpio3_base->res2[0], 27, 1, 0); 
	}
	else
	{
		for (i=0; i<times; i++)
		{
			sr32((u32)&gpio3_base->setdataout, 27, 1, 1);	
			mdelay(300);
			sr32((u32)&gpio3_base->res2[0], 27, 1, 0); 
			mdelay(300);
		}

	}	

	zoom_pwm_enable(0);

}// end of - key_light_flash -
#endif //CONFIG_KEY_LIGHT_NOTIFICATION
//&*&*&*HC2_20110902, Add key light notification

//&*&*&*HC1_20110503, Adjust long press time (ref. bsp spec v0.6)
#if CONFIG_PWRKEY_LONGPRESS
#define PWRKEY_PRESS_TIME_STARTUP 1
#define PWRKEY_PRESS_TIME_CHARGING 2
// power key long press check
int pwrkey_press_check(u32 sec, int boot_reason)
{
	int ret = 1;
	int i;
	int step = 250;
	int time_unit = 1000;

	//printf("S pwrkey_press_check(%d)...\n", sec);

	// limit the long press check time		
	if (sec < 0 || sec > 10)
		return 0;

	if (boot_reason == BOOTUP_POWER_KEY)
	{
		
		if (sec%2)
			time_unit = 500;			
		else
			sec/=2;	

		step = 500;
	}	

	for (i=0; i<sec*time_unit; i+=step)
	{
		mdelay(step);

		// verify the STS_PWON bit (active high with pwr key press)	
		if ((twl4030_hw_sts_get() & 0x1))
			continue;
		else
		{
			ret = 0;
			break;
		}	
	}
	return ret;	
}
#endif // CONFIG_PWRKEY_LONGPRESS
//&*&*&*HC2_20110503, Adjust long press time (ref. bsp spec v0.6)

void pre_charging_configure()
{
	gpio_t *gpio2_base = (gpio_t *)OMAP34XX_GPIO2_BASE;
	gpio_t *gpio3_base = (gpio_t *)OMAP34XX_GPIO3_BASE;

	printf("pre_charging_configure...\n");
	
	// turn off usb power (1.5V, 1.8V and 3.1V)
	twl4030_usb_ldo_off();

	if(g_BoardID == BOARD_ID_EVT2)
	{
		/** Disable LCD_PWR_EN GPIO_47 **/
		sr32((u32)&gpio2_base->oe, 15, 1, 0);
		sr32((u32)&gpio2_base->res2[0], 15, 1, 0); 	
	}
	else
	{
		/** Disable LCD_PWR_EN GPIO_88 **/
		sr32((u32)&gpio3_base->oe, 24, 1, 0);
		sr32((u32)&gpio3_base->res2[0], 24, 1, 0); 
	}
}

void post_charging_configure()
{
	gpio_t *gpio2_base = (gpio_t *)OMAP34XX_GPIO2_BASE;
	gpio_t *gpio3_base = (gpio_t *)OMAP34XX_GPIO3_BASE;

	printf("post_charging_configure...\n");

	if(g_BoardID == BOARD_ID_EVT2)
	{
		/** Enable LCD_PWR_EN GPIO_47 **/
		sr32((u32)&gpio2_base->oe, 15, 1, 0);
		sr32((u32)&gpio2_base->setdataout, 15, 1, 1);
	}
	else
	{
		/** Enable LCD_PWR_EN GPIO_88 **/
		sr32((u32)&gpio3_base->oe, 24, 1, 0);
		sr32((u32)&gpio3_base->setdataout, 24, 1, 1);
	}
}

int off_mode_custom_model_retry(void)
{
	union power_supply_propval BatteryVol = {0,};
	int ret = 0;

	printf("\n S off_mode_custom_model_retry\n");

	charging_mode(CHARGING_STOP);
	mdelay(2000);
	ds278x_battery_get_property(POWER_SUPPLY_PROP_VOLTAGE_NOW, &BatteryVol);

	if (BatteryVol.intval != 1)
	{
		ret = max17043_write_custom_model();
		mdelay(500);	
		max17043_quick_start();
		mdelay(1000);
	}
	else
		ret = 1;

	printf("\n E off_mode_custom_model_retry, ret=%d\n", ret);

	return ret;	
}

void charging_led_notification(int chg, int soc)
{
	static int led_flash;
	if (chg)
		chg_led_set(1);
	else
	{
		if(led_flash == 0)
		{
			chg_led_set(1);
			led_flash = 1;
		}
		else
		{
			chg_led_set(0);
			led_flash = 0;
		}
	}
		
}

void off_mode_charging( void )
{
	int bootup_reason = BOOTUP_UNKNOWN;
#if CONFIG_MAX17043_GAUGE
	union power_supply_propval BatteryVol = {0,};
	union power_supply_propval BatteryCap = {0,};
	int model_count = 0;
#elif CONFIG_TWL4030_MADC_VBAT
	int BatteryCap = 0, BatteryVol = 0;
#endif

#ifdef CONFIG_DISCHARGING_EXPERIMENTATION
	int quick_once = 1;
#endif

#if CONFIG_MAX17043_GAUGE
	select_bus(1, 100);	
	off_mode_charging_info.write_model_false = max17043_write_custom_model();
	mdelay(500);
	//#ifndef CONFIG_DISCHARGING_EXPERIMENTATION
	//if(!off_mode_charging_info.write_model_false)
	//{
		//max17043_quick_start();
		//mdelay(1000);	
	//}
	//#endif
#elif CONFIG_TWL4030_MADC_VBAT
	twl4030_vadc_onoff(1);
	twl4030_madc_init();
#endif

	bootup_reason = off_mode_charging_info.bootup_reason;
	
	while (1)
	{
#if CONFIG_MAX17043_GAUGE
		// select I2C2
		select_bus(1, 100);	
		ds278x_battery_get_property(POWER_SUPPLY_PROP_VOLTAGE_NOW, &BatteryVol);
		ds278x_battery_get_property(POWER_SUPPLY_PROP_CAPACITY, &BatteryCap);
		if (off_mode_charging_info.write_model_false)
		{
			model_count++;
			if ((model_count%2) == 0)
			{
				off_mode_charging_info.write_model_false = off_mode_custom_model_retry();
			}
		}
#elif CONFIG_TWL4030_MADC_VBAT
		BatteryVol = twl4030_get_battery_voltage();
		printf("**** Off Mode Charging(from MADC) *****\n");
		printf("****      VOLTAGE: %d   ********\n", BatteryVol);
		printf("****  End Of Off Mode Charging  ****\n");
#endif
		//mdelay(1000);
		// select I2C1
		select_bus(0, 100);		

		off_mode_charging_info.cable_exist = dc_ok_detect();
		#ifdef CONFIG_DISCHARGING_EXPERIMENTATION
		off_mode_charging_info.cable_exist = 0;
		#endif

		if ((twl4030_bootup_reason_get() & BOOTUP_POWER_KEY) || (bootup_reason & BOOTUP_POWER_KEY))
		{

//&*&*&*HC1_20110503, Adjust long press time (ref. bsp spec v0.6)
//&*&*&*HC1_20110428, enable pwr key long press function 
#if CONFIG_PWRKEY_LONGPRESS
			if ((bootup_reason != BOOTUP_POWER_KEY) && (!pwrkey_press_check(PWRKEY_PRESS_TIME_CHARGING, bootup_reason)))
			{
				bootup_reason = BOOTUP_UNKNOWN;
				continue;		
			}
#endif // CONFIG_PWRKEY_LONGPRESS			
//&*&*&*HC2_20110428, enable pwr key long press function 
//&*&*&*HC2_20110503, Adjust long press time (ref. bsp spec v0.6)

#if (defined(CONFIG_TWL4030_MADC_VBAT) || defined(CONFIG_MAX17043_GAUGE))
			//&*&*&*AL1_20110528, different voltage between cable exist and not.
			
#if CONFIG_MAX17043_GAUGE
			if((BatteryVol.intval < POWER_ON_VOLTAGE_NO_CABLE) && (off_mode_charging_info.cable_exist == 1))
			{
				bootup_reason = BOOTUP_UNKNOWN;
				#if CONFIG_KEY_LIGHT_NOTIFICATION
				key_light_flash(3);
				#endif // CONFIG_KEY_LIGHT_NOTIFICATION
				printf("Battery voltage < 3.5V with no cable, can't boot up !!!\n");
			}	
			else if((off_mode_charging_info.CableInType == CABLE_AC) && (off_mode_charging_info.cable_exist == 0))
			{
				if(BatteryVol.intval < POWER_ON_VOLTAGE_WITH_AC_CABLE)
				{
					bootup_reason = BOOTUP_UNKNOWN;
					#if CONFIG_KEY_LIGHT_NOTIFICATION
					key_light_flash(3);
					#endif // CONFIG_KEY_LIGHT_NOTIFICATION
					printf("Battery voltage < 3.3V with AC cable, can't boot up !!!\n");
				}
				else
					break;
			}
			else if((off_mode_charging_info.CableInType == CABLE_USB) && (off_mode_charging_info.cable_exist == 0))
			{
				if(BatteryVol.intval < POWER_ON_VOLTAGE_WITH_USB_CABLE)
				{
					bootup_reason = BOOTUP_UNKNOWN;
					#if CONFIG_KEY_LIGHT_NOTIFICATION
					key_light_flash(3);
					#endif // CONFIG_KEY_LIGHT_NOTIFICATION
					printf("Battery voltage < 3.5V with USB cable, can't boot up !!!\n");
				}
				else
					break;
			}
			//&*&*&*AL2_20110528, different voltage between cable exist and not.
#elif CONFIG_TWL4030_MADC_VBAT
			if(BatteryVol < POWER_ON_VOLTAGE)
			{
				bootup_reason = BOOTUP_UNKNOWN;
				printf("Battery voltage < 3.6V with no cable, can't boot up !!!\n");
			}	
#endif
			else
			{
				#if CONFIG_KEY_LIGHT_NOTIFICATION
				 // power on, keep key light on
				key_light_flash(0);
				#endif // CONFIG_KEY_LIGHT_NOTIFICATION
				break;
			}	
#else
			printf("No battery voltage checking implementation! !!\n");
			break;
#endif
//&*&*&*HC2_20110103, enable the battery check
//&*&*&*HC2_20101230, disable the battery check
		}
		if(off_mode_charging_info.cable_exist == 1)
		{
			printf("Not a vaild long press !!! Please press and hold pwr key more than 1 sec.\n");
			twl4030_poweroff();
			break;
		}
		else
		{

			if (off_mode_charging_info.CableInType == CABLE_UNKNOW)
			{
				off_mode_charging_info.CableInType = twl4030_get_cable_type();
				if(off_mode_charging_info.CableInType == USB_EVENT_VBUS)
				{
					off_mode_charging_info.CableInType = CABLE_USB;
					printf("\n **** USB Cable: Start 500mA**** \n");
				}
				else
				{
					off_mode_charging_info.CableInType = CABLE_AC;
					printf("\n **** AC Cable: Start 1500mA**** \n");
				}
				
				#if CONFIG_CHARGING_REDUCE_SYS_LOAD
				pre_charging_configure();			
				change_cpu_freq(0);
				change_mem_freq(0);		
				off_mode_charging_info.enter_reduce_frequencce = 1;
				#endif //CHARGING_REDUCE_SYS_LOAD
			}
			if(off_mode_charging_info.CableInType == CABLE_AC)
				charging_mode(CHARGING_START_1000MA);
			else
				charging_mode(CHARGING_START_500MA);
		}

		#ifdef CONFIG_MAX17043_DEBUG_MESSAGE_FORMAT
		select_bus(1, 100);
		max1704x_register_dump();
		mdelay(5000);
		#endif

		mdelay(1000);

		// control charging LED
		off_mode_charging_info.is_charging_complete = chg_status_detect();
		charging_led_notification(off_mode_charging_info.is_charging_complete, BatteryCap.intval);
		off_mode_charging_info.charging_fault = chg_fault_detect();

		/* wait for redeuce frequence to get accurate capacity */
		#ifdef CONFIG_DISCHARGING_EXPERIMENTATION
		if(!off_mode_charging_info.write_model_false && quick_once)
		{
			select_bus(1, 100);
			max17043_quick_start();
			mdelay(1000);	
			quick_once = 0;
		}
		//if(BatteryCap.intval == 2)
		//{
			//select_bus(0, 100);
			//twl4030_poweroff();
			//break;
		//}
		#endif

		if(off_mode_charging_info.charging_fault == 0)
		{
			printf("\n **** charging_fault:%d, charging time out, re-toggle CEN pin.***** \n", off_mode_charging_info.charging_fault);
			charging_mode(CHARGING_STOP);
		}
		
		printf("\n");
	}	
}

int off_mode_start( void )
{
	printf(" +++ off_mode_start +++\n");
	int bootup_reason;

	off_mode_charging_info.battery_info = g_BatteryID;
	printf(" #### Battery ID: %d ####\n", off_mode_charging_info.battery_info);
	off_mode_charging_info.bootup_reason = twl4030_bootup_reason_get();
	bootup_reason = off_mode_charging_info.bootup_reason;
	
	if (bootup_reason == BOOTUP_8S_OFF)
	{
		printf("[pwr key 8s]:  ");
		bootup_reason = BOOTUP_POWER_KEY;
	}
	else if (bootup_reason == BOOTUP_8S_OFF_CABLE)
	{
		printf("[pwr key 8s + cable]:  ");
		bootup_reason = BOOTUP_CABLE_IN;		
	}
//&*&*&*HC1_20110727, Add rtc bootup check 		
	else if (bootup_reason == BOOTUP_8S_OFF_RTC)
	{
		printf("[pwr key 8s + rtc]:  ");
		bootup_reason = BOOTUP_RTC;		
	}
//&*&*&*HC2_20110727, Add rtc bootup check 	
	
	// select I2C0
	select_bus(0, 100);

	if (bootup_reason & BOOTUP_POWER_KEY)
	{
		printf("power key detect...\n");		
		
//&*&*&*HC1_20110503, Adjust long press time (ref. bsp spec v0.6)		
//&*&*&*HC1_20110428, enable pwr key long press function 
#if CONFIG_PWRKEY_LONGPRESS
		if (pwrkey_press_check(PWRKEY_PRESS_TIME_STARTUP, bootup_reason))
			bootup_reason = BOOTUP_POWER_KEY;
		else
			bootup_reason = BOOTUP_UNKNOWN;
#endif // #if CONFIG_PWRKEY_LONGPRESS		
//&*&*&*HC2_20110428, enable pwr key long press function 
//&*&*&*HC2_20110503, Adjust long press time (ref. bsp spec v0.6)
		off_mode_charging_info.bootup_reason  = bootup_reason;
		off_mode_charging();
	}
	else if (bootup_reason &  BOOTUP_CABLE_IN)
	{
		printf("cable detect...\n");
		off_mode_charging_info.bootup_reason  = bootup_reason;
		off_mode_charging();
	}
	else if (bootup_reason & BOOTUP_BATTERY_DETECT)
	{
		printf("battery detect...\n");
		#ifdef CONFIG_DISCHARGING_EXPERIMENTATION
		off_mode_charging();
		#else
		twl4030_poweroff();
		#endif
	}
//&*&*&*HC1_20110727, Add rtc bootup check 		
	else if (bootup_reason & BOOTUP_RTC)
	{
		printf("rtc alarm...\n");
		twl4030_alarm_it_enable(0);
		twl4030_poweroff();
	}
//&*&*&*HC2_20110727, Add rtc bootup check 			
	else
	{
//&*&*&*HC1_20110221, let reboot pass the check 	
		printf("reboot ...\n");		
//&*&*&*HC2_20110221, let reboot pass the check 			
	}

#if CONFIG_CHARGING_REDUCE_SYS_LOAD
	//if (bootup_reason &  BOOTUP_CABLE_IN)
	if(off_mode_charging_info.enter_reduce_frequencce)
	{
		change_mem_freq(1);
		change_cpu_freq(1);
		post_charging_configure();
	}
	printf("end of uBoot boot_reason :%d...\n", bootup_reason);	
#endif //CHARGING_REDUCE_SYS_LOAD	
	return 0;

}

int do_offmode (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (argc == 2 && strcmp (argv[1], "start") == 0)
		off_mode_start();
			
	return 0;
	
}

U_BOOT_CMD(
	offmode, CFG_MAXARGS, 1,	do_offmode,
	"offmode battery- get battery information\n",
);

//&*&*&*SJ1_20110704, add battery barcode function.
int do_write_LicoID (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *pMem = (char *)(0x80000000);
	
	memset(pMem, 0, 512);
	sprintf(pMem, "EAL105123456");
	run_command("mmcinit 1; mmc 1 write 80000000 2dffe00 200", 0);
	
	return 0;
	
}

U_BOOT_CMD(
	write_lico_id, CFG_MAXARGS, 1,	do_write_LicoID,
	"write_lico_id      - Write Lico Battery ID\n",
);

int do_write_McnairID (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *pMem = (char *)(0x80000000);
	
	memset(pMem, 0, 512);
	sprintf(pMem, "EAM105123456");
	run_command("mmcinit 1; mmc 1 write 80000000 2dffe00 200", 0);

	return 0;
	
}

U_BOOT_CMD(
	write_mcnair_id, CFG_MAXARGS, 1,	do_write_McnairID,
	"write_mcnair_id    - Write Mcnair Battery ID\n",
);

int do_write_EmptyID (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *pMem = (char *)(0x80000000);
	
	memset(pMem, 0, 512);
	run_command("mmcinit 1; mmc 1 write 80000000 2dffe00 200", 0);

	return 0;
	
}

U_BOOT_CMD(
	write_empty_battid, CFG_MAXARGS, 1,	do_write_EmptyID,
	"write_empty_battid - Write Empty Battery ID\n",
);
//&*&*&*SJ2_20110704, add battery barcode function. 
