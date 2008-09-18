
#ifndef HWSCAN2_H
#define HWSCAN2_H 1

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define USB_MATCH_VENDOR                0x0001
#define USB_MATCH_PRODUCT               0x0002
#define USB_MATCH_DEV_LO                0x0004
#define USB_MATCH_DEV_HI                0x0008
#define USB_MATCH_DEV_CLASS             0x0010
#define USB_MATCH_DEV_SUBCLASS          0x0020
#define USB_MATCH_DEV_PROTOCOL          0x0040
#define USB_MATCH_INT_CLASS             0x0080
#define USB_MATCH_INT_SUBCLASS          0x0100
#define USB_MATCH_INT_PROTOCOL          0x0200

//#define HAVE_USB

struct modmap_t {
	char *basename;
	char *dirname;
	struct modmap_t *next;
};

struct pcimap_t {
	struct modmap_t *module;
	int vendor, subvendor;
	int device, subdevice;
	int class, class_mask;
	int driver_data;
	struct pcimap_t *next;
};

#ifdef HAVE_USB
struct usbmap_t {
	struct modmap_t *module;
	int match_flags;
	int idVendor, idProduct;
	int bcdDevice_lo, bcdDevice_hi;
	int bDeviceClass, bDeviceSubClass, bDeviceProtocol;
	int bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol;
	int driver_info;
	struct usbmap_t *next;
};

extern struct usbmap_t *usbmap;
#endif

extern struct modmap_t *modmap;
extern struct pcimap_t *pcimap;

extern char modules_dir[];

FILE *fopen_md(char *relative_filename);

void read_modmap();
struct modmap_t *find_module(char *line);

void read_pcimap();
void find_pci(int vendor, int device);
void scan_pci();

#ifdef HAVE_USB
void read_usbmap();
void find_usb(int idVendor, int idProduct,
              int bDeviceClass, int bDeviceSubClass, int bDeviceProtocol,
              int bInterfaceClass, int bInterfaceSubClass,
              int bInterfaceProtocol);
void scan_usb();
#endif

#endif /* HWSCAN2_H */

