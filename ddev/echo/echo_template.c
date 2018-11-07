#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>

#define BUFSIZE 256

static d_open_t		echo_open;
static d_close_t	echo_close;
static d_read_t		echo_read;
static d_write_t	echo_write;

static struct cdevsw echo_cdevsw = {
	.d_version = D_VERSION;
	.d_open = echo_open;
	.d_close = echo_close;
	.d_read = echo_read;
	.d_write = echo_write;
	.d_name = "echo";
};

typedef struct echo {
	char buffer[BUFSIZE];
	int length;
} echo_t;

static echo_t *echo_message;
static struct cdev *echo_dev;

static int
echo_open(struct cdev *dev, int oflags, int devtype, struct thread *td)
{

}

static int
echo_close(struct cdev *dev, int fflag, int devtype, struct thread *td)
{

}

static int
echo_read(struct cdev *dev, struct uio *uio, int ioflag) 
{

}

static int
echo_read(struct cdev *dev, struct uio *uio, int ioflag)
{

}	

static int
echo_modevent(module_t mod __unused, int event, void *arg __unused)
{

}

DEV_MODULE(echo, echo_modevent, NULL);
