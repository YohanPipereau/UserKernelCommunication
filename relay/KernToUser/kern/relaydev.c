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

#define SUBBUF_SIZE 262144
#define N_SUBBUFS 4

struct rchan *chan;

/*
 * create_buf_file() callback.  Creates relay file in debugfs.
 */
static struct dentry *create_buf_file_handler(const char *filename,
                                              struct dentry *parent,
                                              umode_t mode,
                                              struct rchan_buf *buf,
                                              int *is_global)
{
        return debugfs_create_file(filename, mode, parent, buf,
	                           &relay_file_operations);
}

/*
 * remove_buf_file() callback.  Removes relay file from debugfs.
 */
static int remove_buf_file_handler(struct dentry *dentry)
{
        debugfs_remove(dentry);

        return 0;
}

/*
 * relay interface callbacks
 */
static struct rchan_callbacks relay_callbacks =
{
        .create_buf_file = create_buf_file_handler,
        .remove_buf_file = remove_buf_file_handler,
};


static int __init initfn(void)
{
	int size;
	char buf[100];
	struct dentry *dir;

	dir = debugfs_create_dir("relay", NULL);
	if (!dir)
		printk("relay directory creation failed\n");
	else
		printk("relay directory created in debugs mount point\n");

	chan = relay_open("relay", dir, SUBBUF_SIZE, N_SUBBUFS,
			&relay_callbacks, NULL);
	if (!chan)
		printk(KERN_ERR "relay chan creation failed\n");
	else
		printk(KERN_INFO "relay chan created\n");

	sprintf(buf, "AAAAAAAA");
	size = strlen(buf);
	relay_write(chan, buf, size);

	return 0;
}

static void __exit exitfn(void)
{
	relay_close(chan);
	printk(KERN_INFO "close relay chan\n");
}

module_init(initfn);
module_exit(exitfn);
