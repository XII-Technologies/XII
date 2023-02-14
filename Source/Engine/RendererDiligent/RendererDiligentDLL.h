#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERDILIGENT_LIB
#    define XII_RENDERERDILIGENT_DLL XII_DECL_EXPORT
#  else
#    define XII_RENDERERDILIGENT_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_RENDERERDILIGENT_DLL
#endif



#include <Common/interface/RefCntAutoPtr.hpp>

#include <Graphics/GraphicsAccessories/interface/GraphicsAccessories.hpp>
#include <Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <Graphics/GraphicsEngine/interface/SwapChain.h>

#define XII_GAL_DILIGENT_WRAPPED_RELEASE(wrappedDiligentObject) \
  do                                                            \
  {                                                             \
    if ((wrappedDiligentObject) != nullptr)                     \
    {                                                           \
      (wrappedDiligentObject).Release();                        \
    }                                                           \
  } while (0)

#define XII_GAL_DILIGENT_UNWRAPPED_RELEASE(unwrappedDiligentObject) \
  do                                                                \
  {                                                                 \
    if ((unwrappedDiligentObject) != nullptr)                       \
    {                                                               \
      (unwrappedDiligentObject)->Release();                         \
      (unwrappedDiligentObject) = nullptr;                          \
    }                                                               \
  } while (0)
