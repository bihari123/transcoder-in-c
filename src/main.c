
// main.c
#include "../include/cleanup.h"
#include "../include/config.h"
#include "../include/decoder.h"
#include "../include/encoder.h"
#include "../include/monitor.h"
#include "../include/presets.h"
#include "../include/processor.h"
#include "../include/types.h"
#include "../include/utils.h"
#include <libavutil/time.h>
#include <signal.h>
#include <sys/stat.h>

static volatile int keep_running = 1;

static void signal_handler(int signum) {
  (void)signum;
  keep_running = 0;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <output_dir>\n", argv[0]);
    return 1;
  }

  signal(SIGINT, signal_handler);

  TranscoderContext ctx = {0};
  ctx.output_dir = argv[1];
  ctx.running = 1;
  ctx.last_pts = AV_NOPTS_VALUE;
  int ret;

  // Create output directory
  mkdir(ctx.output_dir, 0755);

  printf("Opening input...\n");
  if ((ret = open_input(&ctx)) < 0)
    goto end;

  printf("Initializing decoder...\n");
  if ((ret = init_decoder(&ctx)) < 0)
    goto end;

  // Initialize encoders
  for (int i = 0; i < MAX_QUALITY_LEVELS; i++) {
    printf("Initializing %s encoder...\n", QUALITY_PRESETS[i].name);
    if ((ret = init_encoder(&ctx.encoders[i], ctx.dec_ctx, &QUALITY_PRESETS[i],
                            ctx.output_dir)) < 0)
      goto end;
  }

  write_master_playlist(ctx.output_dir);

  ctx.frame = av_frame_alloc();
  ctx.packet = av_packet_alloc();
  if (!ctx.frame || !ctx.packet) {
    ret = AVERROR(ENOMEM);
    goto end;
  }

  // Start monitoring thread
  ctx.start_time = av_gettime();
  if (pthread_create(&ctx.monitor_thread, NULL, monitor_thread_func, &ctx) !=
      0) {
    fprintf(stderr, "Could not start monitor thread\n");
    ret = -1;
    goto end;
  }

  printf("\nTranscoding started\n");
  printf(
      "Play with: ffplay -fflags nobuffer -flags low_delay %s/master.m3u8\n\n",
      ctx.output_dir);

  // Main loop
  while (keep_running) {
    ret = av_read_frame(ctx.input_ctx, ctx.packet);
    if (ret < 0)
      break;

    if (ctx.packet->stream_index == ctx.video_stream_index) {
      ret = avcodec_send_packet(ctx.dec_ctx, ctx.packet);
      if (ret < 0)
        break;

      while (ret >= 0) {
        ret = avcodec_receive_frame(ctx.dec_ctx, ctx.frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
          break;
        if (ret < 0)
          goto end;

        ret = process_frame(&ctx, ctx.frame);
        if (ret < 0)
          goto end;
      }
    }

    av_packet_unref(ctx.packet);
  }

  printf("\nTranscoding finished\n");

end:
  cleanup(&ctx);
  return ret < 0 ? 1 : 0;
}
