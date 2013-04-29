/*
 * 1. The size of share region is 4KB.
 * 2. The share region locates at the end of the memory. 
 */
#define CRC32_RESIDUE 0xdebb20e3UL /* ANSI X3.66 residue */

#ifndef PAGE_SIZE
# define PAGE_SIZE 4096
#endif

#define SHARE_REGION_SIZE PAGE_SIZE
#define RESERVED_SIZE (SHARE_REGION_SIZE - sizeof(unsigned int) - \
																				   sizeof(enum BoardID_enum) - \
																				   sizeof(unsigned int) - \
																				   (sizeof(char)*64) - \
																				   sizeof(unsigned int) - \
																				   sizeof(unsigned int)) - \
																				   sizeof(enum BatteryID_enum) - \
																				   (sizeof(char)*16)
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
																				   
/* Function Stat */
enum FlagStat_enum {
	FLAG_NOMALL						= 0,
	FLAG_FACTORY_DEFAULT  = 1,
	FLAG_RECOVER_MODE     = 2,
	FLAG_SHUTDOWN3				= 4,
	FLAG_REBOOT						= 8,
};

/* Language ID */
enum Language_enum {
	LANG_ENGLISH						 = 0x0C09,
	LANG_END								 = 0xffff,
};

/* Battery ID */
enum BatteryID_enum {
	BATTERY_ID_UNKNOWN = 0,
	BATTERY_ID_LICO    = 1,
	BATTERY_ID_MCNAIR  = 2,
};

struct share_region_t
{
	unsigned int flags;
	enum BoardID_enum hardware_id;
	unsigned int language;
	char software_version[64];
	unsigned int debug_flag;
	unsigned int status_3G_exist;
	enum BatteryID_enum battery_id;
	char battery_barcode[16];
	unsigned char reserved[RESERVED_SIZE];
	unsigned long checksum;
};

typedef struct share_region_t share_region_t;

#define MEM_512MB_SHARE_REGION_BASE 	(0x9FE00000 - 0x10000)
#define MEM_256MB_SHARE_REGION_BASE 	(0x8FE00000 - 0x10000)

extern share_region_t * const share_region;
extern void get_share_region(void);
extern int ep_get_hardware_id(void);
extern int ep_get_software_version(char *ver, const int len);
extern int ep_get_debug_flag(void);
extern int ep_get_3G_exist(void);
extern void set_recovery_flag(void);
extern int ep_get_battery_id(void);
extern int ep_get_battery_barcode(char *barcode, const int len);
