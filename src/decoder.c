// decoder.c
#include "../include/decoder.h"
#include <libavdevice/avdevice.h>

int open_input(TranscoderContext *ctx) {
  avdevice_register_all();

  const AVInputFormat *input_format = av_find_input_format("v4l2");
  if (!input_format) {
    fprintf(stderr, "V4L2 not found\n");
    return -1;
  }

  AVDictionary *options = NULL;
  av_dict_set(&options, "input_format", "mjpeg", 0);
  av_dict_set(&options, "video_size", "1920x1080", 0);
  // Let the camera use its native framerate
  av_dict_set(&options, "num_buffers", "3", 0);

  int ret = avformat_open_input(&ctx->input_ctx, "/dev/video0", input_format,
                                &options);
  if (ret < 0) {
    fprintf(stderr, "Cannot open webcam: %s\n", av_err2str(ret));
    av_dict_free(&options);
    return ret;
  }
  av_dict_free(&options);

  ret = avformat_find_stream_info(ctx->input_ctx, NULL);
  if (ret < 0) {
    fprintf(stderr, "Cannot find stream info: %s\n", av_err2str(ret));
    return ret;
  }

  ctx->video_stream_index =
      av_find_best_stream(ctx->input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
  if (ctx->video_stream_index < 0) {
    fprintf(stderr, "Cannot find video stream\n");
    return AVERROR_STREAM_NOT_FOUND;
  }

  AVStream *stream = ctx->input_ctx->streams[ctx->video_stream_index];

  // Calculate frame duration from actual stream timebase and framerate
  AVRational frame_rate = stream->avg_frame_rate;
  ctx->frame_duration = av_q2d(av_inv_q(frame_rate));
  ctx->last_pts = AV_NOPTS_VALUE;

  printf("Input format: %dx%d @ %d/%d fps (%.3f ms per frame)\n",
         stream->codecpar->width, stream->codecpar->height, frame_rate.num,
         frame_rate.den, ctx->frame_duration * 1000);

  return 0;
}

int init_decoder(TranscoderContext *ctx) {
  AVStream *stream = ctx->input_ctx->streams[ctx->video_stream_index];
  const AVCodec *decoder = avcodec_find_decoder(stream->codecpar->codec_id);
  if (!decoder) {
    fprintf(stderr, "Cannot find decoder\n");
    return -1;
  }

  ctx->dec_ctx = avcodec_alloc_context3(decoder);
  if (!ctx->dec_ctx) {
    fprintf(stderr, "Cannot allocate decoder context\n");
    return AVERROR(ENOMEM);
  }

  int ret = avcodec_parameters_to_context(ctx->dec_ctx, stream->codecpar);
  if (ret < 0) {
    fprintf(stderr, "Cannot copy decoder params: %s\n", av_err2str(ret));
    return ret;
  }

  ret = avcodec_open2(ctx->dec_ctx, decoder, NULL);
  if (ret < 0) {
    fprintf(stderr, "Cannot open decoder: %s\n", av_err2str(ret));
    return ret;
  }

  return 0;
}
