#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERCORE_LIB
#    define XII_RENDERERCORE_DLL XII_DECL_EXPORT
#  else
#    define XII_RENDERERCORE_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_RENDERERCORE_DLL
#endif

#define XII_EMBED_FONT_FILE XII_ON
