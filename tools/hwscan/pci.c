/*
 * Note: we don't use any kind of indexes (btrees, etc) here, because
 * that would add more overhead than it would improve performance.
 */

#include "hwscan2.h"

struct pcimap_t *pcimap = NULL;

void read_pcimap()
{
	char line[4096];
	struct pcimap_t *newpci;

	FILE *f = fopen_md("modules.pcimap");
	while ( fgets(line, 4095, f) != NULL ) {
		line[4095] = 0;

		if ( line[0] == '#' ) continue;
		newpci = malloc(sizeof(struct pcimap_t));

		sscanf(line, "%*s %i %i %i %i %i %i %i",
			&newpci->vendor, &newpci->device,
			&newpci->subvendor, &newpci->subdevice,
			&newpci->class, &newpci->class_mask,
			&newpci->driver_data);

		newpci->module = find_module(line);

		newpci->next = pcimap;
		pcimap = newpci;
	}
	fclose(f);
}

void find_pci(int vendor, int device)
{
	struct pcimap_t *p;

	for (p=pcimap; p!=NULL; p=p->next) {
		if ( p->vendor != vendor ) continue;
		if ( p->device != device ) continue;
		printf("PCI Card: %s\n", p->module->basename);
		char command[128] = "modprobe ";
		strcat(command, p->module->basename);
		
		system(command);
//		printf("file: %s\n", p->module->dirname);
	}
}

void scan_pci()
{
	char line[4096];
	int busnr, devnr, vendor, device, rc;

	FILE *f = fopen("/proc/bus/pci/devices", "r");
	while ( fgets(line, 4095, f) != NULL ) {
		line[4095] = 0;

		rc = sscanf(line, "%02x%02x %04x%04x",
			&busnr, &devnr, &vendor, &device);
		if ( rc != 4 ) continue;
		find_pci(vendor, device);
	}
	fclose(f);
}

