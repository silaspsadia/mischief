#include <sys/types.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/sysent.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/syscall.h>
#include <sys/sysproto.h>

static sy_call_t *syscalldata[SYS_MAXSYSCALL];

static int 
veriscan(struct thread *td, void *syscall_args)
{
	for (int i = 0; i < SYS_MAXSYSCALL; i++) 
		if (syscalldata[i] != sysent[i].sy_call) {
			uprintf("[!!!] Detected anomaly at sysent[] offset %i\n"
				"@%p\n", i, sysent[i].sy_call);
			sysent[i].sy_call = syscalldata[i];
			if (syscalldata[i] == sysent[i].sy_call) 
				uprintf("[***] Successfully patched KOM event at offset %i\n", i);
			else 
				uprintf("[!!!] Failed to patch KOM event at offset %i\n", i);	

		}
		
	return 0;
}

static struct sysent veriscan_sysent = {
	0,
	veriscan
};

int offset = NO_SYSCALL;

static int
load(struct module *module, int cmd, void *args)
{
	int error = 0;

	switch (cmd) {
		case MOD_LOAD:
			for (int i = 0; i < SYS_MAXSYSCALL; i++) 
				syscalldata[i] = sysent[i].sy_call;
			uprintf("Veriscan system call registered at offset %i\n", offset);
			break;
		case MOD_UNLOAD:
			uprintf("Veriscan system call unloaded at offset %i\n", offset);
			break;
		default:
			error = EOPNOTSUPP;
			break;
	} 

	return error;
}

SYSCALL_MODULE(veriscan, &offset, &veriscan_sysent, load, NULL);
