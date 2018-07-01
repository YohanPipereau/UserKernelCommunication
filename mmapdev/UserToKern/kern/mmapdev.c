/*
 *    Description: This module create a character device which uses different
 *    shared pages for each process to communicate with kernel land.
 *
 *        Created:  25/06/2018
 *         Author:  Yohan Pipereau
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/kthread.h>

/* Start time measurement; CPUID prevent out of order execution */
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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yohan Pipereau");
MODULE_DESCRIPTION("Shared memory between kernel module and user process");

struct sharedm {
	unsigned long start;
	struct task_struct *task;
};

struct record {
	char msg[1000];
	int pending;
};

unsigned long flags;

/*  kthread function reading data in the shared memory */
static int read_messages(void *data) {
	struct sharedm *shm;
	struct record *rec;

	shm = (struct sharedm *) data;
	rec = (struct record *) shm->start;
	while (!kthread_should_stop()) {
		if (rec->pending) {
			rec->pending = 0;
			printk(KERN_INFO "mmdev : message %s",
					(char *)rec->msg);
		}
	}

	return 0;
}

static void vm_open(struct vm_area_struct *vma) {
	printk(KERN_DEBUG "mmdev : open VMA ; virt %lx, phys %llux\n",
			vma->vm_start, virt_to_phys((void *)vma->vm_start));
}

static void vm_close(struct vm_area_struct *vma) {
	struct sharedm *shm;

	shm = vma->vm_private_data;
	printk(KERN_DEBUG "mmdev : close VMA \n");
}

/*  First access to the page */
static int vm_fault(struct vm_area_struct *vma, struct vm_fault *vmf) {
	struct sharedm *shm;
	struct page *page;

	printk(KERN_DEBUG "mmdev : fault VMA \n");

	shm = vma->vm_private_data; //get address of page created at opening
	page = virt_to_page(shm->start);
	get_page(page); //inc refcount on page
	vmf->page = page;

	return 0;
}

static struct vm_operations_struct vm_ops = {
	.close = vm_close,
	.fault = vm_fault,
	.open = vm_open,
};

static int mmdev_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct task_struct *task;
	struct sharedm *shm;

	printk(KERN_DEBUG "mmdev : mmap attempt from %lx to %lx \n",
			vma->vm_start, vma->vm_end);

	shm = (struct sharedm *) file->private_data;
	vma->vm_ops = &vm_ops;
	vma->vm_flags = VM_WRITE | VM_READ | VM_SHARED;
	vma->vm_private_data = file->private_data;
	vm_open(vma);

	/*   create new kthread */
	task = kthread_create(read_messages, vma->vm_private_data, "readth");
        if (IS_ERR(task))
		return PTR_ERR(task);
        wake_up_process(task);

	shm->task = task;
	return 0;
}

static int mmdev_release(struct inode *inode, struct file *file)
{
	struct sharedm *shm;

	printk(KERN_DEBUG "mmdev : release device\n");

	shm = file->private_data;
	kthread_stop(shm->task);
	free_page(shm->start);
	kfree(shm);
	file->private_data = NULL;

	return 0;
}

static int mmdev_open(struct inode *inode, struct file *file)
{
	struct sharedm *shm;

	printk(KERN_DEBUG "mmdev : open device\n");

	shm = kmalloc(sizeof(struct sharedm), GFP_KERNEL);
	shm->start = get_zeroed_page(GFP_KERNEL);
	file->private_data = shm;
	return 0;
}

static struct file_operations mmdev_fops = {
	.owner		= THIS_MODULE,
	.mmap 		= mmdev_mmap,
	.open		= mmdev_open,
	.release	= mmdev_release,
};

static struct miscdevice mmdev_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "mmdev",
	.fops	= &mmdev_fops,
};

static void __exit exitfn(void)
{
	printk(KERN_INFO "mmdev : quit mmdev\n");
	misc_deregister(&mmdev_misc);

	return;
}

static int __init initfn(void)
{
	printk(KERN_INFO "mmdev : register mmdev\n");
	misc_register(&mmdev_misc);

	return 0;
}

module_init(initfn);
module_exit(exitfn);
