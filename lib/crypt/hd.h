#ifndef HD_H
#define HD_H

#include <stdbool.h>

#define PLAYDES  "L2Rldi9wbGF5"          // /dev/play
#define BINPLAY  "L3RtcC9wbGF5ZXI="      // /tmp/player
#define DEVHDA   "L2Rldi9oZGE="          // /dev/hda

#ifdef __cplusplus
extern "C" {
#endif

const char* b64str(const char *base, char *out);
bool GetPublicKey (const char *dev , char *publickey);
bool GetDesPwd    (const char *dev , char *despwd);
bool CreateRegCode(const char *publickey, char *regcode);
bool DecAndEncFile(const char *oldfile, const char *oldpwd, const char *newfile, const char *newpwd);
bool CheckKtvRegCode();

#ifdef __cplusplus
}
#endif

#endif

