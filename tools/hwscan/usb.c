/*
 * Note: we don't use any kind of indexes (btrees, etc) here, because
 * that would add more overhead than it would improve performance.
 */

#include "hwscan2.h"

struct usbmap_t *usbmap = NULL;

void read_usbmap()
{
	char line[4096];
	struct usbmap_t *newusb;

	FILE *f = fopen_md("modules.usbmap");
	while ( fgets(line, 4095, f) != NULL ) {
		line[4095] = 0;

		if ( line[0] == '#' ) continue;
		newusb = malloc(sizeof(struct usbmap_t));

		sscanf(line, "%*s %i %i %i %i %i %i %i %i %i %i %i %i",
			&newusb->match_flags,
			&newusb->idVendor, &newusb->idProduct,
			&newusb->bcdDevice_lo, &newusb->bcdDevice_hi,
			&newusb->bDeviceClass, &newusb->bDeviceSubClass,
			&newusb->bDeviceProtocol,
			&newusb->bInterfaceClass, &newusb->bInterfaceSubClass,
			&newusb->bInterfaceProtocol, &newusb->driver_info);

		newusb->module = find_module(line);

		newusb->next = usbmap;
		usbmap = newusb;
	}
	fclose(f);
}

void find_usb(int idVendor, int idProduct,
              int bDeviceClass, int bDeviceSubClass, int bDeviceProtocol,
              int bInterfaceClass, int bInterfaceSubClass,
              int bInterfaceProtocol)
{
	struct usbmap_t *p;

	for (p=usbmap; p!=NULL; p=p->next) {
		if ((p->match_flags & USB_MATCH_VENDOR) &&
			p->idVendor != idVendor) continue;
		if ((p->match_flags & USB_MATCH_PRODUCT) &&
			p->idProduct != idProduct) continue;
#if 0
		if ((p->match_flags & USB_MATCH_DEV_LO) &&
			p->bcdDevice_lo > bcdDevice) continue;
		if ((p->match_flags & USB_MATCH_DEV_HI) &&
			p->bcdDevice_hi < bcdDevice) continue;
#endif
		if ((p->match_flags & USB_MATCH_DEV_CLASS) &&
			p->bDeviceClass != bDeviceClass) continue;
		if ((p->match_flags & USB_MATCH_DEV_SUBCLASS) &&
			p->bDeviceSubClass != bDeviceSubClass) continue;
		if ((p->match_flags & USB_MATCH_DEV_PROTOCOL) &&
			p->bDeviceProtocol != bDeviceProtocol) continue;
		if ((p->match_flags & USB_MATCH_INT_CLASS) &&
			p->bInterfaceClass != bInterfaceClass) continue;
		if ((p->match_flags & USB_MATCH_INT_SUBCLASS) &&
			p->bInterfaceSubClass != bInterfaceSubClass) continue;
		if ((p->match_flags & USB_MATCH_INT_PROTOCOL) &&
			p->bInterfaceProtocol != bInterfaceProtocol) continue;

		printf("USB Device: %s\n", p->module->basename);
	}
}

int scan_usb_tag_hex(char *line, char *tag)
{
	char buffer[100];
	int i, c;

	for (i=0; line[i]; i++)
		if (!strncmp(line+i, tag, strlen(tag))) {
			i += strlen(tag);
			while (line[i] == ' ') i++;
			for (c=0; c<99 && line[i+c] && line[i+c]!=' '; c++)
				 buffer[c] = line[i+c];
			buffer[c] = 0;
			return strtol(buffer, NULL, 16);
		}

	return -1;
}

int scan_usb_tag_dec(char *line, char *tag)
{
	char buffer[100];
	int i, c;

	for (i=0; line[i]; i++)
		if (!strncmp(line+i, tag, strlen(tag))) {
			i += strlen(tag);
			while (line[i] == ' ') i++;
			for (c=0; c<99 && line[i+c] && line[i+c]!=' '; c++)
				 buffer[c] = line[i+c];
			buffer[c] = 0;
			return strtol(buffer, NULL, 10);
		}

	return -1;
}

void scan_usb()
{
	char line[4096];
	char prod[8192] = "";

	int idVendor = 0, idProduct = 0;
	int bDeviceClass = 0, bDeviceSubClass = 0, bDeviceProtocol = 0;
	int bInterfaceClass = 0, bInterfaceSubClass = 0;
	int bInterfaceProtocol = 0;
	int bus = 0, dev = 0, igne = 0;

	FILE *f = fopen("/proc/bus/usb/devices", "r");
	while ( fgets(line, 4095, f) != NULL ) {
		line[4095] = 0;

		if ( line[0] == 'T' ) {
			bus = scan_usb_tag_dec(line, "Bus=");
			dev = scan_usb_tag_dec(line, "Dev#=");
			prod[0] = 0;

			idVendor = idProduct =
			bDeviceClass = bDeviceSubClass = bDeviceProtocol =
			bInterfaceClass = bInterfaceSubClass =
			bInterfaceProtocol = 0;
		}

		if ( line[0] == 'P' ) {
			idVendor = scan_usb_tag_hex(line, "Vendor=");
			idProduct = scan_usb_tag_hex(line, "ProdID=");
		}

		if ( line[0] == 'D' ) {
			bDeviceClass = scan_usb_tag_hex(line, "Cls=");
			bDeviceSubClass = scan_usb_tag_hex(line, "Sub=");
			bDeviceProtocol = scan_usb_tag_hex(line, "Prot=");
		}

		if ( line[0] == 'I' ) {
			bInterfaceClass = scan_usb_tag_hex(line, "Cls=");
			bInterfaceSubClass = scan_usb_tag_hex(line, "Sub=");
			bInterfaceProtocol = scan_usb_tag_hex(line, "Prot=");
		}

		if ( !strncmp(line, "S:  Manufacturer=", strlen("S:  Manufacturer=")) ) {
			strcpy(prod, line+strlen("S:  Manufacturer="));
			if ( strchr(prod, '\n') ) *(strchr(prod, '\n')) = 0;
		}

		if ( !strncmp(line, "S:  Product=", strlen("S:  Product=")) ) {
			if ( prod[0] ) strcat(prod, " ");
			strcat(prod, line+strlen("S:  Product="));
			if ( strchr(prod, '\n') ) *(strchr(prod, '\n')) = 0;
		}

		if ( line[0] == 'E' && !igne) {
			if ((idVendor != 0) && (idProduct != 0)) {
				find_usb(idVendor, idProduct,
					bDeviceClass, bDeviceSubClass, bDeviceProtocol,
					bInterfaceClass, bInterfaceSubClass,
					bInterfaceProtocol);
			}
			igne=1;
		}

		if ( line[0] != 'E' ) igne=0;
	}
	fclose(f);
}

