#ifndef __MAIN_CONSTANTS_H__
#define __MAIN_CONSTANTS_H__

/*
 * Fullscreen mode and target FPS
 *
 * In windowed mode GLFW will automatically set the target framerate
 * to that of the compositor (usually 60fps). In fullscreen mode this
 * can be set by the user.
 *
 * NOTE: Some versions of GLFW seem to ignore this also in
 * fullscreen mode
 */
#define TER_WIN_FULLSCREEN_ENABLE false
#define TER_WIN_FULLSCREEN_TARGET_FPS 30

/*
 * Swap interval. Set to N to start rendering the next frame only after
 * N monitor refreshes have occured. Set to 1 to render as fast as possible
 * but trying to avoid tearing effects. Set to 0 to render as fast as
 * possible without waiting for monitor refreshes, which might produce tearing
 * but is useful for benchmarking.
 *
 * NOTE: some drivers might ignore this setting.
 */
#define TER_WIN_SWAP_INTERVAL 1

/*
 * Window resolution
 */
#if TER_WIN_FULLSCREEN_ENABLE
#define TER_WIN_WIDTH  1600
#define TER_WIN_HEIGHT 900
#else
#define TER_WIN_WIDTH  800
#define TER_WIN_HEIGHT 600
#endif

/*
 * Projection properties
 */
#define TER_FOV 45.0f
#define TER_ASPECT_RATIO (((float) TER_WIN_WIDTH) / ((float) TER_WIN_HEIGHT))
#define TER_NEAR_PLANE 0.01f
#define TER_FAR_PLANE 300.0f
#define TER_FAR_PLANE_SKY 5000.0f

/*
 * Debug options
 */
#define TER_DEBUG_SHOW_WATER_TILES false
#define TER_DEBUG_SHOW_SHADOW_MAP_TILE false
#define TER_DEBUG_SHOW_FPS true
#define TER_DEBUG_SHOW_BOUNDING_BOXES false

#define TER_DEBUG_TRACE_ENABLE true
#define TER_DEBUG_TRACE_DEFAULT_ENABLE false
#define TER_DEBUG_TRACE_FPS_ENABLE false
#define TER_DEBUG_TRACE_VBO_ENABLE false
#define TER_DEBUG_TRACE_OBJ_LOAD_ENABLE false
#define TER_DEBUG_TRACE_RENDER_ENABLE false
#define TER_DEBUG_TRACE_SHADER_ENABLE true

/*
 * Enable camera collision detection
 */
#define TER_CAMERA_COLLISION_ENABLE true

/*
 * If camera collision detection is enabled:
 * - The minimum height of the camera above the terrain.
 * - The maximum height of the camera
 */
#define TER_CAMERA_MIN_HEIGHT 1.0f
#define TER_CAMERA_MAX_HEIGHT 20.0f

/*
 * Number of samples for multisampling. Set to 0 to disable multisampling.
 */
#define TER_MULTISAMPLING_SAMPLES 4

/*
 * Terrain resolution and size
 */
#define TER_TERRAIN_VX 251
#define TER_TERRAIN_VZ 251
#define TER_TERRAIN_TILE_SIZE 0.5f

/*
 * Water height and tile size.
 *
 * Notice that too large tiles might harm water quality
 */
#define TER_TERRAIN_WATER_TILE_SIZE 5.0f
#define TER_TERRAIN_WATER_HEIGHT -1.5f

/*
 * Water refraction / reflection texture resolutions
 */
#define TER_WATER_REFRACTION_TEX_W 400
#define TER_WATER_REFRACTION_TEX_H 300
#define TER_WATER_REFLECTION_TEX_W 800
#define TER_WATER_REFLECTION_TEX_H 600

/*
 * We sample the depth information in the refraction texture to make the water
 * transparent in shallow areas and avoid glitches due to the distortion and
 * precision errors during texture sampling. Because the terrain right at edge
 * of the water will be at exactly the water height and surrounding terrain
 * will normally be higher than the water it will be clipped so when we sample
 * depth at the edge of the water we might get 0 (what we want) or we might
 * get the maximum distance if there is any precision problem, which leads to
 * non-tranparent and distorted water at the edge which will be glitchy. This
 * offset can be used to prevent this by adding a small vertical offset to the
 * clipping plane so that geometry that is slightly above water level is
 * processed and we store depth information also for areas nearby water edges,
 * giving room for precision errors. Increasing the size of the refraction
 * texture will also help (the value should be 0 or positive).
 */
#define TER_WATER_REFRACTION_CLIPPING_OFFSET 0.25f

/*
 * Same as with the refraction texture. Normally, we should have no glitches
 * when sampling reflections at the edge of the water because we have no
 * distortion (or very small levels of distortion) thanks to the depth
 * information stored in the refraction texture. Still, if the distortion
 * of the water is big and/or the resolution of the reflection texture is
 * low enough, we might also run into glitches where we sample the reflection
 * texture outside the reflected area rendered to it. Adding a small offset so
 * that we also render geometry slightly below the water level should help hide
 * the glitches if that ever happens (the value should be 0 or positive).
 */
#define TER_WATER_REFLECTION_CLIPPING_OFFSET 0.0f

/*
 * Water parameters: reflectiveness, distortion, wave speed, etc.
 */
#define TER_WATER_REFLECTIVENESS 0.75f /* Between 0 and 1 */
#define TER_WATER_DISTORTION 0.015f
#define TER_WATER_WAVE_SPEED 0.0015f

/*
 * Enable object shadowing on water refraction / reflection textures
 */
#define TER_WATER_REFRACTION_SHADOWS_ENABLE false
#define TER_WATER_REFLECTION_SHADOWS_ENABLE true

/*
 * Object instances of the same model use instanced rendering, so we
 * map the model matrix for all of them in a single vertex buffer object.
 * This determines the size of the buffer, and thus, imposes a limit on
 * the maximum number of objects of a particular model we can draw
 * in single draw call.
 *
 * 32KB is sufficient to render up to 512 instances of each model in
 * a single draw call, but a larger buffer allows us to re-use the same
 * buffer through multiple render calls, where each call uses a different
 * segment of the buffer. This helps prevent GPU stalls between different
 * passes.
 */
#define TER_MODEL_MAX_INSTANCED_VBO_BYTES (64 * 1024)

/* We use multi-buffering for dynamically updated instanced data
 * to prevent stalls between on-going draw commands reading data from the
 * instanced buffer and data uploads to the same buffer triggered by new
 * draw commands being emitted by the application in subsequent frames
 * or even the same frame (we render model data multiple times in the same
 * frame for the various passes we use). This setting indicates the number
 * of buffers to use (each of size TER_MAX_INSTANCED_VBO_BYTES).
 */
#define TER_MODEL_NUM_INSTANCED_BUFFERS 2

/* Maximum number of materials and textures per model
 *
 * Warning: these numbers cannot be larger than the arrays in the model shaders
 * (variants * materials, variants * textures)
 *
 * FIXME: We only really support 1 texture per model.
 */
#define TER_MODEL_MAX_VARIANTS  4
#define TER_MODEL_MAX_MATERIALS 4
#define TER_MODEL_MAX_TEXTURES  4

/*
 * Enable clipping (at the distances indicated below)
 *
 * Minimum clipping distance should be 15.0f, since that is the minimum distance
 * from the far clipping plane at which the shaders will start fading out
 * objects.
 */
#define TER_OBJECT_RENDERER_ENABLE_CLIPPING true

/*
 * Enable clipping of the terrain surface
 *
 * Instead of having a static index buffer, we compute the (smaller) clipped
 * index buffer in each frame.
 */
#define TER_TERRAIN_ENABLE_CLIPPING true


/*
 * Distance at which we stop rendering under water objects to the refraction
 * texture
 */
#define TER_WATER_REFRACTION_CLIPPING_DISTANCE 35.0f

/*
 * Whether we want to record depth information of underwater objects (other than
 * the terrain) in the refraction texture. If we do, and they are close to the
 * surface, then water around them will be transparent and lack distortion
 * to prevent rendering artifacts, but because it is transparent it means that
 * we will see the underwater section of the object. This means that we can't
 * fade the object out smoothly when it is about to be clipped (because no
 * matter the alpha we use to render it to the refraction texture that part of
 * the texture will be rendered with alpha close to 0 on the water surface).
 * The consequence of that is that when the object is clipped we can clearly see
 * its underwater section suddenly disappear from the view, at least if the
 * clipping distance is not high enough to hide this from the viewer.
 */
#define TER_WATER_REFRACTION_RECORD_OBJECT_DEPTH true

/*
 * Distance at which we stop rendering objects above water level to the reflection
 * texture
 */
#define TER_WATER_REFLECTION_CLIPPING_DISTANCE 60.0f

/*
 * Enable dynamic light
 *
 * Moves the light source continuously around the scene.
 *
 * If disabled, the shadow map is computed just once in the first frame
 * using a large shadow clipping volume (which reduces a bit shadow quality).
 * Static lighting significantly increases performance, since shadow map
 * computations are expensive.
 */
#define TER_DYNAMIC_LIGHT_ENABLE true

/*
 * Speed of the light in rotation degrees per frame
 * (only applies if dynamic light is enabled)
 */
#define TER_DYNAMIC_LIGHT_SPEED 0.15f

/*
 * Enable shadow map clipping (at shadow distance)
 */
#define TER_SHADOW_RENDERER_ENABLE_CLIPPING true

/* Area around the camera where shadows are casted. Larger values
 * decrease shadow quality. Too small values can make relatively
 * close obects not cast a shadow.
 */
#define TER_SHADOW_DISTANCE 40.0f

/* Shadow map resolution. Shadow quality increases with size.
 */
#define TER_SHADOW_MAP_SIZE  (4.0f * 1024.0f)

/* Number of Cascade Shadow Mapping levels to use. Between 1 (just a single
 * level, so no cascade) and 4 (the maximum supported).
 */
#define TER_SHADOW_CSM_NUM_LEVELS 3

/* Distances covered by each shadow map cascade level. Distances are expressed
 * as a percentage (between 0 and 1) of TER_SHADOW_DISTANCE.
 *
 * - The last level (index TER_SHADOW_CSM_NUM_LEVELS - 1) must be 1.0, so we
 * cover all of TER_SHADOW_DISTANCE.
 * - All distances should be stricly larger than 0.
 * - Each level should cover a strictly larger distance than the previous.
 *
 * The code has assertions to verify these rules.
 *
 * This default setup looks like this:
 *
 *      NearPlane                    SHADOW_DIST
 *          [------|------|------|------]
 * csm[0]   ^^^^^^^^^^ 35%
 * csm[1]             ^^^^^^^^^^ 70%
 * csm[2]                       ^^^^^^^^ 100%
 *
 * Notice that the shadow frustum is larger the further from the near plane
 * so evenly splitting the limit shadow distance between the levels does not
 * mean that they both use an equally sized shadow volumes.
 */
static float __attribute__ ((unused))
TER_SHADOW_CSM_DISTANCES[4] = { 0.35f, 0.7f, 1.0f, 0.0f };


/* Size of the shadow map for each CSM level. As a percentage (between 0 and 1)
 * of TER_SHADOW_MAP_SIZE.
 *
 * This setup favours quality of shadows that are close to the camera at the
 * expense of reducing resolution for more distant shadows.
 */
static float __attribute__ ((unused))
TER_SHADOW_CSM_MAP_SIZES[4] = { 1.0f, 0.75f, 0.5f, 0.0f };

/* Number of samples for shadow antialiasing:
 *
 * Num. samples = (ShadowPFC * 2.0 + 1.0)^2
 *
 * Larger values produce better quality shadows. A value of 0 disables PFC and
 * will only sample once.
 */
#define TER_SHADOW_PFC 1

/*
 * Same as above, but applied when only when computing water reflection and
 * refraction textures (i.e. shadows on the objects reflected and refracted on
 * the water).
 */
#define TER_SHADOW_PFC_WATER 0

/*
 * Shadow map update interval (in frames)
 *
 * Indicates how often should the shadow map be updated. A value of 1 is
 * the most demanding (update every frame).
 */
#define TER_SHADOW_UPDATE_INTERVAL 1

/*
 * Enable the bloom filter
 */
#define TER_BLOOM_FILTER_ENABLE true

/*
 * How much we down scale the blur image with respect to the original
 * resolution of the scene. The value should be in the range (0.0, 1.0].
 * The lower the value the more blur we get and the more intense the bloom
 * effect becomes.
 */
#define TER_BLOOM_BLUR_SCALE    0.2f

/*
 * Bloom luminance factor
 *
 * Smaller values make the bloom brightness filter less selective, leading to
 * more pixels being selected for the bloom effect.
 *
 * Basically, if L is the luminance of a particular pixel this produces
 * pixel * L^TER_BLOOM_LUMINANCE_FACTOR. Since L values are in the range
 * [0, 1], the larger the value, the smaller the intensity of the output
 * pixel, leading to a less intense bloom effect.
 */
#define TER_BLOOM_LUMINANCE_FACTOR 3.0f

/*
 * Virtual texture IDs
 */
#define TER_TEX_TERRAIN_HEIGHTMAP_01    0
#define TER_TEX_TERRAIN_SURFACE_01     10
#define TER_TEX_SKY_BOX_01             20
#define TER_TEX_WATER_DUDV_01          30
#define TER_TEX_WATER_NORMAL_01        40

#endif
