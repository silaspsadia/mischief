#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h> 
#include <unistd.h>

#define BUFSIZE 4096

/* vscand
 * ======
 * - Intended to work with the veriscan module
 * - TODO: make extensible
 */

int 
main(int argc, char *argv[])
{
	/* FIXME 
	if (argc != 2) {
		printf("Usage: %s <syscall no>\n", argv[0]);
		exit(1);
	}
	*/

	char buf[BUFSIZE];
	int i, fd0, fd1, fd2;
	pid_t pid, sid;
	struct rlimit rl;
	struct stat st;

	if (getrlimit(RLIMIT_NOFILE, &rl) < 0) 
		exit(-1);

	if ((pid = fork()) < 0)
		exit(-1);
	else if (pid != 0)
		exit(0);

	if ((sid = setsid()) < 0)
		exit(-1);

	if (chdir("/") < 0)
		exit(-1);

	if (rl.rlim_max == RLIM_INFINITY)
		rl.rlim_max = 1024;

	for (i = 0; i < rl.rlim_max; i++)
		close(i);

	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);

	// XXX: make sure these get used to put everything in one place
	char *logdir = "~/.vscan/";
	char *logpath = "~/.vscan/vscan.log";

	if (stat(logdir, &st) == -1) {
		mkdir(logdir, 0700);
	}

	while (1) {
		// XXX: this is intended to be a quick hack. Make extensible.
		system("perl -e 'syscall(210);' > vscan.log");
		sleep(10);
	}
}
