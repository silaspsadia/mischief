#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/sysent.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>

#define BUFSIZE 4096

/* vscand
 * ======
 * - Intended to work with the veriscan module
 * - TODO: make extensible
 */

int 
main(void)
{
	int i, fd0, fd1, fd2;
	pid_t pid, sid;
	
	char errbuf[_POSIX2_LINE_MAX];
	kvm_t *kd;
	struct nlist nl[] = { {NULL}, {NULL} };
	struct sysent savebuf[564];
	struct sysent getbuf[564];

	struct rlimit rl;
	struct stat st;

	// Get the sysent table and save it
	nl[0].n_name = "sysent";
	
	kd = kvm_openfiles(NULL, NULL, NULL, O_RDWR, errbuf);
	if (!kd) {
		fprintf(stderr, "error: %s\n", errbuf);
		exit(-1);
	}

	if (kvm_nlist(kd, nl) < 0) {
		fprintf(stderr, "error: %s\n", kvm_geterr(kd));
		exit(-1);
	}

	if (!nl[0].n_value) {
		fprintf(stderr, "error: sysent table not found\n");
		exit(-1);
	}

	if (kvm_read(kd, nl[0].n_value, savebuf, 
		564 * sizeof(struct sysent)) < 0) {
		fprintf(stderr, "error: failed to retrieve sysent\n");
		exit(-1);
	}

	for (i = 0; i < 564; i++) {
		kvm_read(kd, nl[0].n_value + i * sizeof(struct sysent),
			&savebuf[i], sizeof(struct sysent));
		printf("Retrieved system call @%p\n", (struct sy_call_t *)savebuf[i].sy_call);
	}

	/**
	// Setup the daemon
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

	**/
	while (1) {
		printf("[***] Scanning system call table for hooks...\n");
		for (i = 0; i < 564; i++) {
			kvm_read(kd, nl[0].n_value + i * sizeof(struct sysent),
				&getbuf[i], sizeof(struct sysent));
			//printf("getbuf[%i].sycall: %p\n", i, (struct sy_call_t*)getbuf[i].sy_call);
			if (savebuf[i].sy_call != getbuf[i].sy_call) {
				printf("[!!!] Found system call hook @offset %i\n", i);
				if (kvm_write(kd, nl[0].n_value + i * sizeof(struct sysent),
					&savebuf[i], sizeof(struct sysent)) < 0) 
					printf("[!!!] Failed to patch hook @offset %i\n", i);
				else
					printf("[***] Successfully patched hook @offset %i\n", i);

			}	
		}
		sleep(1);
	}
}
