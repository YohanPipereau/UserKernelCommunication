/*
 *    Description: Please register this module after launching nlreaders.
 *        Created:  20/05/2018
 *         Author:  Yohan Pipereau
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <net/netlink.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/seq_file_net.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yohan Pipereau");
MODULE_DESCRIPTION("Netlink : Send message from kernel land to user land.");

/* Multicast group */
#define MYMGRP 21
#define MSG_SIZE 1024
#define NB_MSG 100000

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

static struct task_struct *task;
static struct sock *nl_sock;

/* Thread function sending messages (will die eventually). */
static int send_messages(void *data)
{
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	struct sk_buff *skb; //SKB data to send
	struct nlmsghdr *nlha; //netlink header for answer
	uint64_t start, end;
	static int seq = 0;
	int rc;

	if (!netlink_has_listeners(nl_sock, MYMGRP))
		do_exit(0);

	start_timer();
	while (seq < NB_MSG) {
		/*  allocate SKBuffer freed in multicast */
		skb = nlmsg_new(MSG_SIZE, 0);
		if (!skb) {
			printk(KERN_ERR "SKB mem alloc failed\n");
			return -ENOMEM;
		}

		/* Link nlha to sbb_out */
		nlha = nlmsg_put(skb, 0, seq, NLMSG_DONE, MSG_SIZE, 0);
		if (!nlha) {
			printk(KERN_ERR "SKB mem insufficient for header\n");
			return -ENOMEM;
		}

		memcpy(nlmsg_data(nlha), &seq, MSG_SIZE);
		nlmsg_end(skb, nlha); //end construction of nl message

		/* free skb meanwhile */
		rc = nlmsg_multicast(nl_sock, skb, 0, MYMGRP, GFP_KERNEL);
		if (rc != 0)
			printk(KERN_ERR "[seq=%d] Sending error %d, waiting\n",
					seq, rc);
		else
			seq ++;
	}
	stop_timer();

	start = ((uint64_t)cycles_high << 32) | cycles_low;
	end = ((uint64_t)cycles_high1 << 32) | cycles_low1;
	printk(KERN_INFO "%llu clock cycles\n", end-start);
	do_exit(0);
}

static int __init nlkern_init(void)
{
	struct netlink_kernel_cfg cfg = {
		.groups = MYMGRP,
		.flags = NL_CFG_F_NONROOT_SEND,
	};
	int rc;

	printk(KERN_INFO "Register module\n");

	nl_sock = netlink_kernel_create(&init_net, NETLINK_USERSOCK, &cfg);
	if (!nl_sock) {
		printk(KERN_ALERT "error creating socket\n");
		return -ECHILD;
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
	return rc;
}

static void __exit nlkern_exit(void)
{
	printk(KERN_INFO "Quit module\n");

	netlink_kernel_release(nl_sock);
}

module_init(nlkern_init);
module_exit(nlkern_exit);
