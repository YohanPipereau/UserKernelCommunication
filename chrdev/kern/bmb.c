#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/kref.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/rwsem.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/vmalloc.h>
#include <linux/preempt.h>
#include <linux/hardirq.h>

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
MODULE_AUTHOR("Quentin Bouget");

struct record {
	struct list_head	list;
	struct kref		kref;
	size_t			size;
	char			message[];
};

static DECLARE_RWSEM(records_lock);
static DECLARE_WAIT_QUEUE_HEAD(record_wqueue);
unsigned long flags;

static void
record_release(struct kref *kref)
{
	struct record *record = container_of(kref, struct record, kref);

	down_write(&records_lock);
	list_del(&record->list);
	up_write(&records_lock);
	if (sizeof(*record) + record->size <= PAGE_SIZE)
		kfree(record);
	else
		vfree(record);
}

static struct record *dummy_record;
#define records_list (&dummy_record->list)
static atomic_t reader_count = ATOMIC_INIT(0);

static int
bmb_open(struct inode *inode, struct file *file)
{
	if (file->f_mode & FMODE_READ) {
		struct record *record;

		down_write(&records_lock);
		record = list_last_entry(records_list, struct record, list);
		if (!kref_get_unless_zero(&record->kref)) {
			kref_get(&dummy_record->kref);
			record = dummy_record;
		}
		atomic_inc(&reader_count);
		up_write(&records_lock);

		file->private_data = record;
	}
	return 0;
}

static bool
something_to_read(struct record *record)
{
	return !list_is_last(&record->list, records_list);
}

static ssize_t
bmb_read(struct file *file, char __user *buffer, size_t count, loff_t *offset)
{
	struct record *record = file->private_data;
	struct record *next;
	struct list_head *last;
	size_t read_bytes = 0;
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	uint64_t start, end;

	if (file->f_flags & O_NONBLOCK && !something_to_read(record))
		return -EWOULDBLOCK;
	else if (wait_event_interruptible(record_wqueue,
					  something_to_read(record)))
		return -ERESTARTSYS;

	down_read(&records_lock);
	last = records_list->prev;
	up_read(&records_lock);

	list_for_each_entry_continue(record, last, list) {
		if (record->size > count - read_bytes)
			break;

		start_timer();
		if (copy_to_user(buffer, record->message, record->size))
			return -EFAULT;
		stop_timer();

		read_bytes += record->size;
		buffer += record->size;
	}
	if (last == &record->list && record->size <= count - read_bytes) {
		start_timer();
		if (copy_to_user(buffer, record->message, record->size))
			return -EFAULT;
		stop_timer();
		read_bytes += record->size;
	} else {
		last = record->list.prev;
	}

	record = file->private_data;
	list_for_each_entry_safe_from(record, next, last, list)
		kref_put(&record->kref, record_release);

	file->private_data = record;

	start = ( ((uint64_t)cycles_high << 32) | cycles_low );
	end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
	printk(KERN_INFO "%llu clock cycles :: Kern>>User\n", (end-start));

	return read_bytes;
}

static ssize_t
bmb_write(struct file *file, const char __user *buffer, size_t count,
	    loff_t *offset)
{
	struct record *record;
	unsigned int _reader_count = atomic_read(&reader_count);
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	uint64_t start, end;

	if (_reader_count == 0)
		return count;

	if (sizeof(*record) + count <= PAGE_SIZE)
		record = kmalloc(sizeof(*record) + count, GFP_KERNEL);
	else
		record = vmalloc(sizeof(*record) + count);
	if (record == NULL)
		return -ENOMEM;

	start_timer();
	if (copy_from_user(record->message, buffer, count)) {
		if (sizeof(*record) + count <= PAGE_SIZE)
			kfree(record);
		else
			vfree(record);
		return -EFAULT;
	}
	stop_timer();
	record->size = count;
	kref_init(&record->kref);

	down_write(&records_lock);
	_reader_count = atomic_read(&reader_count);
	if (_reader_count == 0) {
		up_write(&records_lock);
		if (sizeof(*record) + count <= PAGE_SIZE)
			kfree(record);
		else
			vfree(record);
		return count;
	}
	for (; _reader_count > 1; _reader_count--)
		kref_get(&record->kref);
	list_add_tail(&record->list, records_list);
	up_write(&records_lock);

	wake_up_interruptible(&record_wqueue);

	start = ( ((uint64_t)cycles_high << 32) | cycles_low );
	end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
	printk(KERN_INFO "%llu clock cycles :: User>Kern\n", (end-start));

	return count;
}

static int
bmb_release(struct inode *inode, struct file *file)
{
	if (file->f_mode & FMODE_READ) {
		struct record *record = file->private_data;
		struct record *next;
		struct list_head *last;

		down_write(&records_lock);
		last = records_list->prev;
		atomic_dec(&reader_count);
		up_write(&records_lock);

		list_for_each_entry_safe_from(record, next, last, list)
			kref_put(&record->kref, record_release);
		kref_put(&record->kref, record_release);
	}
	return 0;
}

static struct file_operations bmb_fops = {
	.owner		= THIS_MODULE,
	.open		= bmb_open,
	.read		= bmb_read,
	.write		= bmb_write,
	.release	= bmb_release,
};

static struct miscdevice bmb_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "bmb",
	.fops	= &bmb_fops,
};

static int __init
bmb_init(void)
{
	int rc;

	printk(KERN_INFO "Register bmb \n");

	preempt_disable(); /*disable preemption on our CPU*/
	raw_local_irq_save(flags); /*disable hard interrupts on our CPU*/

	dummy_record = kmalloc(sizeof(*dummy_record), GFP_KERNEL);
	if (dummy_record == NULL)
		return -ENOMEM;
	INIT_LIST_HEAD(&dummy_record->list);
	kref_init(&dummy_record->kref);
	dummy_record->size = 0;

	rc = misc_register(&bmb_misc);
	if (rc < 0)
		kref_put(&dummy_record->kref, record_release);

	return rc;
}

static void __exit
bmb_exit(void)
{
	misc_deregister(&bmb_misc);

	if (!kref_put(&dummy_record->kref, record_release))
		printk(KERN_ALERT "Broken refcount!\n");

	raw_local_irq_restore(flags); /*enable hard interrupts on our CPU*/
	preempt_enable();/*we enable preemption*/

	printk(KERN_INFO "Quit bmb \n");

	return;
}

module_init(bmb_init);
module_exit(bmb_exit);
