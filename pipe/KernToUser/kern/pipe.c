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
#include <linux/kthread.h>

#define KUC_MAGIC  0x191C /*Lustre9etLinC */
#define NB_MSG 10000 /* nb of msg sent */
#define MAXSIZE 1024-sizeof(struct msg_hdr) /* max size */
#define IOCTL_WRITE 0x2121

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

struct file *pipe_file;
struct task_struct *task;

/* struct imported from Lustre project */
struct msg_hdr {
	int msglen;
	int seq;
};

static inline char *msg_data(const struct msg_hdr *msg)
{
	return (char*) msg + sizeof(struct msg_hdr);
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

	return rc;
}
static long pipe_unlocked_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg) {
	int fd;

	if (cmd == IOCTL_WRITE) {
		if (copy_from_user(&fd, (char *) arg, sizeof(int)) != 0) {
			return -EFAULT; //Bad Address
		}
		printk(KERN_DEBUG "File descriptor : %d\n", fd);
		pipe_file = fget(fd); //cast write pipe fd in struct file*
	} else {
		printk(KERN_ERR "wrong ioctl\n");
	}

	return 0;
}

ssize_t pipe_write(struct file *filep, const char __user *buf, size_t len,
		loff_t *off) {
	printk(KERN_DEBUG "write detected \n");
	wake_up_process(task);
	return 0;
}

static int send_messages(void *data)
{
	int seq = 0;
	struct msg_hdr *msg;
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	uint64_t start, end;
	char template[MAXSIZE];
	int cmpt;

	for (cmpt = 0; cmpt < 1000; cmpt++) {
		template[cmpt] = 'A';
	}
	template[cmpt] = '\0';

	printk(KERN_DEBUG "read start\n");

	msg = kzalloc(MAXSIZE, GFP_KERNEL);

	start_timer();
	for (; seq < NB_MSG; seq++) {
		msg->msglen = sizeof(struct msg_hdr) + strlen(template);
		msg->seq = seq;
		strcpy(msg_data(msg), template);
		msg_put(pipe_file, msg);
	}
	stop_timer();

	start = ( ((uint64_t)cycles_high << 32) | cycles_low );
	end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
	printk(KERN_INFO "%llu clock cycles\n", (end-start));

	kfree(msg);
	msg = NULL;
	return 0;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = pipe_unlocked_ioctl,
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

	task = kthread_create(send_messages, NULL, "netlink-thread");
        if (IS_ERR(task))
                return PTR_ERR(task);

	return 0;
}

static void __exit exitfn(void)
{
	printk(KERN_INFO "pipe : module unregistered\n");
	misc_deregister(&pipe_device);
}

module_init(initfn);
module_exit(exitfn);
