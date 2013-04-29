/*
 * (C) Copyright 2010 FOXCONN
 * Author : aimar liu <aimar.ts.liu@foxconn.com>
 * 
 */

#include <common.h>
#include <exports.h>
#include <asm/types.h>
#include <off_mode.h>
#include <command.h>
#include <malloc.h>
#include <devices.h>
#include <asm/io.h>
#include <asm-arm/arch-omap3/mux.h>
#include <hardware_version.h>

typedef unsigned long  uint32;
typedef long           int32;
typedef unsigned short uint16;
typedef short          int16;
typedef unsigned char  uint8;
typedef signed char    int8;

// use 591_1_051111.INI.txt for LiCo battery pack
char lico_data0[] = {0xA2,0x30,0xAE,0x60,0xB0,0x00,0xB2,0x30,
         0xB3,0xC0,0xB4,0x90,0xB5,0x00,0xB6,0x20};

char lico_data1[] = {0xB6,0x70,0xB8,0xD0,0xBA,0x90,0xBD,0xF0,
         0xC0,0xA0,0xC4,0x10,0xC7,0xD0,0xD0,0xB0};

char lico_data2[] = {0x04,0x00,0x28,0x00,0x16,0x20,0x23,0x00,
         0x30,0x00,0x8A,0xD0,0x1F,0xA0,0x6F,0xE0};

char lico_data3[] = {0x12,0xD0,0x21,0x90,0x0E,0xF0,0x11,0xD0,
         0x0F,0xB0,0x0F,0xF0,0x0B,0x00,0x0B,0x00};

// use 590_1_051111.INI.txt for McNair battery pack
char mcnair_data0[] = {0x85,0xB0,0xB4,0xE0,0xB8,0x20,0xB9,0xA0,
		 0xBC,0x00,0xBC,0x70,0xBD,0x00,0xBD,0x60};

char mcnair_data1[] = {0xBE,0x10,0xBF,0xA0,0xC1,0x60,0xC4,0x70,
	     0xC6,0x10,0xC7,0xE0,0xCB,0x40,0xD0,0xC0};

char mcnair_data2[] = {0x00,0x70,0x0F,0xE0,0x1C,0xE0,0x1B,0xF0,
		 0x59,0x60,0x64,0xF0,0x6F,0x20,0x46,0x60};

char mcnair_data3[] = {0x28,0xF0,0x1B,0xD0,0x1A,0x50,0x18,0x90,
	     0x16,0xC0,0x12,0xA0,0x10,0xF0,0x10,0xF0};

// use 622_2_081011.INI.txt for LiCo battery pack
char lico_new_data0[] = {0x87,0x70,0xAD,0xB0,0xAE,0xA0,0xB3,0xD0,
         0xB7,0x40,0xBA,0xC0,0xBC,0x10,0xBC,0xD0};

char lico_new_data1[] = {0xBD,0x20,0xBD,0x90,0xBE,0xE0,0xC1,0xB0,
         0xC4,0x80,0xC5,0xD0,0xCA,0xF0,0xD0,0x70};

char lico_new_data2[] = {0x00,0x80,0x1E,0x70,0x10,0x00,0x0E,0x20,
         0x0C,0x00,0x2E,0x00,0x52,0x00,0x68,0x90};

char lico_new_data3[] = {0x4F,0xE0,0x2A,0xB0,0x15,0xC0,0x15,0xC0,
         0x1E,0xF0,0x0E,0xF0,0x10,0xB0,0x10,0xB0};



static int ds278x_read(u8 reg, int *rt_value, int b_single)
{
	int ret;
	int alen = 1;
	int lineBytes = 2;
	if(b_single)
		lineBytes = 1;
	ret = i2c_read(DS2786_CHIP_SLAVE_ADDRESS, reg, alen, (uchar *)rt_value, lineBytes);
	if(ret)
			puts ("Error reading the chip.\n");
	//*rt_value = be16_to_cpu(*rt_value);

	return ret;
}


static int ds278x_write(u8 reg, unsigned int *rt_value, int b_single)
{
	int ret;
	int alen = 1;
	int lineBytes = 2;
	if(b_single)
		lineBytes = 1;
	ret = i2c_write(DS2786_CHIP_SLAVE_ADDRESS, reg, alen, (uchar *)rt_value, lineBytes);
	if(ret)
			puts ("Error write the chip.\n");
	//*rt_value = be16_to_cpu(*rt_value);

	return ret;
}

static int ds278x_read_multi(u8 reg, uchar *rt_value, int lineBytes)
{
	int ret;
	int alen = 1;
	ret = i2c_read(DS2786_CHIP_SLAVE_ADDRESS, reg, alen, (uchar *)rt_value, lineBytes);
	if(ret)
			puts ("Error reading the chip.\n");
	//*rt_value = be16_to_cpu(*rt_value);

	return ret;
}


static int ds278x_write_multi(u8 reg, uchar *rt_value, int lineBytes)
{
	int ret;
	int alen = 1;
	ret = i2c_write(DS2786_CHIP_SLAVE_ADDRESS, reg, alen, (uchar *)rt_value, lineBytes);
	if(ret)
		puts ("Error write the chip.\n");

	return ret;
}

void max1704x_register_dump()
{
	int versoin = 0;
	int ret = 0;
	int rcomp = 0;
	int mode = 0;
	int alert_status = 0;

	ret = ds278x_read(MAX17043_REG_VERSION, &versoin, 0);
	if (ret) {
		printf("error reading VERSION, ret=%d\n", ret);
		return ret;
	}
	printf("Version(0x%04x)	", versoin);

	ret = ds278x_read(MAX17043_REG_CONFIG, &rcomp, 1);
	if (ret) {
		printf("error reading rcomp, ret=%d\n", ret);
		return ret;
	}
	printf("RCOMP(0x%02x)	", rcomp);

	ret = ds278x_read(MAX17043_REG_MODE, &mode, 0);
	if (ret) {
		printf("error reading mode, ret=%d\n", ret);
		return ret;
	}
	printf("Mode(0x%04x)	", mode);

	ret = ds278x_read(MAX17043_REG_CONFIG+1, &alert_status, 1);
	if (ret) {
		printf("error reading alert_threshold, ret=%d\n", ret);
		return ret;
	}
	printf("AlertThreshold(0x%02x)	", alert_status);
}

int temperature_value_change(int reg_value)
{
	uint8 value_high_bit;
	uint8 value_low_bit;
	int16 value;
	int temp;
	
	value_high_bit = reg_value;
	value_low_bit = reg_value>>8;
	value = ((value_high_bit<<8) |value_low_bit);

	temp = (value/32) * 125 / 1000;

	return temp;
}

static int ds278x_battery_temperature()
{
	int ret;
	int temp = 0;

	ret = ds278x_read(DS278x_REG_TEMP_MSB, &temp, 0);
	if (ret) {
		printf("error reading temperature, ret=%d\n", ret);
		return ret;
	}

	return temperature_value_change(temp);
}

int voltage_value_change(int reg_value)
{
	int value_high_bit = 0;
	int value_low_bit = 0;
	int value = 0;
	
	value_high_bit = (reg_value & 0xff)<<4;
	value_low_bit = (reg_value & 0xff00)>>12;
	value = (value_high_bit + value_low_bit);
	value = value * 125;

	printf("VCELL(%dV)	", value);
	
	return value;
}

static int ds2786_battery_voltage()
{
	int ret;
	int volt = 0;

	ret = ds278x_read(MAX17043_REG_VCELL, &volt, 0);
	if (ret) {
		printf("error reading voltage, ret=%d\n", ret);
		return ret;
	}

	return voltage_value_change(volt);
}

int current_value_change(int reg_value)
{

	uint8 value_high_bit;
	uint8 value_low_bit;
	int16 value;
	int curr;

	value_high_bit = reg_value;
	value_low_bit = reg_value>>8;
	value = ((value_high_bit<<8) |value_low_bit);

	curr = (value/16)*(25*1000)/10;

	return curr;

}

static int ds2786_battery_current()
{
	int ret;
	int curr = 0;

	ret = ds278x_read(DS278x_REG_CURRENT_MSB, &curr, 0);
	if (ret) {
		printf("error reading current, ret=%d\n", ret);
		return ret;
	}

	return current_value_change(curr);
}

static int ds2786_battery_rsoc()
{
	int ret;
	int rsoc = 0;
	int rsoc_lsb = 0;
	int rsoc_msb = 0;

	ret = ds278x_read(MAX17043_REG_SOC, &rsoc, 1);
	rsoc = rsoc >> 1;
	if (ret) {
		printf("error reading relative State-of-Charge, ret=%d\n", ret);
		return ret;
	}

	ret = ds278x_read(MAX17043_REG_SOC, &rsoc_msb, 1);
	ret = ds278x_read(MAX17043_REG_SOC+1, &rsoc_lsb, 1);
	rsoc_lsb = (((rsoc_msb & 0x01)<< 9) | rsoc_lsb)*2/1000;
	printf("SOC(%d.%d%)	", rsoc, rsoc_lsb);

	if(rsoc > 100)
		rsoc = 100;

	return rsoc; 
}

int ds278x_battery_get_property(enum power_supply_property psp,
										union power_supply_propval *val)
{
		switch (psp) {
			case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			case POWER_SUPPLY_PROP_PRESENT:
					val->intval = ds2786_battery_voltage();
					if (psp == POWER_SUPPLY_PROP_PRESENT)
							val->intval = val->intval <= 1 ? 0 : 1;
					break;
			case POWER_SUPPLY_PROP_CURRENT_NOW:
					val->intval = ds2786_battery_current();
					break;
			case POWER_SUPPLY_PROP_CAPACITY:
					val->intval = ds2786_battery_rsoc();
					break;
			case POWER_SUPPLY_PROP_TEMP:
					val->intval = ds278x_battery_temperature();
					break;
			default:
					return -1;
		}

		return 0;
}

void charging_gpio_set(int dcm, int iusb, int usus, int cen)
{

#if 0 // there is no connection
	gpio_t *gpio1_base = (gpio_t *)OMAP34XX_GPIO1_BASE;
#endif
	gpio_t *gpio2_base = (gpio_t *)OMAP34XX_GPIO2_BASE;
	gpio_t *gpio4_base = (gpio_t *)OMAP34XX_GPIO4_BASE;

#if 0 // there is no connection
	sr32((u32)&gpio1_base->oe, 11, 1, 0); // USUS: GPIO-11
#endif
	sr32((u32)&gpio2_base->oe, 10, 1, 0); // CEN: GPIO-42
	sr32((u32)&gpio2_base->oe, 29, 1, 0); // IUSB: GPIO-61
	sr32((u32)&gpio4_base->oe, 8, 1, 0); // DCM: GPIO-104	

	if (dcm)
		sr32((u32)&gpio4_base->setdataout, 8, 1, 1); 
	else
		//sr32((u32)&gpio4_base->cleardataout, 8, 1, 1); 
		sr32((u32)&gpio4_base->res2[0], 8, 1, 0); 

	if (!iusb)
	{
		sr32((u32)&gpio2_base->setdataout, 29, 1, 1); 
		//sr32((SCM_REGISTER_BASE+CONTROL_PADCONF_GPMC_nBE1), 3, 2, 3);
	}	
	else
	{
		//sr32((u32)&gpio2_base->cleardataout, 29, 1, 1); 
		sr32((u32)&gpio2_base->res2[0], 29, 1, 0); 
		//sr32((SCM_REGISTER_BASE+CONTROL_PADCONF_GPMC_nBE1), 3, 2, 1);
	}	

#if 0 // there is no connection
	if (usus)
		sr32((u32)&gpio1_base->setdataout, 11, 1, 1); 
	else
		//sr32((u32)&gpio1_base->cleardataout, 11, 1, 1); 
		sr32((u32)&gpio1_base->res2[0], 11, 1, 0); 	
#endif

	if (cen)
		sr32((u32)&gpio2_base->setdataout, 10, 1, 1); 
	else
		//sr32((u32)&gpio2_base->cleardataout, 10, 1, 1); 	
		sr32((u32)&gpio2_base->res2[0], 10, 1, 0);		


}

void charging_mode(enum charging_operation co)
{

	int dcm = 0;
	int iusb = 0;
	int usus = 0;
	int cen = 0;

	if (co > CHARGING_STOP || co < CHARGING_START_100MA)
		return;	

	/*----------------------------------------------------------------------
				DCM(GPIO-104)	IUSB(GPIO- 61)	USUS(GPIO-11)	CEN(GPIO-42)
		1000mA	H				X				X				L
		500mA	L				H				L				L
		100mA	L				L				L				L
	-----------------------------------------------------------------------*/	


	switch (co)
	{
		case CHARGING_START_100MA:
			printf("\n **** CHARGING_START_100MA...**** \n");
			break;

		case CHARGING_START_500MA:
			iusb = 1;
			break;	

		case CHARGING_START_1000MA:
			dcm = 1;
			break;		
			
		case CHARGING_STOP:
			cen = 1;
/* herry ++ frequence control 20110607 */
			iusb = 1;
/* herry -- frequence control 20110607 */
			printf("\n **** CHARGING_STOP... **** \n");			
			break;
		default:
			cen = 1;
			printf("\n **** UNKNOWN ERROR !!! **** \n");			
			break;
	}

	charging_gpio_set(dcm, iusb, usus, cen);
	
}

int chg_fault_detect(void)
{
	int value;
	gpio_t *gpio2_base = (gpio_t *)OMAP34XX_GPIO2_BASE;

	// get CHG_FAULT pin (gpio-43)
	value =  read_gpio_value((u32)&gpio2_base->datain, 11);
	return value;
}

int dc_ok_detect(void)
{
	int value;
	gpio_t *gpio2_base = (gpio_t *)OMAP34XX_GPIO2_BASE;

	// get DETECT_DEVICE pin (gpio-51)
	value =  read_gpio_value((u32)&gpio2_base->datain, 19);
	return value;
}

int chg_status_detect(void)
{
	int value;
	gpio_t *gpio5_base = (gpio_t *)OMAP34XX_GPIO5_BASE;

	// get CHG pin (gpio-149)
	value =  read_gpio_value((u32)&gpio5_base->datain, 21);
	printf("CHARGING_STATUS %d	", value);
	return value;
}

void chg_led_set(int on)
{
	int value;
	gpio_t *gpio2_base = (gpio_t *)OMAP34XX_GPIO2_BASE;

	// set LED_EN pin (gpio-59) 
	sr32((u32)&gpio2_base->oe, 27, 1, 0);

	if (on)
		sr32((u32)&gpio2_base->setdataout, 27, 1, 1); 	
	else
		sr32((u32)&gpio2_base->res2[0], 27, 1, 0);
}
/* herry ++ frequence control 20110607 */
int max17043_quick_start(void)
{
	int ret;
	uchar val[2];

	val[0]= 0x40;
	val[1]= 0x00;
	ret = i2c_write_bytes(DS2786_CHIP_SLAVE_ADDRESS, 0x06, val, 2);
	if (ret) {
		printf( "[max17043 gauge]Writing quick start fail!! \n");
		return ret;
	}
	return ret;
}
/* herry -- frequence control 20110607 */
void ds2786_gauge_reset(void)
{
	uchar reset_msb = 0x54; 
	uchar reset_lsb = 0x00;
	ds278x_write(MAX17043_REG_COMMAND, &reset_msb, 1);
	ds278x_write(MAX17043_REG_COMMAND+1, &reset_lsb, 1);
}

/* 12. Write RCOMP Register to a value of 0x00h */
static int max17043_write_final_rcomp(void)
{	
	int ret, alert;
	uchar val[1];

	if(g_BatteryID != BATTERY_ID_MCNAIR)
	{
		//val[0]= 0x75;
		val[0]= 0x89;
		val[1]= 0x1C;
	}
	else
	{
		val[0]= 0xB6;
		val[1]= 0x1C;
	}
	ret = i2c_write_bytes(DS2786_CHIP_SLAVE_ADDRESS, MAX17043_REG_CONFIG, val, 2);
	if (ret) {
		printf( "[max17043 gauge]Read MSB of CONFIG register(Rcomp) fail!! \n");
		return ret;
	}
	
	return ret;
}

/* 11. Lock Model Access */
static int max17043_lock_model(void)
{
	int ret ;
	uchar val[2];

	val[0]= 0x00;
	val[1]= 0x00;
	
	ret = i2c_write_bytes(DS2786_CHIP_SLAVE_ADDRESS, 0x3E, val, 2);
	if (ret) {
		printf( "[max17043 gauge]Writing lock model access fail!!\n");
		return ret;
	}
	return ret;
}

/* 10. Restore RCOMP and OCV */
static int max17043_restore_rcomp_ocv( uchar *val)
{	
	int ret;	
	ret = i2c_write_bytes(DS2786_CHIP_SLAVE_ADDRESS, 0x0C, val, 4);
	if (ret) {
		printf( "[max17043 gauge]restore rcomp and ocv fail!! \n");
		return ret;
	}

	return ret;
}


/* 9. Read SOC Register and Compare to expected result */
static int max17043_read_soc(uchar *rt_value)
{
	int ret;
	uchar rsoc[2];
	int i;
	ret = ds278x_read_multi(MAX17043_REG_SOC, rsoc, 2);
	if (ret) {
		printf("[max17043 gauge]Reading rcomp and ocv data fail!!\n");
		return ret;
	}
	for(i=0;i<2;i++)
	{
		rt_value[i] = rsoc[i];
	}

	return ret;
}


/*
* 5. Write the Model, once the model is unlocked, the host software must write the 64 byte model to the max17044.
* The model is located between memory locations 0x40h and 0x7Fh.
*/
static int max17043_write_model(void)
{	
	int ret;
	char *data0;
	char *data1;
	char *data2;
	char *data3;

	if(g_BatteryID != BATTERY_ID_MCNAIR)
	{
		//data0 = lico_data0;
		//data1 = lico_data1;
		//data2 = lico_data2;
		//data3 = lico_data3;
		data0 = lico_new_data0;
		data1 = lico_new_data1;
		data2 = lico_new_data2;
		data3 = lico_new_data3;
	}
	else
	{
		data0 = mcnair_data0;
		data1 = mcnair_data1;
		data2 = mcnair_data2;
		data3 = mcnair_data3;
	}
	
	/* herry -- frequence control 20110607 */
	ret = i2c_write_bytes(DS2786_CHIP_SLAVE_ADDRESS, 0x40,data0,16);
	if (ret) {
		printf( "[max17043 gauge]writing model(0x40) fail!! \n");
	}

	ret = i2c_write_bytes(DS2786_CHIP_SLAVE_ADDRESS, 0x50,data1,16);
	if (ret) {
		printf( "[max17043 gauge]writing model(0x50) fail!! \n");
	}

	ret = i2c_write_bytes(DS2786_CHIP_SLAVE_ADDRESS, 0x60,data2,16);
	if (ret) {
		printf( "[max17043 gauge]writing model(0x60) fail!! \n");
	}

	ret = i2c_write_bytes(DS2786_CHIP_SLAVE_ADDRESS, 0x70,data3,16);
	if (ret) {
		printf( "[max17043 gauge]writing model(0x70) fail!! \n");
	}

	return ret;
}

/* 4. Write RCCOMP Register to a Maximum value of 0xFF00h */
static int max17043_write_rccomp(void)
{	
	int ret;
	uchar val[2];

	val[0]= 0xFF;
	val[1]= 0x00;
	ret = i2c_write_bytes(DS2786_CHIP_SLAVE_ADDRESS, 0x0C, val, 2);
	if (ret) {
		printf( "[max17043 gauge]Writing rcomp fail!! \n");
		return ret;
	}

	return ret;
}

/* 3. Write OCV Register */
static int max17043_write_ocv(void)
{	
	int ret ;
	uchar val[2];
	if(g_BatteryID != BATTERY_ID_MCNAIR)
	{
		// OCVTest = 55984
		//val[0]= 0xDA;
		//val[1]= 0xB0;
		// OCVTest = 55920
		val[0]= 0xDA;
		val[1]= 0x70;
	}
	else
	{
    	// OCVTest = 56000
		val[0]= 0xDA;
		val[1]= 0xC0;
	}
	ret = i2c_write_bytes(DS2786_CHIP_SLAVE_ADDRESS, 0x0E, val, 2);
	if (ret) {
		printf("[max17043 gauge]Writing OCV fail!! \n");
		return ret;
	}

	return ret;
}

/* 2. Read Original RCOMP and OCV Register */
static int max17043_read_rcomp_ocv(uchar *rt_value)
{
	int ret;
	uchar rsoc[4];
	int i;
	ret = ds278x_read_multi(MAX17043_REG_CONFIG, rsoc, 4);
	if (ret) {
		printf("[max17043 gauge]Reading rcomp and ocv data fail!!\n");
		return ret;
	}
	for(i=0;i<4;i++)
	{
		rt_value[i] = rsoc[i];
	}

	return ret;
}

/* 1. Unlock Model Access */
static int max17043_ulock_model_access( void )
{
	int ret ;
	uchar val[2];
	val[0]= 0x4A;
	val[1]= 0x57;
	ret = i2c_write_bytes(DS2786_CHIP_SLAVE_ADDRESS,0x3E, val, 2);
	if (ret) {
		printf( "[max17043 gauge]Writing unlock model access fail!!\n");
		return ret;
	}

	return ret;
}

int max17043_write_custom_model( void )
{
	uchar rt_value[4];
	u8 soc[2];
	int status = 0;

	max17043_ulock_model_access();
	max17043_read_rcomp_ocv(rt_value);
	max17043_write_ocv();
	max17043_write_rccomp();
	max17043_write_model();
	udelay(400000);
	max17043_write_ocv();
	udelay(400000);
	
	max17043_read_soc(soc);

	if(g_BatteryID != BATTERY_ID_MCNAIR)
	{
		// SOCCheckA = 226, SOCCheckB = 228
		//if(soc[0]>=0xE2 && soc[0]<=0xE4)
		if(soc[0]>=0xF0 && soc[0]<=0xF2)
			printf("[max17043 gauge]LiCo battery custom model was loaded success.\n");
		else	
		{
			printf("[max17043 gauge]Model was loaded fail!!\n");
			status = 1;
		}
	}
	else
	{
		// SOCCheckA = 240, SOCCheckB = 242
		if(soc[0]>=0xF0 && soc[0]<=0xF2)
			printf("[max17043 gauge]NcNair battery custom model was loaded success.\n");
		else
		{
			printf("[max17043 gauge]Model was loaded fail!!\n");
			status = 1;
		}
	}

	max17043_restore_rcomp_ocv(rt_value);
	max17043_lock_model();
	max17043_write_final_rcomp();

	return status;	
}


