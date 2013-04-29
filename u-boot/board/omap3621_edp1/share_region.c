/***********************************************************
 * We can pass variables to the kernal through share region. 
 * Then we can use this region to do many things.
 ***********************************************************/
#include <common.h>
#include <share_region.h>

share_region_t * const share_region = (share_region_t *)SHARE_REGION_BASE;
unsigned int gRebootRecovery = 0;
unsigned int gSkipCheckDiag = 0;

unsigned long lowlevel_crc32(unsigned long crc, const unsigned char *buf, unsigned int len);
/* Return the CRC of the bytes buf[0..len-1]. */
static unsigned bg_crc32(unsigned char *buf, int len) 
{
   /* uboot crc32() has opposite semantics than the linux kernel crc32_le() */
   return crc32(0L, buf, len);
}

static int check_share_region(void)
{
	int result = 0;

	if ( share_region )
	{
		unsigned long computed_residue, checksum;

		checksum = share_region->checksum;

		computed_residue = ~bg_crc32((unsigned char *)share_region, SHARE_REGION_SIZE);

		result = CRC32_RESIDUE == computed_residue;
	}

	if ( result ) {
		//printf("share region: pass\n");
	} else {
		//printf("share region: fail\n");
	}

	return result;
}

void save_share_region(void)
{
    if ( share_region ) {
        share_region->checksum = bg_crc32((unsigned char *)share_region, SHARE_REGION_SIZE - sizeof (unsigned long) );
    }
}

void share_region_handle(void)
{
  if (FLAG_SKIP_CHECK_DIAG == share_region->flags) {
	gSkipCheckDiag = 1;
	printf("share_region->flags=%d\n", share_region->flags);	
  }  
  if (FLAG_RECOVER_MODE == share_region->flags) {
	gRebootRecovery = 1;
	printf("share_region->flags=%d\n", share_region->flags);	
  }
  if(!check_share_region())
  {
		//printf("Share region: CRC invalid.\n"); 
	memset(share_region, 0x0, sizeof(share_region_t));
  }
	save_share_region();
}
