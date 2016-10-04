#ifndef __TER_MAIN_H__
#define __TER_MAIN_H__

#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <string.h>

#include <glib.h>
#include <math.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GLFW/glfw3.h>

#include <SDL.h>

#define GLM_FORCE_RADIANS 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ter-texture.h"
#include "ter-util.h"
#include "ter-camera.h"
#include "ter-cache.h"
#include "ter-light.h"
#include "ter-shader-program.h"
#include "ter-terrain.h"
#include "ter-sky-box.h"
#include "ter-model.h"
#include "ter-object.h"
#include "ter-object-catalog.h"
#include "ter-object-renderer.h"
#include "ter-render-texture.h"
#include "ter-tile.h"
#include "ter-water-tile.h"
#include "ter-shadow-map.h"
#include "ter-shadow-box.h"
#include "ter-shadow-renderer.h"

#include "main-constants.h"

#endif
