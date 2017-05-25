

//Encodes frames into MPEG video file
#pragma once

extern "C" {
	#include <ffmpeg/libavutil/opt.h>
	#include <ffmpeg/libavcodec/avcodec.h>
	#include <ffmpeg/libavutil/channel_layout.h>
	#include <ffmpeg/libavutil/common.h>
	#include <ffmpeg/libavutil/imgutils.h>
	#include <ffmpeg/libavutil/mathematics.h>
	#include <ffmpeg/libavutil/samplefmt.h>
	#include <ffmpeg/libavformat/avformat.h>
	#include <ffmpeg/libavformat/avio.h>
	#include <ffmpeg/libavutil/frame.h>
	#include <ffmpeg/libavutil/avassert.h>
	#include <ffmpeg/libavutil/timestamp.h>
	#include <ffmpeg/libavformat/avformat.h>
	#include <ffmpeg/libswscale/swscale.h>
	#include <ffmpeg/libswresample/swresample.h>
}
#include "external\glad.h"
#include <string>

//Libs included here to make this more portable
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")


#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

#define SCALE_FLAGS SWS_BICUBIC

// a wrapper around a single output AVStream
typedef struct OutputStream {
	AVStream *st;

	/* pts of the next frame that will be generated */
	int64_t next_pts;
	int samples_count;

	AVFrame *frame;
	AVFrame *tmp_frame;

	float t, tincr, tincr2;

	struct SwsContext *sws_ctx;
	struct SwrContext *swr_ctx;
} OutputStream;



class VideoEncoder
{
public:
	VideoEncoder();
	~VideoEncoder();

	bool IsEncoding() { return fmt != NULL; }
	void BeginEncoding(const std::string& filename);
	void EndEncoding();


	void EncodeFrame();	//Called By Window!!!

protected:
	void RGB2Yuv420p(AVFrame *frame, uint8_t *rgb, const int &width, const int &height);




	void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt);
	int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt);
	void add_stream(OutputStream *ost, AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id, int width, int height);
	AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);
	void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
	GLubyte* frame_data = NULL;
	void fill_yuv_image(AVFrame *pict, int frame_index, int width, int height);
	AVFrame *get_video_frame(OutputStream *ost);
	int write_video_frame(AVFormatContext *oc, OutputStream *ost);

	void close_stream(AVFormatContext *oc, OutputStream *ost);

protected:
	int crop_x, crop_y; //Crop to fit allowed video dimensions

	OutputStream		video_st;
	AVOutputFormat		*fmt;
	AVFormatContext		*oc;
	AVCodec				*video_codec;
	AVDictionary		*opt = NULL;

	GLuint				m_CopyPixelBuffer;
};