#include "main.h"

/* ====================== Globals ======================== */

/* Target window */
GLFWwindow *window;

/* FPS tracking */
unsigned fps_frames = 0;
double fps_frame_start = 0.0;
double fps_frame_end = 0.0;
double fps_total_time = 0.0;
double fps_total_run_time = 0.0;
unsigned fps_total_run_frames = 0;
double fps_last_frame_time = 1.0 / 60.0;
double fps_slowest_frame_time = 0.0f;
double fps_fastest_frame_time = 1000000.0f;
unsigned fps_skip_frame = 15; /* Skip first N frames for stats */

/* Projection and View matrices */
glm::mat4 Projection, ProjectionSky, View, ViewInv, ProjectionOrtho;

/* Terrain */
TerTerrain *terrain = NULL;

/* Skybox */
TerSkyBox *skybox = NULL;

/* Water */
TerWaterTile *water = NULL;

/* Shadow renderer */
TerShadowRenderer *shadow_renderer = NULL;

/* Object renderer */
TerObjectRenderer *obj_renderer = NULL;

/* Global texture manager */
TerTextureManager *tex_mgr = NULL;

/* List of shaders */
GList *shader_list = NULL;

/* Scene FBO */
TerRenderTexture *scene_ms_fbo = NULL;
TerRenderTexture *scene_fbo = NULL;

/* Bloom filter */
TerBloomFilter *bloom_filter = NULL;

/* Objects to load. For new object types add:
 *
 * - the enum value in TerObjectType
 * - the number of instances in object_count[]
 * - the scale constructor in obj_constructors[]
 * - the OBJ file path in obj_model_list[]
 * - Add to allow_underwater_object() if needed
 * - Add to allow_rotated_object() if needed
 */
typedef TerObject * (*OBJ_CONSTRUCTOR)(float x, float y, float z, float s);
typedef enum {
   TER_OBJECT_TYPE_TREE1 = 0,
   TER_OBJECT_TYPE_TREE2,
   TER_OBJECT_TYPE_TREE3,
   TER_OBJECT_TYPE_ROCK1,
   TER_OBJECT_TYPE_ROCK2,
   TER_OBJECT_TYPE_GRASS1,
   TER_OBJECT_TYPE_GRASS2,
   TER_OBJECT_TYPE_LAST,
} TerObjectType;

unsigned object_count[TER_OBJECT_TYPE_LAST] = {
  125, 125, 30,   // Trees
  25, 25,         // Rocks
  125, 125,       // Grass
};

OBJ_CONSTRUCTOR obj_constructors[TER_OBJECT_TYPE_LAST] = {
   ter_object_catalog_new_tree1_scale,
   ter_object_catalog_new_tree2_scale,
   ter_object_catalog_new_tree3_scale,
   ter_object_catalog_new_rock1_scale,
   ter_object_catalog_new_rock2_scale,
   ter_object_catalog_new_grass1_scale,
   ter_object_catalog_new_grass2_scale,
};

typedef struct {
   const char *path;
   const char *key;
} TerModelLoadItem;

TerModelLoadItem obj_model_list[TER_OBJECT_TYPE_LAST] = {
   { "../models/tree.obj",   "model/tree-01" },
   { "../models/tree2.obj",  "model/tree-02" },
   { "../models/tree3.obj",  "model/tree-03" },
   { "../models/rock.obj",   "model/rock-01" },
   { "../models/rock2.obj",  "model/rock-02" },
   { "../models/grass.obj",  "model/grass-01" },
   { "../models/grass2.obj", "model/grass-02" },
};

/**
 * Init GLFW and setup an OpenGL 3.3 context
 */
static void
setup_glfw()
{
   if (!glfwInit()) {
      printf("ERROR: could not initialize GLFW\n");
      exit(1);
   }

   glfwWindowHint(GLFW_SAMPLES, 0);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_REFRESH_RATE, TER_WIN_FULLSCREEN_TARGET_FPS);

   window = glfwCreateWindow(TER_WIN_WIDTH, TER_WIN_HEIGHT, "Terrain Demo",
                             TER_WIN_FULLSCREEN_ENABLE ?
                             glfwGetPrimaryMonitor() : NULL,
                             NULL);
   if (window == NULL) {
      printf("ERROR: failed to create GLFW window\n");
      glfwTerminate();
      exit(1);
   }

   glfwMakeContextCurrent(window);
   glfwSwapInterval(TER_WIN_SWAP_INTERVAL);
}

static void
create_model_variants()
{
   TerMaterial variant_mat[TER_MODEL_MAX_MATERIALS];
   unsigned variant_tids[TER_MODEL_MAX_TEXTURES];

   TerModel *model = (TerModel *) ter_cache_get("model/tree-01");

   variant_mat[0] = model->materials[0];
   variant_mat[1] = model->materials[1];
   variant_mat[1].diffuse = glm::vec3(0.6f, 0.2f, 0.1f);
   variant_mat[1].specular = glm::vec3(0.1f, 0.1f, 0.1f);
   ter_model_add_variant(model, variant_mat, variant_tids, 2);

   variant_mat[0] = model->materials[0];
   variant_mat[1] = model->materials[1];
   variant_mat[1].diffuse = glm::vec3(0.4f, 0.4f, 0.1f);
   variant_mat[1].specular = glm::vec3(0.1f, 0.1f, 0.1f);
   ter_model_add_variant(model, variant_mat, variant_tids, 2);

   model = (TerModel *) ter_cache_get("model/tree-02");

   variant_mat[1] = model->materials[1];
   variant_mat[0] = model->materials[0];
   variant_mat[0].diffuse = glm::vec3(0.5f, 0.1f, 0.4f);
   variant_mat[0].specular = glm::vec3(0.1f, 0.1f, 0.1f);
   ter_model_add_variant(model, variant_mat, variant_tids, 2);

   variant_mat[1] = model->materials[1];
   variant_mat[0] = model->materials[0];
   variant_mat[0].diffuse = glm::vec3(0.1f, 0.4f, 0.6f);
   variant_mat[0].specular = glm::vec3(0.1f, 0.1f, 0.1f);
   ter_model_add_variant(model, variant_mat, variant_tids, 2);

   model = (TerModel *) ter_cache_get("model/tree-03");

   variant_mat[1] = model->materials[1];
   variant_mat[0] = model->materials[0];
   variant_mat[0].diffuse = glm::vec3(0.64f, 0.64f, 0.1f);
   variant_mat[0].specular = glm::vec3(0.1f, 0.1f, 0.1f);
   ter_model_add_variant(model, variant_mat, variant_tids, 2);
}

static void
load_models()
{
   /* Terrain */
   terrain =
      ter_terrain_new(TER_TERRAIN_VX, TER_TERRAIN_VZ, TER_TERRAIN_TILE_SIZE);
   ter_terrain_set_heights_from_texture(terrain, TER_TEX_TERRAIN_HEIGHTMAP_01,
                                        0.0f, 12.0f);
   ter_terrain_build_mesh(terrain);
   terrain->material.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
   terrain->material.ambient = glm::vec3(1.0f, 1.0f, 1.0f);
   terrain->material.specular = glm::vec3(0.15f, 0.15f, 0.15f);
   terrain->material.shininess = 28.0f;

   float tw = ter_terrain_get_width(terrain);
   float td = ter_terrain_get_depth(terrain);

   ter_cache_set("models/terrain", terrain);

   /* Water */
   TerTextureManager *texmgr =
      (TerTextureManager *) ter_cache_get("textures/manager");
   unsigned water_dudv_tex =
      ter_texture_manager_get_tid(texmgr, TER_TEX_WATER_DUDV_01);
   unsigned water_normal_tex =
      ter_texture_manager_get_tid(texmgr, TER_TEX_WATER_NORMAL_01);
   water = ter_water_tile_new(2.5f, -2.5f, tw + 2.5f, -td - 2.5f,
                              TER_TERRAIN_WATER_HEIGHT,
                              TER_TERRAIN_WATER_TILE_SIZE,
                              water_dudv_tex, water_normal_tex);

   ter_cache_set("water/water-tile-01", water);

   /* OBJ models */
   for (int i = 0; i < TER_OBJECT_TYPE_LAST; i++) {
      TerModel *model = ter_model_load_obj(obj_model_list[i].path);
      ter_cache_set(obj_model_list[i].key, model);
   }

   create_model_variants();
}

static void
free_obj_models()
{
   for (int i = 0; i < TER_OBJECT_TYPE_LAST; i++) {
      TerModel *m = (TerModel *) ter_cache_get(obj_model_list[i].key);
      ter_model_free(m);
   }
}

static void
load_skybox()
{
   skybox = ter_skybox_new(2000.0f, TER_TEX_SKY_BOX_01);
   if (TER_DYNAMIC_LIGHT_ENABLE)
      skybox->speed = TER_DYNAMIC_LIGHT_SPEED;
   ter_cache_set("skybox/skybox-01", skybox);
}

static void
legalize_object_position(float *x, float *y, float *z)
{
   if (*x < 1.0f)
      *x = 1.0f;
   else if (*x >= ter_terrain_get_width(terrain))
      *x = ter_terrain_get_width(terrain) - 1.0f;

   if (*z > -1.0f)
      *z = -1.0f;
   else if (*z <= -ter_terrain_get_depth(terrain))
      *z = ter_terrain_get_depth(terrain) + 1.0f;
}

static bool
allow_underwater_object(int t)
{
   assert(t >= 0 && t < TER_OBJECT_TYPE_LAST);

   switch (t) {
   case TER_OBJECT_TYPE_ROCK1:
   case TER_OBJECT_TYPE_ROCK2:
      return true;
   default:
      return false;
   }
}

static bool
allow_rotated_object(int t)
{
   assert(t >= 0 && t < TER_OBJECT_TYPE_LAST);

   switch (t) {
   case TER_OBJECT_TYPE_ROCK1:
   case TER_OBJECT_TYPE_ROCK2:
   case TER_OBJECT_TYPE_GRASS1:
   case TER_OBJECT_TYPE_GRASS2:
      return true;
   default:
      return false;
   }
}

static void
load_objects()
{
   unsigned num_objects = 0;

   obj_renderer = ter_object_renderer_new();
   ter_cache_set("rendering/obj-renderer", obj_renderer);

   int max_x = (TER_TERRAIN_VX - 1) * TER_TERRAIN_TILE_SIZE;
   int max_z = (TER_TERRAIN_VZ - 1) * TER_TERRAIN_TILE_SIZE;

   float offset_y = -0.05f;

   for (int i = 0; i < TER_OBJECT_TYPE_LAST; i++) {
      OBJ_CONSTRUCTOR constructor = obj_constructors[i];
      for (unsigned j = 0; j < object_count[i]; j++) {
         float x = random() % max_x;
         float z = -(random() % max_z);
         float y = ter_terrain_get_height_at(terrain, x, z);
         legalize_object_position(&x, &y, &z);

         /* We only allow certain objects to be submerged */
         if (y < TER_TERRAIN_WATER_HEIGHT - 0.25f &&
             !allow_underwater_object(i)) {
            j--; /* Try again */
            continue;
         }

         float s = 1.0f + ((float)(random() % 100)) / 150.0f;
         TerObject *o = constructor(x, y + offset_y, z, s);

         if (allow_rotated_object(i))
            o->rot.y = ((float)(random() % 360));

         if (o->model->num_variants > 1)
            o->variant = random() % o->model->num_variants;

         ter_object_update_box(o);

         /* Make sure we don't place objects in places where they collide
          * with other existing objects
          */
         GList *objects = ter_object_renderer_get_solid(obj_renderer);
         GList *iter = objects;
         while (iter) {
            TerObject *o2 = (TerObject *) iter->data;
            if (ter_object_collision(o2, &o->box))
               break;
            iter = g_list_next(iter);
         }
         if (iter != NULL) {
            j--; /* Try again*/
            continue;
         }

         ter_object_renderer_add_object(obj_renderer, o);
         num_objects++;
      }
   }

   ter_dbg(LOG_DEFAULT, "MAIN: INFO: Loaded %d objects\n", num_objects);
}

static void
add_shader(const char *key, void *sh)
{
   ter_cache_set(key, sh);
   shader_list = g_list_prepend(shader_list, sh);
}

static void
load_shaders()
{
   void *sh;

   /* Terrain */
   sh = ter_shader_program_terrain_new();
   add_shader("program/terrain", sh);
   sh = ter_shader_program_terrain_shadow_new();
   add_shader("program/terrain-shadow", sh);

   /* Skybox */
   sh = ter_shader_program_skybox_new();
   add_shader("program/skybox", sh);

   /* Model solid */
   sh = ter_shader_program_model_solid_new();
   add_shader("program/model-solid", sh);
   sh = ter_shader_program_model_solid_shadow_new();
   add_shader("program/model-solid-shadow", sh);

   /* Model texture */
   sh = ter_shader_program_model_tex_new();
   add_shader("program/model-tex", sh);
   sh = ter_shader_program_model_tex_shadow_new();
   add_shader("program/model-tex-shadow", sh);

   /* 2D Tiles */
   sh = ter_shader_program_tile_new();
   add_shader("program/tile", sh);

   /* Water */
   sh = ter_shader_program_water_new();
   add_shader("program/water", sh);

   /* Shadow Map */
   sh = ter_shader_program_shadow_map_new();
   add_shader("program/shadow-map", sh);
   sh = ter_shader_program_shadow_map_instanced_new();
   add_shader("program/shadow-map-instanced", sh);

   /* Bounding box */
   sh = ter_shader_program_box_new();
   add_shader("program/box", sh);

   /* Bloom - brightness */
   sh = ter_shader_program_filter_brightness_select_new(
      "../shaders/bloom-brightness.vert",
      "../shaders/bloom-brightness.frag");
   add_shader("program/bloom-brightness", sh);

   /* Bloom - horizontal blur */
   sh = ter_shader_program_filter_blur_new("../shaders/bloom-hblur.vert",
                                           "../shaders/bloom-blur.frag");
   add_shader("program/bloom-hblur", sh);

   /* Bloom - vertical blur */
   sh = ter_shader_program_filter_blur_new("../shaders/bloom-vblur.vert",
                                           "../shaders/bloom-blur.frag");
   add_shader("program/bloom-vblur", sh);

   /* Bloom - combine */
   sh = ter_shader_program_filter_combine_new("../shaders/bloom-combine.vert",
                                              "../shaders/bloom-combine.frag");
   add_shader("program/bloom-combine", sh);
}

static void
free_shaders()
{
   g_list_free_full(shader_list, (GDestroyNotify) ter_shader_program_free);
}

static void
load_textures()
{
   tex_mgr = ter_texture_manager_new(256);
   ter_cache_set("textures/manager", tex_mgr);

   ter_texture_manager_load(tex_mgr,
                            "../textures/terrain-heightmap-01.png",
                            TER_TEX_TERRAIN_HEIGHTMAP_01);

   ter_texture_manager_load(tex_mgr,
                            "../textures/terrain-surface-01.png",
                            TER_TEX_TERRAIN_SURFACE_01);

   ter_texture_manager_load(tex_mgr,
                            "../textures/water-dudv-01.png",
                            TER_TEX_WATER_DUDV_01);

   ter_texture_manager_load(tex_mgr,
                            "../textures/water-normal-01.png",
                            TER_TEX_WATER_NORMAL_01);

   const char *sky_box_files[6] = {
      "../textures/sky-right.png",
      "../textures/sky-left.png",
      "../textures/sky-top.png",
      "../textures/sky-bottom.png",
      "../textures/sky-back.png",
      "../textures/sky-front.png",
   };
   ter_texture_manager_load_cube(tex_mgr, sky_box_files, TER_TEX_SKY_BOX_01);
}

static void
load_lights()
{
   /* Directional light */
   glm::vec3 dir = glm::vec3(100.0f, 80.0f, -100.0f); /* Vector to light source */
   glm::vec3 diffuse(0.65f, 0.40f, 0.1f);
   glm::vec3 ambient(0.15f, 0.15f, 0.15f);
   glm::vec3 specular(1.0f, 0.85f, 0.7f);
   TerLight *light = ter_light_new_directional(dir, diffuse, ambient, specular);
   light->rot_speed = glm::vec3(0.0f, skybox->speed, 0.0f);
   ter_cache_set("light/light0", light);
}

static void
free_lights()
{
   TerLight *light = (TerLight *) ter_cache_get("light/light0");
   ter_light_free(light);
}

static void
setup_gl()
{
   glEnable(GL_CULL_FACE);

   glEnable(GL_DEPTH_TEST);
   glClearDepth(1.0);
   glDepthFunc(GL_LESS);
   glClear(GL_DEPTH_BUFFER_BIT);

   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT);
}

/**
 * Loads the GL scene and configures the GL pipeline
 */
static void
setup_scene()
{
   setup_gl();

   /* Load resources */
   load_shaders();
   load_textures();
   load_models();
   load_objects();
   load_skybox();
   load_lights();

   /* Multi-sampled scene FBO */
   if (TER_MULTISAMPLING_SAMPLES > 1) {
      scene_ms_fbo =
         ter_render_texture_new(TER_WIN_WIDTH, TER_WIN_HEIGHT,
                                true, true, false, true);
   }

   /* Single-sampled scene FBO */
   scene_fbo =
      ter_render_texture_new(TER_WIN_WIDTH, TER_WIN_HEIGHT,
                             true, scene_ms_fbo ? false : true, false, false);

   /* Bloom filter */
   if (TER_BLOOM_FILTER_ENABLE)
      bloom_filter = ter_bloom_filter_new();

   /* Projection matrix */
   Projection = glm::perspective(DEG_TO_RAD(TER_FOV), TER_ASPECT_RATIO,
                                 TER_NEAR_PLANE, TER_FAR_PLANE);
   ter_cache_set("matrix/Projection", &Projection);

   ProjectionSky = glm::perspective(DEG_TO_RAD(TER_FOV), TER_ASPECT_RATIO,
                                    TER_NEAR_PLANE, TER_FAR_PLANE_SKY);
   ter_cache_set("matrix/ProjectionSky", &ProjectionSky);

   ProjectionOrtho = glm::ortho(0.0f, (float)TER_WIN_WIDTH,
                                0.0f, (float)TER_WIN_HEIGHT);
   ter_cache_set("matrix/ProjectionOrtho", &ProjectionOrtho);

   TerCamera *camera = ter_camera_new(53.0, 3.0, -37.0,   // Position
                                      0.0, 15.0, 0.0);  // Direction
   ter_cache_set("camera/main", camera);

   /* Shadow renderer */
   TerLight *light = (TerLight *) ter_cache_get("light/light0");
   shadow_renderer = ter_shadow_renderer_new(light, camera);
   ter_cache_set("rendering/shadow-renderer", shadow_renderer);

   /* 2D Tiles */
   float tw = TER_WIN_WIDTH / 3.0f;
   float th = tw / TER_ASPECT_RATIO;
   TerTile *tile;
   tile = ter_tile_new(tw , th, 0.0f, TER_WIN_HEIGHT - th,
                       water->reflection->texture);
   ter_cache_set("tile/tile-water-reflection", tile);
   tile = ter_tile_new(tw, th, tw, TER_WIN_HEIGHT - th,
                       water->refraction->texture);
   ter_cache_set("tile/tile-water-refraction", tile);
   tile = ter_tile_new(tw, th, 2 * tw, TER_WIN_HEIGHT - th,
                       shadow_renderer->shadow_map->map->depth_texture);
   ter_cache_set("tile/tile-shadow-map", tile);
}

static void
update_terrain_index_buffer()
{
   TerCamera *cam = (TerCamera *) ter_cache_get("camera/main");

   /* Compute the maximum distance we would like to consider depending
    * on whether we are looking towards -Z/+Z or -X/+X
    *
    * With that distance, compute a camera clipping cuboid and use that
    * to clip the terrain.
    */

   /* Normalize the angle to 0..360 */
   int angle_y = (((int) roundf(cam->rot.y) % 360));
   if (angle_y < 0)
      angle_y += 360;

   float ref_z;
   if (angle_y <= 90 || angle_y >= (360 - 90)) /* towards -Z */
      ref_z = terrain->depth * terrain->step;
   else
      ref_z = 0.0f;
   float far_dist_z = fabsf(ref_z - (-cam->pos.z));

   float ref_x;
   if (angle_y >= 180) /* Towards +X */
      ref_x = terrain->width * terrain->step;
   else
      ref_x = 0.0f;
   float far_dist_x = fabsf(ref_x - cam->pos.x);

   float far_dist = MAX(far_dist_x, far_dist_z);

   TerClipVolume clip;
   ter_camera_get_clipping_box_for_distance(cam, far_dist, &clip);

   ter_terrain_update_index_buffer_for_clip_volume(terrain, &clip);
}

static void
render_water_refraction()
{
   /* We only want to render vertices below the water level */
   float Y_clip_height = water->h + TER_WATER_REFRACTION_CLIPPING_OFFSET;
   glm::vec4 clip_plane = glm::vec4(0.0f, -1.0f, 0.0f, Y_clip_height);
   ter_cache_set("clip/clip-plane-0", &clip_plane);

   /* Render to the refraction texture */
   ter_render_texture_start(water->refraction);
      /* Skip color clearing, we are only going to texture from the part
       * of the color buffer we render
       */
      glClear(GL_DEPTH_BUFFER_BIT |
              (TER_DEBUG_SHOW_WATER_TILES ? GL_COLOR_BUFFER_BIT : 0));
      glEnable(GL_CLIP_DISTANCE0);

      /* We don't want to render stuff outside the viewing frustum or objects
       * above the water level
       */
      TerClipVolume clip;
      TerCamera *cam = (TerCamera *) ter_cache_get("camera/main");
      ter_camera_get_clipping_box_for_distance(
         cam, TER_WATER_REFRACTION_CLIPPING_DISTANCE, &clip);

      if (clip.y0 < Y_clip_height) {
         if (clip.y1 >= Y_clip_height)
            clip. y1 = Y_clip_height;
         /* Refracted objects also write to the depth texture. This means that
          * water depth around them will be small which will make it
          * transparent and not distorted when rendered by the water shader.
          * The good thing about this is that reflection/refraction around
          * partially underwater objects does not present artifacts. The bad
          * thing is that because the water is trasparent we see the underwater
          * object instead of the refraction texture and when the object
          * is clipped from the refraction texture we can clearly see the under
          * water section of the object pop out of view.
          */
         if (!TER_WATER_REFRACTION_RECORD_OBJECT_DEPTH)
            glDisable(GL_DEPTH_TEST);

         /* Because the refraction texture includes depth information we
          * render it with TER_FAR_PLANE so that the water shader can use it.
          */
         ter_object_renderer_render_clipped(obj_renderer,
            TER_WATER_REFRACTION_CLIPPING_DISTANCE, TER_FAR_PLANE,
            true,
            TER_WATER_REFRACTION_SHADOWS_ENABLE, TER_SHADOW_PFC_WATER,
            &clip,
            "water refraction");

         if (!TER_WATER_REFRACTION_RECORD_OBJECT_DEPTH)
            glEnable(GL_DEPTH_TEST);
      }

      ter_terrain_render(terrain, TER_WATER_REFRACTION_SHADOWS_ENABLE);

      glDisable(GL_CLIP_DISTANCE0);
   ter_render_texture_stop(water->refraction);
}

static void
render_water_reflection()
{
   /* Adjust the camera to capture reflection image */
   TerCamera *cam = (TerCamera *) ter_cache_get("camera/main");
   float height = cam->pos.y - water->h;
   cam->pos.y -= 2 * height;
   cam->rot.x = -cam->rot.x;
   View = ter_camera_get_view_matrix(cam);
   ViewInv = glm::inverse(View);
   ter_cache_set("matrix/View", &View);
   ter_cache_set("matrix/ViewInv", &ViewInv);

   /* We only want to render vertices above the water level */
   float Y_clip_height = water->h - TER_WATER_REFLECTION_CLIPPING_OFFSET;
   glm::vec4 clip_plane = glm::vec4(0.0f, 1.0f, 0.0f, -Y_clip_height);
   ter_cache_set("clip/clip-plane-0", &clip_plane);

   /* Render to the reflection texture */
   ter_render_texture_start(water->reflection);
      /* Skip color clearing, we are only going to texture from the part
       * of the color buffer we render.
       */
      glClear(GL_DEPTH_BUFFER_BIT);

      /* We want to render the skybox first, so that blended object models
       * fade against it
       */
      ter_skybox_render(skybox);

      glEnable(GL_CLIP_DISTANCE0);

      /* We don't want to render stuff outside the viewing frustum or objects
       * under the water level
       */
      TerClipVolume clip;
      ter_camera_get_clipping_box_for_distance(
         cam, TER_WATER_REFLECTION_CLIPPING_DISTANCE, &clip);
      if (clip.y1 >= Y_clip_height) {
         if (clip.y0 < Y_clip_height)
            clip. y0 = Y_clip_height;

         ter_object_renderer_render_clipped(obj_renderer,
            TER_WATER_REFLECTION_CLIPPING_DISTANCE,
            TER_WATER_REFLECTION_CLIPPING_DISTANCE,
            true,
            TER_WATER_REFLECTION_SHADOWS_ENABLE,
            TER_SHADOW_PFC_WATER,
            &clip,
            "water reflection");
      }

      ter_terrain_render(terrain, TER_WATER_REFLECTION_SHADOWS_ENABLE);

      glDisable(GL_CLIP_DISTANCE0);

   ter_render_texture_stop(water->reflection);

   /* Put camera back to its original state */
   cam->pos.y += 2 * height;
   cam->rot.x = -cam->rot.x;
   View = ter_camera_get_view_matrix(cam);
   ViewInv = glm::inverse(View);
   ter_cache_set("matrix/View", &View);
   ter_cache_set("matrix/ViewInv", &ViewInv);
}

static void
render_water_textures()
{
   TerCamera *cam = (TerCamera *) ter_cache_get("camera/main");

   /* Only update the refraction texture if we dirty the camera. This
    * has some visible effect since lighting parameters might have changed
    * with dynamic lighting and affect the brightness of the refraction, but it
    * is barely noticeable. Saves some FPS when the camera is static.
    *
    * FIXME: If lighting parameters change drastically, we might want to
    * update the texture too.
    */
   if (cam->dirty)
      render_water_refraction();

   render_water_reflection();
}

static void
render_objects(bool enable_shadows)
{
   TerClipVolume clip;
   TerCamera *cam = (TerCamera *) ter_cache_get("camera/main");
   ter_camera_get_clipping_box_for_distance(
      cam, TER_FAR_PLANE, &clip);
   ter_object_renderer_render_clipped(obj_renderer,
      TER_FAR_PLANE, TER_FAR_PLANE,
      true,
      enable_shadows, TER_SHADOW_PFC,
      &clip,
      "scene objects");
}

static void
render_bounding_boxes()
{
   ter_object_renderer_render_boxes(obj_renderer);
}

static inline void
render_water_texture_tiles()
{
   TerTile *tile;
   tile = (TerTile *) ter_cache_get("tile/tile-water-refraction");
   ter_tile_render(tile);
   tile = (TerTile *) ter_cache_get("tile/tile-water-reflection");
   ter_tile_render(tile);
}

static inline void
render_shadow_map_tile()
{
   TerTile *tile = (TerTile *) ter_cache_get("tile/tile-shadow-map");
   ter_tile_render(tile);
}

static void
render_shadow_map()
{
   static bool shadow_map_rendered = false;
   static int shadow_map_age = 0;

   /* If we have dynamic lighting enabled then we want to update the shadow
    * map once every certain number of frames. If we have static lighting
    * we only want to render the shadow map once
    */
   if (TER_DYNAMIC_LIGHT_ENABLE || !shadow_map_rendered) {
      if (!shadow_map_rendered ||
          shadow_map_age == TER_SHADOW_UPDATE_INTERVAL) {
         shadow_map_rendered = ter_shadow_renderer_render(shadow_renderer);
         shadow_map_age = 0;
      } else {
         shadow_map_age++;
      }
   }
}

static inline void
render_2d_tiles()
{
   glDisable(GL_DEPTH_TEST);

   if (TER_DEBUG_SHOW_WATER_TILES)
      render_water_texture_tiles();

   if (TER_DEBUG_SHOW_SHADOW_MAP_TILE)
      render_shadow_map_tile();

   glEnable(GL_DEPTH_TEST);
}

static void
render_result()
{
   TerRenderTexture *fbo = scene_ms_fbo ? scene_ms_fbo : scene_fbo;

   /* Render everything to an FBO */
   ter_render_texture_start(fbo);

      /* No need to clear the color buffer, we are going to render all pixels */
      if (fbo->is_multisampled)
         glEnable(GL_MULTISAMPLE);

      glClear(GL_DEPTH_BUFFER_BIT);

      if (TER_DEBUG_SHOW_BOUNDING_BOXES)
         render_bounding_boxes();

      /* The rendering order affects performance, at least on Intel, probably
       * because of some early Z-kills. Try to render first things that can
       * be closer to the camera and last things that are further away and/or
       * more expensive to render.
       */
      render_objects(true);
      ter_terrain_render(terrain, true);
      ter_water_tile_render(water);
      ter_skybox_render(skybox);

      if (fbo->is_multisampled)
         glDisable(GL_MULTISAMPLE);
   ter_render_texture_stop(fbo);

   /* Resolve the multi-sampled FBO */
   if (scene_ms_fbo)
      ter_render_texture_blit(scene_ms_fbo, scene_fbo);

   /* Post-processing */
   fbo = scene_fbo;
   if (bloom_filter) {
      glDisable(GL_DEPTH_TEST);
      fbo = ter_bloom_filter_run(bloom_filter, scene_fbo);
      glEnable(GL_DEPTH_TEST);
   }

   /* Render to the window */
   ter_render_texture_blit_to_window(fbo, TER_WIN_WIDTH, TER_WIN_HEIGHT);
}

/**
 * Renders the current frame
 */
static void
render_scene()
{
   /* Render shadow map */
   render_shadow_map();

   /* After rendering the shadow map, subsequent rendering passes will
    * always render the same region of the terrain, so update the index
    * buffer with the current clipping region only once
    */
   if (TER_TERRAIN_ENABLE_CLIPPING)
      update_terrain_index_buffer();

   /* Render water textures */
   render_water_textures();

   /* Render scene */
   render_result();

   /* Render 2D tile overlays */
   render_2d_tiles();
}

static bool
check_for_camera_collision(TerCamera *cam)
{
   TerBox *cam_box = ter_camera_get_box(cam);

   /* Check for collisions against objects only, we correct the camera's
    * height automatically if it collides against the terrain.
    *
    * FIXME: split the terrain in sectors and assign objects to them, then
    * only test for collisions with objects in the same sector as the camera.
    */
   GList *objects = ter_object_renderer_get_solid(obj_renderer);
   GList *iter = objects;
   while (iter) {
      TerObject *o = (TerObject *) iter->data;
      if (ter_object_collision(o, cam_box))
         return true;
      iter = g_list_next(iter);
   }

   return false;
}

static void
clamp_camera_to_terrain(TerCamera *cam, TerTerrain *terrain)
{
   /* Keep camera within the terrain boundaries */
   cam->pos.x = CLAMP(cam->pos.x, 0.0f, ter_terrain_get_width(terrain));
   cam->pos.z = CLAMP(cam->pos.z, -ter_terrain_get_depth(terrain), 0.0f);

   /* Keep camera at a constant height above the terrain and within limits */
   float th = MAX(ter_terrain_get_height_at(terrain, cam->pos.x, cam->pos.z),
                  TER_TERRAIN_WATER_HEIGHT);
   cam->pos.y = CLAMP(cam->pos.y,
                      th + TER_CAMERA_MIN_HEIGHT, TER_CAMERA_MAX_HEIGHT);
}

static void
move_camera(TerCamera *cam, float speed)
{
   /* Handle rotation first, we don't care for collisions here  */
   if (glfwGetKey(window, GLFW_KEY_LEFT) != GLFW_RELEASE) {
      ter_camera_rotate(cam, 0.0f, speed * 1.0f, 0.0f);
   }
   else if (glfwGetKey(window, GLFW_KEY_RIGHT) != GLFW_RELEASE) {
      ter_camera_rotate(cam, 0.0f, speed * (-1.0f), 0.0f);
   }

   if (glfwGetKey(window, GLFW_KEY_PAGE_UP) != GLFW_RELEASE) {
      ter_camera_rotate(cam, speed * 1.0f, 0.0f, 0.0f);
   }
   else if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) != GLFW_RELEASE) {
      ter_camera_rotate(cam, speed * (-1.0f), 0.0f, 0.0f);
   }

   /* Handle stepping */
   if (glfwGetKey(window, GLFW_KEY_UP) != GLFW_RELEASE)
      speed *= 0.15f;
   else if (glfwGetKey(window, GLFW_KEY_DOWN) != GLFW_RELEASE)
      speed *= -0.15f;
   else
      return; /* Not stepping */

   glm::vec3 prev_pos = cam->pos;
   ter_camera_step(cam, speed, 1, 1, 1);

   if (TER_CAMERA_COLLISION_ENABLE) {
      clamp_camera_to_terrain(cam, terrain);
      if (prev_pos != cam->pos && check_for_camera_collision(cam)) {
         /* We detected a collision. Instead of just reverting the camera to
          * its old position (stopping it dead), try moving along each axis
          * separately checking for collisions. That way, if movement in any
          * axis is possible we let it happen.
          */
         cam->pos = prev_pos;
         for (int i = 0; i < 3; i++) {
            ter_camera_step(cam, speed, i == 0, i == 1, i == 2);
            clamp_camera_to_terrain(cam, terrain);
            if (prev_pos != cam->pos && check_for_camera_collision(cam))
               cam->pos = prev_pos;
            prev_pos = cam->pos;
         }
      }
   }
}

/**
 * Updates the scene
 */
static void
update_scene()
{
   TerCamera *cam = (TerCamera *) ter_cache_get("camera/main");

   /* We want the camera to have the same speed no matter the target framerate
    * or the real framerate we run at, so we define camera speeds for 60fps
    * and we apply a speed factor to these depending on how fast or slow
    * was the previous frame we rendered.
    */
   float fps = 1.0f / (float) fps_last_frame_time;
   float speed = 60.0f / fps;

   /* Move camera */
   move_camera(cam, speed);

   /* Update view matrix */
   View = ter_camera_get_view_matrix(cam);
   ViewInv = glm::inverse(View);
   ter_cache_set("matrix/View", &View);
   ter_cache_set("matrix/ViewInv", &ViewInv);

   /* Update water, skybox, light, etc */
   ter_water_tile_update(water);
   ter_skybox_update(skybox);

   TerLight *light = (TerLight *) ter_cache_get("light/light0");
   ter_light_update(light);
}

static inline void
frame_start()
{
   if (TER_DEBUG_SHOW_FPS) {
      fps_frame_start = glfwGetTime();
   }
}

static inline void
frame_end()
{
   if (TER_DEBUG_SHOW_FPS) {
      /* Skip the first frame */
      if (fps_skip_frame == 0) {
         fps_frame_end = glfwGetTime();

         fps_last_frame_time = fps_frame_end - fps_frame_start;
         fps_total_time += fps_last_frame_time;
         fps_frames++;

         if (fps_last_frame_time < fps_fastest_frame_time)
            fps_fastest_frame_time = fps_last_frame_time;
         if (fps_last_frame_time > fps_slowest_frame_time)
            fps_slowest_frame_time = fps_last_frame_time;

         if (fps_frames == 60) {
            ter_dbg(LOG_FPS,
                    "STATS: INFO: FPS: %.2f\n", fps_frames / fps_total_time);
            fps_total_run_time += fps_total_time;
            fps_total_run_frames += fps_frames;
            fps_total_time = 0.0;
            fps_frames = 0;
         }
      } else {
         fps_skip_frame--;
      }
   }

   TerCamera *cam = (TerCamera *) ter_cache_get("camera/main");
   cam->dirty = false;
}

static void
show_statistics()
{
   if (TER_DEBUG_SHOW_FPS) {
      if (TER_WIN_SWAP_INTERVAL != 0) {
         printf("STATS: WARNING: FPS: swap interval is not 0, frame times "
                "below include idle time.\n");
      }

      double avg_fps = fps_total_run_frames / fps_total_run_time;
      double avg_frame_time = 1.0 / avg_fps;
      printf("STATS: INFO: FPS: average: %.2f fps\n", avg_fps);
      printf("STATS: INFO: avg. frame time: %.3f ms\n", avg_frame_time * 1000);
      printf("STATS: INFO: FPS: fastest frame: %.3f ms\n",
              fps_fastest_frame_time * 1000);
      printf("STATS: INFO: FPS: slowest frame: %.3f ms\n",
              fps_slowest_frame_time * 1000);

      double load_at_60fps = 100.0 * avg_frame_time / (1.0 / 60.0);
      double load_at_30fps = 100.0 * avg_frame_time / (1.0 / 30.0);
      printf("STATS: INFO: FPS: load @ 60fps: %.2f%%\n", load_at_60fps);
      printf("STATS: INFO: FPS: load @ 30fps: %.2f%%\n", load_at_30fps);
   }
}

/**
 * Free allocated resources and deinit GLFW
 */
static void
teardown()
{
   if (scene_ms_fbo)
      ter_render_texture_free(scene_ms_fbo);
   if (scene_fbo)
      ter_render_texture_free(scene_fbo);
   if (bloom_filter)
      ter_bloom_filter_free(bloom_filter);
   ter_object_renderer_free(obj_renderer);
   free_obj_models();
   ter_terrain_free(terrain);
   ter_shadow_renderer_free(shadow_renderer);
   ter_water_tile_free(water);
   ter_skybox_free(skybox);
   ter_texture_manager_free(tex_mgr);
   free_shaders();
   free_lights();
   ter_cache_clear();
   glfwTerminate();
}

/**
 * Main loop
 */
int
main()
{
   srandom(time(NULL));

   setup_glfw();
   setup_scene();

   do {
      frame_start();

      update_scene();

      render_scene();

      glfwSwapBuffers(window);
      glfwPollEvents();

      frame_end();
   } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
            glfwWindowShouldClose(window) == 0);

   show_statistics();
   teardown();

   return 0;
}
