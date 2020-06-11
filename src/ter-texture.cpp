#include "ter-texture.h"

#include <glib.h>
#include <string.h>

#include <SDL_image.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

#define ENABLE_ANISOTROPY true
#define ANISOTROPY_VALUE 4.0f

static unsigned
create_texture(SDL_Surface *image)
{
   unsigned texture_id;

   glGenTextures(1, &texture_id);
   glBindTexture(GL_TEXTURE_2D, texture_id);

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->w, image->h, 0, GL_RGBA,
                GL_UNSIGNED_BYTE, image->pixels);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D,
                   GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -1.0f);

   glGenerateMipmap(GL_TEXTURE_2D);

   if (ENABLE_ANISOTROPY) {
     float max_anisotropy;
     glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);
     float value = (ANISOTROPY_VALUE > max_anisotropy) ?
      max_anisotropy : ANISOTROPY_VALUE;
     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, value);
   }

   return texture_id;
}

static unsigned
create_cube_texture(SDL_Surface *image[6])
{
   unsigned texture_id;

   glGenTextures(1, &texture_id);
   glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

   for (int i = 0; i < 6; i++) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA,
                   image[i]->w, image[i]->h, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, image[i]->pixels);
   }

   return texture_id;
}

/**
 * Loads a texure from a file and associates it with a given virtual
 * texture id. Returns the OpenGL texture ID associated to the
 * virtual texture id.
 */
unsigned
ter_texture_manager_load(TerTextureManager *manager,
                         const char *file, unsigned vtid)
{
  SDL_Surface *image = IMG_Load(file);
  if (!image) {
    g_warning ("Texture Manager: Failed to load file '%s'\n", file);
    return 0;
  }

  unsigned gl_texture_id = create_texture(image);
  manager->textures[vtid].tid = gl_texture_id;
  manager->textures[vtid].file = g_strdup(file);
  manager->textures[vtid].height = image->h;
  manager->textures[vtid].width = image->w;
  manager->textures[vtid].image = image;
  return gl_texture_id;
}

/**
 * Creates a new texture manager with a certain texture capacity
 */
TerTextureManager *
ter_texture_manager_new(unsigned capacity)
{
 if (capacity == 0)
    return NULL;

  TerTextureManager *manager = g_new0(TerTextureManager, 1);
  manager->textures = g_new0(TerTexture, capacity);
  manager->capacity = capacity;
  return manager;
}

/**
 * Free a texture manager object
 */
void
ter_texture_manager_free(TerTextureManager *manager)
{
  for (unsigned i = 0; i < manager->capacity; i++) {
    if (manager->textures[i].tid > 0) {
      glDeleteTextures(1, &manager->textures[i].tid);
      g_free(manager->textures[i].file);
    }
  }
  g_free(manager->textures);
  g_free(manager);
}

/**
 * Free a texture manager object but keep the GL textures
 */
void
ter_texture_manager_free_nogl(TerTextureManager *manager)
{
  for (unsigned i = 0; i < manager->capacity; i++) {
    if (manager->textures[i].tid > 0) {
      g_free(manager->textures[i].file);
    }
  }
  g_free(manager->textures);
  g_free(manager);
}

/**
 * Get the GL texture ID for a given virtual texture id
 */
unsigned
ter_texture_manager_get_tid(TerTextureManager *manager, unsigned vtid)
{
  if (vtid >= manager->capacity)
    return 0;

  return manager->textures[vtid].tid;
}

SDL_Surface *
ter_texture_manager_get_image(TerTextureManager *manager, unsigned vtid)
{
  if (vtid >= manager->capacity)
    return NULL;

  return manager->textures[vtid].image;
}

/**
 * Expands the maximum capacity of the texture manager
 */
void
ter_texture_manager_expand(TerTextureManager *manager, unsigned slots)
{
  TerTexture *tmp = manager->textures;
  manager->textures = g_new0(TerTexture, manager->capacity + slots);
  memcpy(manager->textures, tmp, manager->capacity * sizeof (TerTexture));
  manager->capacity += slots;
  g_free(tmp);
}

/**
 * Return tid and size of a given texture
 */
void
ter_texture_manager_get(TerTextureManager *manager,
                        unsigned vtid, unsigned *tid, unsigned *w, unsigned *h)
{
  if (vtid >= manager->capacity) {
    if (tid)
      *tid = 0;
    if (w)
      *w = 0;
    if (h)
      *h = 0;
  } else {
    if (tid)
      *tid = manager->textures[vtid].tid;
    if (w)
      *w = manager->textures[vtid].width;
    if (h)
      *h = manager->textures[vtid].height;
  }
}

/**
 * Return a list with the data of all loaded textures
 */
TerTextureData *
ter_texture_manager_get_all(TerTextureManager *manager, unsigned *n)
{
  TerTextureData *textures;
  unsigned i, count;

  /* Check how many textures we have loaded */
  for (count = 0, i = 0; i < manager->capacity; i++) {
    if (manager->textures[i].tid != 0)
      count++;
  }

  /* Allocate enough memory for all of them and copy */
  textures = g_new0(TerTextureData, count);
  for (count = 0, i = 0, count = 0; i < manager->capacity; i++) {
    if (manager->textures[i].tid != 0) {
      textures[count].vtid = i;
      textures[count].tid = manager->textures[i].tid;
      textures[count].file = manager->textures[i].file;
      textures[count].width = manager->textures[i].width;
      textures[count].height = manager->textures[i].height;
      count++;
    }
  }

  *n = count;
  return textures;
}

void
ter_texture_data_free(TerTextureData *gtd)
{
  g_free(gtd->file);
  g_free(gtd);
}

unsigned
ter_texture_manager_load_cube(TerTextureManager *manager,
                              const char *file[6], unsigned vtid)
{
  SDL_Surface *image[6];

   for (int i = 0; i < 6; i++) {
     image[i] = IMG_Load(file[i]);
     if (!image[i]) {
       g_warning ("Texture Manager: Failed to load file '%s'\n", file[i]);
       return 0;
     }
  }

  unsigned gl_texture_id = create_cube_texture(image);
  manager->textures[vtid].tid = gl_texture_id;
  manager->textures[vtid].file = g_strdup(file[0]);
  manager->textures[vtid].height = image[0]->h;
  manager->textures[vtid].width = image[0]->w;
  return gl_texture_id;
}
