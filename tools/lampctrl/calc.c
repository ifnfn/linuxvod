/*
麦克风通道(Mic Channel)

mic 1 BASS 12;
mic 2 BASS 11;

micvolume 1,12;  // 麦克风音量
micdelay  1,4 ;  // 麦克风延时
micecho   1,44;  // 麦克风混响
micbass   1,55;  // 麦克风低音
micmid    1,55;  // 麦克风中音
michigh   1,55;  // 麦克风高音

音乐通道(Music Channel)
music
  musicvolume 12 // 音乐音量
  low

效果(EFFECT)
effect

混响(MIXER)
mixer

1.麦克风音量
2.麦克风延时
3.麦克风混响
4.麦克风低音
4.音乐音量
*/

#include <stdio.h>
#include "calc.h"
#include "y.tab.h"
#include "serial.h"

extern int portid;     // 控制盒串口号
extern FILE *curout;
extern unsigned char curvalue;

void printbin(int x, int len);

int SetSoundParam(int cmd, int value)
{
	switch(cmd) {
		case MUSICVOLUME:
			SendDskCmd("MusicVolume", 1, value); // 设置音乐音量
			return 0;
		case SUBWOOFER:
			SendDskCmd("SubWoofer", 1, value);   // 设置超低音量音量
			return 0;
		case MUSICTREBLE:
			SendDskCmd("MusicTreble", 1, value); // 设置高音音量
			return 0;
		case MUSICBASS:
			SendDskCmd("MusicBass", 1, value);   // 设置低音音量
			return 0;
		case SUBWOOFERFREQ:
			SendDskCmd("SubWooferFreq", 1, value);   // 设置超低分频点
			return 0;
		case MICVOLUME:
			SendDskCmd("MicVolume", 1, value);   // 设置音量音量
			return 0;
		case MICDELAY:
		case MICECHO:
		case MICBASS:
			return 0;
	}
}

static void open_ex(int x)
{
	if (portid > 0) {
//		printbin(x, 8);
		curvalue = x;
		fseek(curout, 0L, SEEK_SET);
		fsync(curout);
		fwrite(&curvalue, sizeof(unsigned char), 1, curout);
		char buf[1024];
		sprintf(buf, "$$%c%c%c%%%%%%", 1, x, 0);
		WriteComm(portid, buf, 8);
	}
	else
		printf("the port close.please use openport.\n");
}

void printbin(int x,int len)
{
	int i;
	unsigned int mask = 1<<len;
	for (i=0;i<len;i++) {
		if ((x<<1) & mask)
			printf("1");
		else
			printf("0");
		x<<=1;
	}
	printf("\n");
}

int ex(nodeType *p)
{
	int value;
	if (!p) return 0;
	switch(p->type) {
		case typeCon: return p->con.value;
		case typeId:  return sym[p->id.i];
		case typeOpr:
			switch(p->opr.oper) {
				case WHILE:
					while(ex(p->opr.op[0])) ex(p->opr.op[1]);
					return 0;
				case FOR:
					for(ex(p->opr.op[0]);ex(p->opr.op[1]);ex(p->opr.op[2])) ex(p->opr.op[3]);
					return 0;
				case IF:
					if (ex(p->opr.op[0]))
						ex(p->opr.op[1]);
					else if (p->opr.nops > 2)
						ex(p->opr.op[2]);
					return 0;
				case PRINT:
					printf("%d\n", ex(p->opr.op[0]));
					return 0;
				case OPEN:
					value = ex(p->opr.op[0]);
					open_ex(value);
					return 0;
				case INPUT:{
//					printbin(curvalue,8);
//					unsigned char x= (curvalue & 0xCF);
//					printbin(x, 8);
					value = ex(p->opr.op[0]);
					if ((value > 0) && (value <=4)) {
						curvalue = (curvalue &0xCF) | ((value - 1) << 4);
						open_ex(curvalue);
					}
					return 0;
				}
				case SET: {
					value =ex(p->opr.op[0]);
					int v =ex(p->opr.op[1]);
//					printf("%d,%d\n", value, v);
//					if (value >= 0) return 0;
					if (v)
						curvalue |= (1<<(value-1));
					else
						curvalue &= ~(1<<(value-1));
					open_ex(curvalue);
				}
					return 0;
				case SLEEP:
					#ifdef WIN32
					sleep(ex(p->opr.op[0]));
					#else
					usleep(ex(p->opr.op[0])*1000);
					#endif
					return 0;
				case OPENPORT:
					value = ex(p->opr.op[0]);
					portid = OpenComm(value, 19200, 8,"1", 'N');
					if (portid == -1){
						fprintf (stderr, "Make sure /dev/ttyS%d not in use or you have enough privilege.\n\n",value);
						exit(-1);
					}
					return 0;
				case DSKPORT:
					value = ex(p->opr.op[0]);
					if (dskConnect(value) == false){
						fprintf (stderr, "diskConnect error\n");
						exit(-1);
					}
					return 0;
				case CLOSEPORT:
					CloseComm(portid);
					portid = -1;
					return 0;
				case MUSICVOLUME:
				case SUBWOOFER:
				case MUSICTREBLE:
				case MUSICBASS:
				case SUBWOOFERFREQ:
				case MICVOLUME:
				case MICDELAY:
				case MICECHO:
				case MICBASS:
					value = ex(p->opr.op[0]);
					SetSoundParam(p->opr.oper, value);
					return 0;
				case ';':    ex(p->opr.op[0]); return ex(p->opr.op[1]);
				case '=':    return sym[p->opr.op[0]->id.i] = ex(p->opr.op[1]);
				case UMINUS: return -ex(p->opr.op[0]);
				case '+':    return ex(p->opr.op[0]) +  ex(p->opr.op[1]);
				case '-':    return ex(p->opr.op[0]) -  ex(p->opr.op[1]);
				case '*':    return ex(p->opr.op[0]) *  ex(p->opr.op[1]);
				case '/':    return ex(p->opr.op[0]) /  ex(p->opr.op[1]);
				case '<':    return ex(p->opr.op[0]) <  ex(p->opr.op[1]);
				case '>':    return ex(p->opr.op[0]) >  ex(p->opr.op[1]);
				case GE:     return ex(p->opr.op[0]) >= ex(p->opr.op[1]);
				case LE:     return ex(p->opr.op[0]) <= ex(p->opr.op[1]);
				case NE:     return ex(p->opr.op[0]) != ex(p->opr.op[1]);
				case EQ:     return ex(p->opr.op[0]) == ex(p->opr.op[1]);
				case AND:    return ex(p->opr.op[0]) && ex(p->opr.op[1]);
				case OR:     return ex(p->opr.op[0]) || ex(p->opr.op[1]);
			}
	}
	return 0;
}
