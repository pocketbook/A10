#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#ifdef CONFIG_BATTERY_MAX17043_BATTERY_VERSION
#include <linux/share_region.h>
#endif

int g_BatteryID = 0;

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



static int max17043_write_code_restore(u8 reg,char *wt_value, struct i2c_client *client)
{
	struct i2c_msg msg[1];
	unsigned char data[5];
	int err;
	int i;
	
	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 5;
	msg->buf = data;

	data[0] = reg;
	for(i=1;i<5;i++)
		data[i]=wt_value[i-1];	
	
	err = i2c_transfer(client->adapter, msg, 1);
	return err;
}

static int max17043_write_code_16(u8 reg,char *wt_value, struct i2c_client *client)
{
	struct i2c_msg msg[1];
	unsigned char data[17];
	int err;
	int i;
	
	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 17;
	msg->buf = data;

	data[0] = reg;
	for(i=1;i<17;i++)
		data[i]=wt_value[i-1];
	
	err = i2c_transfer(client->adapter, msg, 1);
	return err;
}

static int max17043_read_code(u8 reg, char *rt_value, 	struct i2c_client *client)
{
	struct i2c_msg msg[1];
	unsigned char data[4];
	int err;
	int i;
	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 1;
	msg->buf = data;
	/* send memory address */
	data[0] = reg;
	err = i2c_transfer(client->adapter, msg, 1);

	if (err >= 0) {
		msg->len = 4;
		msg->flags = I2C_M_RD;
		err = i2c_transfer(client->adapter, msg, 1);
		if (err >= 0) {		
			for(i = 0;i<4;i++)
			{
				rt_value[i]=data[i];
			}
			return 0;
		}
	}
	return err;
}

static int max17043_write_code(u8 reg, int wt_value, struct i2c_client *client)
{
	struct i2c_msg msg[1];
	unsigned char data[3];
	int err;
	
	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 3;
	msg->buf = data;

	data[0] = reg;
	data[1] = wt_value >> 8;
	data[2] = wt_value & 0xFF;
	err = i2c_transfer(client->adapter, msg, 1);
	
	return err;
}

/* 12. Write RCOMP Register to a value of 0x00h */
static int max17043_write_final_rcomp(struct i2c_client *client)
{	
	int ret;
	u8 alert = i2c_smbus_read_byte_data(client, 0x0D);
	int value;

	if(g_BatteryID != BATTERY_ID_MCNAIR)
	{
		//value = (0x75<<8) | alert;
		value = (0x89<<8) | alert;
	}
	else
	{
		value = (0xB6<<8) | alert;
	}
	
	ret = max17043_write_code(0x0C, value, client);
	if (ret < 0) {
		printk( "[max17043 gauge]Writing final rcomp fail!! \n");
		return ret;
	}
	return ret;
}

/* 11. Lock Model Access */
static int max17043_lock_model(struct i2c_client *client)
{
	int ret ;
	ret = max17043_write_code(0x3E,0x0000,client);
	if (ret < 0) {
		printk( "[max17043 gauge]Writing lock model access fail!!\n");
		return ret;
	}
	return ret;
}

/* 10. Restore RCOMP and OCV */
static int max17043_restore_rcomp_ocv(struct i2c_client *client, char *val)
{	
	int ret;
	//val[0] = 0xB0;
	//val[1] = 0x00;
	ret = max17043_write_code_restore(0x0C, val, client);
	if (ret < 0) {
		printk( "[max17043 gauge]restore rcomp and ocv fail!! \n");
		return ret;
	}

	return ret;
}

/*
* 5. Write the Model, once the model is unlocked, the host software must write the 64 byte model to the max17044.
* The model is located between memory locations 0x40h and 0x7Fh.
*/
static int max17043_write_model(struct i2c_client *client)
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

	ret = max17043_write_code_16(0x40,data0,client);
	if (ret < 0) {
		printk( "[max17043 gauge]writing model(0x40) fail!! \n");
	}

	ret = max17043_write_code_16(0x50,data1,client);
	if (ret < 0) {
		printk( "[max17043 gauge]writing model(0x50) fail!! \n");
	}

	ret = max17043_write_code_16(0x60,data2,client);
	if (ret < 0) {
		printk( "[max17043 gauge]writing model(0x60) fail!! \n");
	}

	ret = max17043_write_code_16(0x70,data3,client);
	if (ret < 0) {
		printk( "[max17043 gauge]writing model(0x70) fail!! \n");
	}

	return ret;
}

/* 4. Write RCCOMP Register to a Maximum value of 0xFF00h */
static int max17043_write_rccomp(struct i2c_client *client)
{	
	int ret;
	ret = max17043_write_code(0x0C, 0xFF00, client);
	if (ret < 0) {
		printk( "[max17043 gauge]Writing rcomp fail!! \n");
		return ret;
	}
	return ret;
}

/* 3. Write OCV Register */
static int max17043_write_ocv(struct i2c_client *client)
{	
	int ret ;
	if(g_BatteryID != BATTERY_ID_MCNAIR)
		//ret = max17043_write_code(0x0E, 0xDAB0, client);
		ret = max17043_write_code(0x0E, 0xDA70, client);
	else 
		ret = max17043_write_code(0x0E, 0xDAC0, client);
	if (ret < 0) {
		printk("[max17043 gauge]Writing OCV fail!! \n");
		return ret;
	}
	return ret;
}

/* 2. Read Original RCOMP and OCV Register */
static int max17043_read_rcomp_ocv(struct i2c_client *client,char *rt_value)
{
	int ret;
	char rsoc[4];
	int i;
	ret = max17043_read_code(0x0C, rsoc, client);
	if (ret < 0) {
		printk("[max17043 gauge]Reading rcomp and ocv data fail!!\n");
		return ret;
	}
	for(i=0;i<4;i++)
	{
		rt_value[i] = rsoc[i];
	}
	//printk("[max17043 gauge]RcompMSB(0x0C) is 0x%02x, RcompLSB(0x0D) is 0x%02x, OCVMSB(0x0E) is 0x%02x, OCVLSB(0x0F) is 0x%02x \n", 
		//rt_value[0], rt_value[1], rt_value[2], rt_value[3]);
	return ret;
}

/* 1. Unlock Model Access */
static int max17043_ulock_model_access(struct i2c_client *client)
{
	int ret ;
	ret = max17043_write_code(0x3E, 0x4A57, client);
	if (ret < 0) {
		printk( "[max17043 gauge]Writing unlock model access fail!!\n");
		return ret;
	}
	return ret;
}

/* 0. wirte custom model sequence. Reference custom_model.pdf from maxim */
void max17043_write_custom_model(struct i2c_client *client)
{	
	char rt_value[4];
	u8 soc1;
	u8 soc2;

	#ifdef CONFIG_BATTERY_MAX17043_BATTERY_VERSION
	g_BatteryID = ep_get_battery_id();
	printk("[max17043 gauge]Enter %s, battery ID:%d.\n", __FUNCTION__, g_BatteryID);
	#endif
	
	max17043_ulock_model_access(client);	          /* 1. unlock model access */
	max17043_read_rcomp_ocv(client, rt_value);	      /* 2. read rcomp and ocv */
	max17043_write_ocv(client);			              /* 3. write OCV */
	max17043_write_rccomp(client);			          /* 4. write rcomp to a maximum value of 0xff00 */
	max17043_write_model(client);			          /* 5. write the model */
	mdelay(200);					                  /* 6. delay at least 150ms */
	max17043_write_ocv(client);			              /* 7. write OCV */
	mdelay(200);					                  /* 8. this delay must between 150ms and 600ms */
	                                                  /* 
	                                                                                         * 9. read soc register and compare to expected result.
	                                                                                         *     if SOC1 >= 0xE1 and SOC1 <= 0xE3 then 
	                                                                                         *     mode was loaded successful else was not loaded successful
	                                                                                        */
	soc1 = i2c_smbus_read_byte_data(client, 0x04);
	soc2 = i2c_smbus_read_byte_data(client, 0x05);
	if(g_BatteryID != BATTERY_ID_MCNAIR)
	{
		// SOCCheckA = 226, SOCCheckB = 228
		//if(soc1>=0xE2 && soc1<=0xE4)
		if(soc1>=0xF0 && soc1<=0xF2)
			printk("[max17043 gauge]LiCo battery custom model was loaded success.\n");
		else	
			printk("[max17043 gauge]Lico Model was loaded fail!!\n");
	}
	else
	{
		// SOCCheckA = 240, SOCCheckB = 242
		if(soc1>=0xF0 && soc1<=0xF2)
			printk("[max17043 gauge]NcNair battery custom model was loaded success.\n");
		else
			printk("[max17043 gauge]NcNair Model was loaded fail!!\n");
	}
	
	max17043_restore_rcomp_ocv(client, rt_value);	   /* 10. restore rcomp and ocv */
	max17043_lock_model(client);			           /* 11. lock model access */
	max17043_write_final_rcomp(client);                		     /* 12. write rcomp to a value of 0x4900 */
	
}
EXPORT_SYMBOL(max17043_write_custom_model);

