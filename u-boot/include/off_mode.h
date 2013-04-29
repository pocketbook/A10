
#ifndef __OFF_MODE_H__
#define __OFF_MODE_H__

/***
 ** Edian - big and little
***/
#define __force	__attribute__((force))
#define __bitwise __attribute__((bitwise))
typedef __u16 __bitwise __be16;
#define __swab16(x) ((__u16)(				\
	(((__u16)(x) & (__u16)0x00ffU) << 8) |			\
	(((__u16)(x) & (__u16)0xff00U) >> 8)))
#define cpu_to_be16(val)	((__force __be16)__swab16((__u16)(val)))
#define be16_to_cpu(val)	(__swab16((__force __u16)(__be16)(val)))

//* Error number
#define	EINVAL		22	/* Invalid argument */

/***
 ** Battery Control
***/

// Slave Address
#define DS2786_CHIP_SLAVE_ADDRESS 0x36
// Battery Register
#define BQ27x00_REG_TEMP		0x06
#define BQ27x00_REG_VOLT		0x08
#define BQ27x00_REG_RSOC		0x0B /* Relative State-of-Charge */
#define BQ27x00_REG_AI			0x14
#define BQ27x00_REG_FLAGS		0x0A

#define MAX17043_REG_VCELL		0x02
#define MAX17043_REG_SOC		  0x04
#define MAX17043_REG_MODE		  0x06 /* Relative State-of-Charge */
#define MAX17043_REG_VERSION	0x08
#define MAX17043_REG_CONFIG		0x0C
#define MAX17043_REG_COMMAND	0xFE

// DS278x
#define DS278x_REG_VOLT_MSB	0x0C
#define DS278x_REG_TEMP_MSB	0x0A
#define DS278x_REG_CURRENT_MSB	0x0E
#define DS2786_REG_RARC		0x02	/* Remaining active relative capacity */

enum charging_supply {
	CABLE_UNKNOW,
	CABLE_AC,
	CABLE_USB,
	
};
enum charging_operation {
	CHARGING_START_100MA,
	CHARGING_START_500MA,
	CHARGING_START_1000MA,
	CHARGING_STOP,
};

enum bootup_reason {
	BOOTUP_UNKNOWN = 0x00,
	BOOTUP_BATTERY_DETECT = 0x40,
	BOOTUP_CABLE_IN = 0x04,
	BOOTUP_POWER_KEY = 0x01,
	BOOTUP_RTC = 0x08,	//&*&*&*HC_20110727, Add rtc bootup check 	
};

enum power_supply_property {
	/* Properties of type `int' */
	POWER_SUPPLY_PROP_PRESENT = 0,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CAPACITY, /* in percents! */
	POWER_SUPPLY_PROP_TEMP,
};
union power_supply_propval {
	int intval;
	const char *strval;
};

struct off_mode_info {
	int bootup_reason;
	int vcell;
	int soc;
	int is_reach_100_percentage;
	int is_charging_complete;
	int CableInType;
	int cable_exist;
	int write_model_false;
	int charging_fault;
	int battery_info;
	int enter_reduce_frequencce;
};

// Export function
extern int ds2786_battery_get_property(enum power_supply_property psp,union power_supply_propval *val);
extern void ds2786_battery_info();
extern void ds2786_gauge_reset(void);
extern int dc_ok_detect(void);
extern int chg_status_detect(void);
extern int chg_fault_detect(void);
extern void chg_led_set(int on);
extern int max17043_write_custom_model( void );

extern void change_mem_freq(u8 mode);
extern void change_cpu_freq(u8 mode);
extern void max1704x_register_dump();

#endif // __OFF_MODE_H__