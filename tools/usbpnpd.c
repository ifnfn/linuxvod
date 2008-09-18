
/* Tab: 4 */

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>

//#define MONITOR_DEVICE_LIST

#define DEBUG 1
#define TIME_TO_WAKEUP_IN_SEC   .25
#define USBFS_PATH      "/proc/bus/usb/"
#define PRG_LOCK        "/var/run/usbpnpd.pid"
#define DEV_LIST        "/proc/bus/usb/devices"
#define USBDEV_PATTERN  "[0-9]??"

#if DEBUG
	#define DBG(arg...)		(printf(arg),puts(""))
	#define ERR(arg...)				\
	{	char __buf[1000];			\
		sprintf(__buf, arg);		\
		perror(__buf);				\
	}
#else
	#define DBG(arg...)
	#define ERR(arg...)
#endif

#define NOT_FOUND	0
#define FOUND		1

typedef struct Device
{
	int installed;
	int tag;
	int idVendor, idProduct;
	int bDeviceClass, bDeviceSubClass, bDeviceProtocol;
	int bInterfaceClass, bInterfaceSubClass;
	int bInterfaceProtocol;
	int bus, dev;

	struct Device *next;
} Device;

static int DaemonReset = 0;

static FILE *cfgFP = NULL;
static int DeviceCnt = 0;
Device *DeviceList = NULL;

#if DEBUG
void _dump_list(Device *head)
{
	if (head)
	{
		DBG("T(%d) Vid: %04X, Pid %04X}",
			head->tag, head->idVendor, head->idProduct);
		_dump_list(head->next);
	}
}
#else
	#define _dump_list(x)
#endif

void _destroy_list(Device *head)
{
	if (head)
	{
		_destroy_list(head->next);
		free(head);
	}
}

void _free_node(Device *node)
{
	if (node) free(node);
}

Device* _find_device(Device *list, int idVendor, int idProduct)
{
	for (; list; list = list->next)
	{
		if ( (list->idVendor == idVendor) && (list->idProduct == idProduct) ){
			list->tag = FOUND;
			return list;
		}
	}
	return NULL;
}

void _set_tag_field(Device *list, int value)
{
	for (; list; list = list->next)
	{
		list->tag = value;
	}
}

void _set_tag_if_match(Device *list, Device *sample, int value)
{
	for (; list; list = list->next)
	{
		if (list->idVendor == sample->idVendor && list->idProduct == sample->idProduct)
			list->tag = value;
	}
}

int _atoi_ok(char *src, int *rval)
{
	char *endp;
	*rval = strtol(src, &endp, 0);
	return (endp > src);
}

static int scan_usb_tag_hex(char *line, char *tag)
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

static int scan_usb_tag_dec(char *line, char *tag)
{
	char buffer[100];
	int i, c;

	for (i=0; line[i]; i++)
	{
		if (!strncmp(line+i, tag, strlen(tag)))
		{
			i += strlen(tag);
			while (line[i] == ' ') i++;
			for (c=0; c<99 && line[i+c] && line[i+c]!=' '; c++)
				buffer[c] = line[i+c];
			buffer[c] = 0;
			return strtol(buffer, NULL, 10);
		}
	}
	return -1;
}

#ifdef MONITOR_DEVICE_LIST
int UpdateDeviceList()
{
	char line[4096];
	char prod[8192] = "";

	int idVendor = 0, idProduct = 0;
	int bDeviceClass = 0, bDeviceSubClass = 0, bDeviceProtocol = 0;
	int bInterfaceClass = 0, bInterfaceSubClass = 0;
	int bInterfaceProtocol = 0;
	int bus = 0, dev = 0, igne = 0;
	_set_tag_field(DeviceList, NOT_FOUND);
	rewind(cfgFP);
	while ( fgets(line, 4095, cfgFP) != NULL )
	{
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
		//if ( prod[0] ) set_context_name("%s", prod);
		if ((idVendor != 0) && (idProduct != 0)) {
			if (_find_device(DeviceList, idVendor, idProduct) == NULL )
			{
				Device *newNode = malloc(sizeof(Device));
				if (newNode)
				{
					newNode->tag                = FOUND;
					newNode->installed	    = 0;
					newNode->idVendor           = idVendor;
					newNode->idProduct          = idProduct;
					newNode->bDeviceClass	    = bDeviceClass;
					newNode->bDeviceSubClass    = bDeviceSubClass;
					newNode->bDeviceProtocol    = bDeviceProtocol;
					newNode->bInterfaceClass    = bInterfaceClass;
					newNode->bInterfaceSubClass = bInterfaceSubClass;
					newNode->bInterfaceProtocol = bInterfaceProtocol;
					newNode->bus                = bus;
					newNode->dev                = dev;

					newNode->next = DeviceList;
					DeviceList    = newNode;
					DeviceCnt++;
				}
			}
		}
		igne=1;
	}

		if ( line[0] != 'E' ) igne=0;
	}
	return 0;
}
#else

void DumpGlob(glob_t *gdata)
{
	char **pathv;
	for (pathv = gdata->gl_pathv; pathv[0]; pathv++ )
	{
		puts(pathv[0]);
	}               
}                       

int IsDirectory(char *path)
{
	int len = strlen(path); 
	return (len && path[len-1] == '/');
}                                       
                                
int GlobRecursively(char *path, char *pattern, glob_t *gbuf)
{                                       
	char fullpath[500];             
	int idx, start = gbuf->gl_pathc, end;   
	int flag = GLOB_MARK;                   
				                                                
	if ( start )
		flag |= GLOB_APPEND;
	sprintf(fullpath, "%s%s", path, pattern);
	if ( glob(fullpath, flag, NULL, gbuf) )
	{
		return -1;
	}
	end = gbuf->gl_pathc;
	for ( idx = start; idx < end; idx++ )
	{
		char *path = gbuf->gl_pathv[idx];
		if ( IsDirectory(path) )
		{
			if ( GlobRecursively(path, pattern, gbuf) )
			{
				return -1;
			}
		}
	}
	return 0;
}

#include "linux/usb.h"

int ReadUSBdeviceInfo(char *path, Device *output)
{
	int fd;
	int rval = -1;
	if ( (fd = open(path, O_NONBLOCK)) >= 0 )
	{
		char buf[100];
		if ( read(fd, buf, sizeof buf) > 0 )
		{
			/* struct usb_device_descriptor *dp = (struct usb_device_descriptor*)buf;
			 * output->vendor_id = dp->idVendor;
			 * output->product_id = dp->idProduct;
			 */
			output->idVendor = *(unsigned short*)(buf+8);
			output->idProduct = *(unsigned short*)(buf+10);
			rval = 0;
		}
		close(fd);
	}
	return rval;
}

int UpdateDeviceList()
{
	char **pathv;
	glob_t gbuf;

	chdir(USBFS_PATH);
	gbuf.gl_pathc = 0;
	if ( GlobRecursively("", USBDEV_PATTERN, &gbuf) )
		return -1;
	_set_tag_field(DeviceList, NOT_FOUND);
	for ( pathv = gbuf.gl_pathv; pathv[0]; pathv++ )
	{
		Device info;
		if ( !IsDirectory(pathv[0])
			&& !ReadUSBdeviceInfo(pathv[0], &info)
			&& (info.idVendor | info.idProduct) )
		{
//			printf("Vendor=%d, Product=%d\n", info.idVendor, info.idProduct);
			if (_find_device(DeviceList, info.idVendor, info.idProduct) == NULL )
			{
				Device *newNode = (Device*)malloc(sizeof(Device));
				if (newNode)
				{
					*newNode           = info; // copy vid and pid
					newNode->tag       = FOUND;
					newNode->installed = 0;
					newNode->next      = DeviceList;
					DeviceList         = newNode;
					DeviceCnt++;
				}
			}
		}
	}
	globfree(&gbuf);
	return 0;
}
#endif

void DoTakeAction(Device *current, int install)
{
//	if (current->bInterfaceClass== 0x08) 
	{
		if (install) {
			printf("/usr/bin/update\n");
			system("/usr/bin/update");
		}
		else
		{
			printf("umount /mnt/usb\n");
			system("umount /mnt/usb");
		}
	}
}

void TakeAction()
{
	UpdateDeviceList();
	Device *ptr, *prior = NULL;
	for (ptr=DeviceList; ptr; ptr = ptr->next)
	{
		if (ptr->tag == NOT_FOUND) {
			printf("Device remove.\n");
			DoTakeAction(ptr, 0);
			if (prior == NULL) {
				_free_node(DeviceList);
				DeviceList = ptr->next;
			} else
			{
				DeviceList->next = ptr->next;
				_free_node(ptr);
			}
		}
		else if (ptr->installed == 0) {
			DoTakeAction(ptr, 1);
			ptr->installed = 1;
		}
	}
	DBG("(*) DeviceList:");
	_dump_list(DeviceList);
}

void handler(int dummy)
{
	if (dummy == SIGHUP)
	{
		DBG("reset daemon");
		DaemonReset = 1;
	}
}

void _background(int argc)
{
	if ( argc < 2 ) daemon(0,0);
}

void _update_lock()
{
	FILE *fp;
	if ( (fp=fopen(PRG_LOCK, "w")) )
	{
		fprintf(fp, "%d\n", getpid());
		fclose(fp);
	}
}

int _is_dup(char *name)
{
	FILE *fp;
	int pid;

	fp = fopen(PRG_LOCK, "r");
	if (fp && fscanf(fp, "%d", &pid) && !kill(pid, 0))
	{
		fclose(fp);
		DBG("already exists");
		return 1;
	}else if(fp)
	{
		fclose(fp);
	}
	return 0;
}

int main(int argc, char **argv)
{
	int fd;
	int rval;
	fd_set fds;
	char *prgname;

	if ( !(prgname = strrchr(argv[0], '/')) )
		prgname = argv[0];
	if ( _is_dup(prgname) )
	{
		char cmd[1000];
		if (argc > 1)
		{
			sprintf(cmd, "killall -HUP %s", prgname);
			system(cmd);
		}
		return 0;
	}
	_background(argc);
	_update_lock();
	signal(SIGHUP, handler);

#ifdef MONITOR_DEVICE_LIST
	if ( (fd = open(DEV_LIST, O_RDONLY)) < 0 )
	{
		ERR("open(" DEV_LIST ")");
		return -1;
	}
	if ( (cfgFP = fdopen(fd, "r")) == NULL )
	{
		ERR("fdopen(" DEV_LIST ")");
		close(fd);
		return -1;
	}
	while (1)
	{
		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		rval = select(fd+1, &fds, NULL, NULL, NULL);
		if (rval == 1)
			TakeAction();
		else if (errno != EINTR)
		{
			DBG("signal caught");
			break;
		}
	}
	close(fd);
#else
	signal(SIGALRM, handler);
	{
		struct itimerval timer;
		long sec = TIME_TO_WAKEUP_IN_SEC;
		long usec = (TIME_TO_WAKEUP_IN_SEC-sec) * 1.0e6;
		timer.it_value.tv_sec   = sec;
		timer.it_value.tv_usec  = usec;
		timer.it_interval = timer.it_value;
		setitimer(ITIMER_REAL, &timer, NULL);
	}
	while (1)
	{
		if (DaemonReset)
		{
		}
		TakeAction();
		pause();
	}
#endif
	return 0;
}

