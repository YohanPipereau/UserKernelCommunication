#include <linux/module.h>
#include <linux/slab.h>
#include "genlbench.h"

struct genlbench_member *member;

static int examplefn(void)
{
	printk(KERN_DEBUG "example callback test\n");

	return 0;
}

static int __init genlbench_test_init(void)
{
	printk(KERN_INFO "genlbench_test: Load module\n");

	member = kmalloc(sizeof(*member), GFP_KERNEL);
	if (!member)
		return -ENOMEM;

	member->stats_ans = kmalloc(sizeof(struct genlbench_stats_answers),
				    GFP_KERNEL);
	if (!member->stats_ans) {
		kfree(member);
		return -ENOMEM;
	}

	member->ioc_ops = kmalloc(sizeof(struct genlbench_ioc_operations),
				  GFP_KERNEL);
	if (!member->ioc_ops) {
		kfree(member);
		kfree(member->stats_ans);
		return -ENOMEM;
	}

	member->stats_ans->example = &examplefn;

	INIT_LIST_HEAD(&member->list);
	genlbench_register(member);

	return 0;
}

static void __exit genlbench_test_exit(void)
{
	printk(KERN_INFO "genlbench_test: Unload module\n");

	genlbench_unregister(member);

	kfree(member->stats_ans);
	kfree(member->ioc_ops);
	kfree(member);
}

module_init(genlbench_test_init);
module_exit(genlbench_test_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Bench Generic netlink test module");
MODULE_AUTHOR("YohanPipereau");
