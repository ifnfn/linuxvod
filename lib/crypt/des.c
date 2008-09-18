#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#ifdef WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

#include "des.h"

//silicon write.

// initial permutation IP
const static char IP_Table[64] = {
	58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36, 28, 20, 12, 4,
	62, 54, 46, 38, 30, 22, 14, 6, 64, 56, 48, 40, 32, 24, 16, 8,
	57, 49, 41, 33, 25, 17,  9, 1, 59, 51, 43, 35, 27, 19, 11, 3,
	61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7
};
// final permutation IP^-1
const static char IPR_Table[64] = {
	40, 8, 48, 16, 56, 24, 64, 32, 39, 7, 47, 15, 55, 23, 63, 31,
	38, 6, 46, 14, 54, 22, 62, 30, 37, 5, 45, 13, 53, 21, 61, 29,
	36, 4, 44, 12, 52, 20, 60, 28, 35, 3, 43, 11, 51, 19, 59, 27,
	34, 2, 42, 10, 50, 18, 58, 26, 33, 1, 41,  9, 49, 17, 57, 25
};
// expansion operation matrix
static const char E_Table[48] = {
	32,  1,  2,  3,  4,  5,  4,  5,  6,  7,  8,  9,
	 8,  9, 10, 11, 12, 13, 12, 13, 14, 15, 16, 17,
	16, 17, 18, 19, 20, 21, 20, 21, 22, 23, 24, 25,
	24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32,  1
};
// 32-bit permutation function P used on the output of the S-boxes
const static char P_Table[32] = {
	16, 7, 20, 21, 29, 12, 28, 17, 1,  15, 23, 26, 5,  18, 31, 10,
	2,  8, 24, 14, 32, 27, 3,  9,  19, 13, 30, 6,  22, 11, 4,  25
};
// permuted choice table (key)
const static char PC1_Table[56] = {
	57, 49, 41, 33, 25, 17,  9,  1, 58, 50, 42, 34, 26, 18,
	10,  2, 59, 51, 43, 35, 27, 19, 11,  3, 60, 52, 44, 36,
	63, 55, 47, 39, 31, 23, 15,  7, 62, 54, 46, 38, 30, 22,
	14,  6, 61, 53, 45, 37, 29, 21, 13,  5, 28, 20, 12,  4
};
// permuted choice key (table)
const static char PC2_Table[48] = {
	14, 17, 11, 24,  1,  5,  3, 28, 15,  6, 21, 10,
	23, 19, 12,  4, 26,  8, 16,  7, 27, 20, 13,  2,
	41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48,
	44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32
};
// number left rotations of pc1
const static char LOOP_Table[16] = {
	1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1
};
// The (in)famous S-boxes
const static char S_Box[8*4*16] = {
	// S1
	14,  4,	13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
	 0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
	 4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
	15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13,
	// S2
	15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
	 3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
	 0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
	13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9,
	// S3
	10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
	13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
	13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
	 1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12,
	// S4
	 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
	13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
	10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
	 3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14,
	// S5
	 2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
	14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
	 4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
	11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3,
	// S6
	12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
	10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
	 9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
	 4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13,
	// S7
	 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
	13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
	 1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
	 6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12,
	// S8
	13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
	 1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
	 7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
	 2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11
};

//////////////////////////////////////////////////////////////////////////

typedef bool (*PSubKey)[16][48];

//////////////////////////////////////////////////////////////////////////

static void DES(char Out[8], char In[8], const PSubKey pSubKey, bool Type);//标准DES加/解密
static void SetKey(const char* Key, int len);// 设置密钥
static void SetSubKey(PSubKey pSubKey, const char Key[8]);// 设置子密钥
static void F_func(bool In[32], const bool Ki[48]);// f 函数
static void S_func(bool Out[32], const bool In[48]);// S 盒代替
static void Transform(bool *Out, bool *In, const char *Table, int len);// 变换
static void Xor(bool *InA, const bool *InB, int len);// 异或
static void RotateL(bool *In, int len, int loop);// 循环左移
static void ByteToBit(bool *Out, const char *In, int bits);// 字节组转换成位组
static void BitToByte(char *Out, const bool *In, int bits);// 位组转换成字节组

static bool SubKey[2][16][48];// 16圈子密钥
static bool Is3DES;// 3次DES标志
static char Tmp[256], deskey[16];

bool DesCrypt(char *Out, char *In, long datalen, const char *Key, int keylen, bool Type)
{
	long i, j;
	if( !( Out && In && Key && (datalen=(datalen+7)&0xfffffff8) ))
		return false;
	SetKey(Key, keylen);
	if( !Is3DES ) {   // 1次DES
		for(i=0, j = datalen>>3; i<j; ++i,Out+=8,In+=8)
			DES(Out, In, &SubKey[0], Type);
	} else{   // 3次DES 加密:加(key0)-解(key1)-加(key0) 解密::解(key0)-加(key1)-解(key0)
		for(i=0, j=datalen>>3; i<j; ++i,Out+=8,In+=8) {
			DES(Out, In,  &SubKey[0], Type);
			DES(Out, Out, &SubKey[1], !Type);
			DES(Out, Out, &SubKey[0], Type);
		}
	}
	return true;
}

void SetKey(const char* Key, int len)
{
	memset(deskey, 0, 16);
	memcpy(deskey, Key, len>16?16:len);
	SetSubKey(&SubKey[0], &deskey[0]);
	Is3DES = len>8 ? (SetSubKey(&SubKey[1], &deskey[8]), true) : false;
}

void DES(char Out[8], char In[8], const PSubKey pSubKey, bool Type)
{
	int i;
	static bool M[64], tmp[32], *Li=&M[0], *Ri=&M[32];
	ByteToBit(M, In, 64);
	Transform(M, M, IP_Table, 64);
	if( Type == ENCRYPT )
	{
		for(i=0; i<16; ++i)
		{
			memcpy(tmp, Ri, 32);
			F_func(Ri, (*pSubKey)[i]);
			Xor(Ri, Li, 32);
			memcpy(Li, tmp, 32);
		}
	}else
	{
		for(i=15; i>=0; --i)
		{
			memcpy(tmp, Li, 32);
			F_func(Li, (*pSubKey)[i]);
			Xor(Li, Ri, 32);
			memcpy(Ri, tmp, 32);
		}
	}
	Transform(M, M, IPR_Table, 64);
	BitToByte(Out, M, 64);
}

void SetSubKey(PSubKey pSubKey, const char Key[8])
{
	static bool K[64], *KL=&K[0], *KR=&K[28];
	ByteToBit(K, Key, 64);
	Transform(K, K, PC1_Table, 56);
	int i;
	for(i=0; i<16; ++i)
	{
		RotateL(KL, 28, LOOP_Table[i]);
		RotateL(KR, 28, LOOP_Table[i]);
		Transform((*pSubKey)[i], K, PC2_Table, 48);
	}
}

void F_func(bool In[32], const bool Ki[48])
{
	static bool MR[48];
	Transform(MR, In, E_Table, 48);
	Xor(MR, Ki, 48);
	S_func(In, MR);
	Transform(In, In, P_Table, 32);
}

void S_func(bool Out[32], const bool In[48])
{
	char i, j, k;
	for(i=0; i<8; ++i,In+=6,Out+=4)
	{
		j = (In[0]<<1) + In[5];
		k = (In[1]<<3) + (In[2]<<2) + (In[3]<<1) + In[4];
		ByteToBit(Out, &S_Box[i*j*k], 4);
	}
}

void Transform(bool *Out, bool *In, const char *Table, int len)
{
	int i;
	for(i=0; i<len; ++i)
		Tmp[i] = In[ Table[i]-1 ];
	memcpy(Out, Tmp, len);
}

void Xor(bool *InA, const bool *InB, int len)
{
	int i;
	for(i=0; i<len; ++i)
		InA[i] ^= InB[i];
}

void RotateL(bool *In, int len, int loop)
{
	memcpy(Tmp, In, loop);
	memcpy(In, In+loop, len-loop);
	memcpy(In+len-loop, Tmp, loop);
}

void ByteToBit(bool *Out, const char *In, int bits)
{
	int i;
	for(i=0; i<bits; ++i)
		Out[i] = (In[i>>3]>>(i&7)) & 1;
}

void BitToByte(char *Out, const bool *In, int bits)
{
	int i;
	memset(Out, 0, bits>>3);
	for(i=0; i<bits; ++i)
		Out[i>>3] |= In[i]<<(i&7);
}

typedef struct deshead
{
	unsigned char Ver; // 版本
	long TLen;         // 文件长度
	char DesKey[17];   // DES密钥密文
} DesHead;

#define BUFSIZE (1024*4)

static size_t GetFPSize(const char *fn)
{
	struct stat buf;
	if (stat(fn, &buf) == 0)
		return buf. st_size;
	else
		return 0;
}

/******************************************************************************/
//	名称：DesEncryptFile
//	功能：加密
//	参数：fh_out,fh_in为输入输出句柄；KeyStr为0结尾的密钥串，长度<=３２３２32，可为任意字符
//	返回：加密成功返回true，否则返回false
//	备注：当密钥长度>8时，系统自动使用3次DES加密
//	更新：2002/12/25
//	作者：朱治国
/******************************************************************************/
bool DesEncryptFile(const char *infile, const char *outfile, const char *KeyStr)
{
	FILE *fh_in, *fh_out;
	if ((fh_in = fopen(infile, "rb")) == NULL)
		return false;
	if ((fh_out = fopen(outfile, "wb")) == NULL)
		return false;
	long len;
	char databuf[BUFSIZE]; // 数据缓冲区

	DesHead deshead;
	memset(&deshead, 0, sizeof(DesHead));
	// 版本信息
	deshead.Ver = 1;
	// 文件长度信息
	deshead.TLen = GetFPSize(infile);

	// 加密密钥串(用于解密时验证密钥的正确性)
	DesCrypt(deshead.DesKey, (char *)KeyStr, 16, KeyStr, strlen(KeyStr), ENCRYPT);
	// 写入信息头
	fwrite((char*)&deshead,sizeof(deshead), 1, fh_out);
	// 读取明文到缓冲区
	while( (len=fread(databuf,1, BUFSIZE, fh_in)) >0 ) {
		// 将缓冲区长度变为8的倍数
		len = ((len+7)>>3)<<3;
		// 在缓冲区中加密
		DesCrypt(databuf,databuf, len, KeyStr, strlen(KeyStr), ENCRYPT);
		// 将密文写入输出文件
		fwrite(databuf, len, 1, fh_out);
	}
	fclose(fh_in);
	fclose(fh_out);
	return true;
}

bool ReadDesHead(FILE *fp, DesHead *deshead, const char *KeyStr)
{
	if (fp==NULL) {
//		printf("Not found file.\n");
		return false;
	}
	// 读取信息头并检查长度
	if (fread(deshead, 1, sizeof(DesHead), fp) != sizeof(DesHead)){
//		printf("Error: the file is a invalid RSA file.\n" );
		return false;
	}
	// 版本控制
	if (deshead->Ver != 1) {
//		printf("The versiong don't decode the file, use new program.\n");
		return false;
	}
	// 解密密钥串
	DesCrypt(deshead->DesKey, deshead->DesKey, 16, KeyStr, strlen(KeyStr), DECRYPT);
	// 验证密钥的正确性
//	printf("%s!\n%s!\n", deshead->DesKey, KeyStr);
	if (memcmp(deshead->DesKey, KeyStr, 10)) {
		printf("Error: DSA passwd is invalid.\n");
		return false;
	}
	return true;
}

/******************************************************************************/
//	名称：DesDecryptFile
//	功能：解密
//	参数：fh_out,fh_in为输入输出句柄；KeyStr为0结尾的密钥串，长度<=32，可为任意字符
//	返回：解密成功返回true，否则返回false
//	备注：当密钥长度>8时，系统自动使用3次DES解密
//	更新：2002/12/25
//	作者：朱治国
/******************************************************************************/
bool DesDecryptFile(const char *infile, const char *outfile, const char *KeyStr)
{
	FILE *fh_in, *fh_out;
	if ((fh_in = fopen(infile, "rb")) == NULL) {
//		printf("open file (%s) error.\n", infile);
		return false;
	}
	long len;
	DesHead deshead;
	char databuf[BUFSIZE];//数据缓冲区

	if (ReadDesHead(fh_in, &deshead, KeyStr) == false){
		fclose(fh_in);
		return false;
	}
	if ((fh_out = fopen(outfile, "wb")) == NULL) {
//		printf("open file (%s) error.\n", outfile);
		return false;
	}
//	printf("Decrypt %s to %s|\n", infile, outfile);

	// 读取密文到缓冲区
	while( (len=fread(databuf, 1, BUFSIZE, fh_in)) >0 ) {
		// 将缓冲区长度变为8的倍数
		len = ((len+7)>>3)<<3;
		// 在缓冲区中解密
		DesCrypt(databuf,databuf,len, KeyStr, strlen(KeyStr), DECRYPT);
		// 将明文写入输出文件
		deshead.TLen -= fwrite(databuf,1, len < deshead.TLen?len:deshead.TLen, fh_out);
	}
	fclose(fh_in);
	fclose(fh_out);
	return true;
}

char *DesDecryptFileToBuf(const char *infile, long *datalen, const char *KeyStr)
{
	FILE *fh_in;
	if ((fh_in = fopen(infile, "rb")) == NULL)
		return false;
	long len, k=0;
	int writesize = 0;
	DesHead deshead;
	char databuf[BUFSIZE];// 临时数据缓冲区
	char *outbuf = NULL;  // 数据缓冲区

	if (ReadDesHead(fh_in, &deshead, KeyStr) == false) {
		fclose(fh_in);
		return false;
	}

	outbuf = (char *) malloc(deshead.TLen);
	*datalen = deshead.TLen;

	// 读取密文到缓冲区
	while( (len=fread(databuf, 1, BUFSIZE, fh_in)) >0 ) {
		// 将缓冲区长度变为8的倍数
		len = ((len+7)>>3)<<3;
		// 在缓冲区中解密
		DesCrypt(databuf,databuf,len, KeyStr, strlen(KeyStr), DECRYPT);
		// 将明文写入输出文件
		writesize = len < deshead.TLen?len:deshead.TLen;
		memcpy(outbuf+k, databuf, writesize);
		k += writesize;
		deshead.TLen -= writesize;
	}
	fclose(fh_in);
	return outbuf;
}

bool DesEncryptBufToFile(char *In, long datalen, const char *outfile, const char *KeyStr)
{
	FILE *fh_out;
	if ((fh_out = fopen(outfile, "wb")) == NULL)
		return false;
	long len, k=0;
	char databuf[BUFSIZE]; // 数据缓冲区

	DesHead deshead;
	// 版本信息
	deshead.Ver = 1;
	// 文件长度信息
	deshead.TLen = datalen;

	// 加密密钥串(用于解密时验证密钥的正确性)
	DesCrypt(deshead.DesKey,deskey,32, KeyStr, strlen(KeyStr), ENCRYPT);
	// 写入信息头
	fwrite((char*)&deshead, sizeof(deshead), 1, fh_out);

	// 读取明文到缓冲区
	while( deshead.TLen > 0 ) {
		len = BUFSIZE < deshead.TLen? BUFSIZE : deshead.TLen;
		memcpy(databuf, In + k, len);
		k += len;
		deshead.TLen -= len;
		// 将缓冲区长度变为8的倍数
		len = ((len+7)>>3)<<3;
		// 在缓冲区中加密
		DesCrypt(databuf,databuf, len, KeyStr, strlen(KeyStr), ENCRYPT);
		// 将密文写入输出文件
		fwrite(databuf, len, 1, fh_out);
	}
	fclose(fh_out);
	return true;
}

// 检查已用DES加密的文件的密码是否正确
bool CheckPasswd(const char *desfile, char *deskey)
{
	FILE *fh_in;
	DesHead deshead;
	if ((fh_in = fopen(desfile, "rb")) == NULL)
		return false;
	bool ok = ReadDesHead(fh_in, &deshead, deskey);
	fclose(fh_in);
	return ok;
}
