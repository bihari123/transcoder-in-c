// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

char *ts_to_str(int64_t ts);
char *time_to_str(int64_t ts, AVRational *tb);
void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt);
void write_master_playlist(const char *output_dir);

#endif // UTILS_H
