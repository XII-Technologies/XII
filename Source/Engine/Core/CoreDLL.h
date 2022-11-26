#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_CORE_LIB
#    define XII_CORE_DLL        XII_DECL_EXPORT
#    define XII_CORE_DLL_FRIEND XII_DECL_EXPORT_FRIEND
#  else
#    define XII_CORE_DLL        XII_DECL_IMPORT
#    define XII_CORE_DLL_FRIEND XII_DECL_IMPORT_FRIEND
#  endif
#else
#  define XII_CORE_DLL
#  define XII_CORE_DLL_FRIEND
#endif
