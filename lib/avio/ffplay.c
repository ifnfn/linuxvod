#define HAVE_AV_CONFIG_H
#include "avformat.h"

#include <time.h>
#include <unistd.h>


enum {
	AV_SYNC_AUDIO_MASTER, /* default choice */
	AV_SYNC_VIDEO_MASTER,
	AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

typedef struct VideoState {
	AVInputFormat *iformat;
	int abort_request;
	AVFormatContext *ic;

	int audio_stream;
	int subtitle_stream;
	int video_stream;
	char filename[1024];
	URLContext *video,*audio;
} VideoState;

/* options specified by the user */
static AVInputFormat *file_iformat;
static AVImageFormat *image_format;
static const char *input_filename;
static int64_t start_time = AV_NOPTS_VALUE;

static VideoState *cur_stream;

static VideoState *global_video_state;

static int decode_interrupt_cb(void)
{
	return (global_video_state && global_video_state->abort_request);
}

static int decode_thread(void *arg)
{
	VideoState *is = arg;
	AVFormatContext *ic;
	int err, i, ret, video_index, audio_index;
	AVPacket pkt1, *pkt = &pkt1;
	AVFormatParameters params, *ap = &params;

	video_index = -1;
	audio_index = -1;
	is->video_stream = -1;
	is->audio_stream = -1;
	is->subtitle_stream = -1;

	global_video_state = is;
	url_set_interrupt_cb(decode_interrupt_cb);

	memset(ap, 0, sizeof(*ap));
	ap->image_format = image_format;
	ap->initial_pause = 1; /* we force a pause when starting an RTSP stream */

	err = av_open_input_file(&ic, is->filename, is->iformat, 0, ap);
	if (err < 0)
	{
		ret = -1;
		goto fail;
	}
	printf("ic->nb_streams=%s(%s)\n", ic->iformat->name, ic->iformat->long_name);
	is->ic = ic;

	err = av_find_stream_info(ic);
	if (err < 0)
	{
		fprintf(stderr, "%s: could not find codec parameters\n", is->filename);
		ret = -1;
		goto fail;
	}
	ic->pb.eof_reached= 0; //FIXME hack, ffplay maybe shouldnt use url_feof() to test for the end

	printf("ic->nb_streams=%d(%s)\n", ic->nb_streams, ic->title);

	/* if seeking requested, we execute it */
	if (start_time != AV_NOPTS_VALUE)
	{
		int64_t timestamp;

		timestamp = start_time;
		/* add the stream start time */
		if (ic->start_time != AV_NOPTS_VALUE)
			timestamp += ic->start_time;
		ret = av_seek_frame(ic, -1, timestamp, AVSEEK_FLAG_BACKWARD);
		if (ret < 0) {
			fprintf(stderr, "%s: could not seek to position %0.3f\n",
					is->filename, (double)timestamp / AV_TIME_BASE);
		}
	}

	for(i = 0; i < ic->nb_streams; i++)
	{
		AVCodecContext *enc = ic->streams[i]->codec;
		switch(enc->codec_type)
		{
			case CODEC_TYPE_AUDIO:
				if (audio_index < 0)
					audio_index = i;
				break;
			case CODEC_TYPE_VIDEO:
				if (video_index < 0)
					video_index = i;
				break;
			default:
				break;
		}
	}

	/* open the streams */
	if (audio_index >= 0)
		is->audio_stream = audio_index;

	if (video_index >= 0) {
		is->video_stream = video_index;
	} else {
	}

	if (is->video_stream < 0 && is->audio_stream < 0)
	{
		fprintf(stderr, "%s: could not open codecs\n", is->filename);
		ret = -1;
		goto fail;
	}

	for(;;)
	{
		if (is->abort_request)
			break;
		ret = av_read_frame(ic, pkt);
		if (ret < 0) {
		if (url_ferror(&ic->pb) == 0) {
				usleep(100); /* wait for user event */
		continue;
		} else
			break;
		}
		if (pkt->stream_index == is->audio_stream) {
			url_write(is->audio, pkt->data, pkt->size);
//			printf("audio_stream\n");
//			packet_queue_put(&is->audioq, pkt);
		}
		else if (pkt->stream_index == is->video_stream) {
			url_write(is->video, pkt->data, pkt->size);
//			printf("video_stream\n");
//			packet_queue_put(&is->videoq, pkt);
		} else if (pkt->stream_index == is->subtitle_stream) {
			printf("subtitle_stream\n");
//			packet_queue_put(&is->subtitleq, pkt);
		} else {
			av_free_packet(pkt);
		}
	}
	/* wait until the end */
	while (!is->abort_request)
		usleep(100);

	ret = 0;
fail:
	/* disable interrupting */
	global_video_state = NULL;

	if (is->ic)
	{
		av_close_input_file(is->ic);
		is->ic = NULL; /* safety */
	}
	url_set_interrupt_cb(NULL);

	if (ret != 0) {
	}
	return 0;
}

static VideoState *stream_open(const char *filename, AVInputFormat *iformat)
{
	VideoState *is;

	is = av_mallocz(sizeof(VideoState));
	if (!is)
		return NULL;
	pstrcpy(is->filename, sizeof(is->filename), filename);
	is->iformat = iformat;
	if( url_open(&is->video, "http://192.168.0.232/a.mpg", URL_WRONLY)<0)
		printf("url_open error\n");
	if( url_open(&is->audio, "http://192.168.0.232/a.mp2", URL_WRONLY)<0)
		printf("url_open error\n");
	decode_thread(is);
	return is;
}

void do_exit(void)
{
	if (cur_stream) {
		cur_stream->abort_request = 1;
		url_close(cur_stream->video);
		url_close(cur_stream->audio);
		cur_stream = NULL;
	}
	exit(0);
}

/* Called from the main */
int main(int argc, char **argv)
{
	/* register all codecs, demux and protocols */
	av_register_all();

	input_filename = argv[1];
	cur_stream = stream_open(input_filename, file_iformat);

	do_exit();
	return 0;
}
