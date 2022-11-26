#pragma once

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RECASTPLUGIN_LIB
#    define XII_RECASTPLUGIN_DLL XII_DECL_EXPORT
#  else
#    define XII_RECASTPLUGIN_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_RECASTPLUGIN_DLL
#endif
