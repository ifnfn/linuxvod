#ifndef BASE64_H
#define BASE64_H

#ifdef cplusplus
extern "C" {
#endif

void to64frombits(unsigned char *out, const unsigned char *in, int inlen);
int from64tobits(char *out, const char *in);

#ifdef cplusplus
}
#endif

#endif

