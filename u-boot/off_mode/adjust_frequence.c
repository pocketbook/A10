/* herry ++ frequence control 20110607 */

#include <common.h>
#include <asm/arch/clocks.h>
#include <asm/arch/bits.h>
#include <asm/io.h>

#define mdelay(n) ({ unsigned long msec = (n); while (msec--) udelay(1000); })

void change_mem_freq(u8 mode)
{

	sr32(CM_CLKEN_PLL, 0, 3, PLL_FAST_RELOCK_BYPASS);
	wait_on_value(BIT0, 0, CM_IDLEST_CKGEN, LDELAY);

	mdelay(500);

	if (mode)
	{
		printf("change mem freq high...\n");
		// 200 MHz
		sr32(CM_CLKSEL1_PLL, 16, 11, 200);	/* Set M */
	}
	else
	{
		printf("change mem freq low...\n");
		// 133 MHz
		sr32(CM_CLKSEL1_PLL, 16, 11, 133);	/* Set M */
	}

	sr32(CM_CLKEN_PLL, 0, 3, PLL_LOCK);		/* lock mode */
	wait_on_value(BIT0, 1, CM_IDLEST_CKGEN, LDELAY);

	mdelay(500);

	printf("CM_CLKSEL1_PLL = 0x%x\n", readl(CM_CLKSEL1_PLL));
}

void change_cpu_freq(u8 mode)
{

	if (mode)
	{
		printf("change cpu freq high...\n");		
		// 600 MHz
		sr32(CM_CLKSEL1_PLL_MPU, 8, 11, 300);	/* Set M */
	}
	else
	{
		printf("change cpu freq low...\n");		
		// 26 MHz
		sr32(CM_CLKSEL1_PLL_MPU, 8, 11, 13);	/* Set M */
	}

	sr32(CM_CLKEN_PLL_MPU, 0, 3, PLL_LOCK); /* lock mode */
	wait_on_value(BIT0, 1, CM_IDLEST_PLL_MPU, LDELAY);	

	printf("CM_CLKSEL1_PLL_MPU = 0x%x\n", readl(CM_CLKSEL1_PLL_MPU));
}
