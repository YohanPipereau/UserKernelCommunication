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

#define KUC_MAGIC  0x191C /*Lustre9etLinC */
#define NB_MSG 1000

/* struct imported from Lustre project */
struct kuc_hdr {
	__u16 kuc_magic;
	__u8  kuc_transport;  /* Each new Lustre feature should use a different
				 transport */
	__u8  kuc_flags;
	__u16 kuc_msgtype;    /* Message type or opcode, transport-specific */
	__u16 kuc_msglen;     /* Including header */
} __attribute__((aligned(sizeof(__u64))));


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
	struct kuc_hdr *kuch = (struct kuc_hdr *)payload;
	ssize_t count = kuch->kuc_msglen;
	loff_t offset = 0;
	int rc = 0;

	if (IS_ERR_OR_NULL(filp))
		return -EBADF;

	if (kuch->kuc_magic != KUC_MAGIC) {
		printk(KERN_ERR "KernelComm: bad magic %x\n",
				kuch->kuc_magic);
		return -ENOSYS;
	}

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
		printk(KERN_DEBUG "Message sent rc=%d, fp=%p\n", rc, filp);

	return rc;
}

static ssize_t pipe_write(struct file *filp, const char __user *buf,
		size_t len, loff_t *off)
{
	int fd;
	int seq = 0;
	struct file *pipe_file;

	if (copy_from_user(&fd, buf, len) != 0) {
		return -EFAULT; //Bad Address
	}

	printk(KERN_DEBUG "File descriptor : %d\n", fd);

	pipe_file = fget(fd);

	for (; seq < NB_MSG; seq++) {
		msg_put(pipe_file, &seq);
	}

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
}

module_init(initfn);
module_exit(exitfn);
