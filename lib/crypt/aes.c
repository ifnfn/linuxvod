#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "aes.h"
#include "md5.h"
#include "base64.h"
#include "hd.h"

#define BUFSIZE (16*512)

#ifdef SELF_AES

#define FFSWAP(type,a,b) do{type SWAP_tmp= b; b= a; a= SWAP_tmp;}while(0)
static const uint8_t rcon[10] = {
	0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

static uint8_t     sbox[256];
static uint8_t inv_sbox[256];
#ifdef CONFIG_SMALL
static uint32_t enc_multbl[1][256];
static uint32_t dec_multbl[1][256];
#else
static uint32_t enc_multbl[4][256];
static uint32_t dec_multbl[4][256];
#endif

static inline void addkey(uint64_t dst[2], uint64_t src[2], uint64_t round_key[2])
{
	dst[0] = src[0] ^ round_key[0];
	dst[1] = src[1] ^ round_key[1];
}

static void subshift(uint8_t s0[2][16], int s, uint8_t *box)
{
	uint8_t (*s1)[16]= s0[0] - s;
	uint8_t (*s3)[16]= s0[0] + s;
	s0[0][0]=box[s0[1][ 0]]; s0[0][ 4]=box[s0[1][ 4]]; s0[0][ 8]=box[s0[1][ 8]]; s0[0][12]=box[s0[1][12]];
	s1[0][3]=box[s1[1][ 7]]; s1[0][ 7]=box[s1[1][11]]; s1[0][11]=box[s1[1][15]]; s1[0][15]=box[s1[1][ 3]];
	s0[0][2]=box[s0[1][10]]; s0[0][10]=box[s0[1][ 2]]; s0[0][ 6]=box[s0[1][14]]; s0[0][14]=box[s0[1][ 6]];
	s3[0][1]=box[s3[1][13]]; s3[0][13]=box[s3[1][ 9]]; s3[0][ 9]=box[s3[1][ 5]]; s3[0][ 5]=box[s3[1][ 1]];
}

static inline int mix_core(uint32_t multbl[4][256], int a, int b, int c, int d)
{
#ifdef CONFIG_SMALL
#define ROT(x,s) ((x<<s)|(x>>(32-s)))
	return multbl[0][a] ^ ROT(multbl[0][b], 8) ^ ROT(multbl[0][c], 16) ^ ROT(multbl[0][d], 24);
#else
	return multbl[0][a] ^ multbl[1][b] ^ multbl[2][c] ^ multbl[3][d];
#endif
}

static inline void mix(uint8_t state[2][4][4], uint32_t multbl[4][256], int s1, int s3)
{
	((uint32_t *)(state))[0] = mix_core(multbl, state[1][0][0], state[1][s1  ][1], state[1][2][2], state[1][s3  ][3]);
	((uint32_t *)(state))[1] = mix_core(multbl, state[1][1][0], state[1][s3-1][1], state[1][3][2], state[1][s1-1][3]);
	((uint32_t *)(state))[2] = mix_core(multbl, state[1][2][0], state[1][s3  ][1], state[1][0][2], state[1][s1  ][3]);
	((uint32_t *)(state))[3] = mix_core(multbl, state[1][3][0], state[1][s1-1][1], state[1][1][2], state[1][s3-1][3]);
}

static inline void crypt(AVAES *a, int s, uint8_t *sbox, uint32_t *multbl)
{
	int r;
	
	for(r=a->rounds-1; r>0; r--){
		mix(a->state, multbl, 3-s, 1+s);
		addkey(a->state[1], a->state[0], a->round_key[r]);
	}
	subshift(a->state[0][0], s, sbox);
}

void av_aes_crypt(AVAES *a, uint8_t *dst, uint8_t *src, int count, uint8_t *iv, int decrypt)
{
	while(count--){
		addkey(a->state[1], src, a->round_key[a->rounds]);
		if(decrypt) {
			crypt(a, 0, inv_sbox, dec_multbl);
			if(iv){
				addkey(a->state[0], a->state[0], iv);
				memcpy(iv, src, 16);
			}
			addkey(dst, a->state[0], a->round_key[0]);
		}else{
			if(iv) addkey(a->state[1], a->state[1], iv);
			crypt(a, 2,     sbox, enc_multbl);
			addkey(dst, a->state[0], a->round_key[0]);
			if(iv) memcpy(iv, dst, 16);
		}
		src+=16;
		dst+=16;
	}
}

static void init_multbl2(uint8_t tbl[1024], int c[4], uint8_t *log8, uint8_t *alog8, uint8_t *sbox)
{
	int i, j;
	for(i=0; i<1024; i++){
		int x= sbox[i>>2];
		if(x) tbl[i]= alog8[ log8[x] + log8[c[i&3]] ];
	}
#ifndef CONFIG_SMALL
	for(j=256; j<1024; j++)
		for(i=0; i<4; i++)
			tbl[4*j+i]= tbl[4*j + ((i-1)&3) - 1024];
#endif
}

// this is based on the reference AES code by Paulo Barreto and Vincent Rijmen
int av_aes_init(AVAES *a, const uint8_t *key, int key_bits, int decrypt) 
{
	int i, j, t, rconpointer = 0;
	uint8_t tk[8][4];
	int KC= key_bits>>5;
	int rounds= KC + 6;
	uint8_t  log8[256];
	uint8_t alog8[512];

	if (!enc_multbl[0][sizeof(enc_multbl)/sizeof(enc_multbl[0][0])-1]) {
		j=1;
		for(i=0; i<255; i++){
			alog8[i]=
			alog8[i+255]= j;
			log8[j]= i;
			j^= j+j;
			if(j>255) j^= 0x11B;
		}
		for(i=0; i<256; i++){
			j= i ? alog8[255-log8[i]] : 0;
			j ^= (j<<1) ^ (j<<2) ^ (j<<3) ^ (j<<4);
			j = (j ^ (j>>8) ^ 99) & 255;
			inv_sbox[j]= i;
			sbox    [i]= j;
		}
		int a[4] = {0xe, 0x9, 0xd, 0xb};
		int b[4] = {0x2, 0x1, 0x1, 0x3};
		init_multbl2(dec_multbl[0], a, log8, alog8, inv_sbox);
		init_multbl2(enc_multbl[0], b, log8, alog8, sbox);
	}
	
	if(key_bits!=128 && key_bits!=192 && key_bits!=256)
		 return -1;
	
	a->rounds= rounds;
	
	memcpy(tk, key, KC*4);
	
	for(t= 0; t < (rounds+1)*16;) {
		memcpy(a->round_key[0][0]+t, tk, KC*4);
		t+= KC*4;
		
		for(i = 0; i < 4; i++)
		tk[0][i] ^= sbox[tk[KC-1][(i+1)&3]];
		tk[0][0] ^= rcon[rconpointer++];
		
		for(j = 1; j < KC; j++){
			if(KC != 8 || j != KC>>1)
				for(i = 0; i < 4; i++) tk[j][i] ^= tk[j-1][i];
			else
				for(i = 0; i < 4; i++) tk[j][i] ^= sbox[tk[j-1][i]];
		}
	}
	
	if(decrypt){
		for(i=1; i<rounds; i++){
			uint8_t tmp[3][16];
			memcpy(tmp[2], a->round_key[i][0], 16);
			subshift(tmp[1], 0, sbox);
			mix(tmp, dec_multbl, 1, 3);
			memcpy(a->round_key[i][0], tmp[0], 16);
		}
	}else{
		for(i=0; i<(rounds+1)>>1; i++){
			for(j=0; j<16; j++)
				 FFSWAP(int, a->round_key[i][0][j], a->round_key[rounds-i][0][j]);
		}
	}
	
	return 0;
}

#else
int av_aes_init(AES_KEY *a, const uint8_t *key, int key_bits, int decrypt) 
{
	uint8_t userkey[17] = {0,};
	strncpy(userkey, key, strlen(key));

	if (decrypt)
		return AES_set_decrypt_key(userkey, key_bits, a);
	else
		return AES_set_encrypt_key(userkey, key_bits, a);
}

void av_aes_crypt(AVAES *a, uint8_t *dst, uint8_t *src, int count, uint8_t *iv, int decrypt)
{
	if (decrypt){
		if (iv == NULL) {
			int i;
			for (i=0; i<count; i++){
				int p = i * AES_BLOCK_SIZE;
				AES_ecb_encrypt(src + p, dst + p, a, AES_DECRYPT);
			}
		}
		else
			AES_cbc_encrypt(src, dst, count * AES_BLOCK_SIZE, a, iv, AES_DECRYPT);

	}
	else {
		if (iv == NULL) {
			int i;
			for (i=0; i<count; i++) {
				int p = i * AES_BLOCK_SIZE;
				AES_ecb_encrypt(src + p, dst + p, a, AES_ENCRYPT);
			}
		}
		else
			AES_cbc_encrypt(src, dst, count * AES_BLOCK_SIZE, a, iv, AES_ENCRYPT);
	}
}
#endif

const int av_aes_size= sizeof(AVAES);

typedef struct AesFile{
	MD5_CTX     md5_ctx;
	AesHead     Head;
	FILE*       fp;
	AVAES*      aesc;
	uint8_t     password[17];
	int         decrypt;
	uint8_t     zzges;
} AesFile;

////////////////////////////////////////////////////////////////////////////////
void AesCryptHead(AesHead *aeshead, AesHead *newhead, int decrypt)
{
	AVAES*      aesc;
	uint8_t     usekey[16] = {'2', '9', '4', '9', '6', '6', 'A', 'z', '$', 0xFF, 0xAB, 0xFE, 0x1A, };
	uint8_t     ivec[16]   = {'2', '9', '4', '9', '6', '6', 'A', 'z', '$', 0xFF, 0xAB, 0xFE, 0x1A, };

	aesc = malloc(av_aes_size);
	av_aes_init(aesc, usekey, 128, decrypt);
	if (newhead == NULL) 
		newhead = aeshead;
	av_aes_crypt(aesc, (uint8_t *)newhead, (uint8_t *)aeshead, sizeof(AesHead) / 16, ivec, decrypt);
	free(aesc);
}

struct AesFile* AesOpenFile(const char *aesfile, uint8_t *passwd, int decrypt)
{
	FILE *fh_in;
	AesFile *aes = NULL;

	if (decrypt) {
		if ((fh_in = fopen(aesfile, "rb")) == NULL)
			return NULL;
		
		aes = (AesFile*)malloc(sizeof(AesFile));
		memset(aes, 0, sizeof(AesFile));
		aes->fp = fh_in;
		if (passwd)
			memcpy(aes->password, passwd, strlen(passwd));

		if (ReadAesHead(fh_in, &aes->Head, aes->password) == 0){
			aes->zzges = 1;
			aes->aesc = malloc(av_aes_size);
			av_aes_init(aes->aesc, aes->password, 128, 1);
		}
	}
	else {
		if ((fh_in = fopen(aesfile, "wb")) == NULL)
			return NULL;
		
		aes = (AesFile*)malloc(sizeof(AesFile));
		memset(aes, 0, sizeof(AesFile));
		aes->fp = fh_in;
		memcpy(aes->password, passwd, strlen(passwd));

		aes->Head.magic[0] = 'Z';
		aes->Head.magic[1] = 'Z';
		aes->Head.magic[2] = 'G';
		aes->Head.magic[3] = 'E';
		aes->Head.magic[4] = 'S';
		MD5Init(&aes->md5_ctx);
	
		fseek(aes->fp, 0, SEEK_END);
		aes->Head.DataLen = ftell(aes->fp);
		fseek(aes->fp, 0, SEEK_SET);

		aes->aesc = malloc(av_aes_size);
		av_aes_init(aes->aesc, aes->password, 128, 0);
		av_aes_crypt(aes->aesc, aes->Head.AesKey, aes->password, 1, NULL, 0);

		AesHead NewHead;
		AesCryptHead(&aes->Head, &NewHead, 0);
		fwrite(&NewHead, sizeof(AesHead), 1, aes->fp);
		av_aes_init(aes->aesc, aes->password, 128, 0);
	}
	aes->decrypt = decrypt;

	return aes;
}

int AesWriteFile(struct AesFile *aes, void *ptr, size_t size, size_t datalen)
{
	if (aes->decrypt) return 0;
	av_aes_crypt(aes->aesc, ptr, ptr, size / 16, NULL, 0);
	fwrite(ptr, 1, size, aes->fp);
	aes->Head.DataLen += datalen;
	MD5Update(&aes->md5_ctx, ptr, size);
	
	return size * 16;
}

int AesReadFile(struct AesFile *aes, void *ptr, size_t size, off_t offset)
{
#define TMP_SIZE 32
	int data_len = 0;
	uint8_t buffer[TMP_SIZE];
	off_t start;
	size_t p_offset, block_size;

	if (aes->decrypt == 0) return 0;
	if (aes->zzges == 0) {
		fseek(aes->fp, offset, SEEK_SET);
		return fread(ptr, 1, size, aes->fp);
	}

	start = offset / 16 * 16;

	fseek(aes->fp, start + sizeof(AesHead), SEEK_SET);

	p_offset = offset - start;
	if (size > TMP_SIZE) 
		block_size = TMP_SIZE;
	else
		block_size = size % 16 > 0 ? (size + 16) / 16 * 16 : size;

//	printf("%s: size=%d, offset=%ld, start=%ld, p_offset=%ld, block_size=%d\n", __FUNCTION__, size, offset, start, p_offset, block_size);
	while ( size > data_len) {
		memset(buffer, 0, block_size);
		int len = fread(buffer, 1, block_size, aes->fp);
		if (len == 0)
			break;
		av_aes_crypt(aes->aesc, buffer, buffer, block_size/16, NULL, 1);

		if (data_len + len > size) 
			len = size - data_len;
//		printf("len=%d, size=%d\n", len, size);
		if ( offset + data_len + len > aes->Head.DataLen) {
			len = aes->Head.DataLen - (offset + data_len) + p_offset;
//			printf("%ld + %d + %d > %ld, len=%d\n", offset, data_len, len, aes->Head.DataLen, len);
		}
//		printf("2: len=%d, size=%d\n", len, size);

		if (len == 0)
			break;
		memcpy(ptr + data_len, buffer + p_offset, len - p_offset);
		data_len += len - p_offset;
		p_offset = 0;
	}
	printf("dat_len=%d\n", data_len);

	return data_len;
}

void AesCloseFile(struct AesFile *aes)
{
	if (aes->decrypt == 0){
		MD5Final(aes->Head.md5, &aes->md5_ctx);
		AesHead NewHead;
		AesCryptHead(&aes->Head, &NewHead, 0);
		fseek(aes->fp, 0, SEEK_SET);
		fwrite(&NewHead, 1, sizeof(AesHead), aes->fp);
	}

	fclose(aes->fp);
	if (aes->aesc)      free(aes->aesc);
	free(aes);
}

int AesEncryptFile(const char *infile, const char *outfile, uint8_t *passwd)
{
	FILE *fh_in;
	uint8_t databuf[BUFSIZE];

	if ((fh_in = fopen(infile, "rb")) == NULL)
		return -1;

	struct AesFile *aes = AesOpenFile(outfile, passwd, 0);
	while( !feof(fh_in) ) {
		memset(databuf, 0, BUFSIZE);
		size_t datalen = fread(databuf, 1, BUFSIZE, fh_in);
		AesWriteFile(aes, databuf, BUFSIZE, datalen);
	}
	AesCloseFile(aes);
	fclose(fh_in);
	
	return 0;
}

AesHead *GetAesHead(struct AesFile *aes)
{
	return &aes->Head;
}

int ReadAesHead(FILE *fp, AesHead *aeshead, uint8_t *passwd)
{
	uint8_t   aesKey[17];
	AVAES*    aesc; 

	if (fp==NULL)
		return -1;

	if (fread(aeshead, 1, sizeof(AesHead), fp) != sizeof(AesHead)) {
		fseek(fp, 0, SEEK_END);
		aeshead->DataLen = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		return -1;
	}

	AesCryptHead(aeshead, NULL, 1);
	if (aeshead->magic[0] != 'Z' ||
		aeshead->magic[1] != 'Z' ||
		aeshead->magic[2] != 'G' ||
		aeshead->magic[3] != 'E' ||
		aeshead->magic[4] != 'S')
	{
		fseek(fp, 0, SEEK_END);
		aeshead->DataLen = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		return -1;
	}

//	printf("aeshead->Head.DataLen=%ld\n", aeshead->DataLen);
	if (passwd) {
		memset(aesKey, 0, 17);
		aesc = malloc(av_aes_size);
		av_aes_init(aesc, passwd, 128, 1);	
		av_aes_crypt(aesc, aesKey, aeshead->AesKey, 1, NULL, 1);
		free(aesc);

		return memcmp(aesKey, passwd, strlen(passwd));
	}
	else
		return 0;
}

int AesDecryptFile(const char *infile, const char *outfile, uint8_t *passwd)
{
	FILE *fh_out;
	uint8_t databuf[BUFSIZE];
	long len;
	off_t offset = 0;

	if ((fh_out = fopen(outfile, "wb")) == NULL)
		return -1;

	struct AesFile *aes = AesOpenFile(infile, passwd, 1);

	while (1) {
		len = AesReadFile(aes, databuf, 4096, offset);
		if (len <=0) 
			break;
//		printf("offset = %ld, len=%d\n", offset, len);
		offset += len;
		fwrite(databuf, len, 1, fh_out);
	}

	int i;
	for (i=0; i<10000; i++) {
		offset = random() % aes->Head.DataLen;
		len = AesReadFile(aes, databuf, 4096, offset);
		if (len > 0) {
			fseek(fh_out, offset, SEEK_SET);
			fwrite(databuf, 1, len, fh_out);
		}
	}

	fclose(fh_out);
	AesCloseFile(aes);

	return 0;
}

char *AesEncryptAndBase64(const char *string, const char *passwd)
{
	uint8_t     password[17] = {0,};
	AVAES*      aesc;
	uint8_t     len, count;
	uint8_t*    new_string;
	uint8_t     base64[1024] = {0, };
	
	len = strlen(string);
	count = len % 16 > 0 ? len / 16 + 1 : len / 16;
	len = count * 16;

	new_string = (uint8_t *)malloc(len + 2);
	memset(new_string, 0, len + 2);
	strncpy(new_string+1, string, len);
	
	strncpy(password, passwd, strlen(passwd));
	
	aesc = malloc(av_aes_size);
	av_aes_init(aesc, password, 128, 0);
	av_aes_crypt(aesc, new_string+1, new_string+1, count, NULL, 0);
	new_string[0] = len;
	free(aesc);

	to64frombits(base64, new_string, len + 1);
	free(new_string);

	return strdup(base64);
}

char *AesDecryptAndBase64(const char *base_string, const char *passwd)
{
	uint8_t    len;
	uint8_t    password[17] = {0, };
	uint8_t    base[1024] = {0, };

	from64tobits(base, base_string);
	len = base[0];
	AVAES *aesc = malloc(av_aes_size);

	strncpy(password, passwd, strlen(passwd));
	av_aes_init(aesc, password, 128, 1);
	av_aes_crypt(aesc, base + 1, base + 1, len / 16, NULL, 1);
	free(aesc);

	return strdup(base + 1);
}

char *AesEncryptAndBase64DefaultPwd(const char *string)
{
	time_t timep;
	struct tm *p;
	char usekey[32] = {0, };

	time(&timep);
	p=gmtime(&timep);
	sprintf(usekey, "%d%d%d%d", p->tm_mday, p->tm_mon, p->tm_year, p->tm_wday);
	return AesEncryptAndBase64(string, usekey);
}

char *AesDecryptAndBase64DefaultPwd(const char *base_string)
{
	time_t timep;
	struct tm *p;
	char usekey[32] = {0, };

	time(&timep);
	p=gmtime(&timep);
	sprintf(usekey, "%d%d%d%d", p->tm_mday, p->tm_mon, p->tm_year, p->tm_wday);
	return AesDecryptAndBase64(base_string, usekey);
}

int AesCheckEncryptFile(const char *filename)
{
        FILE*     fh_in;
	int       err = -1;
	AesHead   Head;

	if ((fh_in = fopen(filename, "rb")) == NULL)
		return -1;

	if (ReadAesHead(fh_in, &Head, NULL) == 0)
		err = 0;

	fclose(fh_in);

	return err;
}

int AesMD5VerifyFile(const char *filename)
{
        FILE*     fh_in;
	int       len;
	AesHead   Head;
	uint8_t   databuf[BUFSIZE];

	if ((fh_in = fopen(filename, "rb")) == NULL)
		return -1;

	if (ReadAesHead(fh_in, &Head, NULL) == 0){
		MD5_CTX md5_ctx;
		uint8_t md5[16];

		MD5Init(&md5_ctx);
		while( !feof(fh_in) ) {
			len = fread(databuf, 1, BUFSIZE, fh_in);
			MD5Update(&md5_ctx, databuf, len);
		}
		fclose(fh_in);
		MD5Final(md5, &md5_ctx);
		if (memcmp(md5, Head.md5, 16) == 0)
			return 0;
	}

	return -1;
}

long GetSizeByFileName(const char *filename)
{
        FILE*     fh_in;
	AesHead   Head;

	if ((fh_in = fopen(filename, "rb")) == NULL)
		return -1;

	memset(&Head, 0, sizeof(AesHead));
	ReadAesHead(fh_in, &Head, NULL);

	fclose(fh_in);
	return Head.DataLen;
}

long GetAesFileSize(struct AesFile *aes)
{
	return aes->Head.DataLen;
}

char *GetPassword(long long id, char *passwd, int len)
{
	char base[256];
	char md5[33] = {0, }, outkey[33] = {0, };

	if (id != 0) {
		sprintf(base, "%lld", id * id);
		MD5(base, md5, 16);
		CreateKey(md5, outkey);

		MD5_CTX context;
		unsigned char digest[17];
		MD5Init(&context);
		MD5Update(&context, outkey, strlen(outkey));
		MD5Final(digest, &context);
		MDPrint(digest, passwd, len / 2);

		return passwd;
	}
	else
		return NULL;
}

#ifdef TEST
int main(int argc, char **argv)
{
	char passwd[33] = {0, };
	GetPassword(atoll(argv[1]), passwd, 16);
	printf("passwd=%s\n", passwd);
	return 0;
//	printf("AesFile=%d\n", sizeof(AesHead));
//
//	AesDecryptFile("a", "b", "cnsczd");
//	return 0;
//

	if (argv[1][0] == 'e') {
		AesEncryptFile(argv[2], argv[3], argv[4]);
		if ( AesMD5VerifyFile(argv[3]) == 0)
			printf("md5 ok\n");
		else
			printf("md5 error\n");
			
	}
	else if (argv[1][0] == '6') {
		char *s = AesEncryptAndBase64DefaultPwd(argv[2]);
		char *b = AesDecryptAndBase64DefaultPwd(s);
		printf("s = %s\n", s);
		printf("b = %s\n", b);
		free(s);
		free(b);
	}
	else
		AesDecryptFile(argv[2], argv[3], argv[4]);

	return 0;
}
#endif
