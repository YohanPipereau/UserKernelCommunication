#include "../genlbench.h"

/* Send unicast message */
static int genlbench_stats_unicast(char *msg, int len);
static int genlbench_ioc_unicast(char *msg, int len);
static int genlbench_hsm_unicast(int portid, u16 magic, u8 transport, u8 flags,
				 u16 msgtype, u16 msglen);

/* Callbacks for command reception */
//TODO : Should stats be pulled by userland process when needed only???
//ie Are stats a transaction ?
static int genlbench_stats_transact(struct sk_buff *skb, struct genl_info *info);
static int genlbench_ioc_transact(struct sk_buff *skb, struct genl_info *info);
