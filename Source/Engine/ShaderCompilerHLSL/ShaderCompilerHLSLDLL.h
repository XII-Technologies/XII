#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_SHADERCOMPILERHLSL_LIB
#    define XII_SHADERCOMPILERHLSL_DLL XII_DECL_EXPORT
#  else
#    define XII_SHADERCOMPILERHLSL_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_SHADERCOMPILERHLSL_DLL
#endif
