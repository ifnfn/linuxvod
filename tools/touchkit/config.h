#ifndef _CONFIG_H_
#define _CONFIG_H_

//#define PRJNAME   TouchKit
#define PRJNAME   
#define _USB_VID  0x3823
#define _USB_PID  0x0001
#define SCR_MAX_MNR                 2
#define SCR_BASE_MNR                180
#define _USBMODULE "TouchKit"

#define _merge(head,tail)               head##tail
#define IDcat(head,tail)                _merge(head,tail)


#endif
