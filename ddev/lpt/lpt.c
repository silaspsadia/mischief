#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/bus.h>
#include <sys/malloc.h>
#include <sys/syslog.h>

#include <machine/bus.h>
#include <sys/rman.h>
#include <machine/resource.h>

#include <dev/ppbus/ppbconf.h>
#include "ppbus_if.h"
#include <dev/ppbus/ppbio.h>
#include <dev/ppbus/ppb_1284.h>

#include <dev/ppbus/lpt.h>
#include <dev/ppbus/lptio.h>

#define LPT_NAME 	"lpt"		/* official driver name	*/
#define LPT_INIT_READY	4		/* wait up to 4 sec	*/
#define LPT_PRI		(PZERO + 8)	/* priority		*/
#define BUF_SIZE	1024		/* sc_buf size		*/
#define BUF_STAT_SIZE	32		/* sc_buf_stat size	*/

struct lpt_data {
	short		sc_state;
	char		sc_primed;
	struct callout	sc_callout;
	u_char		sc_ticks;
	int		sc_irq_rid;
	struct resource	*sc_irq_resource;
	void		*sc_irq_cookie;
	u_short		sc_irq_status;
	void		*sc_buf;
	void		*sc_buf_stat;
	char		*sc_cp;
	device_t	sc_dev;
	struct cdev	*sc_cdev;
	struct cdev	*sc_cdev_bypass;
	char		sc_flags;
	u_char		sc_control;
	short		sc_transfer_count;
};

/* bits for sc_state */
#define LP_OPEN		(1 << 0)	/* device is open		*/
#define LP_ERROR	(1 << 2)	/* error received from printer	*/
#define LP_BUSY		(1 << 3)	/* printer busy writing		*/
#define LP_TIMEOUT	(1 << 5)	/* timeout enabled		*/
#define LP_INIT		(1 << 6)	/* initializ an lpt_open	*/
#define LP_INTERRUPTED	(1 << 7)	/* write call interrupted	*/
#define LP_HAVEBUS	(1 << 8)	/* driver owns the bus		*/

/* bits for sc_ticks */
#define LP_TOUT_INIT	10
#define LP_TOUT_MAX	1

/* bits for sc_irq_status */
#define LP_HAS_IRQ	0x01
#define LP_USE_IRQ	0x02
#define LP_ENABLE_IRQ	0x04
#define LP_ENABLE_EXT	0x10

/* bits for sc_flags */
#define LP_NO_PRIME	0x10
#define LP_PRIME_OPEN	0x20
#define LP_AUTO_LF	0x40
#define LP_BYPASS	0x80

/* masks to get printer status */
#define LP_READY_MASK	(LPS_NERR | LPS_SEL | LPS_OUT | LPS_NBSY)
#define LP_READY	(LPS_NERR | LPS_SEL |         | LPS_NBSY)

/* used in polling code */
#define LPS_INVERT	(LPS_NERR | LPS_SEL |         | LPS_NBSY | LPS_NACK)
#define LPS_MASK	(LPS_NERR | LPS_SEL | LPS_OUT | LPS_NBSY | LPS_NACK)
#define NOT_READY(bus)	((ppb_rstr(bus) ^ LPS_INVERT) & LPS_MASK)
#define MAX_SPIN	20
#define MAX_SLEEP	(hz * 5)

static d_open_t		lpt_open;
static d_close_t	lpt_close;
static d_read_t		lpt_read;
static d_write_t	lpt_write;
static d_ioctl_t	lpt_ioctl;

static struct cdevsw lpt_cdevsw = {
	.d_version =	D_VERSION,
	.d_open =	lpt_open,
	.d_close =	lpt_close,
	.d_read =	lpt_read,
	.d_write =	lpt_write,
	.d_ioctl =	lpt_ioctl,
	.d_name =	LPT_NAME
};

static devclass_t lpt_devclass;

static void
lpt_identify(driver_t *driver, device_t parent)
{
	device_t dev;

	dev = device_find_child(parent, LPT_NAME, -1);
	if (!dev)
		BUS_ADD_CHILD(parent, 0, LPT_NAME, -1);
}

static int
lpt_request_ppbus(device_t dev, int how)
{
	return 0;
}

static int
lpt_port_test(device_t ppbus, u_char data, u_char mask)
{
	int temp, timeout = 10000;

	data &= mask;
	ppb_wdtr(ppbus, data);

	do {
		DELAY(10);
		temp = ppb_rdtr(ppbus) & mask;
	} while (temp != data && --timeout);

	return (temp == data);
}

static int
lpt_detect(device_t dev)
{
	device_t ppbus = device_get_parent(dev);
	static u_char test[18] = {
		0x55,			/* alternating zeroes	*/
		0xaa,			/* alternating ones	*/
		0xfe, 0xfd, 0xfb, 0xf7,	
		0xef, 0xdf, 0xbf, 0x7f,	/* walking zeroes	*/
		0x01, 0x02, 0x04, 0x08,
		0x10, 0x20, 0x40, 0x80	/* walking one		*/
	};

	int i, error, success = 1;

	ppb_lock(ppbus);

	error = lpt_request_pbus(dev, PPB_DONTWAIT);
	if (error) {
		ppb_unlock(ppbus);
		device_printf(dev, "cannot allocate ppbus (%d)!\n", error);
		return 0;
	}

	for (i = 0; i < 18; i++) 
		if (!lpt_port_test(ppbus, test[i], 0xff)) {
			success = 0;
			break;
		}

	ppb_wdtr(ppbus, 0);
	ppb_wctr(ppbus, 0);

	lpt_release_ppbus(dev);
	ppb_unlock(ppbus);

	return success;
	
}

static int
lpt_probe(device_t dev)
{
	if (!lpt_detect(dev))
		return ENXIO;

	device_set_desc(dev, "printer");

	return BUS_PROBE_SPECIFIC;
}

static void
lpt_intr(void *arg)
{
}

static int
lpt_attach(device_t dev)
{
	return 0;
}

static int
lpt_detach(device_t dev)
{
	return 0;
}

static void
lpt_timeout(void *arg)
{
}

static int
lpt_open(struct cdev *dev, int oflags, int devtype, struct thread *td)
{
	return 0;
}

static int
lpt_close(struct cdev *dev, int fflag, int devtype, struct thread *td)
{
	return 0;
}

static int
lpt_read(struct cdev *dev, struct uio *uio, int ioflag)
{
	return 0;
}

static int
lpt_push_bytes(struct lpt_data *sc)
{
	return 0;
}

static int
lpt_write(struct cdev *dev, struct uio *uio, int ioflag)
{
	return 0;
}

static int
lpt_ioctl(struct cdev *dev, u_long cmd, caddr_t data, int fflag,
	struct thread *td)
{
	return 0;
}

static device_method_t lpt_methods[] = {
	DEVMETHOD(device_identify,	lpt_identify),
	DEVMETHOD(device_probe,		lpt_probe),
	DEVMETHOD(device_attach,	lpt_attach),
	DEVMETHOD(device_detach,	lpt_detach),
	{ 0, 0 }
};

static driver_t lpt_driver = {
	LPT_NAME,
	lpt_methods,
	sizeof(struct lpt_data)
};

DRIVER_MODULE(lpt, ppbus, lpt_driver, lpt_devclass, 0, 0);
MODULE_DEPEND(lpt, ppbus, 1, 1, 1);
