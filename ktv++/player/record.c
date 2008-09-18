#include <sys/soundcard.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <stdlib.h>

#include "lame.h"
#include "record.h"
#include "mp3encoder.h"
SCT_Record	recorder;

static lame_global_flags *gf;

int lame_encoder(FILE* OutFileID, short int *WAVBuffer, unsigned char *MP3Buffer, long WAVBufferSize)
{
	int imp3;
	int dwSamples;
	if (WAVBufferSize <= 0)
	return -1;

	if (gf == NULL)
		return -1;
	dwSamples = WAVBufferSize / lame_get_num_channels( gf );

	if ( 1 == lame_get_num_channels( gf )  && WAVBufferSize==2304)
		dwSamples /= 2;

	if ( 1 == lame_get_num_channels( gf ) )
		imp3 = lame_encode_buffer(gf, WAVBuffer, NULL, dwSamples, MP3Buffer,0);
	else
 		imp3 = lame_encode_buffer_interleaved(gf, WAVBuffer, dwSamples, MP3Buffer,0);
	if (imp3 < 0) {
		if (imp3 == -1)
			fprintf(stderr, "mp3 buffer is not big enough... \n");
		else
			fprintf(stderr, "mp3 internal error:  error code=%i\n", imp3);
		return 1;
	}
	if (fwrite(MP3Buffer, 1, imp3,  OutFileID) != (unsigned int)imp3){
		fprintf(stderr, "Error writing mp3 output \n");
		return 1;
	}
	return 0;
}

int InitMP3Convert(void)
{
	if (NULL == (gf = lame_init())) {
		fprintf(stderr, "fatal error during initialization\n");
		return 1;
	}
	lame_set_num_channels(gf, 1);      // lame_set_num_samples(gf, 176400);
	lame_set_brate(gf, 16);
	lame_set_mode(gf, 3);
	lame_set_quality(gf, 2);           // lame_set_VBR(gf, vbr_off);
	lame_set_bWriteVbrTag(gf, 0);
	lame_set_in_samplerate(gf, 11025); // lame_set_out_samplerate(gf, 44100);
	lame_init_bitstream( gf );
	lame_init_params( gf );
	return 0;
}

void FinishConvert(FILE *fid)
{
	lame_mp3_tags_fid(gf,fid);
}

static void RecorderThread(void *val) // Â¼ÒôÏß³Ì
{
	int AudioID;
	FILE* FAudio;
	long ReadSize = 0;
	char SongCode[20];
	unsigned char *ip;
	int *op;
	int *iop;
	int b = sizeof(int)*8;
	int i;
	pthread_mutex_lock(&recorder.RecordMutex);
	strcpy(SongCode, (char *)val);

	AudioID = open("/dev/audio", O_RDWR);
	recorder.MP3FD = fopen(SongCode, "wb+");

	if(AudioID == 0 || recorder.MP3FD == NULL){
		puts("error in open the device!");
		goto Unload;
	}

	ioctl(AudioID,SNDCTL_DSP_RESET,(char *)&i);
	ioctl(AudioID,SNDCTL_DSP_SYNC, (char *)&i);
	//i=1;
	//ioctl(AudioID,SNDCTL_DSP_NONBLOCK,(char *)&i);
	i=11000;  ioctl(AudioID, SNDCTL_DSP_SPEED,      (char *)&i);
	i=1;      ioctl(AudioID, SNDCTL_DSP_CHANNELS,   (char *)&i);
	i=8;      ioctl(AudioID, SNDCTL_DSP_SETFMT,     (char *)&i);
	i=3;      ioctl(AudioID, SNDCTL_DSP_SETTRIGGER, (char *)&i);
	i=3;      ioctl(AudioID, SNDCTL_DSP_SETFRAGMENT,(char *)&i);
	i=1;      ioctl(AudioID, SNDCTL_DSP_PROFILE,    (char *)&i);

	FAudio = fdopen(AudioID, "r");
	while( !recorder.StopRecord ){
		if (recorder.Pause){
			continue;
		}
		ReadSize = fread(recorder.RecordBuf, 1, READ_SIZE, FAudio);
		op = recorder.RecordBuf + ReadSize;
		ip = (unsigned char *)recorder.RecordBuf;
		iop = recorder.RecordBuf+ReadSize;
		for(i=ReadSize; i>0; i--){
			*--op = (ip[i] ^ 0x80)<< (b-8) | 0x7F<<(b-16);
			recorder.WAVBuffer[i] = *--iop >>(8*sizeof(int)-16);
		}
		lame_encoder(recorder.MP3FD, recorder.WAVBuffer, recorder.MP3Buffer, ReadSize);
	}
	FinishConvert( recorder.MP3FD );
	fclose( recorder.MP3FD );
	close(AudioID);
	fclose(FAudio);
Unload:
	pthread_mutex_unlock( &recorder.RecordMutex );
	pthread_exit(0);
}

bool InitRecorder(void)
{
	if ( InitMP3Convert() ){
		fprintf(stderr, "%s\n", "Dont Init MP3Convert");
		return false;
	}
	pthread_mutex_init( &recorder.RecordMutex, NULL);
	recorder.StopRecord = true;
	recorder.MP3Buffer = (unsigned char *)
		malloc(1.25*(READ_SIZE/lame_get_num_channels(gf))+7200);
	if (recorder.MP3Buffer == NULL){
		fprintf(stderr, "%s\n", "Dont Allocation memory for MP3 buffer");
		return false;
	}
	char	s[50];
	strcat(s, "/");
	strcat(recorder.SavePath, s);
	PauseRecord(false);
	return true;
}

void DeinitRecorder(void)
{
	StopRecord();
	free(recorder.MP3Buffer);
	pthread_mutex_destroy( &recorder.RecordMutex);
}

bool IsRecording(void){
	return !recorder.StopRecord;
}

void PauseRecord(bool val)
{
	recorder.Pause = val;
	return ;
}

bool StartRecord(unsigned char *SongCode)
{
	if (!recorder.StopRecord){
		if ( !StopRecord() ){
			return false;
		}
	}
	if (strlen(SongCode) == 0)
		return false;
	PauseRecord(false);
	recorder.StopRecord = false;
	pthread_create(&recorder.record, NULL, (void *)&Recorded, (void *)SongCode);
	return	true;
}

bool StopRecord( void )
{
	void *retval;
	if ( recorder.StopRecord == true)
		return true;
	PauseRecord(false);
	recorder.StopRecord = true;
	pthread_mutex_lock( &recorder.RecordMutex );
	pthread_mutex_unlock( &recorder.RecordMutex );
	pthread_join( recorder.record, &retval );
	return true;
}

bool ReRecord()
{
	if (IsRecording()){
		fflush(recorder.MP3FD);
		return true;
	}
	return false;
}

#if 0
int main(void)
{
	pthread_t record;
	int retval;
	if ( InitMP3Convert() ){
		puts("Dont Init MP3Convert");
		exit(-1);
	}
	pthread_mutex_init( &t, NULL);
	pthread_create(&record, NULL, (void *)&Recorded, (void *)"60015798");
	sleep(5);
	StopRecord = true;
	pthread_mutex_lock(&t);
	pthread_mutex_unlock(&t);
	pthread_join(&record, &retval);
	DeinitMP3Convert();
	pthread_mutex_destroy(&t);
	return 0;
}
#endif
