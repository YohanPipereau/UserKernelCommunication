/*
 *    Description:
 *        Created:  05/07/2018
 *         Author:  Yohan Pipereau
 */

#include <linux/module.h>
#include <linux/relay.h>
#include <linux/fs.h>
#include <linux/debugfs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yohan Pipereau");

#define SUBBUF_SIZE 1024*100
#define N_SUBBUFS 4
#define NB_MSG 100
#define MSGSIZE 1024

#define start_timer() asm volatile ("CPUID\n\t" \
"RDTSC\n\t" \
"mov %%edx, %0\n\t" \
"mov %%eax, %1\n\t": "=r" (cycles_high), \
"=r" (cycles_low):: "%rax", "%rbx", "%rcx", "%rdx");

#define stop_timer() asm volatile("RDTSCP\n\t" \
"mov %%edx, %0\n\t" \
"mov %%eax, %1\n\t" \
"CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1):: \
"%rax", "%rbx", "%rcx", "%rdx");

struct rchan *chan;
struct dentry *dir;

/* create_buf_file callback.  Creates relay file in debugfs. */
static struct dentry *create_buf_file_handler(const char *filename,
                                              struct dentry *parent,
                                              umode_t mode,
                                              struct rchan_buf *buf,
                                              int *is_global)
{
        return debugfs_create_file(filename, mode, parent, buf,
	                           &relay_file_operations);
}

/* remove_buf_file callback.  Removes relay file from debugfs. */
static int remove_buf_file_handler(struct dentry *dentry)
{
        debugfs_remove(dentry);

        return 0;
}

/* relay interface callbacks  */
static struct rchan_callbacks relay_callbacks =
{
        .create_buf_file = create_buf_file_handler,
        .remove_buf_file = remove_buf_file_handler,
};


static int __init initfn(void)
{
	int size;
	int cmpt = NB_MSG;
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	uint64_t start, end;
	char template[MSGSIZE];
	int i;

	for(i=0; i<NB_MSG-1; i++) {
		template[i] = 'A';
	}
	template[i] = '\0';

	dir = debugfs_create_dir("relay", NULL);
	if (!dir)
		printk("relay directory creation failed\n");
	else
		printk("relay directory created in debugfs mount point\n");

	chan = relay_open("relay", dir, SUBBUF_SIZE, N_SUBBUFS,
			&relay_callbacks, NULL);
	if (!chan)
		printk(KERN_ERR "relay open failed\n");
	else
		printk(KERN_INFO "relay open success\n");

	size = strlen(template);

	start_timer();
	for (; cmpt > 0; cmpt--) {
		relay_write(chan, template, size);
	}
	stop_timer();

	start = ( ((uint64_t)cycles_high << 32) | cycles_low );
	end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
	printk(KERN_INFO "%llu clock cycles\n", (end-start));

	return 0;
}

static void __exit exitfn(void)
{
	relay_close(chan);

	debugfs_remove_recursive(dir);
	printk(KERN_INFO "close relay chan\n");
}

module_init(initfn);
module_exit(exitfn);
