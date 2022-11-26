#pragma once

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RMLUIPLUGIN_LIB
#    define XII_RMLUIPLUGIN_DLL XII_DECL_EXPORT
#  else
#    define XII_RMLUIPLUGIN_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_RMLUIPLUGIN_DLL
#endif

#ifndef RMLUI_USE_CUSTOM_RTTI
#  define RMLUI_USE_CUSTOM_RTTI
#endif
