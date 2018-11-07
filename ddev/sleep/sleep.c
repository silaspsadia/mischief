#define INVARIANTS
#define INVARIANT_SUPPORT

#include <sys/types.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/kthread.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/unistd.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/sysctl.h>

#define MAX_EVENT 1

static struct proc *kthread;
static int event;
static struct cv event_cv;
static struct mtx event_mtx;

static struct sysctl_ctx_list clist;
static struct sysctl_oid *poid;

static void
sleep_thread(void *arg)
{
	int ev;
	
	for (;;) {
		mtx_lock(&event_mtx);
		while ((ev = event) == 0) 
			cv_wait(&event_cv, &event_mtx);
		event = 0;
		mtx_unlock(&event_mtx);

		switch (ev) {
		case -1:
			kproc_exit(0);
			break;
		case 0:
			break;
		case 1:
			printf("sleep... is alive and well.\n");
			break;
		default:
			panic("event %d is bogus.\n", event);
		}
	}
}

static int
sysctl_debug_sleep_test(SYSCTL_HANDLER_ARGS)
{
	int error, i = 0;

	error = sysctl_handle_int(oidp, &i, 0, req);
	if (error == 0 && req->newptr != NULL) {
		if (i >= 1 && i <= MAX_EVENT) {
			mtx_lock(&event_mtx);
			KASSERT(event == 0, ("event %d was unhandled",
				event));
			event = i;
			cv_signal(&event_cv);
			mtx_unlock(&event_mtx);
		} else {
			error = EINVAL;
		}	
	}

	return error;
}

static int
load(void *arg)
{
	int error;
	struct proc *p;
	struct thread *td;

	/* Create a new process that uses sleep_thread() */
	error = kproc_create(sleep_thread, NULL, &p, RFSTOPPED, 0, "sleep");
	if (error)
		return error;

	/* Initialize mutex and condition variable */
	event = 0;
	mtx_init(&event_mtx, "sleep event", NULL, MTX_DEF);
	cv_init(&event_cv, "sleep");

	/* Schedule new process to run sleep_thread() */
	td = FIRST_THREAD_IN_PROC(p);
	thread_lock(td);
	TD_SET_CAN_RUN(td);
	sched_add(td, SRQ_BORING);
	thread_unlock(td);
	kthread = p;

	/* Create sysctl debug.sleep.test */
	sysctl_ctx_init(&clist);
	poid = SYSCTL_ADD_NODE(&clist, SYSCTL_STATIC_CHILDREN(_debug),
		OID_AUTO, "sleep", CTLFLAG_RD, 0, "sleep tree");
	SYSCTL_ADD_PROC(&clist, SYSCTL_CHILDREN(poid), OID_AUTO, "test",
		CTLTYPE_INT | CTLFLAG_RW, 0, 0, sysctl_debug_sleep_test, "I",
		"");
	
	return 0;
}

static int
unload(void *arg)
{
	sysctl_ctx_free(&clist);
	mtx_lock(&event_mtx);
	event = -1;
	cv_signal(&event_cv);
	mtx_sleep(kthread, &event_mtx, PWAIT, "sleep", 0);
	mtx_unlock(&event_mtx);
	mtx_destroy(&event_mtx);
	cv_destroy(&event_cv);

	return 0;
}

static int
sleep_modevent(module_t mod __unused, int event, void *arg)
{
	int error = 0;

	switch (event) {
	case MOD_LOAD:
		error = load(arg);
		break;
	case MOD_UNLOAD:
		error = unload(arg);
		break;
	default:
		error = EOPNOTSUPP;
		break;
	}

	return error;
}

static moduledata_t sleep_mod = {
	"sleep",
	sleep_modevent,
	NULL	
};

DECLARE_MODULE(sleep, sleep_mod, SI_SUB_SMP, SI_ORDER_ANY);
