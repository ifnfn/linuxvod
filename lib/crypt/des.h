#ifndef DES_H
#define DES_H

// silicon write.
#include <stdio.h>
#include <stdbool.h>

enum {
	ENCRYPT, DECRYPT
};

#ifdef __cplusplus
extern "C" {
#endif

// Type—ENCRYPT:加密,DECRYPT:解密
// 输出缓冲区(Out)的长度 >= ((datalen+7)/8)*8,即比datalen大的且是8的倍数的最小正整数
// In 可以= Out,此时加/解密后将覆盖输入缓冲区(In)的内容
// 当keylen>8时系统自动使用3次DES加/解密,否则使用标准DES加/解密.超过16字节后只取前16字节
bool DesCrypt(char *Out, char *In, long datalen, const char *Key, int keylen, bool Type);

/******************************************************************************/
// 名称：DesEncryptFile
// 功能：加密文件
// 参数：fh_out,fh_in为输入输出句柄；KeyStr为0结尾的密钥串，长度<=３２３２32，可为任意字符
// 返回：加密成功返回true，否则返回false
// 备注：当密钥长度>8时，系统自动使用3次DES加密
// 更新：2002/12/25
// 作者：朱治国
/******************************************************************************/
bool DesEncryptFile(const char *infile, const char *outfile, const char *KeyStr);

/******************************************************************************/
// 名称：DesEncryptBufToFile
// 功能：加密缓冲区并写入文件
// 参数：fh_out,fh_in为输入输出句柄；KeyStr为0结尾的密钥串，长度<=３２３２32，可为任意字符
// 返回：加密成功返回true，否则返回false
// 备注：当密钥长度>8时，系统自动使用3次DES加密
// 更新：2002/12/25
// 作者：朱治国
/******************************************************************************/
bool DesEncryptBufToFile(char *In, long datalen, const char *outfile, const char *KeyStr);

/******************************************************************************/
// 名称：DesDecryptFile
// 功能：解密文件
// 参数：fh_out,fh_in为输入输出句柄；KeyStr为0结尾的密钥串，长度<=32，可为任意字符
// 返回：解密成功返回true，否则返回false
// 备注：当密钥长度>8时，系统自动使用3次DES解密
// 更新：2002/12/25
// 作者：朱治国
/******************************************************************************/
bool DesDecryptFile(const char *infile, const char *outfile, const char *KeyStr);

/******************************************************************************/
// 名称：DesDecryptFile
// 功能：解密文件，并写入缓冲区
// 参数：fh_out,fh_in为输入输出句柄；KeyStr为0结尾的密钥串，长度<=32，可为任意字符
// 返回：解密成功返回true，否则返回false
// 备注：当密钥长度>8时，系统自动使用3次DES解密
// 更新：2002/12/25
// 作者：朱治国
/******************************************************************************/
char *DesDecryptFileToBuf(const char *infile, long *datalen, const char *KeyStr);

/*****************************************************************************/
// 检查已用DES加密的文件的密码是否正确
// desfile : 已用DES加密的文件，deskey：加密的密码
/*****************************************************************************/
bool CheckPasswd(const char *desfile, char *deskey);

#ifdef __cplusplus
}
#endif

#endif

