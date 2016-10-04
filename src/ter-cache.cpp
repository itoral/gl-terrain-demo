#include "ter-cache.h"

#include <glib.h>

static GHashTable *_ter_cache =
   g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

void
ter_cache_set(const char *key, void *ptr)
{
   g_hash_table_insert(_ter_cache, g_strdup(key), ptr);
}

void *
ter_cache_get(const char *key)
{
   return g_hash_table_lookup(_ter_cache, key);
}

void
ter_cache_clear()
{
   g_hash_table_destroy(_ter_cache);
}

