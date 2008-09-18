/*****************************************************************************
*  constants.h : 
*  REALmagic Quasar Hardware Library Demux
*  Created by Kevin Vo
*  Copyright Sigma Designs Inc
*  Sigma Designs Proprietary and confidential
*  Created on 10/29/2001
*****************************************************************************/

#ifndef __CONSTANTS_H_
#define __CONSTANTS_H_

#define SUCCESS_DEMUX    0
#define EXIT_DEMUX       1
#define STOP_DEMUX       2
#define END_OF_STREAM    3

/////////////////////////////////////////
//MPEG 4 visual object defines
#define VIDEO_OBJECT_SEQUENCE_START_CODE	0x000001B0	// reserved in Mpeg2
#define VIDEO_OBJECT_SEQUENCE_END_CODE		0x000001B1	// reserved in Mpeg2
#define USER_DATA_START_CODE				0x000001B2	// same as Mpeg2
#define GROUP_VOP_START_CODE				0x000001B3	// SEQUENCE_HEADER for Mpeg2
#define VIDEO_SESSION_ERROR_CODE			0x000001B4	// SEQUENCE_ERROR_CODE for Mpeg2
#define VIDEO_OBJECT_START_CODE				0x000001B5	// EXTENSION_START_CODE for Mpeg2
#define VOP_START_CODE						0x000001B6	// reserved in Mpeg2
// 0x000001B6 - 0x000001B9 reserved for Mpeg4
#define FACE_OBJECT_START_CODE				0x000001BA	// PACK_START_CODE for Mpeg2
#define FACE_OBJECT_PLANE_START_CODE		0x000001BB	// SYSTEM_START_CODE for Mpeg2
#define MESH_OBJECT_START_CODE				0x000001BC	// PROGRAM_STREAM_MAP for Mpeg2
#define MESH_OBJECT_PLANE_START_CODE		0x000001BD	// PRIVATE_STREAM_1 for Mpeg2
#define STILL_TEXTURE_OBJECT_START_CODE		0x000001BE	// PADDING_STREAM for Mpeg2
#define TEXTURE_SPATIAL_LAYER_START_CODE	0x000001BF	// PRIVATE_STREAM_2 for Mpeg2
#define TEXTURE_SNR_LAYER_START_CODE		0x000001C0	// AUDIO_STREAM for Mpeg2
// 0x000001C1 - 0x000001C5 reserved for Mpeg4
// TBD system start codes 0x000001C6 - 0x000001FF	// AUDIO_STREAM for Mpeg2

// CheckMPG4type function
#define MAXHEADERSIZE	16
#define SIZEPLUSTYPE	8
#define NUMBERTYPES		6
#define TYPENAMELENGTH	4
#define MPEG4TYPES		3
#define STSDOFFSET		8
/////////////////////////////////////////
//MPEG 1, 2 system, video, audio defines
#define VIDEO_STREAM          0x000001E0
#define AUDIO_STREAM          0x000001C0
#define AC3_PCM_DTS_STREAM    0x000001BD    // Private Stream 1
#define PACK_START_CODE       0x000001BA
#define SYSTEM_START_CODE     0x000001BB
#define AC3_HEADER            0x0B77
#define DTS_HEADER            0x7FFE8001

#define SEQUENCE_HEADER       0x000001B3
#define EXTENSION_START_CODE  0x000001B5

#define PROGRAM_STREAM_MAP    0x000001BC
#define ECM_STREAM            0x000001F0
#define EMM_STREAM            0x000001F1
#define PROGRAM_STREAM_DIR    0x000001FF
#define DSMCC_STREAM          0x000001F2
#define TYPE_E_STREAM         0x000001F8
#define PRIVATE_STREAM_2      0x000001BF
#define PRIVATE_STREAM_1      0x000001BD
#define PADDING_STREAM        0x000001BE

#define AC3_SUBSTREAM_ID      0x10    // 1000 0***b
#define DTS_SUBSTREAM_ID      0x11    // 1000 1***b
#define PCM_SUBSTREAM_ID      0x14    // 1010 0***b
#define SUB_SUBSTREAM_ID      0x1     // 001* ****b
#define SDDS_SUBSTREAM_ID     0x12	// 1001 0***b

// Transport stream
#define M2T_SYNC_BYTE            0x47
#define TRANSPORT_PACKET_LENGTH  188

// DVD VOB files
// #define DVD_PROGRAM_MUX_RATE     0xF8C38901
#define DVD_PROGRAM_MUX_RATE 0x0189C3F8
#define DVD_PACKET_SIZE          2048

#define ISO_11172_END_CODE	0x000001B9	// End of stream

// File type
#define FT_UNKNOWN            0
#define FT_MPEG_VIDEO         1
#define FT_MPEG_AUDIO         2
#define FT_MPEG_SYSTEM        3
#define FT_MPEG_AVI           4
#define FT_MPEG2_VIDEO        5
#define FT_MPEG2_SYSTEM       6
#define FT_MPEG2_TRANSPORT    7
#define FT_DVD_VMG            8
#define FT_DVD_VTS	          9
#define FT_AC3_AUDIO         10
#define FT_PES               11
#define FT_MPEG4_VIDEO       12
#define FT_DTS_AUDIO         13
#define FT_MPEG4_SYSTEM      14
#define FT_MPEG4_SYSTEMAUDIO 15
#define FT_MPEG4_SYSTEMVIDEO 16
#define FT_QUICKTIME	     17
#define FT_DVD_VOB           18

#define AUDIO_MASK            0xFFF0
#define AUDIO_HEADER          0xFFF0
#define AUDIO_FREQ_441        44100
#define AUDIO_FREQ_48         48000
#define AUDIO_FREQ_32         32000
#define AUDIO_FREQ_96         96000
#define AUDIO_FREQ_RESERVED   3

// The following defines have the same values as in
// Hardware Library but different names.
#define MM_AUDIO_FORMAT_MPEG1     1
#define MM_AUDIO_FORMAT_MPEG2     2
#define MM_AUDIO_FORMAT_AC3       3
#define MM_AUDIO_FORMAT_PCM       4
#define MM_AUDIO_FORMAT_DTS       5
#define MM_AUDIO_FORMAT_DVD_AUDIO 6
#define MM_AUDIO_FORMAT_ADPCM     7

#define MM_VIDEO                      0
#define MM_AUDIO                      1
#define MM_AUDIO_FORMAT               INT
#define MM_MEDIASUBTYPE_NULL          0
#define MM_MEDIASUBTYPE_MPEG1Payload  MM_AUDIO_FORMAT_MPEG1
#define MM_MEDIASUBTYPE_DOLBY_AC3     MM_AUDIO_FORMAT_AC3
#define MM_MEDIASUBTYPE_PCM           MM_AUDIO_FORMAT_PCM
#define MM_MEDIASUBTYPE_DTS           MM_AUDIO_FORMAT_DTS
#ifndef HRESULT
#define HRESULT                       LONG
#endif
#define MM_AM_MEDIA_TYPE              INT
#define MM_REFIID                     INT
#define MM_OK                         0
#define MM_NOTIMPL                    1


//#define INVALID_LENGTH 0x00000000FFFFFFFF
#define MIN_PACKET_HEADER_SEARCH 3000

#ifndef MAKEWORD
#define MAKEWORD(a, b)  ((WORD)(((BYTE)(a & 0xff)) | ((WORD)((BYTE)(b & 0xff))) << 8))
#endif

#ifndef MAKELONG
#define MAKELONG(a, b)  ((LONG)(((WORD)(a & 0xffff)) | ((DWORD)((WORD)(b & 0xffff))) << 16))
#endif

#define MATCHBYTESTREAM(startPointer, compareToThis) \
MAKELONG(MAKEWORD(*(startPointer+3),*(startPointer+2)),MAKEWORD(*(startPointer+1),*startPointer)) == compareToThis


#endif
