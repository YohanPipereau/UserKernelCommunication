/*
 *    Description:
 *        Created:  20/05/2018
 *         Author:  Yohan Pipereau
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <net/netlink.h>
#include <linux/seq_file_net.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yohan Pipereau");
MODULE_DESCRIPTION("Multicast Hello world message !");

/* Multicast group */
#define MYMGRP 0x1

struct sock *nl_sock;

static void recv_message(struct sk_buff *skb)
{
	int err;
	struct sk_buff *skb_out; //data received in socket is represented by SKB
	struct nlmsghdr *nlheader;
	int msg_size;

	printk(KERN_INFO "receiving message from uspace\n");

	nlheader = (struct nlmsghdr *)skb->data; //get header from received msg
	printk(KERN_INFO "Netlink received msg payload:%s\n",
	                (char *)nlmsg_data(nlheader));
	msg_size = nlmsg_len(nlheader); //get length of received message

	/*  allocate SKBuffer */
	skb_out = nlmsg_new(msg_size, 0);
	if (!skb_out) {
	        printk(KERN_ERR "SKB mem alloc failed\n");
	        return;
	}

	nlheader = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
	if (!nlheader) {
	        printk(KERN_ERR "SKB mem insufficient for header\n");
	        return;
	}

	err = nlmsg_multicast(nl_sock, skb_out, 0, MYMGRP, GFP_KERNEL);
	if (err < 0) {
		printk(KERN_INFO "Multicast failed\n");
	}
}

static int __init initfn(void)
{
	struct netlink_kernel_cfg cfg = {
		.groups = MYMGRP,
		.input = recv_message,
		.flags = NL_CFG_F_NONROOT_SEND,
	};

	printk(KERN_INFO "Register nlkern \n");

	nl_sock = netlink_kernel_create(&init_net, NETLINK_USERSOCK, &cfg);
	if (!nl_sock) {
		printk(KERN_ALERT "Error creating socket\n");
		return -10;
	}

	return 0;
}

static void __exit exitfn(void)
{
	printk(KERN_INFO "Quit nlkern\n");

	netlink_kernel_release(nl_sock);
}

module_init(initfn);
module_exit(exitfn);
