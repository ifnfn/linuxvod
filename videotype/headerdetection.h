/*****************************************************************************/

/*
 *     headerdetection.h -- REALmagic file header detection
 *
 *      Copyright (C) 2001 Sigma Designs
 *                    originally written by Kevin Vo <kevin_vo@sdesigns.com>
 *                    adapted by Pascal Cannenterre <pascal_cannenterre@sdesigns.com>
 *     
 */

/*****************************************************************************/
#ifndef __HEADERDETECTION_H__
#define __HEADERDETECTION_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef LINUX
  #include "osdef.h"
#else
  #include <windows.h>  
#endif
  
#include "mpegheaders.h"

typedef struct
{
	int iBitsPerSample;
	int iSampleRate;
	int iNumAudioChannels;
} PcmAudio;

typedef struct
{
	int bIsAc3;
	int bIsPcm;
	int bIsDts;
	int bIsMpeg1;
	PcmAudio pcmAudio;  // if audio type is PCM, this struct will be used
}  AudioSubtype;

typedef struct
{
	int SR32, SR441, SR48;
} SampleRateTab;

typedef struct
{
  int fileType;
  int audioFormat;
  AudioSubtype audioSubtype;
  unsigned long sampleRate;
  int TSProgramCount;
  int audioChannelCount;
  int videoChannelCount;
} FileStreamType;

BOOL IsPESFile(const unsigned char *pSearchBuffer, unsigned long dwSearchBufferSize);

MM_AUDIO_FORMAT IdentifyAudioSubtype(unsigned char* pBuffer, unsigned long dwLength, 
												 AudioSubtype* audioSubtype, int iStreamType );
MM_AUDIO_FORMAT IdentifyFileAudioSubtype(char* pSourceFile, AudioSubtype* audioSubtype,
												 int iStreamType );

UINT IdentifyHeader(const unsigned char *pSearchBuffer, unsigned long dwSearchBufferSize);
UINT IdentifyFileHeader(char *pFileName);

UINT CheckMPG4type(char *pFileName);

DWORD GetFileAudioFrequency(char *pFile, int iStreamType);
DWORD GetAudioFrequency(unsigned char* pBuffer, unsigned long lLength, int iStreamType);

DWORD GetFileAc3AudioFrequency(char *pFileName);
DWORD GetAc3AudioFrequency(unsigned char* pBuffer, unsigned long dwLength);

void ReverseBytes(unsigned char *pBuffer, unsigned long dwLength);
BOOL IsFileAc3Reverse(char* pFile);
BOOL IsDvdVOB(const unsigned char* pSearchBuffer, unsigned long dwBufferSearchSize);
BOOL IsAc3Reverse(const unsigned char* pBuffer, unsigned long dwBufferSearchSize);

INT GetFileTSProgramCounter(char* pFile);
INT GetTSProgramCounter(const unsigned char* pBuffer, unsigned long dwLength);

void GetFileAVChannelCounter(char* pFile, int* iAudio, int* iVideo, int iStreamType);
void GetAVChannelCounter(const unsigned char* pBuffer, unsigned long dwLength, int* iAudio, 
								 int* iVideo, int iStreamType);

BOOL SetFirstAccessUnitPtr(const unsigned char* pBuffer, unsigned long dwLength, WORD* firstAccessPointer);

void GetFileProperties(char * fileName, FileStreamType * streamType);


#define MM_RDONLY     0x1
#define MM_WRONLY     0x2
#define MM_RDWR       0x4
#define MM_CREAT      0x8
#define MM_BINARY     0x10
// used by _open(...)
#define MM_S_IREAD    0x40
#define MM_S_IWRITE   0x80

#endif /*__HEADERDETECTION_H__ */
