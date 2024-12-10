// buffer.c
#include "../include/buffer.h"
#include "../include/config.h"
#include <libavutil/avutil.h>

int init_buffer_manager(BufferManager *mgr) {
  mgr->buffer = av_malloc(BUFFER_SIZE);
  if (!mgr->buffer) {
    return AVERROR(ENOMEM);
  }
  mgr->size = BUFFER_SIZE;
  mgr->write_pos = 0;
  mgr->read_pos = 0;
  mgr->is_full = 0;
  pthread_mutex_init(&mgr->mutex, NULL);
  return 0;
}

void free_buffer_manager(BufferManager *mgr) {
  if (mgr->buffer) {
    av_free(mgr->buffer);
    mgr->buffer = NULL;
  }
  pthread_mutex_destroy(&mgr->mutex);
}
