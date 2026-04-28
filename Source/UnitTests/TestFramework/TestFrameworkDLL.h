/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_TESTFRAMEWORK_LIB
#    define XII_TEST_DLL XII_DECL_EXPORT
#  else
#    define XII_TEST_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_TEST_DLL
#endif

enum class xiiTestAppRun
{
  Continue,
  Quit
};
