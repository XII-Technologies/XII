#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>

// BEGIN-DOCS-CODE-SNIPPET: dll-export-defines
// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_SAMPLEGAMEPLUGIN_LIB
#    define XII_SAMPLEGAMEPLUGIN_DLL XII_DECL_EXPORT
#  else
#    define XII_SAMPLEGAMEPLUGIN_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_SAMPLEGAMEPLUGIN_DLL
#endif
// END-DOCS-CODE-SNIPPET
