#include <sys/socket.h>
#include <unistd.h>
#include <linux/netlink.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define MAX_PAYLOAD 1024 /* maximum payload size*/
#define MYMGRP 0x1

struct msghdr msg; //must be global if declared in main -> failed

int main()
{
	int sock_fd;
	struct iovec iov;
	struct sockaddr_nl src_addr, dest_addr;
	struct nlmsghdr *nlh = NULL;
	int group = MYMGRP;

	sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USERSOCK);
	if(sock_fd<0) {
		perror ("socket creation failed\n");
		return errno;
	}

	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_groups = MYMGRP;
	src_addr.nl_pid = getpid(); /* self pid */

	if (bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr)) < 0) {
		perror("binding failed");
		return errno;
	}

	if (setsockopt(sock_fd, 270, NETLINK_ADD_MEMBERSHIP, &group,
				sizeof(group)) < 0) {
        	perror("setsockopt < 0\n");
        	return -1;
    	}

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0; /* For Linux Kernel */
	dest_addr.nl_groups = MYMGRP;

	/*  This removes the need for the remote address to be explicitly
	 *  checked every time a datagram is received */
	if (connect(sock_fd,(struct sockaddr *) &dest_addr,
				sizeof(dest_addr)) == -1) {
		perror("connect failed\n");
		return errno;
	}

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
	nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_flags = 0;

	printf("Type message to send\n");
	while(1) {
		scanf("%s", (char*) NLMSG_DATA(nlh));
		iov.iov_base = (void *)nlh;
		iov.iov_len = nlh->nlmsg_len;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		if (sendmsg(sock_fd,&msg,0) == -1) {
			perror("sendmsg failed");
			return errno;
		}
	}
	free(nlh);
	nlh = NULL;
	close(sock_fd);
}
