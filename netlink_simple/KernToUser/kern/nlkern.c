/*
 *    Description: Please register this module after launching nlreaders.
 *        Created:  20/05/2018
 *         Author:  Yohan Pipereau
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <net/netlink.h>
#include <linux/preempt.h>
#include <linux/hardirq.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/seq_file_net.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yohan Pipereau");
MODULE_DESCRIPTION("Netlink : Send message from kernel land to user land.");

/* Multicast group */
#define MYMGRP 0x1
#define MSG_SIZE 1024
#define NB_MSG 1024

struct task_struct *task;
struct sock *nl_sock;
unsigned long flags;

/*
 * this is a thread function which will die eventually
 */
static int send_messages(void *data)
{
	struct sk_buff *skb_out; //SKB data to send
	struct nlmsghdr *nlha; //netlink header for answer
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	uint64_t start, end;
	static int seq = 0;
	int i;

	/* Start time measurement; CPUID prevent out of order execution */
	asm volatile ("CPUID\n\t"
			"RDTSC\n\t"
			"mov %%edx, %0\n\t"
			"mov %%eax, %1\n\t": "=r" (cycles_high),
			"=r" (cycles_low):: "%rax", "%rbx", "%rcx", "%rdx");

	for (i = 0; i < NB_MSG; i++) {
		/*  allocate SKBuffer freed in multicast */
		skb_out = nlmsg_new(MSG_SIZE, 0);
		if (!skb_out) {
			printk(KERN_ERR "SKB mem alloc failed\n");
		}

		/* Link nlha to sbb_out */
		nlha = nlmsg_put(skb_out, 0, seq,
				NLMSG_DONE, MSG_SIZE, 0);
		if (!nlha) {
			printk(KERN_ERR "SKB mem insufficient for header\n");
		}

		memcpy(nlmsg_data(nlha), "A", MSG_SIZE);
		nlmsg_end(skb_out, nlha); //end construction of nl message

		/* free skb_out meanwhile */
		nlmsg_multicast(nl_sock, skb_out, 0, MYMGRP, GFP_KERNEL);
		seq ++;
	}

	/* End time measurement */
	asm volatile("RDTSCP\n\t"
			"mov %%edx, %0\n\t"
			"mov %%eax, %1\n\t"
			"CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
			"%rax", "%rbx", "%rcx", "%rdx");

	start = ( ((uint64_t)cycles_high << 32) | cycles_low );
	end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
	printk(KERN_INFO "%llu clock cycles\n", (end-start));

	do_exit(0);
}

static int __init initfn(void)
{
	struct netlink_kernel_cfg cfg = {
		.groups = MYMGRP,
		.flags = NL_CFG_F_NONROOT_SEND,
	};

	printk(KERN_INFO "Register nlkern \n");

	preempt_disable(); /*disable preemption on our CPU*/
	raw_local_irq_save(flags); /*disable hard interrupts on our CPU*/

	/*  create netlink socket */
	nl_sock = netlink_kernel_create(&init_net, NETLINK_USERSOCK, &cfg);
	if (!nl_sock) {
		printk(KERN_ALERT "Error creating socket\n");
		return -10;
	}

	/*  create new kthread */
	task = kthread_create(send_messages, NULL, "netlink-thread");
        if (IS_ERR(task))
                return PTR_ERR(task);

        wake_up_process(task);
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
