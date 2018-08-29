#define BENCH_ATTR_MAX		3 		/* arbitrary */
#define BENCH_GENL_FAMILY_NAME	"bench"
#define BENCH_GENL_VERSION 	0x1

/* * * * * * Lustre policies enumeration * * * * */

/* Configuration policy attributes, used for STATS */
enum {
	STATS_UNSPEC, /* Needed : Kernel don't parse type 0 */
	STATS_REQUEST,
	STATS_RAW,
	__BENCH_STATS_ATTR_MAX,
};
#define BENCH_STATS_ATTR_MAX (__BENCH_STATS_ATTR_MAX - 1)

/* Configuration policy attributes, used for IOC */
enum {
	IOC_UNSPEC, /* Needed : Kernel don't parse type 0 */
	IOC_REQUEST,
	IOC_RAW,
	__BENCH_IOC_ATTR_MAX,
};
#define BENCH_IOC_ATTR_MAX (__BENCH_IOC_ATTR_MAX - 1)

/* Configuration policy attributes, used for HSM */
enum {
	HSM_UNSPEC, /* Needed : Kernel don't parse type 0 */
	HSM_MAGIC,
	HSM_TRANSPORT,
	HSM_FLAGS,
	HSM_MSGTYPE,
	HSM_MSGLEN,
	__BENCH_HSM_ATTR_MAX,
};
#define BENCH_HSM_ATTR_MAX (__BENCH_HSM_ATTR_MAX - 1)
#define BENCH_MAX_ATTR BENCH_HSM_ATTR_MAX

/* * * * * * Available commands for Lustre * * * * */

enum {
	BENCH_CMD_STATS,
	BENCH_CMD_IOC,
	BENCH_CMD_HSM,
	__BENCH_TYPE_MAX,
};
#define BENCH_TYPE_MAX (__BENCH_TYPE_MAX - 1)

/* * * * * * IOC REQUEST codes * * * * */

#define IOC_REQUEST_EXAMPLE	1

/* * * * * * STATS REQUEST codes * * * * */

#define STATS_REQUEST_EXAMPLE	1

struct ioc_example_struct {
	int	example_int;
	short	example_short;
};
