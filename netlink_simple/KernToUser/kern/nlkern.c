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
MODULE_DESCRIPTION("Netlink: send message from kernel land to user land.");

/* Multicast group */
#define MYMGRP 0x1
#define MSG_SIZE 1024
#define NB_MSG 1024

static struct task_struct *task;
static struct sock *nl_sock;
static unsigned long flags;

/*
 * Thread function that sends a few messages to the netlink socket and
 * exits on its own.
 */
static int send_messages(void *data)
{
	unsigned int cycles_low, cycles_high, cycles_low1, cycles_high1;
	struct nlmsghdr *nlha;
	struct sk_buff *skb;
	uint64_t start, end;
	int i;

	/* Start time measurement; CPUID prevent out of order execution */
	asm volatile ("CPUID\n\t"
		      "RDTSC\n\t"
		      "mov %%edx, %0\n\t"
		      "mov %%eax, %1\n\t": "=r" (cycles_high),
		      "=r" (cycles_low):: "%rax", "%rbx", "%rcx", "%rdx");

	/* Send NB_MSG to the netlink socket */
	for (i = 0; i < NB_MSG; i++) {
		skb = nlmsg_new(MSG_SIZE, 0);
		if (!skb) {
			printk(KERN_ERR "SKB mem alloc failed\n");
			return -ENOMEM;
		}

		/* Build the message to send */

		/* Build the header */
		nlha = nlmsg_put(skb, 0, i, NLMSG_DONE, MSG_SIZE, 0);
		if (!nlha) {
			printk(KERN_ERR "SKB mem insufficient for header\n");
			return -ENOMEM;
		}

		/* Include the payload */
		snprintf(nlmsg_data(nlha), 2, "A");

		/* Conclude the contruction of the message */
		nlmsg_end(skb, nlha);

		/* Send the buffer and decrement its refcount */
		nlmsg_multicast(nl_sock, skb, 0, MYMGRP, GFP_KERNEL);
	}

	/* End time measurement */
	asm volatile("RDTSCP\n\t"
		     "mov %%edx, %0\n\t"
		     "mov %%eax, %1\n\t"
		     "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
		     "%rax", "%rbx", "%rcx", "%rdx");

	start = ((uint64_t)cycles_high << 32) | cycles_low;
	end = ((uint64_t)cycles_high1 << 32) | cycles_low1;
	printk(KERN_INFO "%llu clock cycles\n", end - start);

	do_exit(0);
}

static int __init nlkern_init(void)
{
	struct netlink_kernel_cfg cfg = {
		.groups = MYMGRP,
		.flags = NL_CFG_F_NONROOT_SEND,
	};
	int rc;

	printk(KERN_INFO "Register nlkern\n");

	/* It is important to disable preemption and hard interrupts in order
	 * for the clock cycle measurements to be accurate.
	 */
	preempt_disable();
	raw_local_irq_save(flags);

	nl_sock = netlink_kernel_create(&init_net, NETLINK_USERSOCK, &cfg);
	if (!nl_sock) {
		printk(KERN_ALERT "Error creating socket\n");
		rc = -ECHILD;
		goto out_restore_preemption_and_interrupts;
	}

	/* Create the thread that will send messages to the netlink socket */
	task = kthread_create(send_messages, NULL, "netlink-kproducer");
	if (IS_ERR(task)) {
		rc = PTR_ERR(task);
		goto out_netlink_kernel_release;
	}

	wake_up_process(task);
	return 0;

out_netlink_kernel_release:
	netlink_kernel_release(nl_sock);
out_restore_preemption_and_interrupts:
	preempt_enable(); /* we enable preemption */
	raw_local_irq_restore(flags); /* enable hard interrupts on our CPU */
	return rc;
}

static void __exit nlkern_exit(void)
{
	printk(KERN_INFO "Quit nlkern\n");

	netlink_kernel_release(nl_sock);

	raw_local_irq_restore(flags);
	preempt_enable();
}

module_init(nlkern_init);
module_exit(nlkern_exit);
