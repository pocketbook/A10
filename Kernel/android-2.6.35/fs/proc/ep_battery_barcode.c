#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>
#include <linux/io.h>

#include <linux/share_region.h>

static int barcode_proc_ep_show(struct seq_file *m, void *v)
{
	char barcode[16];
	
	ep_get_battery_barcode(barcode, sizeof(barcode));
	seq_printf(m, "%s", barcode);
	
	return 0;
}

static int barcode_proc_ep_open(struct inode *inode, struct file *file)
{
	return single_open(file, barcode_proc_ep_show, NULL);
}

static const struct file_operations barcode_proc_ep_fops = {
	.open		= barcode_proc_ep_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_barcode_ep_init(void)
{
	proc_create("ep_battery_barcode", 0, NULL, &barcode_proc_ep_fops);
	return 0;
}
module_init(proc_barcode_ep_init);
