// types.h
#ifndef TYPES_H
#define TYPES_H

#include "config.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <pthread.h>

typedef struct QualityPreset {
  int width;
  int height;
  int fps;
  int bitrate;
  const char *name;
  int keyframe_interval;
  int dropped_frames;
  int total_frames;
} QualityPreset;

typedef struct BufferManager {
  uint8_t *buffer;
  size_t size;
  size_t write_pos;
  size_t read_pos;
  pthread_mutex_t mutex;
  int is_full;
} BufferManager;

typedef struct EncoderContext {
  AVCodecContext *enc_ctx;
  AVStream *stream;
  AVFormatContext *fmt_ctx;
  struct SwsContext *sws_ctx;
  AVFrame *scaled_frame;
  BufferManager buffer_mgr;
  QualityPreset *preset;
  int64_t next_pts;
  int64_t last_dts;
  int64_t last_pts;
  double segment_start_time;
  int segment_index;
  double frame_time;
  int64_t first_pts;
} EncoderContext;

typedef struct TranscoderContext {
  AVFormatContext *input_ctx;
  AVCodecContext *dec_ctx;
  AVFrame *frame;
  AVPacket *packet;
  EncoderContext encoders[MAX_QUALITY_LEVELS];
  char *output_dir;
  int video_stream_index;
  pthread_t monitor_thread;
  volatile int running;
  int64_t start_time;
  double frame_duration;
  int64_t last_pts;
} TranscoderContext;

#endif // TYPES_H
