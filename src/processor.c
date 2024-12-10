#include "../include/processor.h"
#include "../include/utils.h"
#include <libswscale/swscale.h>
int process_frame(TranscoderContext *ctx, AVFrame *frame) {
  // Frame timing check
  if (ctx->last_pts != AV_NOPTS_VALUE) {
    double elapsed =
        (frame->pts - ctx->last_pts) *
        av_q2d(ctx->input_ctx->streams[ctx->video_stream_index]->time_base);

    if (elapsed < ctx->frame_duration * 0.5) {
      if (DEBUG_MODE) {
        printf("Skipping frame: elapsed=%.3fms, required=%.3fms\n",
               elapsed * 1000, ctx->frame_duration * 1000);
      }
      return 0; // Skip this frame
    }
  }

  ctx->last_pts = frame->pts;

  int ret;
  for (int i = 0; i < MAX_QUALITY_LEVELS; i++) {
    EncoderContext *enc = &ctx->encoders[i];
    QualityPreset *preset = enc->preset;

    pthread_mutex_lock(&enc->buffer_mgr.mutex);

    if (enc->buffer_mgr.is_full) {
      pthread_mutex_unlock(&enc->buffer_mgr.mutex);
      preset->dropped_frames++;
      continue;
    }

    // Scale frame
    ret = sws_scale(enc->sws_ctx, (const uint8_t *const *)frame->data,
                    frame->linesize, 0, frame->height, enc->scaled_frame->data,
                    enc->scaled_frame->linesize);

    if (ret < 0) {
      pthread_mutex_unlock(&enc->buffer_mgr.mutex);
      preset->dropped_frames++;
      continue;
    }

    // Frame timing
    if (enc->first_pts == AV_NOPTS_VALUE) {
      enc->first_pts = frame->pts;
      enc->next_pts = 0;
    }

    // Calculate PTS in encoder timebase
    int64_t pts_diff = frame->pts - enc->first_pts;
    AVRational tb_mult =
        av_mul_q(ctx->input_ctx->streams[ctx->video_stream_index]->time_base,
                 av_inv_q(enc->enc_ctx->time_base));
    enc->scaled_frame->pts = av_rescale_q(
        pts_diff, ctx->input_ctx->streams[ctx->video_stream_index]->time_base,
        enc->enc_ctx->time_base);

    // Encode frame
    ret = avcodec_send_frame(enc->enc_ctx, enc->scaled_frame);
    if (ret < 0) {
      pthread_mutex_unlock(&enc->buffer_mgr.mutex);
      preset->dropped_frames++;
      continue;
    }

    while (ret >= 0) {
      AVPacket *packet = av_packet_alloc();
      ret = avcodec_receive_packet(enc->enc_ctx, packet);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        av_packet_free(&packet);
        break;
      }
      if (ret < 0) {
        av_packet_free(&packet);
        pthread_mutex_unlock(&enc->buffer_mgr.mutex);
        preset->dropped_frames++;
        continue;
      }

      // Set packet timing
      packet->stream_index = 0;
      av_packet_rescale_ts(packet, enc->enc_ctx->time_base,
                           enc->stream->time_base);

      // Debug timing
      if (DEBUG_MODE) {
        log_packet(enc->fmt_ctx, packet);
      }

      ret = av_interleaved_write_frame(enc->fmt_ctx, packet);
      av_packet_free(&packet);
      if (ret < 0) {
        pthread_mutex_unlock(&enc->buffer_mgr.mutex);
        preset->dropped_frames++;
        continue;
      }

      preset->total_frames++;
    }

    pthread_mutex_unlock(&enc->buffer_mgr.mutex);
  }

  return 0;
}
