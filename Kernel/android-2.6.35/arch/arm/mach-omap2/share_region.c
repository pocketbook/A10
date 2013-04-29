#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/crc32.h>
#include <linux/share_region.h>
#include <linux/mm.h>

struct share_region_t *g_share_region = NULL;
	
void get_share_region(void)
{
	unsigned int mem_size = (num_physpages >> (20 - PAGE_SHIFT));
	unsigned int __iomem *share_region_mem=NULL;
	unsigned int *pMme=NULL;
	int i;	

	g_share_region = (struct share_region_t *)kmalloc(SHARE_REGION_SIZE, GFP_KERNEL);
	memset(g_share_region, 0, SHARE_REGION_SIZE);
	
	if (mem_size < 256)
		return;
	
	if (mem_size > 256)
		share_region_mem = ioremap(MEM_512MB_SHARE_REGION_BASE, SHARE_REGION_SIZE);
	else   
		share_region_mem = ioremap(MEM_256MB_SHARE_REGION_BASE, SHARE_REGION_SIZE);

	if (share_region_mem == NULL) {
		printk("ioremap(SHARE_REGION_BASE, SHARE_REGION_SIZE) fail\n");
		return;
	}
	pMme = (unsigned int *)g_share_region;
	for (i=0 ; i<(SHARE_REGION_SIZE/sizeof(unsigned int)) ; i++) {
		pMme[i] = readl(share_region_mem+i);
	}
	iounmap(share_region_mem);

	printk("hw=%d,sw=%s\n", g_share_region->hardware_id, g_share_region->software_version);
}

EXPORT_SYMBOL(get_share_region);

int ep_get_hardware_id(void)
{
	return g_share_region->hardware_id;
}

EXPORT_SYMBOL(ep_get_hardware_id);

int ep_get_software_version(char *ver, const int len)
{
	if (!(g_share_region->software_version[0] == 'A' || 
		    g_share_region->software_version[0] == 'B' ||
		    g_share_region->software_version[0] == 'R' ||
		    g_share_region->software_version[0] == 'M' ||
		    g_share_region->software_version[0] == 'G' ||
		    g_share_region->software_version[0] == 'F' ||
		    g_share_region->software_version[0] == 'D'))
	{
		sprintf(ver, "000.00.00.000.000000.0000.0000.0000.0000.0000");
		return -1;	
	}
	memcpy(ver, g_share_region->software_version, len);
	ver[(len-1)] = '\0';

	return 0;
}

EXPORT_SYMBOL(ep_get_software_version);

int ep_get_debug_flag(void)
{
	return (g_share_region->debug_flag);
}

EXPORT_SYMBOL(ep_get_debug_flag);

int ep_get_3G_exist(void)
{
	return (g_share_region->status_3G_exist);
}

EXPORT_SYMBOL(ep_get_3G_exist);

int ep_get_battery_id(void)
{
	return (g_share_region->battery_id);
}

EXPORT_SYMBOL(ep_get_battery_id);

int ep_get_battery_barcode(char *barcode, const int len)
{
	if (BATTERY_ID_UNKNOWN == g_share_region->battery_id) {
		return -1;
	}
	memcpy(barcode, g_share_region->battery_barcode, len);
	barcode[(len-1)] = '\0';

	return 0;
}

EXPORT_SYMBOL(ep_get_battery_barcode);

unsigned int save_share_region(void)
{
	unsigned int mem_size = (num_physpages >> (20 - PAGE_SHIFT));
	unsigned int __iomem *share_region_mem=NULL;
	unsigned int *pMme=NULL;
	int i;

	if (mem_size < 256)
		return -1;	
		
	if (mem_size > 256)
		share_region_mem = ioremap(MEM_512MB_SHARE_REGION_BASE, SHARE_REGION_SIZE);
	else
		share_region_mem = ioremap(MEM_256MB_SHARE_REGION_BASE, SHARE_REGION_SIZE);

	if (share_region_mem == NULL) {
		printk("ioremap(SHARE_REGION_BASE, SHARE_REGION_SIZE) fail\n");
		return -1;
	}
	pMme = (unsigned int *)g_share_region;
	for (i=0 ; i<(SHARE_REGION_SIZE/sizeof(unsigned int)) ; i++) {
		writel(pMme[i], share_region_mem+i);
	}
	iounmap(share_region_mem);

	return 0;
}

EXPORT_SYMBOL(save_share_region);

void set_recovery_flag(void)
{
	g_share_region->flags = FLAG_RECOVER_MODE;
	save_share_region();
}

EXPORT_SYMBOL(set_recovery_flag);
