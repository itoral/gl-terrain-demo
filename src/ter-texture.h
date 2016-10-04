#ifndef __TER_TEXTURE_H__
#define __TER_TEXTURE_H__

#include <SDL_image.h>
#include <glib.h>

typedef struct {
  unsigned tid;
  unsigned vtid;
  unsigned width, height;
  char *file;
} TerTextureData;

typedef struct {
  unsigned tid;
  char *file;
  int width, height;
  SDL_Surface *image;
} TerTexture;

typedef struct {
  TerTexture *textures;
  unsigned capacity;
} TerTextureManager;

unsigned ter_texture_manager_load(TerTextureManager *manager,
                                  const char *file, unsigned vtid);
unsigned ter_texture_manager_load_cube(TerTextureManager *manager,
                                       const char *file[6], unsigned vtid);

TerTextureManager *ter_texture_manager_new(unsigned capacity);
void ter_texture_manager_free(TerTextureManager *manager);
void ter_texture_manager_free_nogl(TerTextureManager *manager);
unsigned ter_texture_manager_get_tid(TerTextureManager *manager, unsigned vtid);
SDL_Surface *ter_texture_manager_get_image(TerTextureManager *manager,
                                           unsigned vtid);
void ter_texture_manager_expand(TerTextureManager *manager, unsigned slots);
void ter_texture_manager_get(TerTextureManager *manager,
                             unsigned vtid, unsigned *tid,
                             unsigned *w, unsigned *h);

TerTextureData *ter_texture_manager_get_all(TerTextureManager *manager,
                                            unsigned *n);
void ter_texture_data_free(TerTextureData *gtd);

#endif
