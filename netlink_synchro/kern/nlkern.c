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

struct task_struct *task;
struct sock *nl_sock;
/* Determine if kernel carry on sending messages to listeners. */

wait_queue_head_t send_wq;
DECLARE_WAIT_QUEUE_HEAD(send_wq);


/* callback function for message reception. */
static void recv_message(struct sk_buff *skb)
{
	struct nlmsghdr *nlhr; //netlink header for received message
	int msg_size;

	nlhr = (struct nlmsghdr *)skb->data; //get header from received msg
	msg_size = nlmsg_len(nlhr); //get length of received message
}


/* Thread function sending messages (will die eventually). */
static int send_messages(void *data)
{
	struct sk_buff *skb_out; //SKB data to send
	struct nlmsghdr *nlha; //netlink header for answer
	static int seq = 0;
	int err;
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	uint64_t start, end;


	if (!netlink_has_listeners(nl_sock, MYMGRP))
		do_exit(0);

	start_timer();
	while (seq < NB_MSG) {
		/*  allocate SKBuffer freed in multicast */
		skb_out = nlmsg_new(MSG_SIZE, 0);
		if (!skb_out) {
			printk(KERN_ERR "nlkern: SKB mem alloc failed\n");
		}

		/* Link nlha to sbb_out */
		nlha = nlmsg_put(skb_out, 0, seq,
				NLMSG_DONE, MSG_SIZE, NLM_F_ACK);
		if (!nlha) {
			printk(KERN_ERR "nlkern: SKB mem insufficient for header\n");
		}

		memcpy(nlmsg_data(nlha), &seq, MSG_SIZE);
		nlmsg_end(skb_out, nlha); //end construction of nl message

		/* free skb_out meanwhile */
		err = nlmsg_multicast(nl_sock, skb_out, 0, MYMGRP, GFP_KERNEL);
		if (err != 0) {
			printk(KERN_ERR "[seq=%d] Sending error %d, waiting\n",
					seq, err);
		} else {
			seq ++;
		}
	}
	stop_timer();

	start = ( ((uint64_t)cycles_high << 32) | cycles_low );
	end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
	printk(KERN_INFO "%llu clock cycles\n", (end-start));
	do_exit(0);
}

static int __init initfn(void)
{
	struct netlink_kernel_cfg cfg = {
		.groups = MYMGRP,
		.input = recv_message,
		.flags = NL_CFG_F_NONROOT_SEND,
	};

	printk(KERN_INFO "nlkern: Register module\n");

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
	printk(KERN_INFO "nlkern: Quit module\n");

	netlink_kernel_release(nl_sock);
}

module_init(initfn);
module_exit(exitfn);
