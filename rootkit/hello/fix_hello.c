#include <fcntl.h>
#include <kvm.h> 
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define SIZE 0x30

unsigned char nop_code[] = 
	"\x90\x90";

int
main(int argc, char *argv[])
{
	int i, jns_offset, call_offset;
	char errbuf[_POSIX2_LINE_MAX];
	kvm_t *kd;
	struct nlist nl[] = { {NULL}, {NULL}, {NULL}, };
	unsigned char hello_code[SIZE], call_operand[4];

	kd = kvm_openfiles(NULL, NULL, NULL, O_RDWR, errbuf);
	if (kd == NULL) {
		fprintf(stderr, "ERROR: %s\n", errbuf);
		exit(-1);
	}	
}
