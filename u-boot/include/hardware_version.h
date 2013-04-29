#ifndef __HARDWARE_VERSION_H__
#define __HARDWARE_VERSION_H__

/* Board Version */
enum BoardID_enum {
	BOARD_VERSION_UNKNOWN = 0,
	BOARD_ID_EVB  = 1,
	BOARD_ID_PRO  = 2,
	BOARD_ID_EVT1 = 3,
	BOARD_ID_EVT2 = 4,
	BOARD_ID_EVT3 = 5,
	BOARD_ID_DVT1 = 6,
	BOARD_ID_DVT2 = 7,
	BOARD_ID_PVT  = 8,
	BOARD_ID_MP   = 9,
};

/* Battery ID */
enum BatteryID_enum {
	BATTERY_ID_UNKNOWN = 0,
	BATTERY_ID_LICO    = 1,
	BATTERY_ID_MCNAIR  = 2,
};

extern char g_szBatteryBarCode[];
extern enum BoardID_enum g_BoardID;
extern enum BatteryID_enum g_BatteryID;

#endif /* __HARDWARE_VERSION_H__ */
