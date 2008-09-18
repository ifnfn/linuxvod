#ifndef FFMPEG_AES_H
#define FFMPEG_AES_H

#include <stdint.h>
#include <inttypes.h>

#define SELF_AES

extern const int av_aes_size;
#ifdef SELF_AES
typedef struct AVAES{
	uint8_t round_key[15][4][4];
	uint8_t state[2][4][4];
	int rounds;
}AVAES;
#else
#include "openssl/aes.h"
typedef AES_KEY AVAES;
#endif

struct AesFile;

typedef struct aeshead {
	char       SongCode[10];
	char       SongName[50];
	char       Language[10];
	char       SingerName1[20];
	char       SingerName2[20];
	char       SingerName3[20];
	char       SingerName4[20];
	char       PinYin[20];
	char       WBH[20];
	uint8_t    VolumeK;
	uint8_t    VolumeS;
	char       Num;
	char       Klok;
	char       Sound;
	uint8_t    extend[274];

	uint8_t    magic[5]; 
	long       DataLen;
	uint8_t    md5[16];
	uint8_t    AesKey[16];
} AesHead;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * initializes an AVAES context
 * @param key_bits 128, 192 or 256
 * @param decrypt 0 for encryption, 1 for decryption
 */
int av_aes_init(AVAES *a, const uint8_t *key, int key_bits, int decrypt);

/**
 * encrypts / decrypts.
 * @param count number of 16 byte blocks
 * @param dst destination array, can be equal to src
 * @param src source array, can be equal to dst
 * @param iv initialization vector for CBC mode, if NULL then ECB will be used
 * @param decrypt 0 for encryption, 1 for decryption
 */
void av_aes_crypt(AVAES *a, uint8_t *dst, uint8_t *src, int count, uint8_t *iv, int decrypt);

struct AesFile* AesOpenFile(const char *aesfile, uint8_t *password, int decrypt);

int AesWriteFile   (struct AesFile *aes, void *ptr, size_t size, size_t datalen);
int  AesReadFile   (struct AesFile *aes, void *ptr, size_t size, off_t offset);
void AesCloseFile  (struct AesFile *aes);
long GetAesFileSize(struct AesFile *aes);
AesHead *GetAesHead(struct AesFile *aes);

int ReadAesHead        (FILE *fp, AesHead *aeshead, uint8_t *passwd);

int AesEncryptFile     (const char *infile, const char *outfile, uint8_t *passwd);
int AesDecryptFile     (const char *infile, const char *outfile, uint8_t *passwd);
int AesMD5VerifyFile   (const char *filename, char *md5);
int AesCheckEncryptFile(const char *filename);
long GetAesFileSizeByName(const char *filename);
/**
 * 将字符串string 用aes 加密后通过base64 编码
 */ 
char *AesEncryptString(const char *string, const char *passwd);

/**
 * 将 base_string 用base64解码后再用aes 解密，返回明文字符串
 */ 
char *AesDecryptString(const char *base_string, const char *passwd);
char *AesEncryptStringDefault(const char *string);
char *AesDecryptStringDefault(const char *base_string);
char *GetPassword           (long long id, char *passwd, int len);

#ifdef __cplusplus
}
#endif
#endif 

