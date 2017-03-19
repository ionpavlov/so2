/*
 * SO2 Lab - Linux device drivers (#4)
 * User-space test file
 */

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "../include/so2_cdev.h"

#define DEVICE_PATH	"/dev/so2_cdev"
char buffer[BUFSIZ];
/*
 * prints error message and exits
 */


static void error(const char *message)
{
	perror(message);
	exit(EXIT_FAILURE);
}

/*
 * print use case
 */

static void usage(const char *argv0)
{
	printf("Usage: %s <options>\n options:\n"
			"\tp - print\n"
			"\ts string - set buffer\n"
			"\tg - get buffer\n"
			"\td - down\n"
			"\tu - up\n", argv0);
	exit(EXIT_FAILURE);
}

/*
 * Sample run:
 *  ./so2_cdev_test p		; print ioctl message
 *  ./so2_cdev_test d		; wait on wait_queue
 *  ./so2_cdev_test u		; wait on wait_queue
 */

int main(int argc, char **argv)
{
	int fd, id;

	if (argc < 2)
		usage(argv[0]);

	if (strlen(argv[1]) != 1)
		usage(argv[0]);
	
	fd = open(DEVICE_PATH, O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	switch (argv[1][0]) {
	case 'p':				/* print */
		id = ioctl(fd, MY_IOCTL_PRINT, NULL);
		if (id < 0)
			perror("print");
		break;
	case 's':				/* set buffer */
		if (argc < 3)
			usage(argv[0]);
		id = ioctl(fd, MY_IOCTL_SET_BUFFER, argv[2]);
		if (id < 0) {
			perror("set");
		}
		break;
	case 'g':				/* get buffer */
		id = ioctl(fd, MY_IOCTL_GET_BUFFER, buffer);
		if (id < 0)
			perror("get");
		printf("%s\n", buffer);
		break;
	case 'd':				/* down */
		id = ioctl(fd, MY_IOCTL_DOWN, NULL);
		if (id < 0)
			perror("down");
		break;
	case 'u':				/* up */
		id = ioctl(fd, MY_IOCTL_UP, NULL);
		if (id < 0)
			perror("up");
		break;
	default:
		error("Wrong parameter");
	}

	close(fd);

	return 0;
}
