#ifndef FFMPEG_AES_H
#define FFMPEG_AES_H

#include <stdint.h>
#include <inttypes.h>

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
	unsigned char Version;

	char SongCode[9];
	char SongName[51];
	char Language[9];
	char SingerName[19];
	unsigned char VolumeK;
	unsigned char VolumeS;
	char Num;
	char Klok;
	char Sound;
	long TLen;
	uint8_t AesKey[16];
} AesHead;
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

int AesWriteFile(struct AesFile *aes, void *ptr, size_t size);
int AesReadFile(struct AesFile *aes, void *ptr, size_t size);
void AesCloseFile(struct AesFile *aes);
int AesEncryptFile(const char *infile, const char *outfile, uint8_t *KeyStr);
 
int ReadAesHead(FILE *fp, AesHead *aeshead, uint8_t *KeyStr);
int AesDecryptFile(const char *infile, const char *outfile, uint8_t *KeyStr);

#endif /* FFMPEG_AES_H */
