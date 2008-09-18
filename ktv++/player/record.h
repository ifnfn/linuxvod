#ifndef RECORDER_H
#define RECORDER_H

#define READ_SIZE	4096

typedef struct tagRecord{
	int RecordBuf[READ_SIZE];
	unsigned short WAVBuffer[READ_SIZE];
	unsigned char *MP3Buffer;
	pthread_mutex_t RecordMutex;
	bool StopRecord;
	unsigned char SavePath[255];
	pthread_t record;
	bool Pause;
	FILE* MP3FD;
}SCT_Record;

extern SCT_Record recorder;
bool	StopRecord( void );
bool	StartRecord(unsigned char *SongCode);
void	DeinitRecorder(void);
bool	InitRecorder(void);
bool	IsRecording(void);
void	PauseRecord(bool val);
bool	ReRecord(void);

#endif
