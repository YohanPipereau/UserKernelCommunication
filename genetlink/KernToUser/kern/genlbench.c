#include <net/genetlink.h>
#include <net/netlink.h>
#include <linux/module.h>

/*
 * This implementation is based on generic netlink.
 *
 * Message Formats :
 * +---------+---------------------------+-----------------+
 * |NL HDR   |   |ATTR1 HDR|   |ATTR1 PAY|   |ATTR2 HDR|   |
 * |struct   |pad|struct   |pad|attribute|pad|struct   |...|
 * |nlmsghdr |   |nlattr   |   |payload  |   |nlattr   |   |
 * +---------+-------------+-------------+-------------+---+
 *
 */


#define BENCH_ATTR_MAX		5 		/* arbitrary */
#define BENCH_GENL_FAMILY_NAME	"bench"
#define BENCH_GENL_VERSION 	0x1

/** genlbench_unicast - send a unicast message to a userland process.
 * @param msg Message to send.
 * @param len Length of the message.
 * @param pid PID of the destination processus.
 */
static int genlbench_unicast(char *msg, int len, int pid);

static struct genl_family bench_genl_family = {
	.hdrsize = 0, /* no family specific header */
	.name = BENCH_GENL_FAMILY_NAME,
	.version = BENCH_GENL_VERSION,
	.maxattr = BENCH_ATTR_MAX,
	.module = THIS_MODULE,
};

static int __init genlbench_init(void)
{
	printk(KERN_INFO "genlbench: module loaded\n");

	if (genl_register_family(&bench_genl_family))
		goto family_register_fail;

	return 0;

family_register_fail:
	printk(KERN_ERR "failed to register bench fammily\n");
	return -EINVAL;
}


static void __exit genlbench_exit(void)
{
	printk(KERN_INFO "genlbench: module unloaded\n");

	genl_unregister_family(&bench_genl_family);
}

module_init(genlbench_init);
module_exit(genlbench_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("User-Kernel communication with genetlink");
MODULE_AUTHOR("YohanPipereau");
