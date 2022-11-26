#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERDX11_LIB
#    define XII_RENDERERDX11_DLL XII_DECL_EXPORT
#  else
#    define XII_RENDERERDX11_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_RENDERERDX11_DLL
#endif


#define XII_GAL_DX11_RELEASE(d3dobj) \
  do                                 \
  {                                  \
    if ((d3dobj) != nullptr)         \
    {                                \
      (d3dobj)->Release();           \
      (d3dobj) = nullptr;            \
    }                                \
  } while (0)
