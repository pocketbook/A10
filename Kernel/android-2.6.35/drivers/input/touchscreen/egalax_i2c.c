/*
 *
 * Touch Screen I2C Driver for EETI Controller
 *
 * Copyright (C) 2000-2011  eGalax_eMPIA Technology Inc.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define RELEASE_DATE "2011/07/12 for Foxconn"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/freezer.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/poll.h>
#include <linux/kfifo.h>
#include <linux/version.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
	#include <linux/earlysuspend.h>
	static struct early_suspend egalax_early_suspend;
#endif

// Global define to enable function
//#define _GENERIC_LINUX
//#define _IDLE_MODE_SUPPORT
#define _FALLING_EDGE_IRQ


#define ABS_MT_TRACKING_ID	(ABS_MT_BLOB_ID+1)
#define FLIP_XY(x, y)			{ \
						short tmp; \
						tmp = (x); \
						(x) = (y); \
						(y) = tmp; \
					}
#define INVERT_X(x)		((2047) - (x))
#define INVERT_Y(y)		((2047) - (y))

#ifdef CONFIG_PANEL_PORTRAIT  //Protrait 270
static u8 flip_xy=1;
static u8 reverse_x=1;
static u8 reverse_y=0;	
#elif defined (CONFIG_PANEL_PORTRAIT_REVERSED) //Protrait_reversed 90
static u8 flip_xy=1;
static u8 reverse_x=0;
static u8 reverse_y=1;
#elif  defined(CONFIG_PANEL_LANDSCAPE_REVERSED) //Landscape_reversed  180
static u8 flip_xy=0;
static u8 reverse_x=1;
static u8 reverse_y=1;
#else  //Landscape  0
static u8 flip_xy=0;
static u8 reverse_x=0;
static u8 reverse_y=0;	
#endif


static int global_major = 0; // dynamic major by default 
static int global_minor = 0;

#define MAX_EVENTS		600
#define MAX_I2C_LEN		10
#define FIFO_SIZE		8192 //(PAGE_SIZE*2)
#define MAX_SUPPORT_POINT	5
#define REPORTID_MOUSE		0x01
#define REPORTID_VENDOR		0x03
#define REPORTID_MTOUCH		0x04

/// ioctl command ///
#define EGALAX_IOC_MAGIC	0x72
#define	EGALAX_IOCWAKEUP	_IO(EGALAX_IOC_MAGIC, 1)
#define EGALAX_IOC_MAXNR	1

// running mode
#define MODE_STOP	0
#define MODE_WORKING	1
#define MODE_IDLE	2
#define MODE_SUSPEND	3

// controller type
#define TYPE_NONE	0
#define TYPE_7200	1
#define TYPE_7700	2

struct point_data {
	short Status;
	short X;
	short Y;
};

struct _egalax_i2c {
	struct workqueue_struct *ktouch_wq;
	struct work_struct work_irq;
#ifdef _IDLE_MODE_SUPPORT
	struct delayed_work delay_work_idle;
#endif
	struct delayed_work delay_work_loopback;
	struct mutex mutex_wq;
	struct i2c_client *client;
	unsigned char work_state;
	unsigned char skip_packet;
	unsigned char downCnt;
	unsigned char TPType;
};

struct egalax_char_dev
{
	int OpenCnts;
	struct cdev cdev;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
	struct kfifo* pDataKFiFo;
#else
	struct kfifo DataKFiFo;
#endif
	unsigned char *pFiFoBuf;
	spinlock_t FiFoLock;
	struct semaphore sem;
	wait_queue_head_t fifo_inq;
};

static struct _egalax_i2c *p_egalax_i2c_dev = NULL;	// allocated in egalax_i2c_probe
static struct egalax_char_dev *p_char_dev = NULL;	// allocated in init_module
static atomic_t egalax_char_available = ATOMIC_INIT(1);
static atomic_t wait_command_ack = ATOMIC_INIT(0);
static struct class *egalax_class;
static struct input_dev *input_dev = NULL;
static struct point_data PointBuf[MAX_SUPPORT_POINT];

#define DBG_MODULE	0x00000001
#define DBG_CDEV	0x00000002
#define DBG_PROC	0x00000004
#define DBG_POINT	0x00000008
#define DBG_INT		0x00000010
#define DBG_I2C		0x00000020
#define DBG_SUSP	0x00000040
#define DBG_INPUT	0x00000080
#define DBG_CONST	0x00000100
#define DBG_IDLE	0x00000200
#define DBG_WAKEUP	0x00000400
#define DBG_BUTTON	0x00000800
static unsigned int DbgLevel = DBG_MODULE|DBG_SUSP;

#define PROC_FS_NAME	"egalax_dbg"
#define PROC_FS_MAX_LEN	8
static struct proc_dir_entry *dbgProcFile;

#define EGALAX_DBG(level, fmt, args...)  { if( (level&DbgLevel)>0 ) \
					printk( KERN_DEBUG "[egalax_i2c]: " fmt, ## args); }

#define IDLE_INTERVAL	7 // second


//&*&*&*JJ1_20110623 for NPI read firmware version.
static char touch_firmware_version[5];

/* Iniatial firmware_version for application registered touch listener*/
static ssize_t egalax_firmware_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
// #cat /sys/devices/platform/i2c_omap.2/i2c-2/2-0004/firmware_version
	
			int loop=3,count,i,test_count=20;
			u8 j_buf[10];
			
		if((touch_firmware_version[0] == 0x58)&&(touch_firmware_version[1] == 0x2e)&&
					(touch_firmware_version[2] == 0x58)&&(touch_firmware_version[3] == 0x58))
		{			
				for(i=0;i<test_count;i++)
				{
					u8 cmdbuf[10]={0x03, 0x03, 0x0A, 0x01, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00};	
					i2c_master_send(p_egalax_i2c_dev->client, cmdbuf, 10);
					do{
						count = i2c_master_recv(p_egalax_i2c_dev->client, j_buf, 10);
					}while(count==EAGAIN && --loop);
				
//					printk("[0]=0x%x,[1]=0x%x,[2]=0x%x,[3]=0x%x,[4]=0x%x,[5]=0x%x,[6]=0x%x,[7]=0x%x,[8]=0x%x,[9]=0x%x \n", 
//					j_buf[0],j_buf[1],j_buf[2],j_buf[3],j_buf[4],j_buf[5],j_buf[6],j_buf[7],j_buf[8],j_buf[9]);
							
					if((j_buf[0]==0x3)&&(j_buf[2]==0xa)&&(j_buf[4]==0x44))
						break; 
				}
		  	
					printk("Jimmy_xxx [0]=0x%x,[1]=0x%x,[2]=0x%x,[3]=0x%x,[4]=0x%x,[5]=0x%x,[6]=0x%x,[7]=0x%x,[8]=0x%x,[9]=0x%x \n", 
					j_buf[0],j_buf[1],j_buf[2],j_buf[3],j_buf[4],j_buf[5],j_buf[6],j_buf[7],j_buf[8],j_buf[9]);
	  		
				if((j_buf[0]==0x3)&&(j_buf[2]==0xa)&&(j_buf[4]==0x44))
				{
					touch_firmware_version[0] = j_buf[5];
					touch_firmware_version[1] = j_buf[6];
					touch_firmware_version[2] = j_buf[7];
					touch_firmware_version[3] = j_buf[8];		
					touch_firmware_version[4] = j_buf[9];		
				}
				else
				{
					touch_firmware_version[0] = 0x58;
					touch_firmware_version[1] = 0x2e;
					touch_firmware_version[2] = 0x58;
					touch_firmware_version[3] = 0x58;				
					touch_firmware_version[4] = 0x58;				
				}
		}
	
  printk("[Touch]:(%s) touch_firmware_version = %s \n",__FUNCTION__, touch_firmware_version);
	
	return sprintf(buf, "%s\n", touch_firmware_version);
}
static DEVICE_ATTR(firmware_version, S_IRUGO|S_IWUSR|S_IWGRP, egalax_firmware_show, NULL);

static struct attribute *egalax_attributes[] = {
	&dev_attr_firmware_version.attr,
	NULL
};

static const struct attribute_group egalax_attr_group = {
	.attrs = egalax_attributes,
};
//&*&*&*JJ2_20110623 for NPI read firmware version.

static int wakeup_controller(int irq)
{
	int ret=0;
	int gpio = irq_to_gpio(irq);

	disable_irq(irq);
	
	gpio_direction_output(gpio, 0);
	if( p_egalax_i2c_dev->TPType == TYPE_7700 )
		mdelay(5);
	else
		udelay(200);
	gpio_direction_input(gpio); //return to high level
	
	enable_irq(irq);
	
	EGALAX_DBG(DBG_WAKEUP, " INT wakeup touch controller done\n");
	
	return ret;
}

static int egalax_cdev_open(struct inode *inode, struct file *filp)
{
	struct egalax_char_dev *cdev;

	cdev = container_of(inode->i_cdev, struct egalax_char_dev, cdev);
	if( cdev == NULL )
	{
        	EGALAX_DBG(DBG_CDEV, " No such char device node \n");
		return -ENODEV;
	}
	
	if( !atomic_dec_and_test(&egalax_char_available) )
	{
		atomic_inc(&egalax_char_available);
		return -EBUSY; /* already open */
	}

	cdev->OpenCnts++;
	filp->private_data = cdev;// Used by the read and write metheds

#ifdef _IDLE_MODE_SUPPORT
	// check and wakeup controller if necessary
	cancel_delayed_work_sync(&p_egalax_i2c_dev->delay_work_idle);
	if( p_egalax_i2c_dev->work_state == MODE_IDLE )
		wakeup_controller(p_egalax_i2c_dev->client->irq);
#endif

	EGALAX_DBG(DBG_CDEV, " CDev open done!\n");
	try_module_get(THIS_MODULE);
	return 0;
}

static int egalax_cdev_release(struct inode *inode, struct file *filp)
{
	struct egalax_char_dev *cdev; // device information

	cdev = container_of(inode->i_cdev, struct egalax_char_dev, cdev);
	if( cdev == NULL )
	{
		EGALAX_DBG(DBG_CDEV, " No such char device node \n");
		return -ENODEV;
	}

	atomic_inc(&egalax_char_available); /* release the device */

	filp->private_data = NULL;
	cdev->OpenCnts--;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
	kfifo_reset( cdev->pDataKFiFo );
#else
	kfifo_reset( &cdev->DataKFiFo );
#endif

#ifdef _IDLE_MODE_SUPPORT
	queue_delayed_work(p_egalax_i2c_dev->ktouch_wq, &p_egalax_i2c_dev->delay_work_idle, HZ*IDLE_INTERVAL);
#endif

	EGALAX_DBG(DBG_CDEV, " CDev release done!\n");
	module_put(THIS_MODULE);
	return 0;
}

#define MAX_READ_BUF_LEN	50
static char fifo_read_buf[MAX_READ_BUF_LEN];
static ssize_t egalax_cdev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	int read_cnt, ret, fifoLen;
	struct egalax_char_dev *cdev = file->private_data;
	
	if( down_interruptible(&cdev->sem) )
		return -ERESTARTSYS;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
	fifoLen = kfifo_len(cdev->pDataKFiFo);
#else
	fifoLen = kfifo_len(&cdev->DataKFiFo);
#endif

	while( fifoLen<1 ) /* nothing to read */
	{
		up(&cdev->sem); /* release the lock */
		if( file->f_flags & O_NONBLOCK )
			return -EAGAIN;

	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
		if( wait_event_interruptible(cdev->fifo_inq, kfifo_len( cdev->pDataKFiFo )>0) )
	#else
		if( wait_event_interruptible(cdev->fifo_inq, kfifo_len( &cdev->DataKFiFo )>0) )
	#endif
		{
			return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
		}

		if( down_interruptible(&cdev->sem) )
			return -ERESTARTSYS;
	}

	if(count > MAX_READ_BUF_LEN)
		count = MAX_READ_BUF_LEN;

	EGALAX_DBG(DBG_CDEV, " \"%s\" reading fifo data\n", current->comm);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
	read_cnt = kfifo_get(cdev->pDataKFiFo, fifo_read_buf, count);
#else
	read_cnt = kfifo_out_locked(&cdev->DataKFiFo, fifo_read_buf, count, &cdev->FiFoLock);
#endif

	ret = copy_to_user(buf, fifo_read_buf, read_cnt)?-EFAULT:read_cnt;

	up(&cdev->sem);
	
	return ret;
}

static ssize_t egalax_cdev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
	struct egalax_char_dev *cdev = file->private_data;
	int ret=0;
	char *tmp;

	if( down_interruptible(&cdev->sem) )
		return -ERESTARTSYS;

	if (count > MAX_I2C_LEN)
		count = MAX_I2C_LEN;

	tmp = kmalloc(count, GFP_KERNEL);
	if(tmp==NULL)
	{
		up(&cdev->sem);
		return -ENOMEM;
	}

	if(copy_from_user(tmp, buf, count))
	{
		up(&cdev->sem);
		kfree(tmp);
		return -EFAULT;
	}
	
	ret = i2c_master_send(p_egalax_i2c_dev->client, tmp, count);

	up(&cdev->sem);
	EGALAX_DBG(DBG_CDEV, " I2C writing %zu bytes.\n", count);
	kfree(tmp);

	return ret;
}

static int egalax_proc_read(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data )
{
	int ret;
	
	EGALAX_DBG(DBG_PROC, " \"%s\" call proc_read\n", current->comm);
	
	if(offset > 0)  /* we have finished to read, return 0 */
		ret  = 0;
	else 
		ret = sprintf(buffer, "Debug Level: 0x%08X\nRelease Date: %s\n", DbgLevel, RELEASE_DATE);

	return ret;
}

static int egalax_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char procfs_buffer_size = 0; 
	int i;
	unsigned char procfs_buf[PROC_FS_MAX_LEN+1] = {0};

	EGALAX_DBG(DBG_PROC, " \"%s\" call proc_write\n", current->comm);

	procfs_buffer_size = count;
	if(procfs_buffer_size > PROC_FS_MAX_LEN ) 
		procfs_buffer_size = PROC_FS_MAX_LEN+1;
	
	if( copy_from_user(procfs_buf, buffer, procfs_buffer_size) ) 
	{
		EGALAX_DBG(DBG_PROC, " proc_write faied at copy_from_user\n");
		return -EFAULT;
	}

	DbgLevel = 0;
	for(i=0; i<procfs_buffer_size-1; i++)
	{
		if( procfs_buf[i]>='0' && procfs_buf[i]<='9' )
			DbgLevel |= (procfs_buf[i]-'0');
		else if( procfs_buf[i]>='A' && procfs_buf[i]<='F' )
			DbgLevel |= (procfs_buf[i]-'A'+10);
		else if( procfs_buf[i]>='a' && procfs_buf[i]<='f' )
			DbgLevel |= (procfs_buf[i]-'a'+10);
		
		if(i!=procfs_buffer_size-2)
			DbgLevel <<= 4;
	}

	DbgLevel = DbgLevel&0xFFFFFFFF;

	EGALAX_DBG(DBG_PROC, " Switch Debug Level to 0x%08X\n", DbgLevel);

	return count; // procfs_buffer_size;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
static int egalax_cdev_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long args)
{	
	//struct egalax_char_dev *cdev = file->private_data;
	int ret=0;

	if(_IOC_TYPE(cmd) != EGALAX_IOC_MAGIC)
		return -ENOTTY;
	if(_IOC_NR(cmd) > EGALAX_IOC_MAXNR)
		return -ENOTTY;

	if(_IOC_DIR(cmd) & _IOC_READ)
		ret = !access_ok(VERIFY_WRITE, (void __user*)args, _IOC_SIZE(cmd));
	else if(_IOC_DIR(cmd) & _IOC_WRITE)
		ret = !access_ok(VERIFY_READ, (void __user*)args, _IOC_SIZE(cmd));

	if(ret)
		return -EFAULT;

	EGALAX_DBG(DBG_CDEV, " Handle device ioctl command\n");
	switch (cmd)
	{
		case EGALAX_IOCWAKEUP:
			ret = wakeup_controller(p_egalax_i2c_dev->client->irq);
			break;
		default:
			ret = -ENOTTY;
			break;
	}

	return ret;
}
#endif

static unsigned int egalax_cdev_poll(struct file *filp, struct poll_table_struct *wait)
{
	struct egalax_char_dev *cdev = filp->private_data;
	unsigned int mask = 0;
	int fifoLen;
	
	down(&cdev->sem);
	poll_wait(filp, &cdev->fifo_inq,  wait);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
	fifoLen = kfifo_len(cdev->pDataKFiFo);
#else
	fifoLen = kfifo_len(&cdev->DataKFiFo);
#endif

	if( fifoLen > 0 )
		mask |= POLLIN | POLLRDNORM;    /* readable */
	if( (FIFO_SIZE - fifoLen) > MAX_I2C_LEN )
		mask |= POLLOUT | POLLWRNORM;   /* writable */

	up(&cdev->sem);
	return mask;
}

static int LastUpdateID = 0;
static void ProcessReport(unsigned char *buf, struct _egalax_i2c *p_egalax_i2c)
{
	int i, cnt_down=0, cnt_up=0;
	short X=0, Y=0, ContactID=0, Status=0;
	bool bNeedReport = false;

	Status = buf[1]&0x01;
	ContactID = (buf[1]&0x7C)>>2;
	X = ((buf[3]<<8) + buf[2])>>4;
	Y = ((buf[5]<<8) + buf[4])>>4;
	
	if( !(ContactID>=0 && ContactID<MAX_SUPPORT_POINT) )
	{
		EGALAX_DBG(DBG_POINT, " Get I2C Point data error [%02X][%02X][%02X][%02X][%02X][%02X]\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
		return;
	}

	
	if (flip_xy) { 
		FLIP_XY(X, Y);
	}
	if (reverse_x) {
		X = INVERT_X(X);
	}
	if (reverse_y) {
		Y = INVERT_Y(Y);
	}
			
	PointBuf[ContactID].X = X;
	PointBuf[ContactID].Y = Y;
	if(PointBuf[ContactID].Status!=Status)
	{
		if(Status)
			p_egalax_i2c->downCnt++;
		else if( PointBuf[ContactID].Status>0 )
			p_egalax_i2c->downCnt--;
		
		PointBuf[ContactID].Status = Status;
		bNeedReport = true;
	}

	EGALAX_DBG(DBG_POINT, " Get Point[%d] Update: Status=%d X=%d Y=%d DownCnt=%d\n", ContactID, Status, X, Y, p_egalax_i2c->downCnt);

	// Send point report
	if( bNeedReport || (ContactID <= LastUpdateID) )
	{
		for(i=0; i<MAX_SUPPORT_POINT; i++)
		{
			if(PointBuf[i].Status >= 0)
			{
			#ifdef _GENERIC_LINUX
				input_report_key(input_dev, BTN_LEFT, PointBuf[i].Status);
				input_report_abs(input_dev, ABS_X, PointBuf[i].X);
				input_report_abs(input_dev, ABS_Y, PointBuf[i].Y);
				input_sync(input_dev);
			#else
				input_report_abs(input_dev, ABS_MT_TRACKING_ID, i);
				input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, PointBuf[i].Status);
				input_report_abs(input_dev, ABS_MT_POSITION_X, PointBuf[i].X);
				input_report_abs(input_dev, ABS_MT_POSITION_Y, PointBuf[i].Y);
				input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, 0);
				input_mt_sync(input_dev);
			#endif

				if(PointBuf[i].Status == 0)
				{
					PointBuf[i].Status--;
					cnt_up++;
				}
				else
					cnt_down++;
			}
		}

	#ifndef _GENERIC_LINUX	
		input_sync(input_dev);
	#endif
		EGALAX_DBG(DBG_POINT, " Input sync point data done! (Down:%d Up:%d)\n", cnt_down, cnt_up);
	}

	LastUpdateID = ContactID;
}

static struct input_dev * allocate_Input_Dev(void)
{
	int ret;
	struct input_dev *pInputDev=NULL;

	pInputDev = input_allocate_device();
	if(pInputDev == NULL)
	{
		EGALAX_DBG(DBG_MODULE, " Failed to allocate input device\n");
		return NULL;//-ENOMEM;
	}

	pInputDev->name = "eGalax_Touch_Screen";
	pInputDev->phys = "I2C";
	pInputDev->id.bustype = BUS_I2C;
	pInputDev->id.vendor = 0x0EEF;
	pInputDev->id.product = 0x0020;
	pInputDev->id.version = 0x0001;
	
	set_bit(EV_ABS, pInputDev->evbit);
#ifdef _GENERIC_LINUX
	set_bit(EV_KEY, pInputDev->evbit);
	__set_bit(BTN_LEFT, pInputDev->keybit);
	input_set_abs_params(pInputDev, ABS_X, 0, 2047, 0, 0);
	input_set_abs_params(pInputDev, ABS_Y, 0, 2047, 0, 0);
#else
	input_set_abs_params(pInputDev, ABS_MT_POSITION_X, -1, 2048, 0, 0);
	input_set_abs_params(pInputDev, ABS_MT_POSITION_Y, -1, 2048, 0, 0);
	input_set_abs_params(pInputDev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(pInputDev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(pInputDev, ABS_MT_TRACKING_ID, 0, MAX_SUPPORT_POINT, 0, 0);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
	input_set_events_per_packet(pInputDev, MAX_EVENTS);
#endif

	ret = input_register_device(pInputDev);
	if(ret) 
	{
		EGALAX_DBG(DBG_MODULE, " Unable to register input device.\n");
		input_free_device(pInputDev);
		pInputDev = NULL;
	}
	
	return pInputDev;
}

static int egalax_i2c_measure(struct _egalax_i2c *egalax_i2c)
{
	struct i2c_client *client = egalax_i2c->client;
	u8 x_buf[MAX_I2C_LEN]={0};
	int count, loop=3, ret=0;

	EGALAX_DBG(DBG_INT, " egalax_i2c_measure\n");

	do{
		count = i2c_master_recv(client, x_buf, MAX_I2C_LEN);
	}while(count==EAGAIN && --loop);

	if( count<0 || (x_buf[0]!=REPORTID_VENDOR && x_buf[0]!=REPORTID_MTOUCH) )
	{
		EGALAX_DBG(DBG_I2C, " I2C read error data with Len=%d hedaer=%d\n", count, x_buf[0]);
		return -1;
	}

	EGALAX_DBG(DBG_I2C, " I2C read data with Len=%d\n", count);
	if(x_buf[0]==REPORTID_VENDOR)
	{
		atomic_set(&wait_command_ack, 1);
		EGALAX_DBG(DBG_I2C, " I2C get vendor command packet\n");

		if(egalax_i2c->TPType == TYPE_NONE)
		{
			if(x_buf[5]=='V' && x_buf[6]=='e' && x_buf[7]=='n' && x_buf[8]=='u' && x_buf[9]=='s')
				egalax_i2c->TPType = TYPE_7700;
			else if(x_buf[5]=='P' && x_buf[6]=='C' && x_buf[7]=='A' && x_buf[8]=='P')
				egalax_i2c->TPType = TYPE_7200;

			EGALAX_DBG(DBG_MODULE, " Get Device type=%d\n", egalax_i2c->TPType);
		}
	}

	if( egalax_i2c->skip_packet > 0 )
		return count;

	if( count==MAX_I2C_LEN && x_buf[0]==REPORTID_MTOUCH && egalax_i2c->work_state==MODE_WORKING) // check buffer len & header
	{
		ProcessReport(x_buf, egalax_i2c);
		return count;
	}

	if( count>0 && p_char_dev->OpenCnts>0 ) // If someone reading now! put the data into the buffer!
	{
		loop=3;
		do {
		#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
			ret = wait_event_timeout(p_char_dev->fifo_inq, (FIFO_SIZE-kfifo_len(p_char_dev->pDataKFiFo))>=count, HZ);
		#else
			ret = wait_event_timeout(p_char_dev->fifo_inq, kfifo_avail(&p_char_dev->DataKFiFo)>=count, HZ);
		#endif
		}while(ret<=0 && --loop);

		if(ret>0) // fifo size is ready
		{
		#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
			ret = kfifo_put(p_char_dev->pDataKFiFo, x_buf, count);
		#else
			ret = kfifo_in_locked(&p_char_dev->DataKFiFo, x_buf, count, &p_char_dev->FiFoLock);
		#endif

			wake_up_interruptible( &p_char_dev->fifo_inq );
		}
		else
			EGALAX_DBG(DBG_CDEV, " [Warning] Can't write data because fifo size is overflow.\n");
	}

	return count;
}

static void egalax_i2c_wq_irq(struct work_struct *work)
{
	struct _egalax_i2c *egalax_i2c = container_of(work, struct _egalax_i2c, work_irq);
	struct i2c_client *client = egalax_i2c->client;
	int gpio = irq_to_gpio(client->irq);

	EGALAX_DBG(DBG_INT, " egalax_i2c_wq run\n");

	mutex_lock(&egalax_i2c->mutex_wq);

#ifdef _IDLE_MODE_SUPPORT
	cancel_delayed_work_sync(&egalax_i2c->delay_work_idle);
	if(egalax_i2c->work_state == MODE_IDLE)
		egalax_i2c->work_state = MODE_WORKING;
#endif

	/*continue recv data*/
	while( !gpio_get_value(gpio) )
	{
		egalax_i2c_measure(egalax_i2c);
		schedule();
	}
		
	if( egalax_i2c->skip_packet > 0 )
		egalax_i2c->skip_packet = 0;
	
#ifdef _IDLE_MODE_SUPPORT
	if( p_char_dev->OpenCnts<=0 && egalax_i2c->work_state==MODE_WORKING )
		queue_delayed_work(egalax_i2c->ktouch_wq, &egalax_i2c->delay_work_idle, HZ*IDLE_INTERVAL);
#endif

	mutex_unlock(&egalax_i2c->mutex_wq);	

#ifndef _FALLING_EDGE_IRQ
	enable_irq(client->irq);
#endif

	EGALAX_DBG(DBG_INT, " egalax_i2c_wq leave\n");
}

static irqreturn_t egalax_i2c_interrupt(int irq, void *dev_id)
{
	struct _egalax_i2c *egalax_i2c = (struct _egalax_i2c *)dev_id;

	EGALAX_DBG(DBG_INT, " INT with irq:%d\n", irq);

#ifndef _FALLING_EDGE_IRQ
	disable_irq_nosync(irq);
#endif
	queue_work(egalax_i2c->ktouch_wq, &egalax_i2c->work_irq);

	return IRQ_HANDLED;
}

#ifdef _IDLE_MODE_SUPPORT
static void egalax_i2c_wq_idle(struct work_struct *work)
{
	struct _egalax_i2c *egalax_i2c = container_of(work, struct _egalax_i2c, delay_work_idle.work);
	unsigned char buf[] = {0x03, 0x06, 0x0A, 0x04, 0x36, 0x3F, 0x01, 0x00, 0, 0};
	int ret=0;

	if(!egalax_i2c)
	{
		EGALAX_DBG(DBG_MODULE, " %s return with pointer error:%p\n", __func__, egalax_i2c);
		return; 
	}

	if(egalax_i2c->work_state == MODE_WORKING)
	{
		ret = i2c_master_send(egalax_i2c->client, buf, MAX_I2C_LEN);
		if(ret==MAX_I2C_LEN)
		{
			egalax_i2c->work_state = MODE_IDLE;
			EGALAX_DBG(DBG_IDLE, " Set controller to idle mode\n");
		}
		else
			EGALAX_DBG(DBG_IDLE, " Try to set controller to idle failed:%d\n", ret);
	}
}
#endif // #ifdef _IDLE_MODE_SUPPORT

#ifdef CONFIG_HAS_EARLYSUSPEND
static void egalax_i2c_early_suspend(struct early_suspend *handler)
{
	u8 cmdbuf[MAX_I2C_LEN]={0x03, 0x05, 0x0A, 0x03, 0x36, 0x3F, 0x02, 0, 0, 0};

	EGALAX_DBG(DBG_SUSP, " Enter early_suspend state:%d\n", p_egalax_i2c_dev->work_state);
	
	if(!p_egalax_i2c_dev) 
		goto fail_suspend;

	cancel_delayed_work_sync(&p_egalax_i2c_dev->delay_work_loopback);

	if( p_egalax_i2c_dev->TPType == TYPE_7200 )
	{
		EGALAX_DBG(DBG_SUSP, " Device power off and do nothing\n");
	}
	else //if( p_egalax_i2c_dev->TPType == TYPE_7700 )
	{
	#ifdef _IDLE_MODE_SUPPORT
		cancel_delayed_work_sync(&p_egalax_i2c_dev->delay_work_idle);

		if(p_egalax_i2c_dev->work_state == MODE_IDLE)
		{
			EGALAX_DBG((DBG_SUSP|DBG_IDLE), " Wakeup controller from idle mode before entering suspend\n");
			atomic_set(&wait_command_ack, 0);
			if( wakeup_controller(p_egalax_i2c_dev->client->irq)!=0 )
				goto fail_suspend2;
		
			while( !atomic_read(&wait_command_ack) )
				mdelay(2);

			EGALAX_DBG((DBG_SUSP|DBG_IDLE), " Device return to working\n");
		}
	#endif
	
		if( MAX_I2C_LEN != i2c_master_send(p_egalax_i2c_dev->client, cmdbuf, MAX_I2C_LEN) )
			goto fail_suspend2;
	}

#ifdef _FALLING_EDGE_IRQ
	disable_irq(p_egalax_i2c_dev->client->irq);
	cancel_work_sync(&p_egalax_i2c_dev->work_irq);
#endif

	p_egalax_i2c_dev->work_state = MODE_SUSPEND;

	EGALAX_DBG(DBG_SUSP, " Early_suspend done!!\n");
	return;

fail_suspend2:
	p_egalax_i2c_dev->work_state = MODE_SUSPEND;
fail_suspend:
	EGALAX_DBG(DBG_SUSP, " Early_suspend failed!!\n");
	return;
}

static void egalax_i2c_early_resume(struct early_suspend *handler)
{
	short i;
#ifdef _FALLING_EDGE_IRQ
	u8 x_buf[MAX_I2C_LEN]={0};
	int gpio;
#endif

	EGALAX_DBG(DBG_SUSP, " Enter early_resume state:%d\n", p_egalax_i2c_dev->work_state);

	if(!p_egalax_i2c_dev) 
		goto fail_resume;
	
	// re-init parameter
	p_egalax_i2c_dev->downCnt=0;
	for(i=0; i<MAX_SUPPORT_POINT; i++)
	{
		PointBuf[i].Status = -1;
		PointBuf[i].X = PointBuf[i].Y = 0;
	}

#ifdef _FALLING_EDGE_IRQ
	enable_irq(p_egalax_i2c_dev->client->irq);
#endif

	if( p_egalax_i2c_dev->TPType == TYPE_7200 )
	{
		EGALAX_DBG(DBG_SUSP, " Device power on later\n");
		p_egalax_i2c_dev->work_state = MODE_WORKING;
	}
	else //if( p_egalax_i2c_dev->TPType == TYPE_7700 )
	{
		if( wakeup_controller(p_egalax_i2c_dev->client->irq)==0 )
			p_egalax_i2c_dev->work_state = MODE_WORKING;
		else
			goto fail_resume2;

	#ifdef _FALLING_EDGE_IRQ
		gpio = irq_to_gpio(p_egalax_i2c_dev->client->irq);
		while( !gpio_get_value(gpio) )
			i2c_master_recv(p_egalax_i2c_dev->client, x_buf, MAX_I2C_LEN);
	#endif
	}

	atomic_set(&wait_command_ack, 0);
//&*&*&*BC1_110817: fix issue that touch and slide lock icon can't unlock the DUT	
	queue_delayed_work(p_egalax_i2c_dev->ktouch_wq, &p_egalax_i2c_dev->delay_work_loopback, 0);
//&*&*&*BC2_110817: fix issue that touch and slide lock icon can't unlock the DUT	
#ifdef _IDLE_MODE_SUPPORT
	queue_delayed_work(p_egalax_i2c_dev->ktouch_wq, &p_egalax_i2c_dev->delay_work_idle, HZ*IDLE_INTERVAL);
#endif

	EGALAX_DBG(DBG_SUSP, " Early_resume done!!\n");
	return;

fail_resume2:
	p_egalax_i2c_dev->work_state = MODE_WORKING;
fail_resume:
	EGALAX_DBG(DBG_SUSP, " Early_resume failed!!\n");
	return;
}
#endif // #ifdef CONFIG_HAS_EARLYSUSPEND

static void sendLoopback(struct i2c_client *client)
{
	u8 cmdbuf[MAX_I2C_LEN]={0x03, 0x03, 0x0A, 0x01, 0x46, 0, 0, 0, 0, 0};
	u8 cmdbuf2[MAX_I2C_LEN]={0x03, 0x03, 0x0A, 0x01, 0x41, 0, 0, 0, 0, 0};

	if(p_egalax_i2c_dev->TPType == TYPE_NONE)
		i2c_master_send(client, cmdbuf, MAX_I2C_LEN);
	else
		i2c_master_send(client, cmdbuf2, MAX_I2C_LEN);
}

static void egalax_i2c_wq_loopback(struct work_struct *work)
{
	struct _egalax_i2c *egalax_i2c = container_of(work, struct _egalax_i2c, delay_work_loopback.work);

	if(atomic_read(&wait_command_ack) || egalax_i2c->work_state!=MODE_WORKING)
	{
		EGALAX_DBG(DBG_MODULE, " Close egalax_i2c_wq_loopback work\n");
		return;
	}

	sendLoopback(egalax_i2c->client);
	queue_delayed_work(egalax_i2c->ktouch_wq, &egalax_i2c->delay_work_loopback, HZ/4);
}

static int __devinit egalax_i2c_probe(struct i2c_client *client, const struct i2c_device_id *idp)
{
	int ret, i;
	int gpio = irq_to_gpio(client->irq);
#ifdef _FALLING_EDGE_IRQ
	u8 x_buf[MAX_I2C_LEN]={0};
#endif

	EGALAX_DBG(DBG_MODULE, " Start probe\n");

	p_egalax_i2c_dev = (struct _egalax_i2c *)kzalloc(sizeof(struct _egalax_i2c), GFP_KERNEL);
	if (!p_egalax_i2c_dev) 
	{
		EGALAX_DBG(DBG_MODULE, "  Request memory failed\n");
		ret = -ENOMEM;
		goto fail1;
	}

	input_dev = allocate_Input_Dev();
	if(input_dev==NULL)
	{
		EGALAX_DBG(DBG_MODULE, " allocate_Input_Dev failed\n");
		ret = -EINVAL; 
		goto fail2;
	}
	EGALAX_DBG(DBG_MODULE, " Register input device done\n");
	
	for(i=0; i<MAX_SUPPORT_POINT;i++)
	{
		PointBuf[i].Status = -1;
		PointBuf[i].X = PointBuf[i].Y = 0;
	}

	p_egalax_i2c_dev->client = client;
	mutex_init(&p_egalax_i2c_dev->mutex_wq);

	p_egalax_i2c_dev->ktouch_wq = create_singlethread_workqueue("egalax_touch_wq");
	INIT_WORK(&p_egalax_i2c_dev->work_irq, egalax_i2c_wq_irq);

#ifdef _IDLE_MODE_SUPPORT
	INIT_DELAYED_WORK(&p_egalax_i2c_dev->delay_work_idle, egalax_i2c_wq_idle);
	queue_delayed_work(p_egalax_i2c_dev->ktouch_wq, &p_egalax_i2c_dev->delay_work_idle, HZ*IDLE_INTERVAL*2);
#endif

	INIT_DELAYED_WORK(&p_egalax_i2c_dev->delay_work_loopback, egalax_i2c_wq_loopback);
	atomic_set(&wait_command_ack, 0);
	queue_delayed_work(p_egalax_i2c_dev->ktouch_wq, &p_egalax_i2c_dev->delay_work_loopback, HZ);

	p_egalax_i2c_dev->TPType = TYPE_NONE;

	i2c_set_clientdata(client, p_egalax_i2c_dev);

	if( gpio_get_value(gpio) )
		p_egalax_i2c_dev->skip_packet = 0;
	else
		p_egalax_i2c_dev->skip_packet = 1;

	p_egalax_i2c_dev->work_state = MODE_WORKING;

#ifdef _FALLING_EDGE_IRQ
	ret = request_irq(client->irq, egalax_i2c_interrupt, IRQF_DISABLED | IRQF_TRIGGER_FALLING, client->name, p_egalax_i2c_dev);
#else
	ret = request_irq(client->irq, egalax_i2c_interrupt, IRQF_DISABLED | IRQF_TRIGGER_LOW, client->name, p_egalax_i2c_dev);
#endif
	if( ret ) 
	{
		EGALAX_DBG(DBG_MODULE, " Request irq(%d) failed\n", client->irq);
		goto fail3;
	}
	EGALAX_DBG(DBG_MODULE, " Request irq(%d) gpio(%d) with result:%d\n", client->irq, gpio, ret);

#ifdef _FALLING_EDGE_IRQ
	while( !gpio_get_value(gpio) )
		i2c_master_recv(client, x_buf, MAX_I2C_LEN);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	egalax_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	egalax_early_suspend.suspend = egalax_i2c_early_suspend;
	egalax_early_suspend.resume = egalax_i2c_early_resume;
	register_early_suspend(&egalax_early_suspend);
	EGALAX_DBG(DBG_MODULE, " Register early_suspend done\n");
#endif
	
//&*&*&*JJ1_20110623 for NPI read waveform version.
	int r;
	r = sysfs_create_group(&client->dev.kobj, &egalax_attr_group);
	if (r)
		printk("[Touch](%s)failed to create flash sysfs files\n",__FUNCTION__);
	
	int loop=3,count,test_count=20;
	u8 j_buf[10];
	
	for(i=0;i<test_count;i++)
	{
		u8 cmdbuf[10]={0x03, 0x03, 0x0A, 0x01, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00};	
		i2c_master_send(client, cmdbuf, 10);
		do{
			count = i2c_master_recv(client, j_buf, 10);
		}while(count==EAGAIN && --loop);
	
//		printk("[0]=0x%x,[1]=0x%x,[2]=0x%x,[3]=0x%x,[4]=0x%x,[5]=0x%x,[6]=0x%x,[7]=0x%x,[8]=0x%x,[9]=0x%x \n", 
//		j_buf[0],j_buf[1],j_buf[2],j_buf[3],j_buf[4],j_buf[5],j_buf[6],j_buf[7],j_buf[8],j_buf[9]);
				
		if((j_buf[0]==0x3)&&(j_buf[2]==0xa)&&(j_buf[4]==0x44))
			break; 
	}
		
//	printk("Jimmy_xxx [0]=0x%x,[1]=0x%x,[2]=0x%x,[3]=0x%x,[4]=0x%x,[5]=0x%x,[6]=0x%x,[7]=0x%x,[8]=0x%x,[9]=0x%x \n", 
//	j_buf[0],j_buf[1],j_buf[2],j_buf[3],j_buf[4],j_buf[5],j_buf[6],j_buf[7],j_buf[8],j_buf[9]);
	
	if((j_buf[0]==0x3)&&(j_buf[2]==0xa)&&(j_buf[4]==0x44))
	{
		touch_firmware_version[0] = j_buf[5];
		touch_firmware_version[1] = j_buf[6];
		touch_firmware_version[2] = j_buf[7];
		touch_firmware_version[3] = j_buf[8];		
		touch_firmware_version[4] = j_buf[9];		
	}
	else
	{
		touch_firmware_version[0] = 0x58;
		touch_firmware_version[1] = 0x2e;
		touch_firmware_version[2] = 0x58;
		touch_firmware_version[3] = 0x58;				
		touch_firmware_version[4] = 0x58;				
	}
	printk("[Touch]firmware verion = %s \n", touch_firmware_version);
//&*&*&*JJ2_20110623 for NPI read waveform version.

	EGALAX_DBG(DBG_MODULE, " I2C probe done\n");
	return 0;

fail3:
	i2c_set_clientdata(client, NULL);
	destroy_workqueue(p_egalax_i2c_dev->ktouch_wq); 
	free_irq(client->irq, p_egalax_i2c_dev);
	input_unregister_device(input_dev);
	input_dev = NULL;
fail2:
fail1:
	kfree(p_egalax_i2c_dev);
	p_egalax_i2c_dev = NULL;

	EGALAX_DBG(DBG_MODULE, " I2C probe failed\n");
	return ret;
}

static int __devexit egalax_i2c_remove(struct i2c_client *client)
{
	struct _egalax_i2c *egalax_i2c = i2c_get_clientdata(client);

	egalax_i2c->work_state = MODE_STOP;

#ifdef _IDLE_MODE_SUPPORT
	cancel_delayed_work_sync(&egalax_i2c->delay_work_idle);
#endif

	cancel_work_sync(&egalax_i2c->work_irq);

	if(client->irq)
	{
		disable_irq(client->irq);
		free_irq(client->irq, egalax_i2c);
	}
	
	if(egalax_i2c->ktouch_wq) 
		destroy_workqueue(egalax_i2c->ktouch_wq); 

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&egalax_early_suspend);
#endif

	if(input_dev)
	{
		EGALAX_DBG(DBG_MODULE,  " Unregister input device\n");
		input_unregister_device(input_dev);
		input_dev = NULL;
	}

	i2c_set_clientdata(client, NULL);
	kfree(egalax_i2c);
	p_egalax_i2c_dev = NULL;

	return 0;
}

static struct i2c_device_id egalax_i2c_idtable[] = { 
	{ "egalax_i2c", 0 }, 
	{ } 
}; 

MODULE_DEVICE_TABLE(i2c, egalax_i2c_idtable);

static struct i2c_driver egalax_i2c_driver = {
	.driver = {
		.name 	= "egalax_i2c",
	},
	.id_table	= egalax_i2c_idtable,
	.probe		= egalax_i2c_probe,
	.remove		= __devexit_p(egalax_i2c_remove),
};

static const struct file_operations egalax_cdev_fops = {
	.owner	= THIS_MODULE,
	.read	= egalax_cdev_read,
	.write	= egalax_cdev_write,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
	.ioctl	= egalax_cdev_ioctl,
#endif
	.poll	= egalax_cdev_poll,
	.open	= egalax_cdev_open,
	.release= egalax_cdev_release,
};

static void egalax_i2c_ts_exit(void)
{
	dev_t devno = MKDEV(global_major, global_minor);

	if(p_char_dev)
	{
		EGALAX_DBG(DBG_MODULE,  " Unregister character device\n");
		if( p_char_dev->pFiFoBuf )
			kfree(p_char_dev->pFiFoBuf);
	
		cdev_del(&p_char_dev->cdev);
		kfree(p_char_dev);
		p_char_dev = NULL;
	}

	unregister_chrdev_region( devno, 1);

	if(!IS_ERR(egalax_class))
	{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
		class_device_destroy(egalax_class, devno);
#else
		device_destroy(egalax_class, devno);
#endif 
		class_destroy(egalax_class);
	}
	
	i2c_del_driver(&egalax_i2c_driver);

	remove_proc_entry(PROC_FS_NAME, NULL);

	EGALAX_DBG(DBG_MODULE, " Exit driver done!\n");
}

static struct egalax_char_dev* setup_chardev(dev_t dev)
{
	struct egalax_char_dev *pCharDev;
	int result;

	pCharDev = kmalloc(1*sizeof(struct egalax_char_dev), GFP_KERNEL);
	if(!pCharDev) 
		goto fail_cdev;
	memset(pCharDev, 0, sizeof(struct egalax_char_dev));

	spin_lock_init( &pCharDev->FiFoLock );
	pCharDev->pFiFoBuf = kmalloc(sizeof(unsigned char)*FIFO_SIZE, GFP_KERNEL);
	if(!pCharDev->pFiFoBuf)
		goto fail_fifobuf;
	memset(pCharDev->pFiFoBuf, 0, sizeof(unsigned char)*FIFO_SIZE);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
	pCharDev->pDataKFiFo = kfifo_init(pCharDev->pFiFoBuf, FIFO_SIZE, GFP_KERNEL, &pCharDev->FiFoLock);
	if( pCharDev->pDataKFiFo==NULL )
		goto fail_kfifo;
#else
	kfifo_init(&pCharDev->DataKFiFo, pCharDev->pFiFoBuf, FIFO_SIZE);
	if( !kfifo_initialized(&pCharDev->DataKFiFo) )
		goto fail_kfifo;
#endif
	
	pCharDev->OpenCnts = 0;
	cdev_init(&pCharDev->cdev, &egalax_cdev_fops);
	pCharDev->cdev.owner = THIS_MODULE;
	sema_init(&pCharDev->sem, 1);
	init_waitqueue_head(&pCharDev->fifo_inq);

	result = cdev_add(&pCharDev->cdev, dev, 1);
	if(result)
	{
		EGALAX_DBG(DBG_MODULE, " Failed at cdev added\n");
		goto fail_kfifo;
	}

	return pCharDev; 

fail_kfifo:
	kfree(pCharDev->pFiFoBuf);
fail_fifobuf:
	kfree(pCharDev);
fail_cdev:
	return NULL;
}

static int egalax_i2c_ts_init(void)
{
	int result;
	dev_t devno = 0;

	// Asking for a dynamic major unless directed otherwise at load time.
	if(global_major) 
	{
		devno = MKDEV(global_major, global_minor);
		result = register_chrdev_region(devno, 1, "egalax_i2c");
	} 
	else 
	{
		result = alloc_chrdev_region(&devno, global_minor, 1, "egalax_i2c");
		global_major = MAJOR(devno);
	}

	if (result < 0)
	{
		EGALAX_DBG(DBG_MODULE, " Cdev can't get major number\n");
		return 0;
	}

	// allocate the character device
	p_char_dev = setup_chardev(devno);
	if(!p_char_dev) 
	{
		result = -ENOMEM;
		goto fail;
	}

	egalax_class = class_create(THIS_MODULE, "egalax_i2c");
	if(IS_ERR(egalax_class))
	{
		EGALAX_DBG(DBG_MODULE, " Failed in creating class.\n");
		result = -EFAULT;
		goto fail;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
	class_device_create(egalax_class, NULL, devno, NULL, "egalax_i2c");
#else
	device_create(egalax_class, NULL, devno, NULL, "egalax_i2c");
#endif
	EGALAX_DBG(DBG_MODULE, " Register egalax_i2c cdev, major: %d \n",global_major);

	dbgProcFile = create_proc_entry(PROC_FS_NAME, 0666, NULL);
	if (dbgProcFile == NULL) 
	{
		remove_proc_entry(PROC_FS_NAME, NULL);
		EGALAX_DBG(DBG_MODULE, " Could not initialize /proc/%s\n", PROC_FS_NAME);
	}
	else
	{
		dbgProcFile->read_proc = egalax_proc_read;
		dbgProcFile->write_proc = egalax_proc_write;
		EGALAX_DBG(DBG_MODULE, " /proc/%s created\n", PROC_FS_NAME);
	}

	EGALAX_DBG(DBG_MODULE, " Driver init done!\n");
	return i2c_add_driver(&egalax_i2c_driver);

fail:	
	egalax_i2c_ts_exit();
	return result;
}

module_init(egalax_i2c_ts_init);
module_exit(egalax_i2c_ts_exit);

MODULE_AUTHOR("EETI <touch_fae@eeti.com>");
MODULE_DESCRIPTION("egalax touch screen i2c driver");
MODULE_LICENSE("GPL");
