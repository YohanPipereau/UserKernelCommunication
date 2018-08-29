#include "../genlbench.h"

/* Contains possible callbacks for STATS command */
struct genlbench_stats_answers {
	int (*example)(void);
};

/* Contains possible callbacks for IOC command */
struct genlbench_ioc_operations {
	int (*example)(void);
};

/* Send unicast message */
static int genlbench_hsm_unicast(int portid, u16 magic, u8 transport, u8 flags,
				 u16 msgtype, u16 msglen);

/* Callbacks for command reception */
static int genlbench_stats_transact(struct sk_buff *skb, struct genl_info *info);
static int genlbench_ioc_transact(struct sk_buff *skb, struct genl_info *info);
