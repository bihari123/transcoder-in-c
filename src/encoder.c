// encoder.c
#include "../include/encoder.h"
#include "../include/buffer.h"
#include "../include/config.h"
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <sys/stat.h>

int init_encoder(EncoderContext *enc, AVCodecContext *dec_ctx,
                 QualityPreset *preset, const char *output_dir) {
  int ret;

  // Find encoder
  const AVCodec *encoder = avcodec_find_encoder_by_name("libx264");
  if (!encoder) {
    fprintf(stderr, "Could not find H.264 encoder\n");
    return -1;
  }

  // Allocate encoder context
  enc->enc_ctx = avcodec_alloc_context3(encoder);
  if (!enc->enc_ctx) {
    fprintf(stderr, "Could not allocate encoder context\n");
    return AVERROR(ENOMEM);
  }

  // Set encoder parameters
  enc->enc_ctx->width = preset->width;
  enc->enc_ctx->height = preset->height;
  enc->enc_ctx->time_base = (AVRational){1, preset->fps};
  enc->enc_ctx->framerate = (AVRational){preset->fps, 1};
  enc->enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
  enc->enc_ctx->bit_rate = preset->bitrate;
  enc->enc_ctx->rc_min_rate = preset->bitrate;
  enc->enc_ctx->rc_max_rate = preset->bitrate;
  enc->enc_ctx->rc_buffer_size = preset->bitrate / 2;
  enc->enc_ctx->gop_size = preset->keyframe_interval;
  enc->enc_ctx->max_b_frames = 0;
  enc->enc_ctx->refs = 1;
  enc->enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  // Set encoder options
  AVDictionary *opts = NULL;
  av_dict_set(&opts, "preset", "ultrafast", 0);
  av_dict_set(&opts, "tune", "zerolatency", 0);
  av_dict_set(&opts, "profile", "baseline", 0);
  av_dict_set(&opts, "x264opts",
              "no-mbtree:"
              "sync-lookahead=0:"
              "rc-lookahead=0:"
              "sliced-threads=1:"
              "no-scenecut",
              0);

  ret = avcodec_open2(enc->enc_ctx, encoder, &opts);
  av_dict_free(&opts);
  if (ret < 0) {
    fprintf(stderr, "Could not open encoder: %s\n", av_err2str(ret));
    return ret;
  }

  // Initialize buffer manager
  ret = init_buffer_manager(&enc->buffer_mgr);
  if (ret < 0) {
    fprintf(stderr, "Could not initialize buffer manager\n");
    return ret;
  }

  // Create output directory
  char dir_path[1024];
  snprintf(dir_path, sizeof(dir_path), "%s/%s", output_dir, preset->name);
  mkdir(dir_path, 0755);

  // Initialize output format context
  char output_path[1024];
  snprintf(output_path, sizeof(output_path), "%s/%s/stream.m3u8", output_dir,
           preset->name);

  ret = avformat_alloc_output_context2(&enc->fmt_ctx, NULL, "hls", output_path);
  if (ret < 0) {
    fprintf(stderr, "Could not create output context\n");
    return ret;
  }

  // Add video stream
  enc->stream = avformat_new_stream(enc->fmt_ctx, NULL);
  if (!enc->stream) {
    fprintf(stderr, "Could not create output stream\n");
    return AVERROR(ENOMEM);
  }

  ret = avcodec_parameters_from_context(enc->stream->codecpar, enc->enc_ctx);
  if (ret < 0) {
    fprintf(stderr, "Could not copy encoder parameters\n");
    return ret;
  }

  // Configure HLS
  av_opt_set(enc->fmt_ctx->priv_data, "hls_time", "1", 0);
  av_opt_set(enc->fmt_ctx->priv_data, "hls_list_size", "6", 0);
  av_opt_set(enc->fmt_ctx->priv_data, "hls_flags",
             "delete_segments+"
             "append_list+"
             "discont_start+"
             "split_by_time+"
             "program_date_time+"
             "independent_segments",
             0);
  av_opt_set(enc->fmt_ctx->priv_data, "hls_segment_type", "fmp4", 0);
  av_opt_set(enc->fmt_ctx->priv_data, "hls_fmp4_init_filename", "init.mp4", 0);

  // Set segment filenames
  char segment_path[1024];
  snprintf(segment_path, sizeof(segment_path), "%s/%s/segment_%%d.m4s",
           output_dir, preset->name);
  av_opt_set(enc->fmt_ctx->priv_data, "hls_segment_filename", segment_path, 0);

  char part_path[1024];
  snprintf(part_path, sizeof(part_path), "%s/%s/part_%%d.m4s", output_dir,
           preset->name);
  av_opt_set(enc->fmt_ctx->priv_data, "hls_part_filename", part_path, 0);

  // Set part duration
  char buf[32];
  snprintf(buf, sizeof(buf), "%f", PART_DURATION);
  av_opt_set(enc->fmt_ctx->priv_data, "hls_part_target", buf, 0);

  // Open output file
  ret = avio_open(&enc->fmt_ctx->pb, output_path, AVIO_FLAG_WRITE);
  if (ret < 0) {
    fprintf(stderr, "Could not open output file: %s\n", av_err2str(ret));
    return ret;
  }

  // Write format header
  ret = avformat_write_header(enc->fmt_ctx, NULL);
  if (ret < 0) {
    fprintf(stderr, "Could not write header: %s\n", av_err2str(ret));
    return ret;
  }

  // Initialize scaler
  enc->sws_ctx = sws_getContext(dec_ctx->width, dec_ctx->height,
                                dec_ctx->pix_fmt, enc->enc_ctx->width,
                                enc->enc_ctx->height, enc->enc_ctx->pix_fmt,
                                SWS_FAST_BILINEAR, NULL, NULL, NULL);

  if (!enc->sws_ctx) {
    fprintf(stderr, "Could not initialize scaler\n");
    return AVERROR(ENOMEM);
  }

  // Allocate frame
  enc->scaled_frame = av_frame_alloc();
  if (!enc->scaled_frame) {
    fprintf(stderr, "Could not allocate frame\n");
    return AVERROR(ENOMEM);
  }

  enc->scaled_frame->format = enc->enc_ctx->pix_fmt;
  enc->scaled_frame->width = enc->enc_ctx->width;
  enc->scaled_frame->height = enc->enc_ctx->height;

  ret = av_frame_get_buffer(enc->scaled_frame, 32);
  if (ret < 0) {
    fprintf(stderr, "Could not allocate frame buffers\n");
    return ret;
  }

  enc->preset = preset;
  enc->next_pts = 0;
  enc->first_pts = AV_NOPTS_VALUE;
  enc->frame_time = 1.0 / preset->fps;

  printf("Initialized %s encoder: %dx%d @ %d fps, %.2f Mbps\n", preset->name,
         preset->width, preset->height, preset->fps,
         preset->bitrate / 1000000.0);

  return 0;
}
