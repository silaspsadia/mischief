#include <sys/types.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/ioccom.h>

MALLOC_DEFINE(M_ECHO, "echo_buffer", "buffer for echo driver");

#define ECHO_CLEARBUF		_IO('E', 1)
#define ECHO_SETBUFSIZE		_IOW('E', 2, int)

static d_open_t		echo_open;
static d_close_t	echo_close;
static d_read_t		echo_read;
static d_write_t	echo_write;
static d_ioctl_t	echo_ioctl;

static struct cdevsw echo_cdevsw = {
	.d_version = D_VERSION,
	.d_open = echo_open,
	.d_close = echo_close,
	.d_read = echo_read,
	.d_write = echo_write,
	.d_ioctl = echo_ioctl,
	.d_name = "echo"
};

typedef struct echo {
	int bufsize;
	char *buffer;
	int length;
} echo_t;

static echo_t *echo_message;
static struct cdev *echo_dev;

static int
echo_open(struct cdev *dev, int oflags, int devtype, struct thread *td)
{
	uprintf("Opening echo device.\n");
	return 0;
}

static int
echo_close(struct cdev *dev, int fflag, int devtype, struct thread *td)
{
	uprintf("Closing echo device\n");
	return 0;
}

static int
echo_read(struct cdev *dev, struct uio *uio, int ioflag) 
{
	int error = 0;
	int amount;

	amount = MIN(uio->uio_resid,
		(echo_message->length - uio->uio_offset > 0) ?
		echo_message->length - uio->uio_offset : 0);
	
	error = uiomove(echo_message->buffer + uio->uio_offset, amount, uio);
	if (error != 0)
		uprintf("Read failed.\n");
	
	return error;
}

static int
echo_write(struct cdev *dev, struct uio *uio, int ioflag)
{
	int error = 0;
	int amount;

	amount = MIN(uio->uio_resid,
		(echo_message->bufsize - 1 - uio->uio_offset > 0) ?
		echo_message->bufsize - 1 - uio->uio_offset : 0);

	if (amount == 0)
		return error;
		
	error = uiomove(echo_message->buffer, amount, uio);
	if (error != 0) {
		uprintf("Write failed.\n");
		return error;
	}

	echo_message->buffer[amount] = '\0';
	echo_message->length = amount;

	return error;
}	

static int
echo_set_bufsize(int size)
{
	int error = 0;
	
	if (echo_message->bufsize == size)
		return error;

	if (size >= 128 && size <= 512) {
		echo_message->buffer = realloc(echo_message->buffer, size,
			M_ECHO, M_WAITOK);
		echo_message->bufsize = size;

		if (echo_message->length >= size) {
			echo_message->length = size - 1;
			echo_message->buffer[size - 1] = '\0';
		}
	}

	error = EINVAL;

	return error;
}

static int
echo_ioctl(struct cdev *dev, u_long cmd, caddr_t data, int fflag,
	struct thread *td)
{
	int error = 0;

	switch (cmd) {
	case ECHO_CLEARBUF:
		memset(echo_message->buffer, '\0',
			echo_message->bufsize);
		echo_message->length = 0;
		uprintf("Buffer cleared.\n");
		break;
	case ECHO_SETBUFSIZE:
		error = echo_set_bufsize(*(int *)data);
		if (error == 0)
			uprintf("Buffer resized.\n");
		break;
	default:
		error = ENOTTY;
		break;
	}

	return error;
}

static int
echo_modevent(module_t mod __unused, int event, void *arg __unused)
{
	int error = 0;

	switch (event) {
	case MOD_LOAD:
		echo_message = malloc(sizeof(echo_t), M_ECHO, M_WAITOK);
		echo_message->bufsize = 256;
		echo_message->buffer = malloc(echo_message->bufsize,
			M_ECHO, M_WAITOK);
		echo_dev = make_dev(&echo_cdevsw, 0, UID_ROOT, GID_WHEEL,
			0600, "echo");
		uprintf("Echo driver loaded.\n");
		break;
	case MOD_UNLOAD:
		destroy_dev(echo_dev);
		free(echo_message->buffer, M_ECHO);
		free(echo_message, M_ECHO);
		uprintf("Echo driver unloaded.\n");
		break;
	default:
		error = EOPNOTSUPP;
		break;
	}	

	return error;
}

DEV_MODULE(echo, echo_modevent, NULL);
