#include <net/genetlink.h>
#include <net/netlink.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include "genlbench.h"

/*
 * This implementation is based on generic netlink.
 *
 * Message Format:
 * +--------+----------+-------------------+-------------------+---+
 * |NL HDR  |GENL HDR  |ATTR1 HDR|ATTR1 PAY|ATTR2 HDR|ATTR2 PAY|   |
 * |        |          |         |         |         |         |   |
 * |struct  |struct    |struct   |         |struct   |         |...|
 * |nlmsghdr|genlmsghdr|nlattr   |         |nlattr   |         |   |
 * +--------+----------+---------+---------+---------+---------+---+
 *
 * Inspired from drivers/block/nbd.c
 */

/* attribute policy for STATS */
static struct nla_policy bench_stats_attr_policy[BENCH_STATS_ATTR_MAX + 1] = {
	[STATS_TREE]	=	{.type = NLA_NESTED},
};

/* attribute policy for IOCTL command */
static struct nla_policy bench_ioc_attr_policy[BENCH_IOC_ATTR_MAX + 1] = {
	[REQUEST]	=	{.type = NLA_U32},
	[ARGS]		= 	{.type = NLA_NESTED},
};

/* attribute policy for HSM command */
static struct nla_policy bench_hsm_attr_policy[BENCH_HSM_ATTR_MAX + 1] = {
	[MAGIC]		=	{.type = NLA_U16},
	[TRANSPORT]	= 	{.type = NLA_U8},
	[FLAGS]		= 	{.type = NLA_U8},
	[MSGTYPE]	= 	{.type = NLA_U16},
	[MSGLEN]	= 	{.type = NLA_U16},
};

static struct genl_ops bench_genl_ops[] = {
	{
		.cmd = BENCH_CMD_STATS,
		.policy = bench_stats_attr_policy,
		.doit = genlbench_stats_echo,
	},
	{
		.cmd = BENCH_CMD_IOCTL,
		.policy = bench_ioc_attr_policy,
		.doit = genlbench_ioc_echo,
	},
	{
		.cmd = BENCH_CMD_HSM,
		.policy = bench_hsm_attr_policy,
		.doit = genlbench_hsm_recv,
	},
};

static struct genl_family bench_genl_family = {
	.hdrsize = 0, /* no family specific header */
	.name = BENCH_GENL_FAMILY_NAME,
	.version = BENCH_GENL_VERSION,
	.maxattr = BENCH_MAX_ATTR,
	.module = THIS_MODULE,
	.ops = bench_genl_ops,
	.n_ops = ARRAY_SIZE(bench_genl_ops),
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
