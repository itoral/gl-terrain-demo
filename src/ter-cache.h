#ifndef __DRV_CACHE_H__
#define __DRV_CACHE_H__

void ter_cache_set(const char *key, void *ptr);
void *ter_cache_get(const char *key);
void ter_cache_clear();

#endif
