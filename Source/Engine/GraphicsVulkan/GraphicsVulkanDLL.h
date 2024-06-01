#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_GRAPHICSVULKAN_LIB
#    define XII_GRAPHICSVULKAN_DLL XII_DECL_EXPORT
#  else
#    define XII_GRAPHICSVULKAN_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_GRAPHICSVULKAN_DLL
#endif

#define XII_GAL_VULKAN_RELEASE(pObject) \
  do                                    \
  {                                     \
    if ((pObject) != nullptr)           \
    {                                   \
      (pObject)->Release();             \
      (pObject) = nullptr;              \
    }                                   \
  } while (0)

#define VK_BOOL(expression) (expression) ? VK_TRUE : VK_FALSE

////////// Forward Declarations //////////

class xiiGALCommandListVulkan;
class xiiGALCommandQueueVulkan;
class xiiGALDeviceVulkan;
class xiiGALPassVulkan;
class xiiGALSwapChainVulkan;
class xiiGALBottomLevelASVulkan;
class xiiGALBufferVulkan;
class xiiGALBufferViewVulkan;
class xiiGALFenceVulkan;
class xiiGALFramebufferVulkan;
class xiiGALQueryVulkan;
class xiiGALRenderPassVulkan;
class xiiGALSamplerVulkan;
class xiiGALTextureVulkan;
class xiiGALTextureViewVulkan;
class xiiGALTopLevelASVulkan;
class xiiGALInputLayoutVulkan;
class xiiGALShaderVulkan;
class xiiGALBlendStateVulkan;
class xiiGALDepthStencilStateVulkan;
class xiiGALRasterizerStateVulkan;
class xiiGALPipelineStateVulkan;
class xiiGALPipelineResourceSignatureVulkan;
class xiiGALShaderResourceVariableVulkan;
