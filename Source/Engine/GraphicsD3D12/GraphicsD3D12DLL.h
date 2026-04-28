/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_GRAPHICSD3D12_LIB
#    define XII_GRAPHICSD3D12_DLL XII_DECL_EXPORT
#  else
#    define XII_GRAPHICSD3D12_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_GRAPHICSD3D12_DLL
#endif

#define XII_GAL_D3D12_RELEASE(pObject) \
  do                                   \
  {                                    \
    if ((pObject) != nullptr)          \
    {                                  \
      (pObject)->Release();            \
      (pObject) = nullptr;             \
    }                                  \
  } while (0)

#define XII_GAL_D3D12_RELEASE_ARRAY(pArrayObject) \
  do                                              \
  {                                               \
    for (auto pObject : (pArrayObject))           \
    {                                             \
      if (pObject != nullptr)                     \
      {                                           \
        pObject->Release();                       \
        pObject = nullptr;                        \
      }                                           \
    }                                             \
  } while (0)

#define D3D12_BOOL(expression) (expression) ? TRUE : FALSE

#define FORCE_NTDDI_WIN10_19H1 0

////////// Forward Declarations //////////

class xiiGALCommandListD3D12;
class xiiGALCommandQueueD3D12;
class xiiGALDeviceD3D12;
class xiiGALPassD3D12;
class xiiGALSwapChainD3D12;
class xiiGALBottomLevelASD3D12;
class xiiGALBufferD3D12;
class xiiGALBufferViewD3D12;
class xiiGALFenceD3D12;
class xiiGALFramebufferD3D12;
class xiiGALQueryD3D12;
class xiiGALRenderPassD3D12;
class xiiGALSamplerD3D12;
class xiiGALTextureD3D12;
class xiiGALTextureViewD3D12;
class xiiGALTopLevelASD3D12;
class xiiGALInputLayoutD3D12;
class xiiGALShaderD3D12;
class xiiGALBlendStateD3D12;
class xiiGALDepthStencilStateD3D12;
class xiiGALRasterizerStateD3D12;
class xiiGALGraphicsPipelineStateD3D12;
class xiiGALComputePipelineStateD3D12;
class xiiGALRayTracingPipelineStateD3D12;
class xiiGALTilePipelineStateD3D12;
class xiiGALPipelineResourceSignatureD3D12;

struct xiiGALDisplayModeDescriptionD3D12;
struct xiiGALFullScreenModeDescriptionD3D12;
