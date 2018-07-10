/*
 *    Description:  Kernel module to write to a pipe
 *        Created:  09/07/2018
 *         Author:  Yohan Pipereau
*/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/file.h>
#include <linux/slab.h>

#define KUC_MAGIC  0x191C /*Lustre9etLinC */
#define NB_MSG 100 /* nb of msg sent */
#define MAXSIZE 4096 /* max size */

/* struct imported from Lustre project */
struct msg_hdr {
	int msglen;
	int seq;
};

static inline void *msg_data(const struct msg_hdr *msg)
{
	return (unsigned char*) msg + sizeof(struct msg_hdr);
}

/* function imported from Lustre project */
int cfs_kernel_write(struct file *filp, const void *buf, size_t count,
		     loff_t *pos)
{
#ifdef HAVE_NEW_KERNEL_WRITE
	return kernel_write(filp, buf, count, pos);
#else
	mm_segment_t __old_fs = get_fs();
	int rc;

	set_fs(get_ds());
	rc = vfs_write(filp, (__force const char __user *)buf, count, pos);
	set_fs(__old_fs);

	return rc;
#endif
}

/* function imported from Lustre project */
int msg_put(struct file *filp, void *payload)
{
	struct msg_hdr *msg = (struct msg_hdr *)payload;
	ssize_t count = msg->msglen;
	loff_t offset = 0;
	int rc = 0;

	if (IS_ERR_OR_NULL(filp))
		return -EBADF;

	while (count > 0) {
		rc = cfs_kernel_write(filp, payload, count, &offset);
		if (rc < 0)
			break;
		count -= rc;
		payload += rc;
		rc = 0;
	}

	if (rc < 0)
		printk(KERN_WARNING "message send failed (%d)\n", rc);
	else
		printk(KERN_DEBUG "Message sent rc=%d, seq=%d\n", rc, msg->seq);

	return rc;
}

static ssize_t pipe_write(struct file *filp, const char __user *buf,
		size_t len, loff_t *off)
{
	int fd;
	int seq = 0;
	struct file *pipe_file;
	struct msg_hdr *msg;

	if (copy_from_user(&fd, buf, len) != 0) {
		return -EFAULT; //Bad Address
	}

	printk(KERN_DEBUG "File descriptor : %d\n", fd);

	pipe_file = fget(fd); //cast write pipe fd in struct file*

	msg = kzalloc(MAXSIZE, GFP_KERNEL);

	for (; seq < NB_MSG; seq++) {
		msg->msglen = sizeof(struct msg_hdr) + 4;
		msg->seq = seq;
		strcpy(msg_data(msg), "AAAA");
		printk(KERN_DEBUG "msg %s @%p\n", (char*)msg_data(msg), msg_data(msg));
		msg_put(pipe_file, msg);
	}

	kfree(msg);
	msg = NULL;

	return len;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.write = pipe_write,
};

struct miscdevice pipe_device = {
    .minor = MISC_DYNAMIC_MINOR, //automatic minor number to avoid conflict
    .name = "pipe_misc", //accessible in /dev/simple_misc
    .fops = &fops,
};

static int __init initfn(void)
{
	printk(KERN_INFO "pipe : module registered\n");

	misc_register(&pipe_device);

	return 0;
}

static void __exit exitfn(void)
{
	printk(KERN_INFO "pipe : module unregistered\n");
	misc_deregister(&pipe_device);
}

module_init(initfn);
module_exit(exitfn);
