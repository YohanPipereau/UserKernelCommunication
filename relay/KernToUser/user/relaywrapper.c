/*
 *    Description:  Wrapper to read kernel info sentt using relay API.
 *        Created:  05/07/2018
 *         Author:  Yohan Pipereau
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <omp.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>

#define SUBBUF_SIZE 262144
#define N_SUBBUFS 4
#define MSG_SIZE 1024

int main ( int argc, char *argv[] )
{
	int nb_proc = get_nprocs();
	int fd[nb_proc];
	char *buf[nb_proc];
	char base[] = "/sys/kernel/debug/relay/relay";
	char *filename;
	int size;

	/*  Opening */
	for (int i = 0; i < nb_proc; i++) {
		filename = malloc(strlen(base)+i/10);
		sprintf(filename, "%s%d",base,i);
		fd[i] = open(filename, O_RDONLY);
		free(filename);
		filename = NULL;
		if (!fd[i]) {
			perror("failed opening");
			return errno;
		}
		buf[i] = malloc(MSG_SIZE);
	}

	//#pragma omp parallel
	for (int i = 0; i < nb_proc; i++) {
		size = read(fd[i], buf[i], MSG_SIZE);
		if (size != 0)
			printf("[msg] %s\n", buf[i]);
	}


	/* Closing */
	for (int i = 0; i < nb_proc; i++) {
		free(buf[i]);
		buf[i] = NULL;
		close(fd[i]);
		printf("File relay/relay%d closed\n", i);
	}

	return EXIT_SUCCESS;
}

