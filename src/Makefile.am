bin_PROGRAMS = demo

AM_CPPFLAGS = @DEPS_CFLAGS@

demo_SOURCES = \
    main.cpp \
    ter-cache.cpp \
    ter-camera.cpp \
    ter-shader-program.cpp \
    ter-mesh.cpp \
    ter-terrain.cpp \
    ter-light.cpp \
    ter-model.cpp \
    ter-object.cpp \
    ter-object-catalog.cpp \
    ter-object-renderer.cpp \
    ter-tile.cpp \
    ter-water-tile.cpp \
    ter-shadow-map.cpp \
    ter-shadow-box.cpp \
    ter-shadow-renderer.cpp \
    ter-box.cpp \
    ter-texture.cpp \
    ter-render-texture.cpp \
    ter-sky-box.cpp

demo_CFLAGS = \
    -DPREFIX=$(prefix) \
    -D_GNU_SOURCE

demo_LDADD = \
    @DEPS_LIBS@ \
    -lm

MAINTAINERCLEANFILES = \
	*.in \
	*~

DISTCLEANFILES = $(MAINTAINERCLEANFILES)
