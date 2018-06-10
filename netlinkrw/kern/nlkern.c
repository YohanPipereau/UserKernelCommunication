/*
 *    Description:
 *        Created:  20/05/2018
 *         Author:  Yohan Pipereau
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <net/netlink.h>
#include <linux/mutex.h>
#include <linux/seq_file_net.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yohan Pipereau");
MODULE_DESCRIPTION("Multicast Hello world message !");

/* Multicast group */
#define MYMGRP 0x1

struct sock *nl_sock;


static void recv_message(struct sk_buff *skb)
{
	struct sk_buff *skb_out; //SKB data to send
	struct nlmsghdr *nlhr; //netlink header for received message
	int msg_size;
	struct nlmsghdr *nlha; //netlink header for answer
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	uint64_t start, end;

	nlhr = (struct nlmsghdr *)skb->data; //get header from received msg
	msg_size = nlmsg_len(nlhr); //get length of received message

	/* Start time measurement  */
	asm volatile ("CPUID\n\t"
	"RDTSC\n\t"
	"mov %%edx, %0\n\t"
	"mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low)::
	"%rax", "%rbx", "%rcx", "%rdx");

	/*  allocate SKBuffer freed in multicast */
	skb_out = nlmsg_new(msg_size, 0);
	if (!skb_out) {
	        printk(KERN_ERR "SKB mem alloc failed\n");
	}

	/* Link nlha to sbb_out */
	nlha = nlmsg_put(skb_out, nlhr->nlmsg_pid, nlhr->nlmsg_seq,
			NLMSG_DONE, msg_size, 0);
	if (!nlha) {
	        printk(KERN_ERR "SKB mem insufficient for header\n");
	}

	/*  copy head of message payload of receive message to message to send */
	strncpy(nlmsg_data(nlha), nlmsg_data(nlhr), msg_size);

	nlmsg_end(skb_out, nlha); //end construction of nl message

	/* free skb_out meanwhile */
	nlmsg_multicast(nl_sock, skb_out, 0, MYMGRP, GFP_KERNEL);

	/* End time measurement */
	asm volatile("RDTSCP\n\t"
	"mov %%edx, %0\n\t"
	"mov %%eax, %1\n\t"
	"CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
	"%rax", "%rbx", "%rcx", "%rdx");

	start = ( ((uint64_t)cycles_high << 32) | cycles_low );
	end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
	printk(KERN_INFO "%llu clock cycles\n", (end-start));
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
