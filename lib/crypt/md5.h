#ifndef MD5_H
#define MD5_H

typedef unsigned char *POINTER;
typedef unsigned short int UINT2;
typedef unsigned long int UINT4;

/* MD5 context. */
typedef struct {
	UINT4 state[4];                                   /* state (ABCD) */
	UINT4 count[2];        /* number of bits, modulo 2^64 (lsb first) */
	unsigned char buffer[64];                         /* input buffer */
} MD5_CTX;

#ifdef __cplusplus
extern "C" {
#endif

void MD5Init  (MD5_CTX *);
void MD5Update(MD5_CTX *, unsigned char *, unsigned int);
void MD5Final (unsigned char [16], MD5_CTX *);
void MDPrint  (unsigned char digest[16], char *outstr, int len);

#ifdef __cplusplus
}
#endif

#endif
