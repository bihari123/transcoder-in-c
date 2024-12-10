#include "../include/cleanup.h"
#include "../include/buffer.h"
#include "../include/monitor.h"
#include <libswscale/swscale.h>
void cleanup(TranscoderContext *ctx) {
  ctx->running = 0;
  pthread_join(ctx->monitor_thread, NULL);

  print_stats(ctx);

  for (int i = 0; i < MAX_QUALITY_LEVELS; i++) {
    EncoderContext *enc = &ctx->encoders[i];

    // Flush encoders
    avcodec_send_frame(enc->enc_ctx, NULL);
    AVPacket *packet = av_packet_alloc();
    while (avcodec_receive_packet(enc->enc_ctx, packet) >= 0) {
      av_packet_unref(packet);
    }
    av_packet_free(&packet);

    if (enc->scaled_frame)
      av_frame_free(&enc->scaled_frame);
    if (enc->sws_ctx)
      sws_freeContext(enc->sws_ctx);
    if (enc->enc_ctx)
      avcodec_free_context(&enc->enc_ctx);
    if (enc->fmt_ctx) {
      av_write_trailer(enc->fmt_ctx);
      if (enc->fmt_ctx->pb)
        avio_closep(&enc->fmt_ctx->pb);
      avformat_free_context(enc->fmt_ctx);
    }
    free_buffer_manager(&enc->buffer_mgr);
  }

  if (ctx->frame)
    av_frame_free(&ctx->frame);
  if (ctx->packet)
    av_packet_free(&ctx->packet);
  if (ctx->dec_ctx)
    avcodec_free_context(&ctx->dec_ctx);
  if (ctx->input_ctx)
    avformat_close_input(&ctx->input_ctx);
}
