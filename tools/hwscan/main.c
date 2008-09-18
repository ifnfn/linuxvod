/*
 * Note: we don't use any kind of indexes (btrees, etc) here, because
 * that would add more overhead than it would improve performance.
 */

#include "hwscan2.h"

int main(int argc, char **argv)
{
	read_modmap();
#ifdef HAVE_USB	
	read_usbmap();
#endif
	read_pcimap();

	scan_pci();
#ifdef HAVE_USB
	scan_usb();
#endif
	return 0;
}

