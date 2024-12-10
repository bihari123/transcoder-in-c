
// utils.c
#include "../include/utils.h"
#include "../include/config.h"
#include "../include/presets.h"
#include <stdio.h>

char *ts_to_str(int64_t ts) {
  static char str[AV_ERROR_MAX_STRING_SIZE];
  if (ts == AV_NOPTS_VALUE) {
    snprintf(str, sizeof(str), "NOPTS");
  } else {
    snprintf(str, sizeof(str), "%" PRId64 "", ts);
  }
  return str;
}

char *time_to_str(int64_t ts, AVRational *tb) {
  static char str[AV_ERROR_MAX_STRING_SIZE];
  if (ts == AV_NOPTS_VALUE) {
    snprintf(str, sizeof(str), "NOTIME");
  } else {
    double timestamp = ts * av_q2d(*tb);
    snprintf(str, sizeof(str), "%.6f", timestamp);
  }
  return str;
}

void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt) {
  if (!DEBUG_MODE)
    return;

  AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

  printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s "
         "stream_index:%d\n",
         ts_to_str(pkt->pts), time_to_str(pkt->pts, time_base),
         ts_to_str(pkt->dts), time_to_str(pkt->dts, time_base),
         ts_to_str(pkt->duration), time_to_str(pkt->duration, time_base),
         pkt->stream_index);
}

void write_master_playlist(const char *output_dir) {
  char master_path[1024];
  snprintf(master_path, sizeof(master_path), "%s/master.m3u8", output_dir);

  FILE *f = fopen(master_path, "w");
  if (!f)
    return;

  fprintf(f, "#EXTM3U\n");
  fprintf(f, "#EXT-X-VERSION:7\n");

  for (int i = 0; i < MAX_QUALITY_LEVELS; i++) {
    fprintf(f,
            "#EXT-X-STREAM-INF:BANDWIDTH=%d,RESOLUTION=%dx%d,FRAME-RATE=%d\n",
            QUALITY_PRESETS[i].bitrate, QUALITY_PRESETS[i].width,
            QUALITY_PRESETS[i].height, QUALITY_PRESETS[i].fps);
    fprintf(f, "%s/stream.m3u8\n", QUALITY_PRESETS[i].name);
  }

  fclose(f);
}
