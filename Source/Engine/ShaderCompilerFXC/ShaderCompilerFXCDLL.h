#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_SHADERCOMPILERFXC_LIB
#    define XII_SHADERCOMPILERFXC_DLL XII_DECL_EXPORT
#  else
#    define XII_SHADERCOMPILERFXC_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_SHADERCOMPILERFXC_DLL
#endif
