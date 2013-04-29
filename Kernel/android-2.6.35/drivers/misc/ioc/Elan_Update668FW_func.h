
#ifndef __ELAN_UPDATE668FW_FUNC_H__
#define __ELAN_UPDATE668FW_FUNC_H__

#define UPDATE_MODE_SETGPIO								0x1
#define UPDATE_MODE_ROM_EEPROM_CODEOPTION	0x2
#define UPDATE_MODE_EEPROM_ONLY           0x3
#define UPDATE_MODE_ROM_ONLY              0x4

unsigned short Elan_Update668FW_func(unsigned short Elan_FUNCTIONS);
//if input  0x01, means ask 668 status only.
//   return 0x81, means 668 isn't protected.
//   return 0x82, means 668 is protected.

//if input  0x02, means update 668 EEPROM, ROM, CODEOPTION.
//   return 0x80, means update finish.

//if input  0x03, means update 668 EEPROM only.
//   return 0x80, means update finish.

//if input  0x04, means update 668 ROM only.
//   return 0x80, means update finish.

#endif


