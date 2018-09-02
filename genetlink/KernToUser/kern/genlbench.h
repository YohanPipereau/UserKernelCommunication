#include "../genlbench.h"

/* Contains possible callbacks for STATS command */
struct genlbench_stats_answers {
	int (*example)(void);
};

/* Contains possible callbacks for IOC command */
struct genlbench_ioc_operations {
	int (*example)(void);
};

/* Informations to register a module */
struct genlbench_member {
	struct list_head 		list;
	struct genlbench_ioc_operations *ioc_ops;
	struct genlbench_stats_answers 	*stats_ans;
};

/* Register/Unregister a member using IOC data or answering to STATS request */
extern int genlbench_register(struct genlbench_member *member);
extern int genlbench_unregister(struct genlbench_member *member);

/* Send unicast message */
extern int genlbench_hsm_unicast(int portid, u16 magic, u8 transport, u8 flags,
				 u16 msgtype, u16 msglen);
