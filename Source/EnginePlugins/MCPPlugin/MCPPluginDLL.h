/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_MCPPLUGIN_LIB
#    define XII_MCPPLUGIN_DLL XII_DECL_EXPORT
#  else
#    define XII_MCPPLUGIN_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_MCPPLUGIN_DLL
#endif
