// monitor.c
#include "../include/monitor.h"
#include "../include/presets.h"
#include <libavutil/time.h>
#include <unistd.h>
void print_stats(TranscoderContext *ctx) {
  double elapsed_time = (av_gettime() - ctx->start_time) / 1000000.0;
  printf("\rRunning time: %.2f seconds\n", elapsed_time);

  for (int i = 0; i < MAX_QUALITY_LEVELS; i++) {
    QualityPreset *preset = &QUALITY_PRESETS[i];
    double fps = preset->total_frames / elapsed_time;
    double drop_rate =
        preset->total_frames > 0
            ? (double)preset->dropped_frames / preset->total_frames * 100
            : 0;

    printf("%s: %d frames, %d dropped (%.2f%%) - %.2f fps\n", preset->name,
           preset->total_frames, preset->dropped_frames, drop_rate, fps);
  }
  printf("\n");
}

void *monitor_thread_func(void *arg) {
  TranscoderContext *ctx = (TranscoderContext *)arg;
  while (ctx->running) {
    print_stats(ctx);
    sleep(MONITORING_INTERVAL);
  }
  return NULL;
}
