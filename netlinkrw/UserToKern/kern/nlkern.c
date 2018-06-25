/*
 *    Description:
 *        Created:  20/05/2018
 *         Author:  Yohan Pipereau
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <net/netlink.h>
#include <linux/preempt.h>
#include <linux/hardirq.h>
#include <linux/mutex.h>
#include <linux/seq_file_net.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yohan Pipereau");
MODULE_DESCRIPTION("Netlink : receive message from user process.");

/* Multicast group */
#define MYMGRP 0x1

struct sock *nl_sock;
unsigned long flags;

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

static void recv_message(struct sk_buff *skb)
{
	struct nlmsghdr *nlhr; //netlink header for received message
	int msg_size;

	nlhr = (struct nlmsghdr *)skb->data; //get header from received msg
	msg_size = nlmsg_len(nlhr); //get length of received message
}

static int __init initfn(void)
{
	struct netlink_kernel_cfg cfg = {
		.groups = MYMGRP,
		.input = recv_message,
		.flags = NL_CFG_F_NONROOT_SEND,
	};

	printk(KERN_INFO "Register nlkern \n");

	preempt_disable(); /*disable preemption on our CPU*/
	raw_local_irq_save(flags); /*disable hard interrupts on our CPU*/

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

	raw_local_irq_restore(flags); /*enable hard interrupts on our CPU*/
	preempt_enable();/*we enable preemption*/
}

module_init(initfn);
module_exit(exitfn);
