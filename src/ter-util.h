#ifndef __DRV_UTIL_H__
#define __DRV_UTIL_H__

#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>

#define GLM_FORCE_RADIANS 1
#include <glm/glm.hpp>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

#include "main-constants.h"

#define PI M_PI
#define DEG_TO_RAD(x) ((float)((x) * PI / 180.0))
#define RAD_TO_DEG(x) ((float)((x) * 180.0 / PI))
#define RANGED_RANDOM_NEG(n) (random () % n - n / 2)
#define RANGED_RANDOM(n) (random () % n)

typedef struct {
   glm::vec3 ambient;
   glm::vec3 diffuse;
   glm::vec3 specular;
   float shininess;
   unsigned tid;
} TerMaterial;

typedef struct {
   float x0, x1, y0, y1, z0, z1;
} TerClipVolume;

enum {
   LOG_DEFAULT = 0,
   LOG_FPS,
   LOG_VBO,
   LOG_OBJ_LOAD,
   LOG_RENDER,
   LOG_SHADER,
   LOG_LAST
};

static void ter_dbg(unsigned channel, const char *s, ...) __attribute__ ((unused));
#if TER_DEBUG_TRACE_ENABLE
static void
ter_dbg(unsigned channel, const char *s, ...)
{
   static bool enables[] = {
      TER_DEBUG_TRACE_DEFAULT_ENABLE,
      TER_DEBUG_TRACE_FPS_ENABLE,
      TER_DEBUG_TRACE_VBO_ENABLE,
      TER_DEBUG_TRACE_OBJ_LOAD_ENABLE,
      TER_DEBUG_TRACE_RENDER_ENABLE,
      TER_DEBUG_TRACE_SHADER_ENABLE,
   };

   assert(channel < LOG_LAST);
   if (!enables[channel])
      return;

   if(s == NULL)
      return;

   char text[256];
   va_list ap;

   va_start(ap, s);
   vsprintf(text, s, ap);
   va_end(ap);

   printf("%s", text);
}
#else
static void inline
ter_dbg(unsigned channel, const char *, ...) {}
#endif

static inline glm::vec3
vec3(glm::vec4 v)
{
   return glm::vec3(v.x, v.y, v.z);
}

static inline glm::vec4
vec4(glm::vec3 v, float w)
{
   return glm::vec4(v.x, v.y, v.z, w);
}

static inline float
ter_util_vec3_module(glm::vec3 p, int Xaxis, int Yaxis, int Zaxis)
{
  return sqrtf(p.x * p.x * Xaxis + p.y * p.y * Yaxis + p.z * p.z * Zaxis);
}

static inline void
ter_util_vec3_normalize(glm::vec3 *p)
{
   float m = ter_util_vec3_module(*p, 1, 1, 1);
   if (m > 0) {
      p->x /= m;
      p->y /= m;
      p->z /= m;
   }
}

static inline glm::vec3
ter_util_vec3_normal(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
{
   glm::vec3 u1 = p2 - p1;
   glm::vec3 u2 = p3 - p1;
   glm::vec3 normal = glm::vec3(u2.y * u1.z - u2.z * u1.y,
                               -u2.x * u1.z + u2.z * u1.x,
                                u2.x * u1.y - u2.y * u1.x);
   return normal;
}

static inline glm::vec3
ter_util_vec3_product(glm::vec3 u, glm::vec3 v)
{
   glm::vec3 w;
   w.x = u.y * v.z - u.z * v.y;
   w.y = u.z * v.x - u.x * v.z;
   w.z = u.x * v.y - u.y * v.x;
   return w;
}

static inline float
ter_util_vec3_dot(glm::vec3 u, glm::vec3 v)
{
   return u.x * v.x + u.y * v.y + u.z * v.z;
}

#endif
