#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/time.h>
#include <sys/types.h>

#pragma pack(1)
struct my_ts_event  {   /* Used in UCB1x00 style touchscreens (the default) */
	unsigned char packet[5];
};
#pragma pack()

struct ts_event  {   /* Used in UCB1x00 style touchscreens (the default) */
	unsigned short pressure;
	unsigned short x;
	unsigned short y;
	unsigned short pad;
	struct timeval stamp;
};

struct h3600_ts_event { /* Used in the Compaq IPAQ */
	unsigned short pressure;
	unsigned short x;
	unsigned short y;
	unsigned short pad;
};

struct mk712_ts_event { /* Used in the Hitachi Webpad */
	unsigned int header;
	unsigned int x;
	unsigned int y;
	unsigned int reserved;
};

#include "tslib-private.h"

#define BitinXY(frame)  11
#define MAX_DIMENSION  ((1<<BitinXY(0))-1)

#define X(dat) ( ((dat)[1] << 7) | (dat)[2] )
#define Y(dat) ( ((dat)[3] << 7) | (dat)[4] )

int ts_read_raw(struct tsdev *ts, struct ts_sample *samp, int nr)
{
	struct my_ts_event *myevt;
	struct ts_event *evt;
	struct h3600_ts_event *hevt;
	struct mk712_ts_event *mevt;
	int ret;

	char *tseventtype=getenv("TSLIB_TSEVENTTYPE");

	if(tseventtype == NULL) 
		tseventtype = "";
	if( strcmp(tseventtype,"H3600") == 0) { /* iPAQ style h3600 touchscreen events */
		hevt = alloca(sizeof(*hevt) * nr);
		ret = read(ts->fd, hevt, sizeof(*hevt) * nr);
		if(ret >= 0) {
			while(ret >= sizeof(*hevt)) {
				samp->x = hevt->x;
				samp->y = hevt->y;
				samp->pressure = hevt->pressure;
#ifdef DEBUG
					printf("RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
					gettimeofday(&samp->tv,NULL);
					samp++;
					hevt++;
					ret -= sizeof(*hevt);
			}
		}
	} 
	else if( strcmp(tseventtype,"MK712") == 0) { /* Hitachi Webpad events */
		mevt = alloca(sizeof(*mevt) * nr);
		ret = read(ts->fd, mevt, sizeof(*mevt) * nr);
		if(ret >= 0) {
			while(ret >= sizeof(*mevt)) {
				samp->x = (short)mevt->x;
				samp->y = (short)mevt->y;
				if(mevt->header==0)
					samp->pressure=1;
				else
					samp->pressure=0;
#ifdef DEBUG
				printf("RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
				gettimeofday(&samp->tv,NULL);
				samp++;
				mevt++;
				ret -= sizeof(*mevt);
			}
		}
	} 
	else if (strcmp(tseventtype, "UCB1x00") == 0 ){ /* Use normal UCB1x00 type events */
		evt = alloca(sizeof(*evt) * nr);
		ret = read(ts->fd, evt, sizeof(*evt) * nr);
		if(ret >= 0) {
			while(ret >= sizeof(*evt)) {
				samp->x = evt->x;
				samp->y = evt->y;
				samp->pressure = evt->pressure;
#ifdef DEBUG
				printf("RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
				samp->tv.tv_usec = evt->stamp.tv_usec;
				samp->tv.tv_sec = evt->stamp.tv_sec;
				samp++;
				evt++;
				ret -= sizeof(*evt);
			}
		}
	}
	else { 
		int size= sizeof(*myevt) * nr;
		myevt = alloca(size);
		
		ret = read(ts->fd, myevt, size);
		if(ret >= 0) {
			while(ret >= sizeof(*myevt)) {
				samp->pressure = myevt->packet[0] - 128;
				samp->x = MAX_DIMENSION - X(myevt->packet); 
				samp->y = Y(myevt->packet);
				if (samp->x < 0) 
					samp->x = 0;
				else if (samp->x > MAX_DIMENSION) 
					samp->x = MAX_DIMENSION;
				if (samp->y < 0) 
					samp->y = 0;
				else if (samp->y > MAX_DIMENSION) 
					samp->y = MAX_DIMENSION;

#ifdef DEBUG
				printf("RAW READ: x=%d, y=%d[%d]\n",samp->x,samp->y,samp->pressure);
#endif				
				samp++;
				myevt++;
				ret -= sizeof(*myevt);
			}
		}
	}
	ret = nr;

	return ret;
}

static int __ts_read_raw(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
	return ts_read_raw(inf->dev, samp, nr);
}

static const struct tslib_ops __ts_raw_ops =
{
	read:	__ts_read_raw,
};

struct tslib_module_info __ts_raw =
{
	next:	NULL,
	ops:	&__ts_raw_ops,
};
