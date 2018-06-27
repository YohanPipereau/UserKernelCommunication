/*
 *    Description: This module create a character device handling mmap callback
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

struct sharedm {
	unsigned long start; //maybe use private_data?
};

static int mmdev_release(struct inode *inode, struct file *file)
{
	struct sharedm *shm;

	printk(KERN_DEBUG "mmdev : release device\n");

	shm = file->private_data;
	printk(KERN_INFO "message %s\n", (char *)shm->start);
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

static void vm_open(struct vm_area_struct *vma) {
	printk(KERN_DEBUG "mmdev : open VMA ; virt %lx, phys %lx\n",
			vma->vm_start, vma->vm_pgoff << PAGE_SHIFT);
}

static void vm_close(struct vm_area_struct *vma) {
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
	printk(KERN_DEBUG "mmdev : mmap attempt from %lx to %lx \n",
			vma->vm_start, vma->vm_end);

	vma->vm_ops = &vm_ops;
	vma->vm_flags = VM_WRITE | VM_READ | VM_SHARED;
	vma->vm_private_data = file->private_data;
	vm_open(vma);
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
