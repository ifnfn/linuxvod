#ifndef HWIM_H
#define HWIM_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/******************************** hw.h **********************************/
#define ALC_NUMERIC         0x0001 /* 0..9                              */
#define ALC_LCALPHA         0x0002 /* a..z                              */
#define ALC_UCALPHA         0x0004 /* A..Z                              */
#define	ALC_PUNCTUATION     0x0008 /* !",:;?¡¢¡£'()¡­¡¶¡·                */
#define	ALC_SYMBOLS         0x0010 /* #$%&*+-./<=>@£¤                   */
#define ALC_CHINESE_COMMON  0x0020 /* Commonly used Chinese characters  */
#define ALC_CHINESE_RARE    0x0040 /* Rarely used Chinese characters    */
#define ALC_CHINESE_VARIANT 0x0080 /* Variant Chinese characters        */

#ifndef BYTE
#define BYTE unsigned char
#endif

#ifndef WORD
#define WORD unsigned short
#endif

#ifdef __cplusplus
extern "C" {
#endif
int HWRecognize( WORD* pTrace, int nLength, char* pResult, int nCand, WORD wRange );

#ifdef __cplusplus
}
#endif

/*************************************************************************************/

#define NWORD        	6
#define MAXSIZE      	1000
#define NALLOC       	MAXSIZE
#define THRESHOLD_MIN   9
#define THRESHOLD_MAX   169
#if 1
#define REC_ALL ((ALC_NUMERIC)|(ALC_LCALPHA)|(ALC_UCALPHA)|(ALC_PUNCTUATION)|(ALC_SYMBOLS)|(ALC_CHINESE_COMMON)|(ALC_CHINESE_RARE)|(ALC_CHINESE_VARIANT))
#else
#define REC_ALL ((ALC_NUMERIC)|(ALC_LCALPHA)|(ALC_UCALPHA)|(ALC_PUNCTUATION)|(ALC_SYMBOLS))
#endif

#define REC_NUM (ALC_NUMERIC)
#define REC_ENG ((ALC_UCALPHA) | (ALC_LCALPHA) )

struct tagSTROKE {
	WORD startIndex;
	WORD endIndex;
};

struct tagHWDATA {
	WORD m_data[MAXSIZE + 4];
	WORD m_len;
	tagSTROKE strokes[100];
	WORD strokeLen;
};

class hwInput
{
	public:
		tagHWDATA data;
		hwInput();

		~hwInput(){}
		void setMode     (int mode);
		void clearPoint  (void);
		void addPoint    (int x, int y);
		int  process     (char *pResult);
		void startStroke (void);
		void endStroke   (void);
		void removeStroke(void);
		int  strokeCount (void) { return (int)data.strokeLen;}
	private:
		int recmode;
};

#endif
