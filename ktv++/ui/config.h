#include "key.h"
#include "data.h"
#include "xmltheme.h"
#include "strext.h"
#include "selected.h"
#include "mtvconfig.h"
#include "unicode.h"

#ifndef DATAPATH
#define DATAPATH "/ktvdata/"
#endif

#if 1
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#else
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480
#endif

#define MSGBOX_WIDTH  300
#define MSGBOX_HEIGHT 60
#define MSGBOX_TOP  ((SCREEN_HEIGHT - MSGBOX_HEIGHT) / 2)
#define MSGBOX_LEFT ((SCREEN_WIDTH - MSGBOX_WIDTH )  / 2)

#define PEN_WIDTH 8
#define PEN_COLOR rgb2uint32(0, 255, 0)
#define FONT_SIZE 36

#define DEFAULT_FONT DATAPATH"chinese.ttf"
#define MAX_SONG_NUMBER 200

//#define SINGERPICTYPE ".png"
#define SINGERPICTYPE ".jpg"

#define SOUND_BLANK        0
#define SOUND_RECT_HEIGHT 40
#define SOUND_RECT_WIDTH  20 
#define SOUND_RECT_COUNT  20
#define SOUND_SPRITE       0
#define SOUND_WINDOW_WIDTH  (SOUND_RECT_WIDTH * SOUND_RECT_COUNT + 2 * SOUND_BLANK + 20 * SOUND_SPRITE)
#define SOUND_WINDOW_HEIGHT (SOUND_RECT_HEIGHT + 2 * SOUND_BLANK )
#define SOUND_WINDOW_LEFT   ((SCREEN_WIDTH - SOUND_WINDOW_WIDTH  ) / 2)
#define SOUND_WINDOW_TOP    ((SCREEN_HEIGHT - SOUND_WINDOW_HEIGHT) / 2)

#define DEFAULTCHARSET "GBK"

