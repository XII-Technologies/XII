#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_GRAPHICSCORE_LIB
#    define XII_GRAPHICSCORE_DLL XII_DECL_EXPORT
#  else
#    define XII_GRAPHICSCORE_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_GRAPHICSCORE_DLL
#endif

#include <Foundation/Containers/IdTable.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

#define XII_EMBED_FONT_FILE XII_ON
