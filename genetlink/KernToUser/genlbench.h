#define BENCH_ATTR_MAX		3 		/* arbitrary */
#define BENCH_GENL_FAMILY_NAME	"bench"
#define BENCH_GENL_VERSION 	0x1

/* Configuration policy attributes, used for STATS */
enum {
	STATS_TREE,
	__BENCH_STATS_ATTR_MAX,
};
#define BENCH_STATS_ATTR_MAX (__BENCH_STATS_ATTR_MAX - 1)

/* Configuration policy attributes, used for IOC */
enum {
	IOC_REQUEST,
	IOC_ARGS,
	__BENCH_IOC_ATTR_MAX,
};
#define BENCH_IOC_ATTR_MAX (__BENCH_IOC_ATTR_MAX - 1)

/* Configuration policy attributes, used for HSM */
enum {
	HSM_MAGIC,
	HSM_TRANSPORT,
	HSM_FLAGS,
	HSM_MSGTYPE,
	HSM_MSGLEN,
	__BENCH_HSM_ATTR_MAX,
};
#define BENCH_HSM_ATTR_MAX (__BENCH_HSM_ATTR_MAX - 1)
#define BENCH_MAX_ATTR BENCH_HSM_ATTR_MAX

/* Available commands for Lustre */
enum {
	BENCH_CMD_STATS,
	BENCH_CMD_IOCTL,
	BENCH_CMD_HSM,
	__BENCH_TYPE_MAX,
};
#define BENCH_TYPE_MAX (__BENCH_TYPE_MAX - 1)

/* attribute policy for STATS */
static struct nla_policy bench_stats_attr_policy[BENCH_STATS_ATTR_MAX + 1] = {
	[STATS_TREE]	=	{.type = NLA_BINARY},
};
#define BENCH_HSM_ATTR_SIZE 	nla_total_size(3*nla_attr_size(16) \
					       + 2*nla_attr_size(8))

/* attribute policy for IOCTL command */
static struct nla_policy bench_ioc_attr_policy[BENCH_IOC_ATTR_MAX + 1] = {
	[IOC_REQUEST]	=	{.type = NLA_U32},
	[IOC_ARGS]	= 	{.type = NLA_BINARY},
};

/* attribute policy for HSM command */
static struct nla_policy bench_hsm_attr_policy[BENCH_HSM_ATTR_MAX + 1] = {
	[HSM_MAGIC]		=	{.type = NLA_U16},
	[HSM_TRANSPORT]		= 	{.type = NLA_U8},
	[HSM_FLAGS]		= 	{.type = NLA_U8},
	[HSM_MSGTYPE]		= 	{.type = NLA_U16},
	[HSM_MSGLEN]		=	{.type = NLA_U16},
};
