/*
Elan Initial TI OMAP3622 for EM78F668N
*/

#include <asm/gpio.h>
#include <mach/gpio.h>
#include <linux/delay.h>

unsigned short fw_b_area[8453] = {0};
unsigned short *EEPROM_SRC_Addr, *ROM_SRC_Addr, *CODEOPTION_SRC_Addr, *CHECKSUM_SRC_Addr;
unsigned short *EEPROM_DES_Addr, *ROM_DES_Addr, *CODEOPTION_DES_Addr, *CHECKSUM_DES_Addr;

void Elan_Initial_f668(unsigned short *fw_a_area)
{
	printk("[IOC] %s\n", __FUNCTION__);
#if 0
	fw_a_area[0] = 0x001B;
	fw_a_area[15] = 0x002B;
	fw_a_area[16] = 0x003C;
	fw_a_area[31] = 0x004D;
	fw_a_area[240] = 0x00CC;
	fw_a_area[255] = 0x00DD;

	fw_a_area[256] = 0x12AB;
	fw_a_area[271] = 0x03CD;
	fw_a_area[272] = 0x14EF;
	fw_a_area[287] = 0x0555;
	fw_a_area[8432] = 0x1222;
	fw_a_area[8447] = 0x1555;
	
	fw_a_area[8448] = 0x7C58;
	fw_a_area[8449] = 0x706A;
	fw_a_area[8450] = 0x078B;
#endif

	EEPROM_SRC_Addr     = &fw_a_area[0];		//EEPROM_SRC_Addr			= EEPROM Source Addr			 				(256*16bit  & 0x00ff) 
	ROM_SRC_Addr        = &fw_a_area[256];	//ROM_SRC_Addr 				= ROM Source Addr				 					(8192*16bit & 0x7fff) 
	CODEOPTION_SRC_Addr = &fw_a_area[8448];	//CODEOPTION_SRC_Addr = CODEOPTION Source Addr	 				(3*16bit    & 0xffff) 
	CHECKSUM_SRC_Addr   = &fw_a_area[8451];	//CHECKSUM_SRC_Addr   = ROM CHECKSUM Source Add					(1*16bit		& 0xffff) 
																					//CHECKSUM_SRC_Addr+1 = EEPROM CHECKSUM Source Addr			(1*16bit		& 0xffff) 

	EEPROM_DES_Addr     = &fw_b_area[0];		//EEPROM_DES_Addr			= EEPROM Destination Addr			 		(256*16bit  & 0x00ff)
	ROM_DES_Addr        = &fw_b_area[256];	//ROM_DES_Addr 				= ROM Destination Addr				 		(8192*16bit & 0x7fff) 
	CODEOPTION_DES_Addr = &fw_b_area[8448];	//CODEOPTION_DES_Addr = CODEOPTION Destination Addr 		(3*16bit    & 0xffff) 
	CHECKSUM_DES_Addr   = &fw_b_area[8451];	//CHECKSUM_DES_Addr   = ROM CHECKSUM Destination Addr		(1*16bit    & 0xffff)
      																		//CHECKSUM_DES_Addr+1 = EEPROM CHECKSUM Destination Addr(1*16bit    & 0xffff)
}
