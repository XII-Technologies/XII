/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_GAMEENGINE_LIB
#    define XII_GAMEENGINE_DLL XII_DECL_EXPORT
#  else
#    define XII_GAMEENGINE_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_GAMEENGINE_DLL
#endif
