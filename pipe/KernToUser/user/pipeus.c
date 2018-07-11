/*
 *    Description:  userland part in charge of creating pipe and receving data
 *        Created:  09/07/2018
 *         Author:  Yohan Pipereau
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define NB_MSG 100
#define MAXSIZE 4096
#define FILENAME "/dev/pipe_misc"

/* struct imported from Lustre project */
struct msg_hdr {
	int msglen;
	int seq;
};

static inline char *msg_data(const struct msg_hdr *msg)
{
	return (char*) msg + sizeof(struct msg_hdr);
}

int main ( int argc, char *argv[] )
{
	int pipefd[2];
	int misc_fd;
	struct msg_hdr *msg;
	int rc;
	char *buf;

	buf = malloc(MAXSIZE);

	if (pipe(pipefd) < 0)
		return errno;

	misc_fd = open(FILENAME, O_WRONLY);
	if (misc_fd < 0)
		return errno;

	printf("size of msghdr : %d\n", sizeof(struct msg_hdr));
	rc = write(misc_fd, &pipefd[1], sizeof(int));
	if (rc < 0)
		return errno;

	do {
		/*  Read header */
		rc = read(pipefd[0], buf, sizeof(struct msg_hdr));
		if (rc < 0)
			return errno;

		msg = (struct msg_hdr*) buf;

		printf("header read\n");

		/* Message can't be bigger than maxsize */
		if (msg->msglen > MAXSIZE)
			return -EMSGSIZE;

		/* Read payload */
		rc = read(pipefd[0], msg_data(msg),
				msg->msglen - sizeof(struct msg_hdr));
		if (rc < 0)
			return errno;

		printf("[seq=%d] %s\n", msg->seq, msg_data(msg));
	} while (msg->seq < NB_MSG-1);

	close(misc_fd);
	close(pipefd[0]);
	close(pipefd[1]);

	return EXIT_SUCCESS;
}


