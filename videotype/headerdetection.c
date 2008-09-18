/*****************************************************************************/

/*
 *     headerdetection.c -- REALmagic file header detection
 *
 *      Copyright (C) 2001 Sigma Designs
 *                    originally written by Kevin Vo <kevin_vo@sdesigns.com>
 *                    adapted by Pascal Cannenterre <pascal_cannenterre@sdesigns.com>
 *
 */

/*****************************************************************************/
#include "headerdetection.h"
#ifdef LINUX
#include "osdef.h"
#endif

#ifndef READ_BUFFERSIZE
#define READ_BUFFERSIZE 0x40000  // 256KB
//#define READ_BUFFERSIZE 0x80000  // 512KB
//#define READ_BUFFERSIZE 0x100000	// 1MB
#endif

static BYTE quantization_table[] = { 16, 20, 24, 16 };
static DWORD pcm_sample_rate_table[] = { 48000, 96000, 48000, 48000 };

static SampleRateTab Ac3FrameSize[38] = // in 1 word = 2 bytes, 32kHz, 441kHz, 48kHz
{ 
	{96, 69, 64},       {96, 70, 64},       {120, 87, 80},      {120, 88, 80},
	{144, 104, 96},     {144, 105, 96},     {168, 121, 112},    {168, 122, 112},
	{192, 139, 128},    {192, 140, 128},    {240, 174, 160},    {240, 175, 160},
	{288, 208, 192},    {288, 209, 192},    {336, 243, 224},    {336, 244, 224},
	{384, 278, 256},    {384, 279, 256},    {480, 348, 320},    {480, 349, 320},
	{576, 417, 384},    {576, 418, 384},    {672, 487, 448},    {672, 488, 448},
	{768, 557, 512},    {768, 558, 512},    {960, 696, 640},    {960, 697, 640},
	{1152, 835, 768},   {1152, 836, 768},   {1344, 975, 896},   {1344, 976, 896},
	{1536, 1114, 1024}, {1536, 1115, 1024}, {1728, 1253, 1152}, {1728, 1254, 1152},
	{1920, 1393, 1280}, {1920, 1394, 1280}
};

static USHORT BitRate[4][15] =
{
	/* reserved */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	/* layer III */
	{0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320},
	/* layer II */
	{0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384 },
	/* layer I */
	{0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
};


void GetFileProperties(char * fileName, FileStreamType * streamType)
{
	streamType->fileType = IdentifyFileHeader(fileName);
	streamType->audioSubtype.bIsAc3 = streamType->audioSubtype.bIsDts = streamType->audioSubtype.bIsMpeg1 = streamType->audioSubtype.bIsPcm = FALSE;
	streamType->audioFormat = IdentifyFileAudioSubtype(fileName, &(streamType->audioSubtype), streamType->fileType );
	streamType->sampleRate = GetFileAudioFrequency(fileName, streamType->fileType );

	GetFileAVChannelCounter (fileName, &(streamType->audioChannelCount), &(streamType->videoChannelCount), streamType->fileType );
	
	if (streamType->fileType == FT_MPEG2_TRANSPORT){
		streamType->TSProgramCount = 1;
		streamType->TSProgramCount = GetFileTSProgramCounter(fileName);
	}
	else
	  streamType->TSProgramCount = 0; 
}

void CheckSubStreamId(INT iSubStreamId, AudioSubtype* audioSubtype, MM_AUDIO_FORMAT* bAudioFormat)
{
	switch (iSubStreamId >> 3){
		case AC3_SUBSTREAM_ID:
			if (!audioSubtype->bIsAc3){
				audioSubtype->bIsAc3 = TRUE;
				// MmDebugLogfile((MmDebugLevelTrace, "AUDIO_FORMAT_AC3"));
			}
			if (!audioSubtype->bIsDts && !audioSubtype->bIsPcm)
				*bAudioFormat = MM_MEDIASUBTYPE_DOLBY_AC3;
			break;
		case DTS_SUBSTREAM_ID:
			if (!audioSubtype->bIsDts){
				audioSubtype->bIsDts = TRUE;
				// MmDebugLogfile((MmDebugLevelTrace, "AUDIO_FORMAT_DTS"));
			}
			if (!audioSubtype->bIsAc3 && !audioSubtype->bIsPcm)
				*bAudioFormat = MM_MEDIASUBTYPE_DTS;
			break;
		case PCM_SUBSTREAM_ID:
			if (!audioSubtype->bIsPcm){
				audioSubtype->bIsPcm = TRUE;
				// MmDebugLogfile((MmDebugLevelTrace, "AUDIO_FORMAT_PCM"));
			}
			if (!audioSubtype->bIsAc3 && !audioSubtype->bIsDts)
				*bAudioFormat = MM_MEDIASUBTYPE_PCM;
			break;
		case SDDS_SUBSTREAM_ID:
			// MmDebugLogfile((MmDebugLevelTrace, "AUDIOSUBTYPE_SDDS"));
			break;
	}
}

/****f* MMDemux/IdentifyAudioSubtype
 * USAGE
 *  MM_AUDIO_FORMAT IdentifyAudioSubtype(char *pSourceFile, AudioSubtype* audioSubtype,
 *    int iStreamType = FT_UNKNOWN)
 *  MM_AUDIO_FORMAT IdentifyAudioSubtype(unsigned char* pBuffer, unsigned long dwLength, 
 *    AudioSubtype* audioSubtype, int iStreamType = FT_UNKNOWN)
 * DESCRIPTION
 *  Identify the audio subtype of a file.
 * PARAMETERS
 *  char *pSourceFile - Pointer to the source file name
 *  AudioSubtype* audioSubtype - 
 * RETURN VALUE
 *  MM_MEDIASUBTYPE_MPEG1Payload if audio subtype is of type audio stream
 *    (for both mpeg1 and mpeg2).
 *  MM_MEDIASUBTYPE_DOLBY_AC3 if audio subtype is of Ac3 stream.
 *  MM_MEDIASUBTYPE_PCM if audio subtype is of PCM stream.
 *  MM_MEDIASUBTYPE_DTS if the stream contains both AC3 and DTS.
 * NOTES
 *  If using with COM Interface, MM_AUDIO_FORMAT has format as GUID.
 *  Else, MM_AUDIO_FORMAT has format as INT.
*/
/**********************************************************************/
MM_AUDIO_FORMAT IdentifyFileAudioSubtype(char *pSourceFile, AudioSubtype* audioSubtype, int iStreamType)
{
	BYTE bBuffer[READ_BUFFERSIZE];
	DWORD dwReadSize = 0;
	INT i = 0;
	MM_AUDIO_FORMAT audio = MM_MEDIASUBTYPE_NULL;
	FILE *pPlayfile;

	if ((pPlayfile = fopen(pSourceFile, "rb")) == NULL){
		printf("Unable to open file!\n");
		return MM_MEDIASUBTYPE_NULL;
	}

	dwReadSize = fread(bBuffer, sizeof(char), READ_BUFFERSIZE, pPlayfile);
	if (iStreamType == FT_UNKNOWN)
		iStreamType = IdentifyHeader(bBuffer, dwReadSize);

	audio = IdentifyAudioSubtype(bBuffer, dwReadSize, audioSubtype, iStreamType);	
	if (audio == MM_MEDIASUBTYPE_NULL) {
		for (i = 0; i < 3; i++){
			dwReadSize = fread(bBuffer, sizeof(char), READ_BUFFERSIZE, pPlayfile);
			audio = IdentifyAudioSubtype(bBuffer, dwReadSize, audioSubtype, iStreamType);	
			if (audio != MM_MEDIASUBTYPE_NULL)
				break;
		}
	}
	fclose(pPlayfile);
	return audio;
}

MM_AUDIO_FORMAT IdentifyAudioSubtype(unsigned char* pBuffer, unsigned long dwLength, AudioSubtype* audioSubtype, int iStreamType)
{
	DWORD dwBufferIndex = 0;
	DWORD dwCode = 0;
	BYTE byte = 0;
	INT wPacketLength = 0;
	INT wHeaderDataLength = 0;
	MM_AUDIO_FORMAT bAudioFormat = MM_MEDIASUBTYPE_NULL;	

	if (iStreamType == FT_UNKNOWN)
		iStreamType = IdentifyHeader(pBuffer, dwLength);

	if (iStreamType == FT_AC3_AUDIO) {
		audioSubtype->bIsAc3 = TRUE;
		return MM_MEDIASUBTYPE_DOLBY_AC3;
	}

	if (iStreamType == FT_DTS_AUDIO) {
		audioSubtype->bIsDts = TRUE;
		return MM_MEDIASUBTYPE_DTS;
	}

	if (iStreamType == FT_MPEG2_TRANSPORT) {
		while (dwBufferIndex < dwLength){
			byte = *(pBuffer + dwBufferIndex++);
			if (byte == M2T_SYNC_BYTE){
				// Number of bytes have moved so far in the TS packet after the sync byte.
				INT wBytesAdvanced = 0;
				INT bAdaptFieldCtrl;
				
				// Transport error indicator, payload unit start indicator, transport priority and PID
				dwBufferIndex += 2;
				// Transport scrambling control, adaptation field control, continuity counter				
				if (dwBufferIndex >= dwLength)
					break;
				byte = *(pBuffer + dwBufferIndex++);
				bAdaptFieldCtrl = (byte & 0x30) >> 4;      // Bit 5 & 4

 				wBytesAdvanced += 3;

				// Adaptation field presents
				if ((bAdaptFieldCtrl == 0x2) || (bAdaptFieldCtrl == 0x3)){
					INT bAdaptFieldLength = 0;
					bAdaptFieldLength = *(pBuffer + dwBufferIndex++);

					if (bAdaptFieldCtrl == 0x2 && bAdaptFieldLength != 183)
						continue;
					else if (bAdaptFieldCtrl == 0x3 && bAdaptFieldLength < 0 && bAdaptFieldLength > 182)
						continue;
					dwBufferIndex += bAdaptFieldLength;
					wBytesAdvanced += bAdaptFieldLength;
				}
				// Payload presents
				if ((bAdaptFieldCtrl == 0x1) || (bAdaptFieldCtrl == 0x3)){
					DWORD nextTSIndex;
					if (dwBufferIndex + (187 - wBytesAdvanced) >= dwLength)
						break;
					nextTSIndex = dwBufferIndex + (187 - wBytesAdvanced);

					while (dwBufferIndex < nextTSIndex){
						dwCode = (dwCode << 8) | *(pBuffer + dwBufferIndex++);
						if ((dwCode & 0xF0FFFFFF) == AUDIO_STREAM){
							// MmDebugLogfile((MmDebugLevelTrace, "AUDIO_FORMAT_MPEG1"));
							audioSubtype->bIsMpeg1 = TRUE;
							return MM_MEDIASUBTYPE_MPEG1Payload;
						}
						else if (dwCode == AC3_PCM_DTS_STREAM){
							BYTE bSubStreamId;
							// Assume it's AC3 for now.
							// MmDebugLogfile((MmDebugLevelTrace, "AUDIO_FORMAT_AC3"));
							audioSubtype->bIsAc3 = TRUE;
							return MM_MEDIASUBTYPE_DOLBY_AC3;

							wPacketLength = *(pBuffer + dwBufferIndex++);
							wPacketLength = (wPacketLength << 8) | *(pBuffer + dwBufferIndex++);

							// Scrambling control, priority, alignment, copyright and original or copy bits.
							dwBufferIndex++;

							// PTS, DTS, ESCR, ESRate, DSM trick mode, additional copy info, CRC and
							// extension flags.
							dwBufferIndex++;
		
							// Get Header Data Length
							wHeaderDataLength = *(pBuffer + dwBufferIndex++);
							dwBufferIndex += wHeaderDataLength;

							// Sub stream id: AC3 (10000***b), DTS (10001***b), PCM (10100***b), SUB (001*****b),
							// SDDS (10010***b)
							if (dwBufferIndex >= dwLength)
								break;
							bSubStreamId = *(pBuffer + dwBufferIndex++);
							wHeaderDataLength++;
							if ((bSubStreamId >> 5) != SUB_SUBSTREAM_ID){
								// Skip number of frame headers and first access unit pointer
								dwBufferIndex += 3;
								wHeaderDataLength += 3;
								CheckSubStreamId(bSubStreamId, audioSubtype, &bAudioFormat);
								if ((bSubStreamId >> 3) == PCM_SUBSTREAM_ID){
									INT iQuantizationWordLength;
									INT iSampleRate;
									INT iNumAudioChannels;
									
									dwBufferIndex++;
									if (dwBufferIndex >= dwLength)
										break;
									byte = *(pBuffer + dwBufferIndex++);
									iQuantizationWordLength = (byte >> 6) & 0x3;
									iSampleRate = (byte >> 4) & 0x3;
									iNumAudioChannels = (byte & 0x7) + 1;
									audioSubtype->pcmAudio.iBitsPerSample = quantization_table[iQuantizationWordLength];
									audioSubtype->pcmAudio.iSampleRate = pcm_sample_rate_table[iSampleRate];
									audioSubtype->pcmAudio.iNumAudioChannels = iNumAudioChannels;
									dwBufferIndex++;
								}
							}
						}
					}
				}
			}
		}	
	}
	else{ // Program stream
		while (dwBufferIndex < dwLength){
			dwCode = (dwCode << 8) | *(pBuffer + dwBufferIndex++);

			if (dwCode == PACK_START_CODE){
				INT res_stuff;
				
				dwBufferIndex += 9;   // SCR and Mux rate
				if (dwBufferIndex >= dwLength)
					break;
				res_stuff = *(pBuffer + dwBufferIndex++) & 0x7;  // Pack stuffing length,
				dwBufferIndex += res_stuff;
				dwCode = 0;
			}
			else if (dwCode == SYSTEM_START_CODE){
				dwBufferIndex += 8;
				while (TRUE){
					if (dwBufferIndex >= dwLength)
						break;
					byte = *(pBuffer + dwBufferIndex);
					if ((byte >> 7) == 1)	// first bit is 1, skip 3 bytes
						dwBufferIndex += 3;  // Stream id, buffer bound scale and bufer size bound (3 bytes).
					else
						break;
				}
				dwCode = 0;
			}
			else if ((dwCode == PADDING_STREAM) || (dwCode == PRIVATE_STREAM_2) || (dwCode == PROGRAM_STREAM_MAP)){
				if (dwBufferIndex >= dwLength)
					break;
				wPacketLength = *(pBuffer + dwBufferIndex++);
				if (dwBufferIndex >= dwLength)
					break;
				wPacketLength = (wPacketLength << 8) | *(pBuffer + dwBufferIndex++);
				dwBufferIndex += wPacketLength;
				dwCode = 0;
			}
			else if ((dwCode & 0xF0FFFFFF) == VIDEO_STREAM){
				if (dwBufferIndex >= dwLength)
					break;
				wPacketLength = *(pBuffer + dwBufferIndex++);
				if (dwBufferIndex >= dwLength)
					break;
				wPacketLength = (wPacketLength << 8) | *(pBuffer + dwBufferIndex++);
				dwBufferIndex += wPacketLength;
				dwCode = 0;
			}
			else if ((dwCode & 0xF0FFFFFF) == AUDIO_STREAM){
				// MmDebugLogfile((MmDebugLevelTrace, "AUDIO_FORMAT_MPEG1"));
				audioSubtype->bIsMpeg1 = TRUE;
				return MM_MEDIASUBTYPE_MPEG1Payload;
			}
			else if (dwCode == AC3_PCM_DTS_STREAM){	
				BYTE bSubStreamId;

				if (dwBufferIndex + 2 >= dwLength)
					break;
				wPacketLength = *(pBuffer + dwBufferIndex++);
				wPacketLength = (wPacketLength << 8) | *(pBuffer + dwBufferIndex++);

				// Scrambling control, priority, alignment, copyright and original or copy bits.
				dwBufferIndex++;

				// PTS, DTS, ESCR, ESRate, DSM trick mode, additional copy info, CRC and extension flags.
				dwBufferIndex++;

				// Get Header Data Length
				if (dwBufferIndex > dwLength)
					break;
				wHeaderDataLength = *(pBuffer + dwBufferIndex++);
				dwBufferIndex += wHeaderDataLength;

				// Sub stream id: AC3 (10000***b), DTS (10001***b), PCM (10100***b), SUB (001*****b)
				if (dwBufferIndex >= dwLength)
					break;
				bSubStreamId = *(pBuffer + dwBufferIndex++);
				wHeaderDataLength++;
				if ((bSubStreamId >> 5) != SUB_SUBSTREAM_ID){
					// Number of frame headers and first access unit pointer
					dwBufferIndex += 3;
					wHeaderDataLength += 3;
					CheckSubStreamId(bSubStreamId, audioSubtype, &bAudioFormat);
					if ((bSubStreamId >> 3) == PCM_SUBSTREAM_ID){
						INT iQuantizationWordLength;
						INT iSampleRate;
						INT iNumAudioChannels;

						dwBufferIndex++;
						if (dwBufferIndex >= dwLength)
							break;
						byte = *(pBuffer + dwBufferIndex++);
						iQuantizationWordLength = (byte >> 6) & 0x3;
						iSampleRate = (byte >> 4) & 0x3;
						iNumAudioChannels = (byte & 0x7) + 1;
						audioSubtype->pcmAudio.iBitsPerSample = quantization_table[iQuantizationWordLength];
						audioSubtype->pcmAudio.iSampleRate = pcm_sample_rate_table[iSampleRate];
						audioSubtype->pcmAudio.iNumAudioChannels = iNumAudioChannels;
						dwBufferIndex++;
					}
				}
				dwBufferIndex += (wPacketLength - wHeaderDataLength);
			}
		}
	}
	return bAudioFormat;
}

/*f* MMDemux/IdentifyHeader
 * USAGE
 *  UINT IdentifyHeader(char *pFileName)
 *  UINT IdentifyHeader(const BYTE *pSearchBuffer, DWORD dwSearchBufferSize)
 * DESCRIPTION
 *  Search for any MPEG header in order to identify the file type. 
 * PARAMETERS
 *  BYTE *pSearchBuffer : buffer where to load the data.
 *  DWORD dwSearchBufferSize : number of bytes to search. Note that the size
 *    of the valid data must be dwSearchBlockSize + 4.
 * RETURN VALUE
 *  FT_AC3_AUDIO for audio AC3
 *  FT_MPEG_VIDEO for elementary video
 *  FT_MPEG_AUDIO for elementary audio
 *  FT_MPEG_SYSTEM for mpeg1 system
 *  FT_MPEG2_SYSTEM for mpeg2 system
 *  FT_AC3_AUDIO for elementary audio AC3
 *  FT_PES for pes stream
 *  FT_MPEG4_VIDEO for mpeg4 elementary video
 */
/**********************************************************************/
UINT IdentifyFileHeader(char *pFileName)
{
	UINT mpeg4type = CheckMPG4type(pFileName);
	if(mpeg4type == FT_UNKNOWN){
		BYTE bBuffer[READ_BUFFERSIZE];
		DWORD dwSearchBufferSize = 0;

		FILE *pFile = fopen(pFileName, "rb");
		if (!pFile)
			return FT_UNKNOWN;
		dwSearchBufferSize = fread(bBuffer, 1, READ_BUFFERSIZE, pFile);
		fclose(pFile);
		return IdentifyHeader(bBuffer, dwSearchBufferSize);
	}
	else if(mpeg4type == FT_QUICKTIME)
		return FT_UNKNOWN;
	else
		return mpeg4type;
}

UINT IdentifyHeader(const BYTE *pSearchBuffer, DWORD dwSearchBufferSize)
{
	const BYTE *pBuffer  = pSearchBuffer;
	DWORD dwCanBeSystem = 0;
	DWORD dwCanBeSystem2 = 0;
	DWORD dwCanBeMPEGAudio = 0;
	DWORD dwCanBeAC3Audio = 0;
	DWORD dwCanBeDtsAudio = 0;
	DWORD dwCanBeVideo = 0;
	DWORD dwCanBeVideo2 = 0;
	// DWORD dwCanBeVideo4 = 0;
	DWORD dwBufferPos;
	WORD wAudioStart = 0;
	const TCHAR* sVTS = "DVDVIDEO-VTS";
	const TCHAR* sVMG = "DVDVIDEO-VMG";
	
	if (IsDvdVOB(pBuffer, dwSearchBufferSize)) {
		// MmDebugLogfile((MmDebugLevelTrace, "Filetype: AC3_AUDIO"));
		return FT_DVD_VOB;
	}

	if (IsAc3Reverse(pBuffer, dwSearchBufferSize)) {
		// MmDebugLogfile((MmDebugLevelTrace, "Filetype: AC3_AUDIO"));
		return FT_AC3_AUDIO;
	}
	else if (IsPESFile(pBuffer, dwSearchBufferSize)){
		// MmDebugLogfile((MmDebugLevelTrace, "Filetype: PES"));
		return FT_PES;
	}

	// Refuse to open RIFF (AVI and WAV files)
	if (memcmp(pSearchBuffer, "RIFF", 4) == 0){
		// MmDebugLogfile((MmDebugLevelTrace, "Filetype: FT_UNKNOWN"));
		return FT_UNKNOWN;
	}

	// Refuse to open Midi (MID files)
	if (memcmp(pSearchBuffer, "MThd", 4) == 0){
		// MmDebugLogfile((MmDebugLevelTrace, "Filetype: FT_UNKNOWN"));
		return FT_UNKNOWN;
	}

	for (dwBufferPos = 0; dwBufferPos < dwSearchBufferSize; dwBufferPos++){
		if (memcmp(sVTS, pBuffer, 12) == 0) {
			// MmDebugLogfile((MmDebugLevelTrace, "Filetype: DVT_VTS"));
			return FT_DVD_VTS;
		}
		else if (memcmp(sVMG, pBuffer, 12) == 0) {
			// MmDebugLogfile((MmDebugLevelTrace, "Filetype: DVT_VMG"));
			return FT_DVD_VMG;
		}

		if ( MATCHBYTESTREAM(pBuffer, VIDEO_OBJECT_SEQUENCE_START_CODE) ||	// reserved in Mpeg2
			MATCHBYTESTREAM(pBuffer, VOP_START_CODE) )	// reserved in Mpeg2
		{
			// MmDebugLogfile((MmDebugLevelTrace, "Filetype: VideoMpeg4"));
			// for now I care only about mpeg4 video ID <-> VideoObjectLayer
			return FT_MPEG4_VIDEO;
		}
		else if (MATCHBYTESTREAM(pBuffer, PACK_START_CODE)){
			if ((* (pBuffer+4))>>6 == 0x01){
				DWORD dwCanbeMAX = max (dwCanBeVideo,max( dwCanBeAC3Audio, dwCanBeMPEGAudio));
				if (dwCanbeMAX > 8){
					// May be not a system stream if we have found 8 headers before
					if (dwBufferPos == 0)
						dwCanBeSystem2 += 6;
					else
						dwCanBeSystem2 += 2;
				}
				else{
					// MmDebugLogfile((MmDebugLevelTrace, "Filetype: MPEG2_SYSTEM"));
					return FT_MPEG2_SYSTEM;
				}
			}
			else if ((*(pBuffer+4)) >> 4 == 0x02){
				DWORD dwCanbeMAX = max (dwCanBeVideo, 
					max( dwCanBeAC3Audio, dwCanBeMPEGAudio));
				if (dwCanbeMAX > 8){
					// May be not a system stream if we have found 8 headers before
					if (dwBufferPos == 0)
						dwCanBeSystem += 6;
					else
						dwCanBeSystem += 2;
				}
				else{
					// MmDebugLogfile((MmDebugLevelTrace, "Filetype: MPEG_SYSTEM"));
					return FT_MPEG_SYSTEM;
				}
			}
		} 
		else if ((*pBuffer) == M2T_SYNC_BYTE) {
			BOOL bFound = TRUE;
			DWORD dwAd = 0;
			INT   ind = 0;
			for (; (ind < 20) && (dwBufferPos+dwAd < dwSearchBufferSize); ind++){
				bFound &= (*(pBuffer+dwAd) == M2T_SYNC_BYTE);
				if (!bFound)
					break;
				dwAd += TRANSPORT_PACKET_LENGTH;
			}
			if (bFound && (ind >= 10)){
				// MmDebugLogfile((MmDebugLevelTrace, "Filetype: MPEG2_TRANSPORT"));
				return FT_MPEG2_TRANSPORT;
			}
		}


		else if (MATCHBYTESTREAM(pBuffer, SEQUENCE_HEADER) ||MATCHBYTESTREAM(pBuffer, EXTENSION_START_CODE)){
			if (dwBufferPos == 0)
				dwCanBeVideo += 6;	// We need to wait at least a packet size...
			else
				dwCanBeVideo += 2;	// We need to wait at least a packet size...
		}

		else if (dwCanBeVideo && MATCHBYTESTREAM(pBuffer, EXTENSION_START_CODE)){
			dwCanBeVideo2 += 2;
		}
		else if ( MAKEWORD(*(pBuffer+1),*pBuffer) == AC3_HEADER){
			DWORD dwAd = 0;
			INT  wSyncWord = 0;
			BOOL  bFound = TRUE;
			INT   iFrameSize = 0;
			INT ind=0;
			for (; (ind < 20) && (dwBufferPos+dwAd+5 < dwSearchBufferSize); ind++){
				wSyncWord = *(pBuffer+dwAd);
				wSyncWord = (wSyncWord << 8) | *(pBuffer+dwAd+1);
				if (wSyncWord == 0x0B77){
					BYTE byte = *(pBuffer+dwAd+4);
					INT bSampleRate = byte & 0x3;
					INT bFrameSizeCode = byte & 0x3F;

					bFound &= (bFrameSizeCode < 38);
					if (!bFound)
						break;
					if      (bSampleRate == 0) iFrameSize = Ac3FrameSize[bFrameSizeCode].SR48;
					else if (bSampleRate == 1) iFrameSize = Ac3FrameSize[bFrameSizeCode].SR441;
					else if (bSampleRate == 2) iFrameSize = Ac3FrameSize[bFrameSizeCode].SR32;
					// Advance to next syncword
					if (iFrameSize > 0){
						dwAd += (iFrameSize * 2);
						bFound = TRUE;
					}
					else
						dwAd++;
					iFrameSize = 0;
				}
				// Found syncword but unsuccessfull to try next syncword.
				else if (bFound){
					bFound = FALSE;
					break;
				}
				else
					dwAd++;
			}
			if (bFound && (ind >= 10)){
				// MmDebugLogfile((MmDebugLevelTrace, "Filetype: AC3_AUDIO"));
				return FT_AC3_AUDIO;
			}
			if (dwBufferPos == 0)
				dwCanBeAC3Audio += 6;	// We need to wait at least a packet size...
			else
				dwCanBeAC3Audio += 2;	// We need to wait at least a packet size...
		}
		else if ( MATCHBYTESTREAM(pBuffer, DTS_HEADER) ){
			DWORD dwAd = 0;
			BOOL bFound = TRUE;
			INT iFrameSize = 0;
			INT ind = 0;
			for (; (ind < 20) && (dwBufferPos+dwAd+5 < dwSearchBufferSize); ind++){
				if ( MATCHBYTESTREAM((pBuffer+dwAd), DTS_HEADER) ){
					iFrameSize = (((ULONG)(*(pBuffer+dwAd+5) & 0x3) << 12) | 
						((ULONG)(*(pBuffer+dwAd+6)) << 4) |
						((ULONG)(*(pBuffer+dwAd+7) & 0xf0) >> 4));
					iFrameSize++;

					// Advance to next syncword
					dwAd += iFrameSize;
					bFound = TRUE;
				}
				else{
					bFound = FALSE;
					break;
				}
			}
			if (bFound && (ind >= 10)){
				// MmDebugLogfile((MmDebugLevelTrace, "Filetype: DTS_AUDIO"));
				return FT_DTS_AUDIO;
			}
			if (dwBufferPos == 0)
				dwCanBeDtsAudio += 6;	// We need to wait at least a packet size...
			else
				dwCanBeDtsAudio += 2;	// We need to wait at least a packet size...
		}
		else if (!dwCanBeVideo && ((MAKEWORD(*(pBuffer+1), *pBuffer) & AUDIO_MASK) == AUDIO_HEADER)){
			DWORD dwAd = 0;
			INT  wSyncWord = 0;
			BOOL  bFound = TRUE;
			WORD ind=0;
			for (; (ind < 20) && (dwBufferPos+dwAd) < dwSearchBufferSize; ind++){
				wSyncWord = *(pBuffer+dwAd);
				wSyncWord = (wSyncWord << 8) | *(pBuffer+dwAd+1);
				if ((wSyncWord & AUDIO_MASK) == AUDIO_HEADER){
					INT layer;
					INT byte;
					INT bitrateIndex;
					INT sampleFreq;
					INT padding;
					WORD framesize;
					LONG bitrate;
					INT id = (wSyncWord >> 3) & 0x1;

					if (!id)
						break;
					layer = (wSyncWord >> 1) & 0x3;
					byte = *(pBuffer+dwAd+2);
					bitrateIndex = byte >> 4;
					sampleFreq   = (byte >> 2) & 0x3;
					padding      = (byte >> 1) & 0x1;
					framesize = 0;
					bitrate = BitRate[layer][bitrateIndex] * 1000;


					if (sampleFreq == 0){   // 44.1
						if (layer == 3)
							framesize = (WORD) (384 * ((double)bitrate / 44100));
						else if ((layer == 2) || (layer == 1))
							framesize = (WORD) (1152 * ((double)bitrate / 44100));
//						if (padding)
//							dwAd++;
					}
					else if (sampleFreq == 1){// 48
						if (layer == 3)
							framesize = (WORD) (384 * ((double)bitrate / 48000));
						else if ((layer == 2) || (layer == 1))
							framesize = (WORD) (1152 * ((double)bitrate / 48000));
					}
					else if (sampleFreq == 2){  // 32
						if (layer == 3)
							framesize = (WORD) (384 * ((double)bitrate / 32000));
						else if ((layer == 2) || (layer == 1))
							framesize = (WORD) (1152 * ((double)bitrate / 32000));
					}
					dwAd += framesize;
					if (framesize == 0)
						dwAd++;
					// This should depend on padding for 44.1 but can't guarantee, need to do this way to
					// advance to next correct syncword.
					else if (sampleFreq == 0){
						if (*(pBuffer+dwAd) != 0xFF)
							dwAd++;
					}
				}
				else{
					bFound = FALSE;
					break;
				}
			}
			if (bFound && (ind >= 10)){
				// MmDebugLogfile((MmDebugLevelTrace, "Filetype: MPEG_AUDIO"));
				return FT_MPEG_AUDIO;
			}
			if (dwCanBeMPEGAudio == 0)
				wAudioStart = *(WORD *) pBuffer;
			if (wAudioStart == *(WORD *) pBuffer){
				if (dwBufferPos == 0)
					dwCanBeMPEGAudio += 6;	// We need to wait at least a packet size...
				else
					dwCanBeMPEGAudio += 1;	// We need to wait at least a packet size...
			}
		}

		if (dwBufferPos > MIN_PACKET_HEADER_SEARCH){
			// We have waited at least a packet size...
			DWORD dwCanbeMAX = max (dwCanBeVideo, max (dwCanBeAC3Audio, 
				max (dwCanBeMPEGAudio, max (dwCanBeSystem , dwCanBeSystem2))));
			if (dwCanbeMAX == 0)
			{
				// Found Nothing !!!
			}
			else if (dwCanBeSystem == dwCanbeMAX)
			{
				// MmDebugLogfile((MmDebugLevelTrace, "Filetype: MPEG_SYSTEM"));
				return FT_MPEG_SYSTEM;
			}
			else if (dwCanBeSystem2 == dwCanbeMAX)
			{
				// MmDebugLogfile((MmDebugLevelTrace, "Filetype: MPEG2_SYSTEM"));
				return FT_MPEG2_SYSTEM;
			}
			else if (dwCanBeVideo == dwCanbeMAX){
				if (dwCanBeVideo2!=0)
				{
					// MmDebugLogfile((MmDebugLevelTrace, "Filetype: MPEG2_VIDEO"));
					return FT_MPEG2_VIDEO;
				}
				else
				{
					// MmDebugLogfile((MmDebugLevelTrace, "Filetype: MPEG_VIDEO"));
					return FT_MPEG_VIDEO;
				}
			}
/*			else if (dwCanBeAC3Audio == dwCanbeMAX)
			{
				MmDebugLogfile((MmDebugLevelTrace, "Filetype: AC3_AUDIO"));
				return FT_AC3_AUDIO;
			}
			else if (dwCanBeMPEGAudio == dwCanbeMAX)
			{
				MmDebugLogfile((MmDebugLevelTrace, "Filetype: MPEG_AUDIO"));
				return FT_MPEG_AUDIO;
			}
*/
		}
		pBuffer++;	// BYTE pointer inc => @+1
	}
	// MmDebugLogfile((MmDebugLevelTrace, "Filetype: FT_UNKNOWN"));
	return FT_UNKNOWN;
}

#ifdef _VOD_
UINT CheckMPG4type(char *pFileName)
{
	return FT_UNKNOWN;
}
#else
UINT CheckMPG4type(char *pFileName)
{
	BYTE bBuffer[READ_BUFFERSIZE];
	DWORD dwSize;
	DWORD dwCurPos = 0;
	BYTE* pbtype;
	LONG lSearchBufferSize = 0;
	INT inexttype = 0;
	INT iMp4Type = FT_UNKNOWN;
	char* typelist[NUMBERTYPES] = {"moov", "trak", "mdia", "minf", "stbl", "stsd"};
	char* mpeg4types[MPEG4TYPES] = {"mp4v", "mp4a", "mp4s"};

	FILE *file = 0;

	file = fopen(pFileName, "rb");
	if (!file)	return FT_UNKNOWN;

	lSearchBufferSize = fread((char*)&bBuffer, sizeof(BYTE), MAXHEADERSIZE, file);
	if(lSearchBufferSize < SIZEPLUSTYPE) goto end_1;

	dwSize = (((DWORD)bBuffer[0])<<24) | (((DWORD)bBuffer[1])<<16) |
		(((DWORD)bBuffer[2])<<8) | ((DWORD)bBuffer[3]);
	if(dwSize == 0) goto end_1;
	else if(dwSize == 1)
	{
		dwSize =(((DWORD)bBuffer[SIZEPLUSTYPE])<<56) | (((DWORD)bBuffer[SIZEPLUSTYPE+1])<<48) |
			(((DWORD)bBuffer[SIZEPLUSTYPE+2])<<40) | (((DWORD)bBuffer[SIZEPLUSTYPE+3])<<32) |
			(((DWORD)bBuffer[SIZEPLUSTYPE+4])<<24) | (((DWORD)bBuffer[SIZEPLUSTYPE+5])<<16) |
			(((DWORD)bBuffer[SIZEPLUSTYPE+6])<<8)  | ((DWORD)bBuffer[SIZEPLUSTYPE+7]);
	}
	pbtype = &(bBuffer[4]);
	if(strncmp((const char*)pbtype, (const char*)typelist[inexttype], TYPENAMELENGTH) == 0)
	{
		inexttype++;
		dwCurPos += SIZEPLUSTYPE;
	}
	else 
		dwCurPos += dwSize;
	fseek(file, (LONG)dwCurPos, SEEK_SET);
	while(lSearchBufferSize == MAXHEADERSIZE)
	{
		lSearchBufferSize = fread((char*)&bBuffer, sizeof(BYTE), MAXHEADERSIZE, file);
		if(lSearchBufferSize < SIZEPLUSTYPE)
		{
			if(inexttype > NUMBERTYPES - 1) goto end_quicktime;
			goto end_1;
		}
		dwSize = (((DWORD)bBuffer[0])<<24) | (((DWORD)bBuffer[1])<<16) |
			(((DWORD)bBuffer[2])<<8) | ((DWORD)bBuffer[3]);
		if(dwSize == 0)
		{
			if(inexttype > NUMBERTYPES - 1) goto end_quicktime;
			goto end_1;
		}
		else if(dwSize == 1)
		{
			dwSize =(((DWORD)bBuffer[SIZEPLUSTYPE])<<56) | (((DWORD)bBuffer[SIZEPLUSTYPE+1])<<48) |
				(((DWORD)bBuffer[SIZEPLUSTYPE+2])<<40) | (((DWORD)bBuffer[SIZEPLUSTYPE+3])<<32) |
				(((DWORD)bBuffer[SIZEPLUSTYPE+4])<<24) | (((DWORD)bBuffer[SIZEPLUSTYPE+5])<<16) |
				(((DWORD)bBuffer[SIZEPLUSTYPE+6])<<8)  | ((DWORD)bBuffer[SIZEPLUSTYPE+7]);
		}
		pbtype = &bBuffer[4];
		if(inexttype < NUMBERTYPES)
		{
			if(strncmp((const char*)pbtype, (const char*)typelist[inexttype], TYPENAMELENGTH) == 0)
			{
				inexttype++;
				dwCurPos += SIZEPLUSTYPE;
				if(inexttype == NUMBERTYPES) dwCurPos +=STSDOFFSET;
			}
			else 
				dwCurPos += dwSize;
		}
		else
		{
			if(	memcmp(pbtype, mpeg4types[0], TYPENAMELENGTH) == 0 ) goto end_systemvideo;
			if(	memcmp(pbtype, mpeg4types[1], TYPENAMELENGTH) == 0 ) goto end_systemaudio;
			if(	memcmp(pbtype, mpeg4types[2], TYPENAMELENGTH) == 0 ) goto end_system;
			dwCurPos += dwSize;
		}
		if (fseek(file, (LONG)dwCurPos, SEEK_SET) != 0)
		{
			if(inexttype > NUMBERTYPES - 1) goto end_quicktime;
			else goto end_1;
		}
	}

end_system:
	iMp4Type = FT_MPEG4_SYSTEM;
	// MmDebugLogfile((MmDebugLevelTrace, "Filetype: MPEG4_SYSTEM"));
	goto end_1;
end_systemaudio:
	iMp4Type = FT_MPEG4_SYSTEMAUDIO;
	// MmDebugLogfile((MmDebugLevelTrace, "Filetype: MPEG4_SYSTEMAUDIO"));
	goto end_1;
end_systemvideo:
	iMp4Type = FT_MPEG4_SYSTEMVIDEO;
	// MmDebugLogfile((MmDebugLevelTrace, "Filetype: MPEG4_SYSTEMVIDEO"));
	goto end_1;
end_quicktime:
	iMp4Type = FT_QUICKTIME;
end_1:
	fclose(file);
	return iMp4Type;
}
#endif /* _VOD_ */

/****f* MMDemux/IsPESFile
 * USAGE
 *  BOOL IsPESFile (const BYTE *pSearchBuffer, DWORD dwSearchBufferSize)
 * DESCRIPTION
 *  Verify whether the source file is a PES file by looking for the pack and
 *  header start code. Also need to verify the sync byte of Transport Stream.
 * PARAMETERS
 *  const BYTE *pSearchBuffer - a pointer to the data buffer to search.
 *  DWORD dwSearchBufferSize  - size of the search buffer.
 * RETURN VALUE
 *  TRUE if it is a PES file.
 *  FALSE otherwise.
*/
/**********************************************************************/
BOOL IsPESFile(const BYTE *pSearchBuffer, DWORD dwSearchBufferSize)
{
	DWORD dwSyncCode = 0;
	DWORD dwPos = 0;
	BOOL bFound = TRUE;
	DWORD i;

	for (i = 0; i < 4; i++){
		if (*(pSearchBuffer + i) == M2T_SYNC_BYTE){
			BOOL bFound = TRUE;
			DWORD dwPos;
			for (dwPos=0; i+dwPos < min (dwSearchBufferSize, 20 * TRANSPORT_PACKET_LENGTH);
					dwPos+= TRANSPORT_PACKET_LENGTH)
			{
				bFound &= (*(pSearchBuffer+dwPos) == M2T_SYNC_BYTE);
				if (!bFound)
					break;
			}
			if ((bFound) && (dwPos > 10))
				return FALSE;
		}
		dwSyncCode = (dwSyncCode << 8) + (*(pSearchBuffer + i));
	}

	for (; i < dwSearchBufferSize; i++){
		if (dwSyncCode == PACK_START_CODE || dwSyncCode == SYSTEM_START_CODE)
			return FALSE;
		else if (((dwSyncCode & 0xFFFFFFF0) == AUDIO_STREAM) || 
						((dwSyncCode & 0xFFFFFFF0) == VIDEO_STREAM) || (dwSyncCode == AC3_PCM_DTS_STREAM))
		{
			int j;
			DWORD dwPacketLength;
			
			dwPos = i;
			dwPacketLength = *(pSearchBuffer + dwPos++);
			dwPacketLength = (dwPacketLength << 8) + *(pSearchBuffer + dwPos++);
			for (; dwPos < min (dwSearchBufferSize, 10 * dwPacketLength);){
				dwPos += dwPacketLength;
				dwSyncCode = 0;
				for (j = 0; j < 4; j++)
					dwSyncCode = (dwSyncCode << 8) + (*(pSearchBuffer + dwPos++));
				if (((dwSyncCode & 0xFFFFFFF0) == AUDIO_STREAM) || 
						((dwSyncCode & 0xFFFFFFF0) == VIDEO_STREAM) || (dwSyncCode == AC3_PCM_DTS_STREAM))
				{
					bFound = TRUE;
				}
				else
					bFound = FALSE;
				dwPacketLength = *(pSearchBuffer + dwPos++);
				dwPacketLength = (dwPacketLength << 8) + *(pSearchBuffer + dwPos++);
			}
			if (bFound){
				printf("FT_PES\n");
				return TRUE;
			}
		}
		else if (*(pSearchBuffer + i) == M2T_SYNC_BYTE){
			bFound = TRUE;
			for (dwPos=i; i+dwPos < min (dwSearchBufferSize, 20 * TRANSPORT_PACKET_LENGTH);dwPos+= TRANSPORT_PACKET_LENGTH){
				bFound &= (*(pSearchBuffer+dwPos) == M2T_SYNC_BYTE);
				if (!bFound)
					break;
			}
			if ((bFound) && (dwPos > 10))
				return FALSE;
		}
		dwSyncCode = (dwSyncCode << 8) + (*(pSearchBuffer + i));
	}
	return FALSE;
}


/****f* MMDemux/GetAudioFrequency
 * USAGE
 *  DWORD GetAudioFrequency (char *pFile, int iStreamType = FT_UNKNOWN)
 *  DWORD GetAudioFrequency(unsigned char* pBuffer, unsigned long lLength, int iStreamType = FT_UNKNOWN)
 * DESCRIPTION
 *  Get the audio frequency of a stream (a file).
 * PARAMETERS
 *  char *pFile - name of the file to get the frequency.
 * RETURN VALUE
 *  One of the following frequencies:
 *    AUDIO_FREQ_441
 *    AUDIO_FREQ_48
 *    AUDIO_FREQ_32
 *    AUDIO_FREQ_RESERVED
*/
/**********************************************************************/
DWORD GetFileAudioFrequency(char *pFile, int iStreamType)
{
	BYTE bBuffer[READ_BUFFERSIZE];
	DWORD dwReadSize = 0;
	FILE *pPlayfile;

	if ((pPlayfile = fopen(pFile, "rb")) == NULL){
		printf("Unable to open file!\n");
		return FALSE;
	}
	dwReadSize = fread(bBuffer, sizeof(char), READ_BUFFERSIZE, pPlayfile);
	fclose(pPlayfile);

	return GetAudioFrequency(bBuffer, dwReadSize, iStreamType);
}

DWORD GetAudioFrequency(unsigned char* pBuffer, unsigned long lLength, int iStreamType)
{
	DWORD dwBufferIndex = 0;
	DWORD dwSyncCode = 0;
	INT wPacketLength = 0;
	BYTE byte = 0;

	if (iStreamType == FT_UNKNOWN)
		iStreamType = IdentifyHeader(pBuffer, lLength);
	if (iStreamType == FT_AC3_AUDIO)
		return GetAc3AudioFrequency(pBuffer, lLength);
	if (iStreamType == FT_DTS_AUDIO)
		return AUDIO_FREQ_48;

	while (dwBufferIndex < lLength){
		dwSyncCode = (dwSyncCode << 8) + *(pBuffer + dwBufferIndex++);

		if (dwSyncCode == PACK_START_CODE){
			INT res_stuff;
			
			dwBufferIndex += 9;   // SCR and Mux rate
			if (dwBufferIndex >= lLength)
				break;
			res_stuff = *(pBuffer + dwBufferIndex++) & 0x7;  // Pack stuffing length,
			dwBufferIndex += res_stuff;
			dwSyncCode = 0;
		}
		else if (dwSyncCode == SYSTEM_START_CODE){
			dwBufferIndex += 8;
			while (TRUE){
				if (dwBufferIndex >= lLength)
					break;
				byte = *(pBuffer + dwBufferIndex);
				if ((byte >> 7) == 1)	// first bit is 1, skip 3 bytes
					dwBufferIndex += 3;  // Stream id, buffer bound scale and bufer size bound (3 bytes).
				else
					break;
			}
			dwSyncCode = 0;
		}
		else if ((dwSyncCode == PADDING_STREAM) || (dwSyncCode == PRIVATE_STREAM_2) ||
			      (dwSyncCode == PROGRAM_STREAM_MAP))
		{
			if (dwBufferIndex >= lLength)
				break;
			wPacketLength = *(pBuffer + dwBufferIndex++);
			if (dwBufferIndex >= lLength)
				break;
			wPacketLength = (wPacketLength << 8) | *(pBuffer + dwBufferIndex++);
			dwBufferIndex += wPacketLength;
			dwSyncCode = 0;
		}
		else if ((dwSyncCode & 0xF0FFFFFF) == VIDEO_STREAM){
			if (dwBufferIndex >= lLength)
				break;
			wPacketLength = *(pBuffer + dwBufferIndex++);
			if (dwBufferIndex >= lLength)
				break;
			wPacketLength = (wPacketLength << 8) | *(pBuffer + dwBufferIndex++);
			dwBufferIndex += wPacketLength;
			dwSyncCode = 0;
		}
		// PARSE THE AUDIO PES PACKET TO GET THE PAYLOAD.
		else if (((dwSyncCode & 0xF0FFFFFF) == AUDIO_STREAM) || (dwSyncCode == AC3_PCM_DTS_STREAM)){
			INT bHeaderDataLength;

			if (dwBufferIndex + 2 >= lLength)
				break;
			wPacketLength = *(pBuffer + dwBufferIndex++);
			wPacketLength = (wPacketLength << 8) | *(pBuffer + dwBufferIndex++);

			// Scrambling control, priority, alignment, copyright and original bits.
			dwBufferIndex++;

			// PTS, DTS, ESCR, ESRate, DSM trick mode, additional copy info, CRC and extension flags.
			dwBufferIndex++;

			// Header Data Length
			if (dwBufferIndex >= lLength)
				break;
			bHeaderDataLength = *(pBuffer + dwBufferIndex++);
			dwBufferIndex += bHeaderDataLength;

			// Found the PES payload. Start looking for the sync word of audio frame.
			if ((dwSyncCode == AC3_PCM_DTS_STREAM) && (dwBufferIndex + 4 < lLength)){
				// Sub stream id: AC3 (10000***b), DTS (10001***b), PCM (10100***b), SUB (001*****b)
				BYTE bSubstreamId = *(pBuffer + dwBufferIndex++);
				if ((bSubstreamId >> 5) != SUB_SUBSTREAM_ID){
					dwBufferIndex += 3;   // number of frame headers and first access unit pointer
					bHeaderDataLength += 3;
					switch (bSubstreamId >> 3){
						case AC3_SUBSTREAM_ID:
							return GetAc3AudioFrequency(pBuffer + dwBufferIndex, lLength - dwBufferIndex);
						case DTS_SUBSTREAM_ID:
							return AUDIO_FREQ_48;
							break;
						case PCM_SUBSTREAM_ID:
							dwBufferIndex++;
							if (dwBufferIndex >= lLength)
								break;
							byte = *(pBuffer + dwBufferIndex++);
							byte = (byte >> 4) & 0x3;
							if (byte == 0){
								// MmDebugLogfile ((MmDebugLevelLog|MmDebugLevelTrace, "Sample rate: 48K"));
								return AUDIO_FREQ_48;
							}
							else if (byte == 1){
								// MmDebugLogfile ((MmDebugLevelLog|MmDebugLevelTrace, "Sample rate: 96K"));
								return AUDIO_FREQ_96;
							}
							else{
								// MmDebugLogfile ((MmDebugLevelLog|MmDebugLevelTrace, "Sample rate: reserved, use 48Khz"));
								return AUDIO_FREQ_48;
							}
							break;
					}
				}
				dwBufferIndex += (wPacketLength - bHeaderDataLength);
			}
			else{   // Mpeg1 audio
				while (dwBufferIndex + 4 < lLength){
					INT wSyncWord = *(pBuffer + dwBufferIndex++);
					wSyncWord = (wSyncWord << 8) | *(pBuffer + dwBufferIndex++);
					if ((wSyncWord & 0xFFF0) == 0xFFF0) {        // Audio frame header syncword
						BYTE byte = *(pBuffer + dwBufferIndex++);
						INT bSamplingFreq = (byte >> 2) & 3;
						INT bPaddingBit = (byte >> 1) & 1;
						if (bPaddingBit == 1)
							bSamplingFreq = 0;
					
						if (bSamplingFreq == 0){
							// MmDebugLogfile ((MmDebugLevelLog|MmDebugLevelTrace, "Sample rate: 44.1K"));
							return AUDIO_FREQ_441;
						}
						else if (bSamplingFreq == 0x01){
							// MmDebugLogfile ((MmDebugLevelLog|MmDebugLevelTrace, "Sample rate: 48K"));
							return AUDIO_FREQ_48;
						}
						else if (bSamplingFreq == 0x2){
							// MmDebugLogfile ((MmDebugLevelLog|MmDebugLevelTrace, "Sample rate: 32K"));
							return AUDIO_FREQ_32;
						}
						else{
							// MmDebugLogfile ((MmDebugLevelLog|MmDebugLevelTrace, "Sample rate: reserved, use 44.1Khz"));
							return AUDIO_FREQ_441;
						}
					}
				}
			}
			dwSyncCode = 0;
		}
	}
	// MmDebugLogfile ((MmDebugLevelLog|MmDebugLevelTrace, "Unable to identify sample rate, set to 44.1K"));
	return AUDIO_FREQ_441;	// Make this a default.
}


// Reverse every 2 bytes
void ReverseBytes(unsigned char *pBuffer, unsigned long dwLength)
{
	DWORD dwIndex = 0;
	BYTE temp = 0;

	for (; dwIndex < dwLength;){
		temp = *(pBuffer + dwIndex);
		*(pBuffer + dwIndex) = *(pBuffer + dwIndex + 1);
		*(pBuffer + dwIndex + 1) = temp;

		dwIndex += 2;
	}
}


/****f* MMDemux/GetAc3AudioFrequency
 * USAGE
 *  DWORD GetAc3AudioFrequency(char *pFileName)
 *  DWORD GetAc3AudioFrequency(unsigned char* pBuffer, DWORD dwLength)
 * DESCRIPTION
 *  Gets the audio sample rate of AC3
 * PARAMETERS
 *  char* pFileName - File name
 * RETURN VALUE
 *  Audio sample rate
*/
/**********************************************************************/
DWORD GetFileAc3AudioFrequency(char *pFileName)
{
	BYTE bBuffer[READ_BUFFERSIZE];
	DWORD dwReadSize = 0;
	FILE *pFile;

	if ((pFile = fopen(pFileName, "rb")) == NULL){
		printf("Unable to open file!\n");
		return FALSE;
	}
	dwReadSize = fread(bBuffer, sizeof(char), READ_BUFFERSIZE, pFile);
	fclose(pFile);

	return GetAc3AudioFrequency(bBuffer, dwReadSize);
}



DWORD GetAc3AudioFrequency(unsigned char* pBuffer, DWORD dwLength)
{
	DWORD dwBufferIndex = 0;
	INT wSyncCode = 0;

	if (IsAc3Reverse(pBuffer, dwLength))
		ReverseBytes(pBuffer, dwLength);

	while ((dwBufferIndex + 4) < dwLength){
		wSyncCode = (wSyncCode << 8) + *(pBuffer + dwBufferIndex++);
		if (wSyncCode == AC3_HEADER){
			BYTE byte;
			INT bSamplingFreq;
			
			INT crc1 = *(pBuffer + dwBufferIndex++);
			crc1 = (crc1 << 8) + *(pBuffer + dwBufferIndex++);
			byte = *(pBuffer + dwBufferIndex++);
			bSamplingFreq = (byte >> 6) & 0x3;
//			INT bFrameSize = byte & 0x3F;

			if (bSamplingFreq == 1){
				// MmDebugLogfile ((MmDebugLevelLog|MmDebugLevelTrace, "Sample rate: 44.1K"));
				return AUDIO_FREQ_441;
			}
			else if (bSamplingFreq == 0){
				// MmDebugLogfile ((MmDebugLevelLog|MmDebugLevelTrace, "Sample rate: 48K"));
				return AUDIO_FREQ_48;
			}
			else if (bSamplingFreq == 2){
				// MmDebugLogfile ((MmDebugLevelLog|MmDebugLevelTrace, "Sample rate: 32K"));
				return AUDIO_FREQ_32;
			}
			else{
				// MmDebugLogfile ((MmDebugLevelLog|MmDebugLevelTrace, "Sample rate: reserved, use 48KHz"));
				return AUDIO_FREQ_48;
			}
		}
	}
	return AUDIO_FREQ_441;	// make this default
}


/****f* MMDemux/IsAc3Reverse
 * USAGE
 *  BOOL IsAc3Reverse(char* pFile)
 *  BOOL IsAc3Reverse(const BYTE* pBuffer, DWORD dwBufferSearchSize)
 * DESCRIPTION
 *  Determines whether this is an audio AC3 reverse stream.
 * PARAMETERS
 *  const BYTE *pBuffer - a pointer to the data buffer to search.
 *  DWORD dwBufferSearchSize  - size of the search buffer.
 * RETURN VALUE
 *  TRUE if it is reverse; otherwise, FALSE.
*/
/**********************************************************************/
BOOL IsFileAc3Reverse(char* pFile)
{
	BYTE bBuffer[READ_BUFFERSIZE];
	LONG lReadSize = 0;
	FILE *pPlayfile;

	if ((pPlayfile = fopen(pFile, "rb")) == NULL){
		printf("Unable to open file!\n");
		return FALSE;
	}
	lReadSize = fread(bBuffer, sizeof(char), READ_BUFFERSIZE, pPlayfile);
	fclose(pPlayfile);

	return IsAc3Reverse(bBuffer, lReadSize);
}


BOOL IsDvdVOB(const BYTE* pSearchBuffer, DWORD dwBufferSearchSize)
{
	DWORD dwBufferPos;
	DWORD count = 0 ;

	for (dwBufferPos = 0; dwBufferPos < dwBufferSearchSize; dwBufferPos = dwBufferPos + DVD_PACKET_SIZE){
		if ( MATCHBYTESTREAM((pSearchBuffer + dwBufferPos), PACK_START_CODE) ){ 
			if( MATCHBYTESTREAM((pSearchBuffer + dwBufferPos + 10), DVD_PROGRAM_MUX_RATE) ){
				count++;
				// fprintf(stdout, "\nDVD VOB count : %ld\n", count);
				if( count >= 1)
				return TRUE;
			}
		}
	}
	return FALSE;
}

BOOL IsAc3Reverse(const BYTE* pBuffer, DWORD dwBufferSearchSize)
{
	DWORD dwBufferIndex = 0;
	BYTE bSyncCode = 0;
	INT iFrameSize = 0;
	BOOL bAc3Reverse = FALSE;
	int iCounter = 0;
	char szBuf[4];

	while (dwBufferIndex + 6 < dwBufferSearchSize){
		bSyncCode = *(pBuffer + dwBufferIndex++);
		if (bSyncCode == 0x77){
			bSyncCode = *(pBuffer + dwBufferIndex++);
			if (bSyncCode == 0x0B){
				INT bSampleRate;
				INT bFrameSizeCode;
				// Get frame syncinfo
				memcpy(szBuf, pBuffer + dwBufferIndex, 4);
				ReverseBytes((unsigned char*)szBuf, 4);
				bSampleRate = (szBuf[2] >> 6) & 0x3;
				bFrameSizeCode = szBuf[2] & 0x3F;

				if (bFrameSizeCode >= 38)
					return FALSE;
				if (bSampleRate == 0)
					iFrameSize = Ac3FrameSize[bFrameSizeCode].SR48;
				else if (bSampleRate == 1)
					iFrameSize = Ac3FrameSize[bFrameSizeCode].SR441;
				else if (bSampleRate == 2)
					iFrameSize = Ac3FrameSize[bFrameSizeCode].SR32;
				// Advance to next syncword
				if (iFrameSize > 0){
					dwBufferIndex += (iFrameSize * 2 - 2);
					bAc3Reverse = TRUE;
					iCounter++;
				}
				iFrameSize = 0;
			}
			// Found syncword but unsuccessfull to try next syncword.
			else if (bAc3Reverse)
				return FALSE;
		}
		else if (bAc3Reverse)
			return FALSE;
	}
	if (iCounter >= 2)
		return bAc3Reverse;
	else
		return FALSE;
}

/****f* MMDemux/GetTSProgramCounter
 * USAGE
 *  int GetTSProgramCounter(char* pFile)
 *  int GetTSProgramCounter(const BYTE* pBuffer, DWORD dwLength)
 * DESCRIPTION
 *  Determines the number of programs in a transport stream.
 * PARAMETERS
 *  const BYTE *pSearchBuffer - a pointer to the data buffer to search.
 *  DWORD dwSearchBufferSize  - size of the search buffer.
 * RETURN VALUE
 *  Number of the programs.
*/
/**********************************************************************/
INT GetFileTSProgramCounter(char* pFile)
{
	BYTE bBuffer[READ_BUFFERSIZE];
	DWORD dwReadSize = 0;
	FILE *pPlayfile;
	int counter = 0;
	
	if ((pPlayfile = fopen(pFile, "rb")) == NULL){
		// MmDebugLogfile((MmDebugLevelTrace, "Unable to open file!"));
		return 0;
	}
	dwReadSize = fread(bBuffer, sizeof(char), READ_BUFFERSIZE, pPlayfile);
	fclose(pPlayfile);
	
	counter = 0;
	counter = GetTSProgramCounter(bBuffer, dwReadSize);
	if (counter == 0)
		counter = 1;
	
	return counter;
}



INT GetTSProgramCounter(const BYTE* pBuffer, DWORD dwLength)
{
	BYTE bCode = 0;
	DWORD dwBufferIndex = 0;
	int iCounter = 0;

	while (dwBufferIndex < dwLength){
		bCode = *(pBuffer + dwBufferIndex++);
		if (bCode == M2T_SYNC_BYTE){
			BYTE byte = 0;
			WORD wLength = 0;
			INT bPayloadStart;
			INT wPid;
			INT bScrambling;
			INT bAdaptFieldCtrl;
			// Number of bytes have moved so far in the TS packet after the sync byte.
			INT wBytesAdvanced = 0;

			if (dwBufferIndex >= dwLength)
				break;
			byte = *(pBuffer + dwBufferIndex++);
			bPayloadStart = (byte & 0x40) >> 6;        // Bit 6
			wPid = byte & 0x1F;                           // Bits 4-0
			if (dwBufferIndex+1 >= dwLength)
				break;
			wPid = (wPid << 8) | *(pBuffer + dwBufferIndex++);
			byte = *(pBuffer + dwBufferIndex++);
			bScrambling = (byte & 0xC0) >> 6;          // Bit 7 & 6
			bAdaptFieldCtrl = (byte & 0x30) >> 4;      // Bit 5 & 4
 			wBytesAdvanced += 3;

			// Adaptation field presents
			if ((bAdaptFieldCtrl == 0x2) || (bAdaptFieldCtrl == 0x3)){
				INT bAdaptFieldLength = 0;
				bAdaptFieldLength = *(pBuffer + dwBufferIndex++);
				if (bAdaptFieldCtrl == 0x2 && bAdaptFieldLength != 183)
					continue;
				else if ((bAdaptFieldCtrl == 0x3) && (bAdaptFieldLength < 0) && (bAdaptFieldLength > 182))
					continue;
				dwBufferIndex += bAdaptFieldLength;
				wBytesAdvanced += bAdaptFieldLength;
			}
			// Payload presents
			if ((bAdaptFieldCtrl == 0x1) || (bAdaptFieldCtrl == 0x3)){
				if (wPid == 0x1FFF){// Null packet
					// Invalid TS packet?
					if ((bPayloadStart != 0) || (bAdaptFieldCtrl != 1) || (bScrambling != 0))
						continue;
					// Advance to first byte of next packet.
					dwBufferIndex += (187 - wBytesAdvanced);
					continue;
				}
				// Either PES packet or PSI data presents
				else if (bPayloadStart == 1){
					BYTE bPointerField;
					BYTE bTableId;
					if (dwBufferIndex >= dwLength)
						break;
					bPointerField = *(pBuffer + dwBufferIndex++);
					if (bPointerField > (187 - wBytesAdvanced))
						continue;
					// Move the buffer index to the first byte of the section while checking
					// for end of buffer.
					dwBufferIndex += bPointerField;
					if (dwBufferIndex >= dwLength)
						break;
					bTableId = *(pBuffer + dwBufferIndex++);
					if ((bTableId == 0) || (bTableId == 1) || (bTableId == 2) || (bTableId >= 0x40 && bTableId <= 0xFE)){
						wBytesAdvanced += (bPointerField + 2);
						switch (bTableId){
							case 0:		// Program association table (section)
								if (wPid != 0)
									continue;
								{
									BYTE bCode = 0;
									WORD wLength = 0;
									INT bSecSyntaxIndicator;
									INT wSectionLength;
									BYTE bSectionNum;
									BYTE bLastSectionNum;
									
									if (dwBufferIndex >= dwLength)
										continue;
									bCode = *(pBuffer + dwBufferIndex++);
									bSecSyntaxIndicator = bCode >> 7;	// Bit 7
									if ((bSecSyntaxIndicator != 1) || ((bCode & 0x40) != 0))
										continue;
									wSectionLength = (bCode & 0xF);	// Bits 3-0
									if (dwBufferIndex >= dwLength)
										continue;
									wSectionLength = (wSectionLength << 8) | *(pBuffer + dwBufferIndex++);
									if (((wSectionLength & 0xC00) != 0) || (wSectionLength > 1021))
										continue;

									// Transport stream id, 2 bytes
									dwBufferIndex += 2;
									if (dwBufferIndex >= dwLength)
										continue;
									// Version number and current next indicator
									bCode = *(pBuffer + dwBufferIndex++);
									if (dwBufferIndex >= dwLength)
										continue;
									bSectionNum = *(pBuffer + dwBufferIndex++);
									if (dwBufferIndex >= dwLength)
										continue;
									bLastSectionNum = *(pBuffer + dwBufferIndex++);
									wLength += 5;
									while (TRUE){
										INT wProgramNum;
										if (dwBufferIndex + 1 >= dwLength)
											continue;
										wProgramNum = *(pBuffer + dwBufferIndex++);
										wProgramNum = (wProgramNum << 8) | *(pBuffer + dwBufferIndex++);
										dwBufferIndex += 2;
										iCounter++;
										wLength += 4;
										if ((wLength + 4) == wSectionLength)
											break;
									}
									return iCounter;
								}
								break;
							case 1:		// Conditional access table (section)
								if ((wPid != 0) || (bScrambling == 0))
									continue;
								break;
							case 2:		// Program Map Table (section). Look for stream type.
								if (wPid < 0x00010 || wPid > 0x1FFE)
									continue;
								break;
							default:
								break;
						}						
						wBytesAdvanced += wLength;
						// Don't think there's PES payload after each section table. 
						// Advance to first byte of next packet.
						dwBufferIndex += (187 - wBytesAdvanced);
						continue;
					}
					else
					{
						dwBufferIndex -= (bPointerField + 2);
						dwBufferIndex += (187 - wBytesAdvanced);
						continue;
					}
				}
				dwBufferIndex += (187 - wBytesAdvanced);
			}
		}
	}	
	return iCounter;
}

/****f* MMDemux/GetAVChannelCounter
 * USAGE
 *  void GetAVChannelCounter(char* pFile, int* iAudio, int* iVideo, int iStreamType = FT_UNKNOWN)
 *  void GetAVChannelCounter(const BYTE* pBuffer, DWORD dwLength, int* iAudio, int* iVideo,
 *                          int iStreamType = FT_UNKNOWN)
 * DESCRIPTION
 *  Determines the number of video and audio channels
 * PARAMETERS
 *  const BYTE *pBuffer - a pointer to the data buffer to search.
 *  DWORD dwLength  - size of the search buffer.
 *  int* iAudio - pointer to an int that will hold the number of audio channels.
 *  int* iVideo - pointer to an int that will hold the number of video channels.
 * RETURN VALUE
 *  None
*/
/**********************************************************************/
void GetFileAVChannelCounter(char* pFile, int* iAudio, int* iVideo, int iStreamType)
{
	BYTE bBuffer[READ_BUFFERSIZE];
	DWORD dwReadSize = 0;
	FILE *pPlayfile;

	if ((pPlayfile = fopen(pFile, "rb")) == NULL)
	{
		// MmDebugLogfile((MmDebugLevelTrace, "Unable to open file!"));
		return;
	}
	dwReadSize = fread(bBuffer, sizeof(char), READ_BUFFERSIZE, pPlayfile);
	fclose(pPlayfile);
	
	GetAVChannelCounter(bBuffer, dwReadSize, iAudio, iVideo, iStreamType);
}

void GetAVChannelCounter(const BYTE* pBuffer, DWORD dwLength, int* iAudio, int* iVideo,int iStreamType)
{
	DWORD dwCode = 0;
	DWORD dwIndex = 0;

	DWORD dwMpeg1AudioMask = 0x0000;
	DWORD dwAc3Mask = 0x00;
	DWORD dwDtsMask = 0x00;
	DWORD dwPcmMask = 0x00;
	DWORD dwVideoMask = 0x0000;

	*iAudio = 0;
	*iVideo = 0;

	if (iStreamType == FT_UNKNOWN)
		iStreamType = IdentifyHeader(pBuffer, dwLength);
	if (iStreamType == FT_MPEG2_TRANSPORT) {
		*iAudio = 1;
		*iVideo = 1;
		return;
	}
	else if ((iStreamType == FT_MPEG_AUDIO) || (iStreamType == FT_AC3_AUDIO) || (iStreamType == FT_DTS_AUDIO)){
		*iAudio = 1;
		return;
	}
	while (dwIndex < dwLength){
		dwCode = (dwCode << 8) | *(pBuffer + dwIndex++);
		if (((dwCode & 0xFFFFFFF0) == AUDIO_STREAM) || ((dwCode & 0xFFFFFFF0) == VIDEO_STREAM) || (dwCode == AC3_PCM_DTS_STREAM)){
			INT wPacketHeaderLength = 0;
			INT wPacketLength = 0;

			DWORD dwSavedIndex = dwIndex;
			BYTE byte;
			INT bPtsFlag;
			INT bExtFlag;
			BYTE bHeaderDataLength;
			BYTE bCurHeaderDataLength;
			INT wStuffingByte;

			
			wPacketLength = *(pBuffer + dwIndex++);
			if (dwIndex >= dwLength)
				break;
			wPacketLength = (wPacketLength << 8) | *(pBuffer + dwIndex++);

			// Skip byte containing scrambling control, priority, alignment, copyright 
			// and original or copy bits.
			dwIndex++;
			wPacketHeaderLength++;

			// Get PTS, DTS, ESCR, ESRate, DSM trick mode, additional copy info, CRC and
			// extension flags.
			if (dwIndex >= dwLength)
				break;
			byte = *(pBuffer + dwIndex++);
			wPacketHeaderLength++;
			bPtsFlag = byte >> 6;				// Bits 7 & 6, PTS or no
//			INT bESCRFlag = byte & 0x20;			// Bit 5, should be 0
//			INT bESRateFlag = byte & 0x10;			// Bit 4, should be 0
//			INT bDSMTrickModeFlag = byte & 0x8;	// Bit 3, should be 0
//			INT bAddCopyInfoFlag = byte & 0x4;		// Bit 2, should be 0
//			INT bCRCFlag = byte & 0x2;				// Bit 1, should be 0
			bExtFlag = byte & 0x1;				// Bit 0, 0 or 1
		
			// Get Header Data Length
			if (dwIndex >= dwLength)
				break;
			bHeaderDataLength = *(pBuffer + dwIndex++);
			wPacketHeaderLength++;

			// Keep current # of advanced bytes after the PES_header_data_length field.
			// Use this to calculate the stuffing bytes.
			bCurHeaderDataLength = 0;

			// Get PTS
			if (bPtsFlag == 0x2){ // '10'
				dwIndex += 5;
				bCurHeaderDataLength += 5;
			}
			// Check if Extension flag is set to 1 to skip bytes
			if (bExtFlag == 1){
				INT bSTDBufferFlag;

				// Get private data flag, pack header field flag, program packet sequence
				// counter flag, P-STD buffer flag, reserved and PES extention flag2 bits.
				if (dwIndex >= dwLength)
					break;
				byte = *(pBuffer + dwIndex++);
				bCurHeaderDataLength++;	
//				INT bPrivateDataFlag = byte >> 7;			// Bit 7, should be 0
//				INT bPackHeaderFieldFlag = byte & 0x40;	// Bit 6, should be 0
//				INT bSeqCounterFlag = byte & 0x20;			// Bit 5, should be 0
				bSTDBufferFlag = byte & 0x10;			// Bit 4, should be 1
//				INT bReserved = byte & 0xE;				// Bits 3-1, should be 111b
//				INT bExtFlag2 = byte & 0x1;				// Bit 0, should be 0
				// Check if STD Buffer flag is set to 1 to skip STD buffer scale and
				// buffer size (2 bytes).
				if (bSTDBufferFlag == 0x10){	// 10000b or bit 4 is 1
					dwIndex += 2;
					bCurHeaderDataLength += 2;
				}
			}
			// Skip stuffing bytes
			wStuffingByte = bHeaderDataLength - bCurHeaderDataLength;
			dwIndex += wStuffingByte;

			wPacketHeaderLength += bHeaderDataLength ;
			if (wPacketLength <= wPacketHeaderLength){	// wrong pes payload
				dwIndex = dwSavedIndex;
				continue;
			}

			if (dwCode == AC3_PCM_DTS_STREAM){
				BYTE bSubStreamId;

				// Sub stream id: AC3 (10000***b), DTS (10001***b), PCM (10100***b), SUB (001*****b)
				if (dwIndex >= dwLength)
					break;
				bSubStreamId = *(pBuffer + dwIndex++);
				if ((bSubStreamId >> 5) != SUB_SUBSTREAM_ID){
					DWORD shiftCount;
					DWORD newMask;
					
					// Skip number of frame headers and first access unit pointer
					dwIndex += 3;     
					wPacketHeaderLength += 4;

					shiftCount = bSubStreamId & 0x7;
					newMask = 1 << shiftCount;
					switch (bSubStreamId >> 3)
					{
						case AC3_SUBSTREAM_ID:
							if (!((dwAc3Mask >> shiftCount) & 0x1))
							{
								dwAc3Mask = dwAc3Mask | newMask;
								(*iAudio)++;
							}
							break;
						case DTS_SUBSTREAM_ID:
							if (!((dwDtsMask >> shiftCount) & 0x1))
							{
								dwDtsMask = dwDtsMask | newMask;
								(*iAudio)++;
							}					
							break;
						case PCM_SUBSTREAM_ID:
							if (!((dwPcmMask >> shiftCount) & 0x1))
							{
								dwPcmMask = dwPcmMask | newMask;
								(*iAudio)++;
							}						
							break;
					}
				}
			}
			else{
				DWORD shiftCount = dwCode & 0xF;
				DWORD newMask = 1 << shiftCount;
				if ((dwCode & 0xFFFFFFF0) == AUDIO_STREAM){
					if (!((dwMpeg1AudioMask >> shiftCount) & 0x1)){
						dwMpeg1AudioMask = dwMpeg1AudioMask | newMask;
						(*iAudio)++;
					}
				}
				else{
					if (!((dwVideoMask >> shiftCount) & 0x1)){
						dwVideoMask = dwVideoMask | newMask;
						(*iVideo)++;
					}
				}
			}
			dwIndex += (wPacketLength - wPacketHeaderLength);
			dwCode = 0;
		}
	}
}


/****f* MMDemux/GetAc3SyncWordAddr
 * USAGE
 *  BOOL SetFirstAccessUnitPtr(const unsigned char* pBuffer, unsigned long dwLength,
 *    BYTE* addr)
 * DESCRIPTION
 *  Set the first access unit pointer for transport stream.
 * PARAMETERS
 *  const unsigned char* pBuffer - buffer contains the AC3 audio payload
 *  unsigned long dwLength - length of the payload
 *  WORD* firstAccessPointer - 
 * RETURN VALUE
 *  TRUE if first access unit pointer is found
 *  FALSE if not.
*/
/**********************************************************************/
BOOL SetFirstAccessUnitPtr(const unsigned char* pBuffer, unsigned long dwLength,WORD* firstAccessPointer)
{
	DWORD dwBufferIndex = 0;
	WORD wSyncCode = 0;
	INT  iRelativeBytes = 0;

	*firstAccessPointer = 0;
	while (dwBufferIndex < dwLength){
		iRelativeBytes++;
		wSyncCode = (wSyncCode << 8) + *(pBuffer + dwBufferIndex++);
		if (wSyncCode == AC3_HEADER){
			// Found the first access unit.
			*firstAccessPointer = iRelativeBytes-1;
//			MmDebugLogfile((MmDebugLevelTrace, "%d", *firstAccessPointer));
			return TRUE;
		}
	}
	return FALSE;
}
