#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_FEATURELABPLUGIN_LIB
#    define XII_FEATURELABPLUGIN_DLL XII_DECL_EXPORT
#  else
#    define XII_FEATURELABPLUGIN_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_FEATURELABPLUGIN_DLL
#endif
