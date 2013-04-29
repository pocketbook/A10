/*
Elan Update 668 FirmWare Functions
*/ 

#include <linux/delay.h>
#include "Elan668.h"
#include "Elan_Initial_f668.h"
#include "Elan_Update668FW_func.h"

extern unsigned short *EEPROM_SRC_Addr, *ROM_SRC_Addr, *CODEOPTION_SRC_Addr, *CHECKSUM_SRC_Addr;
extern unsigned short *EEPROM_DES_Addr, *ROM_DES_Addr, *CODEOPTION_DES_Addr, *CHECKSUM_DES_Addr;

#define Elan_Define_ICnum				0x668
#define Elan_CODEOPTION_INIT		0x000
#define Elan_CODEOPTION_WR			0x001
#define Elan_CODEOPTION_END			0x002

//Global Variable --------------------------------------------------------------------------------------------
	unsigned short 	Elan_Count[3];
	unsigned short 	Elan_Temp[2];
	unsigned short 	Elan_Prgbuffer[16];
	unsigned short 	Elan_UpdateResult=0;


//Functions ---------------------------------------------------------------------------------------------------
void Elan_Clr_AllPin(void)
{
	Elan_Clr_DAT;
	Elan_Clr_CLK;
	Elan_Clr_RST;
	Elan_Clr_VDD;
}

// jimmy add for set IOC to normal work station
void Elan_Set_AllPin(void)
{
	Elan_Set_DAT;
	//Elan_Set_CLK;
	Elan_Set_RST;
	Elan_Set_VDD;
}

EXPORT_SYMBOL(Elan_Set_AllPin);
//------------------------------------------------------------------------------------------------------------
void Elan_OUTPUT_1CLK(void)
{
	Elan_Define_Delay1us();
	Elan_Set_CLK;
	Elan_Define_Delay1us();
	Elan_Clr_CLK;
	Elan_Define_Delay1us();
}

//------------------------------------------------------------------------------------------------------------
void Elan_Send_Password(unsigned short Elan_Password)
{
	Elan_Set_VDD;
	Elan_Define_Delay100us();
	Elan_Set_RST;
	Elan_Define_Delay5ms();
	Elan_Set_DAT;
	Elan_Define_Delay5ms();

	Elan_Temp[1] = Elan_Password;

	for(Elan_Count[0]=0;Elan_Count[0]<12;Elan_Count[0]++)		//Send 12 bit PW
	{
		Elan_Temp[0] = Elan_Temp[1];
		Elan_Temp[0] &= 0x0800;

		if(Elan_Temp[0] == 0) {
			Elan_Clr_DAT;
		} else { 
			Elan_Set_DAT;
		}
		Elan_OUTPUT_1CLK();

		Elan_Temp[1] <<= 1;
	}

	Elan_Clr_DAT;

	Elan_OUTPUT_1CLK();																			//Send 1 CLK

	for(Elan_Count[0]=0;Elan_Count[0]<288;Elan_Count[0]++)	//Send 256 + 32 CLK
	{
		Elan_OUTPUT_1CLK();
	}
}

//------------------------------------------------------------------------------------------------------------
void Elan_Send_CommandCode(unsigned short Elan_CommandCode)
{
	Elan_Temp[1] = Elan_CommandCode;
	
	for(Elan_Count[0]=0;Elan_Count[0]<6;Elan_Count[0]++) 		//Send 6 bit Command
	{
		Elan_Temp[0] = Elan_Temp[1];
		Elan_Temp[0] &= 0x020;

		if(Elan_Temp[0] == 0) {
			Elan_Clr_DAT;
		} else {
			Elan_Set_DAT;
		}
		Elan_OUTPUT_1CLK();

		Elan_Temp[1] <<= 1;
	}

	Elan_Clr_DAT;
}

//------------------------------------------------------------------------------------------------------------
void Elan_Send_15bitDATA(unsigned short Elan_15bitDATA)
{
	Elan_Clr_DAT;																						//Send start-bit
	Elan_OUTPUT_1CLK();

	Elan_Temp[1] = Elan_15bitDATA;

	for(Elan_Count[0]=0;Elan_Count[0]<15;Elan_Count[0]++) 	//Send 15 bit Data
	{
		Elan_Temp[0] = Elan_Temp[1];
		Elan_Temp[0] &= 0x01;

		if(Elan_Temp[0] == 0) {
			Elan_Clr_DAT;
		} else {
			Elan_Set_DAT;
		}
		Elan_OUTPUT_1CLK();

		Elan_Temp[1] >>= 1;
	}

	Elan_Clr_DAT;																						//Send stop-bit
	Elan_OUTPUT_1CLK();
}

//------------------------------------------------------------------------------------------------------------
void Elan_Send_8bitDATA(unsigned short Elan_8bitDATA)
{
	Elan_Clr_DAT;																						//Send start-bit
	Elan_OUTPUT_1CLK();

	Elan_Temp[1] = Elan_8bitDATA;

	for(Elan_Count[0]=0;Elan_Count[0]<8;Elan_Count[0]++)	 	//Send 8 bit Data
	{
		Elan_Temp[0] = Elan_Temp[1];
		Elan_Temp[0] &= 0x01;

		if(Elan_Temp[0] == 0) {
			Elan_Clr_DAT;
		} else {
			Elan_Set_DAT;
		}
		Elan_OUTPUT_1CLK();

		Elan_Temp[1] >>= 1;
	}

	Elan_Clr_DAT;																						//Send stop-bit
	Elan_OUTPUT_1CLK();
}

//------------------------------------------------------------------------------------------------------------
unsigned short Elan_Read_15bitDATA(void)
{
	Elan_Temp[1]=0;

	Elan_OUTPUT_1CLK();																			//Read start-bit

	for(Elan_Count[0]=0;Elan_Count[0]<15;Elan_Count[0]++)		//Read 15 bit Data
	{
		Elan_OUTPUT_1CLK();

		if(Elan_Read_DAT_Status == 0) {
			Elan_Temp[0]=0;
		} else {
			Elan_Temp[0]=1;
		}
		Elan_Temp[0] <<= Elan_Count[0];
		
		Elan_Temp[1] |= Elan_Temp[0];
	}

	Elan_OUTPUT_1CLK();																			//Read stop-bit

	return(Elan_Temp[1]);
}

//------------------------------------------------------------------------------------------------------------
unsigned short Elan_Read_8bitDATA(void)
{
	Elan_Temp[1]=0;

	Elan_OUTPUT_1CLK();																			//Read start-bit

	for(Elan_Count[0]=0;Elan_Count[0]<8;Elan_Count[0]++)		//Read 8 bit Data
	{
		Elan_OUTPUT_1CLK();

		if(Elan_Read_DAT_Status == 0) {
			Elan_Temp[0]=0;
		} else {
			Elan_Temp[0]=1;
		}
		Elan_Temp[0] <<= Elan_Count[0];
		
		Elan_Temp[1] |= Elan_Temp[0];
	}

	Elan_OUTPUT_1CLK();																			//Read stop-bit

//Peter ++
	Elan_Define_668_DAT_AS_OUTPUT_PIN();
//Pter --

	return(Elan_Temp[1]);
}

//------------------------------------------------------------------------------------------------------------
void Elan_Erase_EEPROM(void)
{
	Elan_Define_Delay1us();
	
	Elan_Send_CommandCode(0x027);							//CLRADD_ALL
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x035);							//SETEEPROM
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x039);							//Programming
	Elan_Define_Delay1us();
	
	Elan_Send_8bitDATA(0x00);
	Elan_Define_Delay1us();
	
	Elan_Send_CommandCode(0x009);							//CHIP
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x017);							//Dis_Erase
	Elan_Define_Delay5us();	
	Elan_Send_CommandCode(0x01D);							//Erase
	Elan_Define_Delay5ms();
	Elan_Send_CommandCode(0x01B);							//Discharge
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x02D);							//CHIP mode end
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x037);							//CLREEPROM
	Elan_Define_Delay1us();
}

//------------------------------------------------------------------------------------------------------------
void Elan_Erase_ROM(void)
{
	Elan_Define_Delay1us();
	
	Elan_Send_CommandCode(0x027);							//CLRADD_ALL
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x011);							//YACLR
	Elan_Define_Delay1us();

	for(Elan_Count[1]=0;Elan_Count[1]<16;Elan_Count[1]++)	//Loop 16 times
	{
		Elan_Send_CommandCode(0x039);							//Programming
		Elan_Define_Delay1us();
	
		Elan_Send_15bitDATA(0x00);
		Elan_Define_Delay1us();
	
		if(Elan_Count[1] == 15)
			break;		

		Elan_Send_CommandCode(0x00F);							//Increment Address
		Elan_Define_Delay1us();
	}

	Elan_Send_CommandCode(0x009);							//CHIP
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x017);							//Dis_Erase
	Elan_Define_Delay5us();
	Elan_Send_CommandCode(0x01D);							//Erase
	Elan_Define_Delay5ms();
	Elan_Send_CommandCode(0x01B);							//Discharge
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x02D);							//CHIP mode end
	Elan_Define_Delay1us();
}

//------------------------------------------------------------------------------------------------------------
void Elan_Erase_CODEOPTION(void)
{
	Elan_Define_Delay1us();
	
	Elan_Send_CommandCode(0x027);							//CLRADD_ALL
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x005);							//SETOPTION
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x011);							//YACLR
	Elan_Define_Delay1us();

	for(Elan_Count[1]=0;Elan_Count[1]<7;Elan_Count[1]++)	//Loop 7 times (0~6)
	{
		Elan_Send_CommandCode(0x039);							//Programming
		Elan_Define_Delay1us();
		
		Elan_Send_15bitDATA(0x00);
		Elan_Define_Delay1us();
		
		Elan_Send_CommandCode(0x00F);							//Increment Address
		Elan_Define_Delay1us();
	}

	Elan_Send_CommandCode(0x017);							//Dis_Erase
	Elan_Define_Delay5us();
	Elan_Send_CommandCode(0x01D);							//Erase
	Elan_Define_Delay5ms();
	Elan_Send_CommandCode(0x01B);							//Discharge
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x019);							//Dis_Write
	Elan_Define_Delay5us();
	Elan_Send_CommandCode(0x01F);							//Write
	Elan_Define_Delay5ms();
	Elan_Send_CommandCode(0x01B);							//Discharge
	Elan_Define_Delay1us();

	Elan_Send_CommandCode(0x00F);							//Increment Address (skip WORD7)
	Elan_Define_Delay1us();

	Elan_Send_CommandCode(0x011);							//YACLR
	Elan_Define_Delay1us();

	for(Elan_Count[1]=0;Elan_Count[1]<3;Elan_Count[1]++)	//Loop 3 times (8~A)
	{
		Elan_Send_CommandCode(0x039);							//Programming
		Elan_Define_Delay1us();
	
		Elan_Send_15bitDATA(0x00);
		Elan_Define_Delay1us();
	
		Elan_Send_CommandCode(0x00F);							//Increment Address
		Elan_Define_Delay1us();
	}

	Elan_Send_CommandCode(0x017);							//Dis_Erase
	Elan_Define_Delay5us();
	Elan_Send_CommandCode(0x01D);							//Erase
	Elan_Define_Delay5ms();
	Elan_Send_CommandCode(0x01B);							//Discharge
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x019);							//Dis_Write
	Elan_Define_Delay5us();
	Elan_Send_CommandCode(0x01F);							//Write
	Elan_Define_Delay5ms();
	Elan_Send_CommandCode(0x01B);							//Discharge
	Elan_Define_Delay1us();
																												//(skip WORDB~F)
	Elan_Send_CommandCode(0x025);							//CLROPTION
	Elan_Define_Delay1us();
}

//------------------------------------------------------------------------------------------------------------
unsigned short Elan_BC_EEPROM(void)
{
	Elan_Send_CommandCode(0x035);							//SETEEPROM
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x023);							//Blank Check
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x027);							//CLRADD_ALL
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x013);							//STR_S
	Elan_Define_Delay1us();

	for(Elan_Count[1]=0;Elan_Count[1]<256;Elan_Count[1]++)	//Loop 256 times
	{
		Elan_Send_CommandCode(0x015);							//Read_Precharge
		Elan_Define_Delay1us();
		Elan_Send_CommandCode(0x03B);							//Read
		Elan_Define_Delay1us();

		Elan_Define_668_DAT_AS_INPUT_PIN();
		Elan_Define_Delay5us();
		
		Elan_Prgbuffer[0] = Elan_Read_8bitDATA();	//Read 8bit Data
		Elan_Define_Delay1us();
		
		Elan_Define_668_DAT_AS_OUTPUT_PIN();
		Elan_Define_Delay5us();

		Elan_Send_CommandCode(0x00F);							//Increment Address
		Elan_Define_Delay1us();

		if(Elan_Prgbuffer[0] != 0)
			break;		
	}

	Elan_Send_CommandCode(0x003);							//STR_E
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x037);							//CLREEPROM
	Elan_Define_Delay1us();
	
	if(Elan_Prgbuffer[0] == 0)
		return(0x00);
	else
		return(0x01);
}

//------------------------------------------------------------------------------------------------------------
unsigned short Elan_BC_ROM(void)
{
	Elan_Send_CommandCode(0x023);							//Blank Check
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x027);							//CLRADD_ALL
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x013);							//STR_S
	Elan_Define_Delay1us();

	for(Elan_Count[2]=0;Elan_Count[2]<512;Elan_Count[2]++)//Loop 512 times
	{
		for(Elan_Count[1]=0;Elan_Count[1]<16;Elan_Count[1]++)//Loop 16 times
		{
			Elan_Send_CommandCode(0x015);							//Read_Precharge
			Elan_Define_Delay1us();
			Elan_Send_CommandCode(0x03B);							//Read
			Elan_Define_Delay1us();

			Elan_Define_668_DAT_AS_INPUT_PIN();
			Elan_Define_Delay5us();
			
			Elan_Prgbuffer[0] = Elan_Read_15bitDATA();//Read 15bit Data
			Elan_Define_Delay1us();
			
			Elan_Define_668_DAT_AS_OUTPUT_PIN();
			Elan_Define_Delay5us();
			
			Elan_Send_CommandCode(0x00F);							//Increment Address
			Elan_Define_Delay1us();

			if(Elan_Prgbuffer[0] != 0)
				break;		
		}
		
	if(Elan_Prgbuffer[0] != 0)
		break;		
	}

	Elan_Send_CommandCode(0x003);							//STR_E
	Elan_Define_Delay1us();

	if(Elan_Prgbuffer[0] == 0)
		return(0x00);
	else
		return(0x02);
}

//------------------------------------------------------------------------------------------------------------
void Elan_Write_EEPROM(void)
{
	Elan_Send_CommandCode(0x027);							//CLRADD_ALL
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x035);							//SETEEPROM
	Elan_Define_Delay1us();
	
	for(Elan_Count[1]=0;Elan_Count[1]<256;Elan_Count[1]++)	//Loop 256 times (Source to EEPROM)
	{
		Elan_Send_CommandCode(0x039);							//Programming
		Elan_Define_Delay1us();

		Elan_Prgbuffer[0] = *(EEPROM_SRC_Addr+Elan_Count[1]);
		Elan_Send_8bitDATA(Elan_Prgbuffer[0]);
		Elan_Define_Delay1us();

		Elan_Send_CommandCode(0x019);							//Dis_Write
		Elan_Define_Delay5us();	
		Elan_Send_CommandCode(0x01F);							//Write
		Elan_Define_Delay5ms();
		Elan_Send_CommandCode(0x01B);							//Discharge
		Elan_Define_Delay1us();
		Elan_Send_CommandCode(0x00F);							//Increment Address
		Elan_Define_Delay1us();
	}
	
	Elan_Send_CommandCode(0x037);							//CLREEPROM
	Elan_Define_Delay1us();
}

//------------------------------------------------------------------------------------------------------------
void Elan_Write_ROM(void)
{
	Elan_Send_CommandCode(0x027);							//CLRADD_ALL
	Elan_Define_Delay1us();
	
	for(Elan_Count[2]=0;Elan_Count[2]<512;Elan_Count[2]++)	//Loop 512 times
	{
		Elan_Send_CommandCode(0x011);							//YACLR
		Elan_Define_Delay1us();

		for(Elan_Count[1]=0;Elan_Count[1]<16;Elan_Count[1]++)	//Loop Send 16 times (Source to ROM)
		{
			Elan_Send_CommandCode(0x039);							//Programming
			Elan_Define_Delay1us();
			
			Elan_Prgbuffer[0] = *(ROM_SRC_Addr+((Elan_Count[2]+1)*16)-16+Elan_Count[1]);
			Elan_Send_15bitDATA(Elan_Prgbuffer[0]);
			Elan_Define_Delay1us();

			if(Elan_Count[1] == 15)
				break;		

			Elan_Send_CommandCode(0x00F);							//Increment Address
			Elan_Define_Delay1us();	
		}
		
		Elan_Send_CommandCode(0x019);							//Dis_Write
		Elan_Define_Delay5us();	
		Elan_Send_CommandCode(0x01F);							//Write
		Elan_Define_Delay5ms();
		Elan_Send_CommandCode(0x01B);							//Discharge
		Elan_Define_Delay1us();
		
		Elan_Send_CommandCode(0x00F);							//Increment Address
		Elan_Define_Delay1us();	
	}
}

//------------------------------------------------------------------------------------------------------------
void Elan_Write_CODEOPTION(unsigned short Elan_WrCODEOPTION)
{
	Elan_Send_CommandCode(0x027);							//CLRADD_ALL
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x005);							//SETOPTION
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x011);							//YACLR
	Elan_Define_Delay1us();

	if(Elan_WrCODEOPTION == Elan_CODEOPTION_INIT)					// Elan_CODEOPTION_INIT flow
	{
		for(Elan_Count[1]=0;Elan_Count[1]<3;Elan_Count[1]++)	//Loop Send 3 times (Source to ROM)
		{
			Elan_Send_CommandCode(0x039);							//Programming
			Elan_Define_Delay1us();
			
			Elan_Prgbuffer[0] = *(CODEOPTION_DES_Addr+Elan_Count[1]);
			Elan_Send_15bitDATA(Elan_Prgbuffer[0]);
			Elan_Define_Delay1us();

			Elan_Send_CommandCode(0x00F);							//Increment Address
			Elan_Define_Delay1us();	
		}

		for(Elan_Count[1]=3;Elan_Count[1]<15;Elan_Count[1]++)	//Loop Send 13 times
		{
			Elan_Send_CommandCode(0x039);							//Programming
			Elan_Define_Delay1us();
			
			Elan_Send_15bitDATA(0x000);
			Elan_Define_Delay1us();
			
			if(Elan_Count[1] == 14)
				break;		

			Elan_Send_CommandCode(0x00F);							//Increment Address
			Elan_Define_Delay1us();
		}
	}
	else if(Elan_WrCODEOPTION == Elan_CODEOPTION_WR)			// Elan_CODEOPTION_WR flow
	{
		for(Elan_Count[1]=0;Elan_Count[1]<11;Elan_Count[1]++)	//Loop 11 times
		{
			Elan_Send_CommandCode(0x039);							//Programming
			Elan_Define_Delay1us();
			
			Elan_Prgbuffer[0] = *(CODEOPTION_SRC_Addr+Elan_Count[1]);
			
			switch (Elan_Count[1])
			{
				case 0x0:
					Elan_Prgbuffer[0] &= 0x7FF8;
					break;
				case 0x1:
					Elan_Prgbuffer[0] &= 0x707F;
					break;
				case 0x2:
					Elan_Prgbuffer[0] &= 0x07FF;
					break;
				default:
					Elan_Prgbuffer[0] = 0x0000;
					break;
			}

			Elan_Send_15bitDATA(Elan_Prgbuffer[0]);
			Elan_Define_Delay1us();

			Elan_Send_CommandCode(0x00F);							//Increment Address
			Elan_Define_Delay1us();
		}
	}
	else																					// Elan_CODEOPTION_END flow
	{
		for(Elan_Count[1]=0;Elan_Count[1]<16;Elan_Count[1]++)	//Loop 16 times
		{
			Elan_Send_CommandCode(0x039);							//Programming
			Elan_Define_Delay1us();
			
			if(Elan_Count[1] == 0)
			Elan_Prgbuffer[0] = *(CODEOPTION_SRC_Addr+Elan_Count[1]);

			switch (Elan_Count[1])
			{
				case 0x0:
					Elan_Temp[0] = Elan_Prgbuffer[0];
					Elan_Temp[0] &= 0x0007;
						if(Elan_Temp[0] == 0)
							Elan_Prgbuffer[0] &= 0x7FFF;
						else
							Elan_Prgbuffer[0] |= 0x0007;
					break;
//				case 0x5:
//					Elan_Prgbuffer[0] = *CHECKSUM_SRC_Addr;
//					Elan_Prgbuffer[0] &= 0x00FF;
//					break;
//				case 0x6:
//					Elan_Prgbuffer[0] = *CHECKSUM_SRC_Addr;
//					Elan_Prgbuffer[0] >>= 8;
//					Elan_Prgbuffer[0] &= 0x00FF;
//					break;
//				case 0x8:
//					Elan_Prgbuffer[0] = *CHECKSUM_DES_Addr;
//					Elan_Prgbuffer[0] &= 0x00FF;
//					break;
//				case 0x9:
//					Elan_Prgbuffer[0] = *CHECKSUM_DES_Addr;
//					Elan_Prgbuffer[0] >>= 8;
//					Elan_Prgbuffer[0] &= 0x00FF;
//					break;
				case 0xA:
					Elan_Prgbuffer[0] = 0x1960;
					break;
				default:
					Elan_Prgbuffer[0] &= 0x0000;
					break;
			}

			Elan_Send_15bitDATA(Elan_Prgbuffer[0]);
			Elan_Define_Delay1us();

			if(Elan_Count[1] == 15)
				break;		

			Elan_Send_CommandCode(0x00F);							//Increment Address
			Elan_Define_Delay1us();
		}
	}

	Elan_Send_CommandCode(0x019);							//Dis_Write
	Elan_Define_Delay5us();
	Elan_Send_CommandCode(0x01F);							//Write
	Elan_Define_Delay5ms();
	Elan_Send_CommandCode(0x01B);							//Discharge
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x025);							//CLROPTION
	Elan_Define_Delay1us();
}

//------------------------------------------------------------------------------------------------------------
void Elan_Read_EEPROM(void)
{
	Elan_Send_CommandCode(0x027);							//CLRADD_ALL
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x035);							//SETEEPROM
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x013);							//STR_S
	Elan_Define_Delay1us();

	*(CHECKSUM_DES_Addr+1) = 0;
	
	for(Elan_Count[1]=0;Elan_Count[1]<256;Elan_Count[1]++)	//Loop 256 times
	{
		Elan_Send_CommandCode(0x015);							//Read_Precharge
		Elan_Define_Delay1us();
		Elan_Send_CommandCode(0x03B);							//Read
		Elan_Define_Delay1us();

		Elan_Define_668_DAT_AS_INPUT_PIN();
		Elan_Define_Delay5us();
		
		*(EEPROM_DES_Addr+Elan_Count[1]) = Elan_Read_8bitDATA();	//Read 8bit Data
		Elan_Define_Delay1us();
		
		Elan_Define_668_DAT_AS_OUTPUT_PIN();
		Elan_Define_Delay5us();

		*(CHECKSUM_DES_Addr+1) += *(EEPROM_DES_Addr+Elan_Count[1]);

		Elan_Send_CommandCode(0x00F);							//Increment Address
		Elan_Define_Delay1us();
	}

	Elan_Send_CommandCode(0x003);							//STR_E
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x037);							//CLREEPROM
	Elan_Define_Delay1us();
}

//------------------------------------------------------------------------------------------------------------
void Elan_Read_ROM(void)
{
	Elan_Send_CommandCode(0x027);							//CLRADD_ALL
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x013);							//STR_S
	Elan_Define_Delay1us();

	*CHECKSUM_DES_Addr = 0;

	for(Elan_Count[2]=0;Elan_Count[2]<512;Elan_Count[2]++)//Loop 512 times
	{
		for(Elan_Count[1]=0;Elan_Count[1]<16;Elan_Count[1]++)//Loop 16 times
		{
			Elan_Send_CommandCode(0x015);							//Read_Precharge
			Elan_Define_Delay1us();
			Elan_Send_CommandCode(0x03B);							//Read
			Elan_Define_Delay1us();

			Elan_Define_668_DAT_AS_INPUT_PIN();
			Elan_Define_Delay5us();
			
			*(ROM_DES_Addr+((Elan_Count[2]+1)*16)-16+Elan_Count[1]) = Elan_Read_15bitDATA();//Read 15bit Data
			Elan_Define_Delay1us();
			
			Elan_Define_668_DAT_AS_OUTPUT_PIN();
			Elan_Define_Delay5us();
			
			*CHECKSUM_DES_Addr += *(ROM_DES_Addr+((Elan_Count[2]+1)*16)-16+Elan_Count[1]);

			Elan_Send_CommandCode(0x00F);							//Increment Address
			Elan_Define_Delay1us();
		}
	}
	Elan_Send_CommandCode(0x003);							//STR_E
	Elan_Define_Delay1us();
}

//------------------------------------------------------------------------------------------------------------
void Elan_Read_CODEOPTION(unsigned short Elan_ReCODEOPTION)
{
	Elan_Send_CommandCode(0x027);							//CLRADD_ALL
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x005);							//SETOPTION
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x013);							//STR_S
	Elan_Define_Delay1us();

	for(Elan_Count[1]=0;Elan_Count[1]<3;Elan_Count[1]++)//Loop 3 times
	{
		Elan_Send_CommandCode(0x015);							//Read_Precharge
		Elan_Define_Delay1us();
		Elan_Send_CommandCode(0x03B);							//Read
		Elan_Define_Delay1us();

		Elan_Define_668_DAT_AS_INPUT_PIN();
		Elan_Define_Delay5us();
			
		Elan_Prgbuffer[0] = Elan_Read_15bitDATA();//Read 15bit Data
		Elan_Define_Delay1us();
		
		Elan_Define_668_DAT_AS_OUTPUT_PIN();
		Elan_Define_Delay5us();
			
		if(Elan_ReCODEOPTION == Elan_CODEOPTION_INIT)			//Elan_CODEOPTION_INIT flow
		{
			switch (Elan_Count[1])
			{
				case 0x0:
					*(CODEOPTION_DES_Addr+Elan_Count[1]) = 0x0004;
					break;
				case 0x1:
					*(CODEOPTION_DES_Addr+Elan_Count[1]) = Elan_Prgbuffer[0] & 0x0F80;
					break;
				case 0x2:
					*(CODEOPTION_DES_Addr+Elan_Count[1]) = Elan_Prgbuffer[0] & 0x7800;
					break;
				default:
					break;
			}
		}
		else if(Elan_ReCODEOPTION == Elan_CODEOPTION_WR)	//Elan_CODEOPTION_WR flow
		{
			switch (Elan_Count[1])
			{
				case 0x0:
					Elan_Temp[0] = *(CODEOPTION_SRC_Addr+Elan_Count[1]);
					Elan_Temp[0] &= 0x0007;
					if(Elan_Temp[0] == 0)
						*(CODEOPTION_DES_Addr+Elan_Count[1]) = Elan_Prgbuffer[0] & 0x7FF8;
					else
						*(CODEOPTION_DES_Addr+Elan_Count[1]) = Elan_Prgbuffer[0] | 0x0007;
					break;
					
				case 0x1:
					*(CODEOPTION_DES_Addr+Elan_Count[1]) = Elan_Prgbuffer[0] & 0x707F;
					break;
					
				case 0x2:
					*(CODEOPTION_DES_Addr+Elan_Count[1]) = Elan_Prgbuffer[0] & 0x07FF;
					break;
					
				default:
					break;
			}
		}
		else																							//Elan_CODEOPTION_END flow
		{
			if(Elan_Count[1] == 0)
			{
					Elan_Temp[0] = Elan_Prgbuffer[0];
					Elan_Temp[0] &= 0x0007;
					if(Elan_Temp[0] == 4)
						Elan_UpdateResult = 0x81;					// 668 status isn't protected.
					else
						Elan_UpdateResult = 0x82;					// 668 status is protected.
			}
		}			
		
		Elan_Send_CommandCode(0x00F);							//Increment Address
		Elan_Define_Delay1us();
	}

	Elan_Send_CommandCode(0x003);							//STR_E
	Elan_Define_Delay1us();
	Elan_Send_CommandCode(0x025);							//CLROPTION
	Elan_Define_Delay1us();
}

//------------------------------------------------------------------------------------------------------------
void Elan_Erase_ALL(void)
{
	Elan_Send_Password(Elan_Define_ICnum);

	Elan_Read_CODEOPTION(Elan_CODEOPTION_INIT);
	Elan_Erase_EEPROM();
	Elan_BC_EEPROM();
	Elan_Erase_ROM();
	Elan_BC_ROM();
	Elan_Erase_CODEOPTION();	
	Elan_Write_CODEOPTION(Elan_CODEOPTION_INIT);
}

//------------------------------------------------------------------------------------------------------------
void Elan_Write_ALL(void)
{
	Elan_Send_Password(Elan_Define_ICnum);
	
	Elan_Write_ROM();
	Elan_Write_EEPROM();
	Elan_Write_CODEOPTION(Elan_CODEOPTION_WR);
}

//------------------------------------------------------------------------------------------------------------
void Elan_Read_ALL(void)
{
	Elan_Send_Password(Elan_Define_ICnum);
	
	Elan_Read_EEPROM();
	Elan_Read_ROM();
	Elan_Read_CODEOPTION(Elan_CODEOPTION_WR);
	Elan_Write_CODEOPTION(Elan_CODEOPTION_END);
}

//------------------------------------------------------------------------------------------------------------
void Elan_Ask_668_Status(void)
{
	Elan_Send_Password(Elan_Define_ICnum);

	Elan_Read_CODEOPTION(Elan_CODEOPTION_END);
}

//------------------------------------------------------------------------------------------------------------
void Elan_Update_EEPROM_only(void)
{
	Elan_Send_Password(Elan_Define_ICnum);
	
	Elan_Erase_EEPROM();
	Elan_Write_EEPROM();
	Elan_Read_EEPROM();
}

//------------------------------------------------------------------------------------------------------------
void Elan_Update_ROM_only(void)
{
	Elan_Send_Password(Elan_Define_ICnum);

	Elan_Erase_ROM();
	Elan_Write_ROM();
	Elan_Read_ROM();
}

//Main Function ----------------------------------------------------------------------------------------------
unsigned short Elan_Update668FW_func(unsigned short Elan_FUNCTIONS)
{
	unsigned short i;

	Elan_Define_668_DAT_AS_OUTPUT_PIN();
	Elan_Define_668_CLK_AS_OUTPUT_PIN();

	switch (Elan_FUNCTIONS)
	{
		case UPDATE_MODE_SETGPIO:
			Elan_Clr_AllPin();
			Elan_Define_Delay500ms();
			Elan_Ask_668_Status();
			Elan_Clr_AllPin();
			Elan_Define_Delay500ms();
			Elan_Set_AllPin();
			break;
		case UPDATE_MODE_ROM_EEPROM_CODEOPTION:
			Elan_Clr_AllPin();
			Elan_Define_Delay500ms();
			Elan_Erase_ALL();
			Elan_Clr_AllPin();
			Elan_Define_Delay500ms();
			Elan_Write_ALL();
			Elan_Clr_AllPin();
			Elan_Define_Delay500ms();
			Elan_Read_ALL();
			Elan_Clr_AllPin();
			Elan_Define_Delay500ms();
			Elan_Set_AllPin();
			Elan_UpdateResult = 0x80;					// EEPROM, ROM, CODEOPTION update finish
			break;
		case UPDATE_MODE_EEPROM_ONLY:
			Elan_Clr_AllPin();
			Elan_Define_Delay500ms();
			Elan_Update_EEPROM_only();
			Elan_Clr_AllPin();
			Elan_Define_Delay500ms();
			Elan_Set_AllPin();
			Elan_UpdateResult = 0x80;					// EEPROM update finish
			break;
		case UPDATE_MODE_ROM_ONLY:
			Elan_Clr_AllPin();
			Elan_Define_Delay500ms();
			Elan_Update_ROM_only();
			Elan_Clr_AllPin();
			Elan_Define_Delay500ms();
			Elan_Set_AllPin();
			Elan_UpdateResult = 0x80;					// ROM update finish
			break;
		default:
			printk("[IOC_UPDATE_MODE] NO FUNCTION\n");
			break;
	}
//Peter ++
	Elan_Define_668_DAT_AS_INPUT_PIN();
	Elan_Define_668_CLK_AS_INPUT_PIN();
//Peter --

	for(i = 0 ; i < 256 ; i++)
	{
		printk("a[%3d] = 0x%04x, ", i, *(EEPROM_SRC_Addr+i)) ;
		printk("b[%3d] = 0x%04x, ", i, *(EEPROM_DES_Addr+i)) ;
		if( (i+1)%2 == 0 )
			printk("\n") ;
	}
	printk("\n") ;
	printk("\n") ;

	for(i = 0 ; i < 64 ; i++)
	{
		printk("a[%3d] = 0x%04x, ", i, *(ROM_SRC_Addr+i)) ;
		printk("b[%3d] = 0x%04x, ", i, *(ROM_DES_Addr+i)) ;
		if( (i+1)%2 == 0 )
			printk("\n") ;
	}
	printk("\n") ;
	printk("\n") ;

	for(i = 0 ; i < 3 ; i++)
	{
		printk("a[%3d] = 0x%04x, ", i, *(CODEOPTION_SRC_Addr+i)) ;
		printk("b[%3d] = 0x%04x, ", i, *(CODEOPTION_DES_Addr+i)) ;
		if( (i+1)%2 == 0 )
			printk("\n") ;
	}
	printk("\n") ;
	printk("\n") ;

	for(i = 0 ; i < 2 ; i++)
	{
		printk("a[%3d] = 0x%04x, ", i, *(CHECKSUM_SRC_Addr+i)) ;
		printk("b[%3d] = 0x%04x, ", i, *(CHECKSUM_DES_Addr+i)) ;
		if( (i+1)%2 == 0 )
			printk("\n") ;
	}
	printk("\n") ;
	printk("\n") ;

	return(Elan_UpdateResult);
}
