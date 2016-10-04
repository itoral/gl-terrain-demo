#ifndef __TER_OBJECT_RENDERER_H__
#define __TER_OBJECT_RENDERER_H__

#include <glib.h>

#include "ter-object.h"
#include "ter-util.h"

typedef struct {
   GHashTable *sets; /* Objects classified by model */
   GList *all;       /* All objects */
   GList *solid;     /* Solid objects (can_collide == true) */
} TerObjectRenderer;

TerObjectRenderer *ter_object_renderer_new();
void ter_object_renderer_free(TerObjectRenderer *r);

void ter_object_renderer_add_object(TerObjectRenderer *r, TerObject *o);
void ter_object_renderer_render_all(TerObjectRenderer *r, bool enable_shadows);
void ter_object_renderer_render_clipped(TerObjectRenderer *r,
                                        float clip_far_plane,
                                        float render_far_plane,
                                        bool enable_blending,
                                        bool enable_shadows,
                                        unsigned shadow_pfc,
                                        TerClipVolume *clip,
                                        const char *stage);

void ter_object_renderer_render_boxes(TerObjectRenderer *r);
GList *ter_object_renderer_get_all(TerObjectRenderer *r);
GList *ter_object_renderer_get_solid(TerObjectRenderer *r);

#endif
