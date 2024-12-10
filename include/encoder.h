// encoder.h
#ifndef ENCODER_H
#define ENCODER_H

#include "types.h"

int init_encoder(EncoderContext *enc, AVCodecContext *dec_ctx,
                 QualityPreset *preset, const char *output_dir);

#endif // ENCODER_H
