#include <net/genetlink.h>
#include <net/netlink.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
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
 * Openvswitch defines four ways to communicate with netlink:
 *
 *   1. Transactions. Similar to syscalls, ioctl or RPC. userspace initiate the
 * 	request which kernel processes synchronously and reply to userspace.
 * 	Transactions implement lost cases by repeating a transaction multiple
 * 	time until it succeed.
 *
 *   2. Dumps. A dump asks
 *
 *   3. Multicast subscriptions.
 *
 *   4. Unicast subscriptions.
 *
 * BENCH provides a single family with 3 available commands/attribute policy:
 *
 *   1. BENCH_CMD_STATS. Transaction to expose statistics to userspace.
 *   	The STATS attribute policy uses nested attributes i.e. STATS_TREE
 *   	attribute contains attributes specified by the developper.
 *
 *   2. BENCH_CMD_IOCTL. Transaction to substitute ioctls.
 *	The IOCTL attribute policy uses REQUEST attribute to detect the type
 *	of request and ARGS as nested attribute to directly
 *
 *   3. BENCH_CMD_HSM. Asynchronous sending from kernel to userland.
 *	The HSM attribute policy provides attributes required for kernel to
 *	user space communication between MDC and copytools.
 *
 * Inspired from :
 *   drivers/block/nbd.c
 *   openvswitch netlink API
 */

/**
 * genlbench_stats_transact - Callback function for STATS transactions.
 * @param skb Incoming Skbuffer.
 * @param info Information for incoming message.
 */
static int genlbench_stats_transact(struct sk_buff *skb, struct genl_info *info)
{
	return 0;
}

/**
 * genlbench_ioc_transact - Callback function for IOCTL transactions.
 * @param skb Incoming Skbuffer.
 * @param info Information for incoming message.
 */
static int genlbench_ioc_transact(struct sk_buff *skb, struct genl_info *info)
{
	return 0;
}

/** genlbench_hsm_recv - Send back NLMSG_ERROR message to the process. */
static int genlbench_hsm_recv(struct sk_buff *skb, struct genl_info *info)
{
	return -EOPNOTSUPP;
}

/* attribute policy for STATS */
static struct nla_policy bench_stats_attr_policy[BENCH_STATS_ATTR_MAX + 1] = {
	[STATS_TREE]	=	{.type = NLA_BINARY},
};

/* attribute policy for IOCTL command */
static struct nla_policy bench_ioc_attr_policy[BENCH_IOC_ATTR_MAX + 1] = {
	[REQUEST]	=	{.type = NLA_U32},
	[ARGS]		= 	{.type = NLA_BINARY},
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
		.doit = genlbench_stats_transact, //mandatory
	},
	{
		.cmd = BENCH_CMD_IOCTL,
		.policy = bench_ioc_attr_policy,
		.doit = genlbench_ioc_transact, //mandatory
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
	.ops = bench_genl_ops,
	.n_ops = ARRAY_SIZE(bench_genl_ops),
	.module = THIS_MODULE,
};

/**
 * genlbench_hsm_unicast - Send unicast HSM message to a process.
 * @param portid destination port ID to send message.
 * @param magic
 * @param transport
 * @param flags
 * @param msgtype
 * @param msglen //TODO mandatory?
 */
static int genlbench_hsm_unicast(int portid, u16 magic, u8 transport, u8 flags,
				 u16 msgtype, u16 msglen)
{
	struct sk_buff *skb;
	void *msg_head; /* user specific header */
	static int seq = 0;
	int size;
	int rc;

	/* allocate space for message */
	size = 3*nla_attr_size(sizeof(u16)) + 2*nla_attr_size(sizeof(u8));
	skb = genlmsg_new(nla_total_size(size), GFP_KERNEL);
	if (skb)
		return -ENOMEM;

	/* create message */
	msg_head = genlmsg_put(skb, 0, seq, &bench_genl_family, 0, BENCH_CMD_HSM);
	if (!msg_head)
		goto msg_build_fail;

	rc = nla_put_u16(skb, MAGIC, magic);
	if (rc != 0)
		goto msg_build_fail;

	rc = nla_put_u8(skb, TRANSPORT, transport);
	if (rc != 0)
		goto msg_build_fail;

	rc = nla_put_u8(skb, FLAGS, flags);
	if (rc != 0)
		goto msg_build_fail;

	rc = nla_put_u16(skb, MSGTYPE, msgtype);
	if (rc != 0)
		goto msg_build_fail;

	rc = nla_put_u16(skb, MSGLEN, msglen);
	if (rc != 0)
		goto msg_build_fail;

	genlmsg_end(skb, msg_head);

	/* send message in init_net namespace */
	return genlmsg_unicast(&init_net, skb, portid);

msg_build_fail:
	nlmsg_free(skb);
	return -ENOMEM;
}

static int __init genlbench_init(void)
{
	int rc;

	printk(KERN_INFO "genlbench: module loaded\n");

	rc = genl_register_family(&bench_genl_family);
	if (rc != 0) {
		printk(KERN_ERR "failed to register bench family\n");
		return rc;
	}

	return 0;
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
