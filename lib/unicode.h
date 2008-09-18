#ifndef UNICODE_H
#define UNICODE_H

#ifdef __cplusplus
extern "C" {
#endif

int Unicode(const char* charset, const char* inbuf, char *outbuf, int MaxLen);
#ifdef __cplusplus
}
#endif

#endif
