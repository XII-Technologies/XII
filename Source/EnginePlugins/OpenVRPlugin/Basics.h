#pragma once

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_OPENVRPLUGIN_LIB
#    define XII_OPENVRPLUGIN_DLL XII_DECL_EXPORT
#  else
#    define XII_OPENVRPLUGIN_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_OPENVRPLUGIN_DLL
#endif
