#include <sys/param.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/mutex.h>

#include <fs/devfs/devfs_int.h>

d_read_t	read_hook;
d_read_t	*read;

int
read_hook(struct cdev *dev, struct uio *uio, int ioflag)
{
	uprintf("[INSERT SOME ACTION HERE]\n");
	return (*read)(dev, uio, ioflag);
}

static int
load(struct module *module, int cmd, void *arg)
{
	int error = 0;
	struct cdev_priv *cdp;

	switch (cmd) {
	case MOD_LOAD:
		mtx_lock(&devmtx);

		TAILQ_FOREACH(cdp, &cdevp_list, cdp_list) {
			if (strcmp(cdp->cdp_c.si_name, "cd_example") == 0) {
				read = cdp->cdp_c.si_devsw->d_read;
				cdp->cdp_c.si_devsw->d_read = read_hook;
				break;
			}
		}

		mtx_unlock(&devmtx);

		break;
	case MOD_UNLOAD:
		mtx_lock(&devmtx);

		TAILQ_FOREACH(cdp, &cdevp_list, cdp_list) {
			if (strcmp(cdp->cdp_c.si_name, "cd_example") == 0) {
				cdp->cdp_c.si_devsw->d_read = read;
				break;
			}
		}
		
		mtx_unlock(&devmtx);

		break;
	default:
		error = EOPNOTSUPP;
		break;
	}

	return error;
}

static moduledata_t cd_example_hook_mod = {
	"cd_example_hook",
	load, 
	NULL
};	

DECLARE_MODULE(cd_example_hook, cd_example_hook_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
