#include <net/genetlink.h>
#include <net/netlink.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include "genlbench_internal.h"

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
 *	There is no STATS attribute policy. Statistics are copied in payload.
 *
 *   2. BENCH_CMD_IOC. Transaction to substitute ioctls.
 *	The IOC attribute policy uses IOC_REQUEST attribute to detect the type
 *	of request. Arguments are copied in payload.
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
 * genlbench_ioc_transact - Callback function for IOC transactions.
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

static struct genl_ops bench_genl_ops[] = {
	{
		.cmd = BENCH_CMD_STATS,
		.doit = genlbench_stats_transact, //mandatory
	},
	{
		.cmd = BENCH_CMD_IOC,
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
	int rc;

	/* allocate space for message */
	skb = genlmsg_new(BENCH_HSM_ATTR_SIZE, GFP_KERNEL);
	if (skb)
		return -ENOMEM;

	/* create message */
	msg_head = genlmsg_put(skb, 0, seq, &bench_genl_family, 0, BENCH_CMD_HSM);
	if (!msg_head)
		goto msg_build_fail;

	rc = nla_put_u16(skb, HSM_MAGIC, magic);
	if (rc != 0)
		goto msg_build_fail;

	rc = nla_put_u8(skb, HSM_TRANSPORT, transport);
	if (rc != 0)
		goto msg_build_fail;

	rc = nla_put_u8(skb, HSM_FLAGS, flags);
	if (rc != 0)
		goto msg_build_fail;

	rc = nla_put_u16(skb, HSM_MSGTYPE, msgtype);
	if (rc != 0)
		goto msg_build_fail;

	rc = nla_put_u16(skb, HSM_MSGLEN, msglen);
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
