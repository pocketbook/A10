/*
 * 1. The size of share region is 4KB.
 * 2. The share region locates at the end of the memory. 
 */
#include <hardware_version.h>

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
/* Function Stat */
enum FlagStat_enum {
	FLAG_NOMALL						= 0,
	FLAG_FACTORY_DEFAULT  = 1,
	FLAG_RECOVER_MODE     = 2,
	FLAG_SHUTDOWN3				= 4,
	FLAG_REBOOT						= 8,
	FLAG_SKIP_CHECK_DIAG	= 0x10,
};

/* Language ID */
enum Language_enum {
	LANG_ENGLISH						 = 0x0C09,
	LANG_END								 = 0xffff,
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

#ifdef CONFIG_EPXX_DDR_512MB
#define MEMORY_ADDRESS		0x9FE00000
#define SHARE_REGION_BASE (MEMORY_ADDRESS - 0x10000)
#else
#define MEMORY_ADDRESS		0x8FE00000
#define SHARE_REGION_BASE (MEMORY_ADDRESS - 0x10000)
#endif

extern void save_share_region();
extern void share_region_handle();
extern share_region_t * const share_region;
extern unsigned int gRebootRecovery;
extern unsigned int gSkipCheckDiag;
