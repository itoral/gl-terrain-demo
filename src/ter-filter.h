#ifndef __TER_FILTER_H__
#define __TER_FILTER_H__

#include "ter-render-texture.h"

typedef struct {
   TerRenderTexture *brightness_fbo;
   TerRenderTexture *hblur_fbo;
   TerRenderTexture *vblur_fbo;
   TerRenderTexture *result_fbo;
} TerBloomFilter;

TerBloomFilter *ter_bloom_filter_new();
TerRenderTexture *ter_bloom_filter_run(TerBloomFilter *bf,
                                       TerRenderTexture *src);
void ter_bloom_filter_free(TerBloomFilter *bf);

#endif
