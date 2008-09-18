#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32

#else
#include <sys/ioctl.h>
#include <linux/hdreg.h>
#endif

#include <errno.h>
#include "md5.h"

#include "hd.h"
#include "des.h"

#define FORMATSTR "JXMlc0Z1Y2tZb3VyTW90aGVyJXM3ODAyMjclcywvQCQhYWREUmEzV3ggMCkoJg=="
#define SiLiCoN   "U2lMaUNvTg=="
#define RSAERROR  "WW91ciBjb21wdXRlciBjYW5ub3QgY3JlYXRlIFJTQSBwdWJsaWNrZXkuIFBsZWFzZSBjaGVja2luZy4uLlxu"
#define RASMSG    "UkFTIFB1YmxpY0tleTolcy4gUGxlYXNlIElucHV0IFJlZ2NvZGU6"

static int b64_decode_table[256] = {
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
	52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
	-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
	15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
	-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
	41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
};

static int b64_decode( const char* str, unsigned char* space, int size )
{
	const char* cp;
	int space_idx, phase;
	int d, prev_d = 0;
	unsigned char c;

	space_idx = 0;
	phase = 0;
	for ( cp = str; *cp != '\0'; ++cp )
	{
		d = b64_decode_table[(int) *cp];
		if ( d != -1 )
		{
			switch ( phase )
			{
				case 0:
					++phase;
					break;
				case 1:
					c = ( ( prev_d << 2 ) | ( ( d & 0x30 ) >> 4 ) );
					if ( space_idx < size )
						space[space_idx++] = c;
					++phase;
					break;
				case 2:
					c = ( ( ( prev_d & 0xf ) << 4 ) | ( ( d & 0x3c ) >> 2 ) );
					if ( space_idx < size )
						space[space_idx++] = c;
					++phase;
					break;
				case 3:
					c = ( ( ( prev_d & 0x03 ) << 6 ) | d );
					if ( space_idx < size )
						space[space_idx++] = c;
					phase = 0;
					break;
			}
			prev_d = d;
		}
	}
	return space_idx;
}

const char* b64str(const char *base, char *out)
{
	int l = b64_decode(base, (unsigned char*)out, 1024);
	out[l] = 0;
	return out;
}

void MD5(char *text, char *md5, int len)
{
	MD5_CTX context;
	unsigned char digest[17];
	MD5Init(&context);
	MD5Update(&context, (unsigned char*)text, strlen(text));
	MD5Final(digest, &context);
	MDPrint(digest, md5, len);
	int i;
	for (i=0;i<len*2;i++)
		if ( (md5[i]>='A') && (md5[i]<='F') ) {
			md5[i]=md5[i]-'A' + '1';

		}
}

static bool gethdinfo(const char *hd, char *hd_serial) // 得到硬盘序列号
{
#ifdef WIN32
	strcpy(hd_serial, "afqefadf");
	return true;
#else
	struct hd_driveid id;
	int fd = open(hd, O_RDONLY|O_NONBLOCK);
	if (fd < 0) {
		return false;
	}

	if(!ioctl(fd, HDIO_GET_IDENTITY, &id)){
		sprintf(hd_serial, "%s",id.serial_no);
		return true;
	} else
	return false;
#endif
}

static bool getcpuinfo(char *cpu_serial) // 得到CPU序列号
{
	cpu_serial[0] = '\0';
	return true;
}

void CreateKey(const char *data, char *key)
{
	char tmp[512], md5str[513], out[100];
	memset(tmp, 0, 512);
	memset(md5str, 0, 513);
	strcpy(tmp, data);
	strcat(tmp, b64str(SiLiCoN, out));

	sprintf(md5str, b64str(FORMATSTR, out),	data, tmp, data, tmp); //:TOTO

	MD5(md5str, tmp, 8);
	strcpy(md5str, tmp);
	int len = 512 - 16;
	while (len > 0)
	{
		MD5(md5str, tmp, 8);
		strcat(md5str, tmp);
		len -= 16;
	}
	MD5(md5str, tmp, 8);
	MD5(tmp, key, 4);
	MD5(key, tmp, 1);
	strcat(key, tmp);
}

bool GetPublicKey(const char *dev, char *publickey) // 生成认证码
{
	char hd[200], cpu[100];
	if (gethdinfo(dev, hd))
;
//		printf("HD /dev/hda serial: %s\n", hd);
	else {
		publickey[0] = 0;
		return false;
	}

	if (!getcpuinfo(cpu)) {
//		printf("No found CPU serial number is %s.\n", cpu);
		return false;
	}
	strcat(hd, cpu);
	CreateKey(hd, publickey);
	return true;
}

bool GetDesPwd(const char *dev, char *despwd)
{
	char pk[50], rc[50], dp[50];
	if (GetPublicKey(dev, pk) == false) {
		return false;
	}
	if (!CreateRegCode(pk, rc))
		return false;
	CreateRegCode(rc, dp);
	if (despwd) 
		strcpy(despwd, dp);
	return true;
}

/* 由认证码 publickey 生成注册码 regcode
 */
bool CreateRegCode(const char *publickey, char *regcode)
{
	if (strlen(publickey) != 10) return false;
	char buf[33], md5[33], tmp[100];
	memset(buf, 0, 33);
	memset(md5, 0, 33);
	memset(tmp, 0, 100);

	strncpy(buf, publickey, 8);
	MD5(buf, md5, 1);
	buf[8] = md5[0];
	buf[9] = md5[1];
	if (strcmp(publickey, buf) == 0) {
		CreateKey(publickey, regcode);
		return true;
	} else
		return false;
}


bool DecAndEncFile(const char *oldfile, const char *oldpwd, const char *newfile, const char *newpwd)
{
	long len = 0;
	char *buf = DesDecryptFileToBuf(oldfile, &len, oldpwd);
	if (buf) {
		bool ok = DesEncryptBufToFile(buf, len, newfile, newpwd);
		free(buf);
		return ok;
	}
	return false;
}

bool CheckKtvRegCode()
{
	char publickey[33], standkey[33], deskey[33], out[512];

	if (GetPublicKey(b64str(DEVHDA, out), publickey) == false) {
		printf(b64str(RSAERROR, out));
		return false;
	}

	if (!CreateRegCode(publickey, standkey))
		return false;

	CreateRegCode(standkey, deskey);
	return CheckPasswd(b64str(PLAYDES, out), deskey);
}
