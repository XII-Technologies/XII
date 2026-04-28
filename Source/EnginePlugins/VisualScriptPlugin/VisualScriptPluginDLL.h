/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_VISUALSCRIPTPLUGIN_LIB
#    define XII_VISUALSCRIPTPLUGIN_DLL XII_DECL_EXPORT
#  else
#    define XII_VISUALSCRIPTPLUGIN_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_VISUALSCRIPTPLUGIN_DLL
#endif
