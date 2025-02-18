#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/UniquePtr.h>

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

#define VK_BOOL(expression) (expression) ? vk::True : vk::False

#define VK_ASSERT_DEBUG(code)                                                                                                                                                                                                          \
  do                                                                                                                                                                                                                                   \
  {                                                                                                                                                                                                                                    \
    auto s = (code);                                                                                                                                                                                                                   \
    XII_ASSERT_DEBUG(static_cast<vk::Result>(s) == vk::Result::eSuccess, "Vulkan call '{0}' failed with: {1} in {2}:{3}", XII_PP_STRINGIFY(code), vk::to_string(static_cast<vk::Result>(s)).data(), XII_SOURCE_FILE, XII_SOURCE_LINE); \
  } while (false)

#define VK_ASSERT_DEV(code)                                                                                                                                                                                                          \
  do                                                                                                                                                                                                                                 \
  {                                                                                                                                                                                                                                  \
    auto s = (code);                                                                                                                                                                                                                 \
    XII_ASSERT_DEV(static_cast<vk::Result>(s) == vk::Result::eSuccess, "Vulkan call '{0}' failed with: {1} in {2}:{3}", XII_PP_STRINGIFY(code), vk::to_string(static_cast<vk::Result>(s)).data(), XII_SOURCE_FILE, XII_SOURCE_LINE); \
  } while (false)

#define VK_LOG_ERROR(code)                                                                                                                                                        \
  do                                                                                                                                                                              \
  {                                                                                                                                                                               \
    auto s = (code);                                                                                                                                                              \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                                       \
    {                                                                                                                                                                             \
      xiiLog::Error("Vulkan call '{0}' failed with: {1} in {2}:{3}", XII_PP_STRINGIFY(code), vk::to_string(static_cast<vk::Result>(s)).data(), XII_SOURCE_FILE, XII_SOURCE_LINE); \
    }                                                                                                                                                                             \
  } while (false)

#define VK_SUCCEED_OR_RETURN_LOG(code)                                                                                                                                            \
  do                                                                                                                                                                              \
  {                                                                                                                                                                               \
    auto s = (code);                                                                                                                                                              \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                                       \
    {                                                                                                                                                                             \
      xiiLog::Error("Vulkan call '{0}' failed with: {1} in {2}:{3}", XII_PP_STRINGIFY(code), vk::to_string(static_cast<vk::Result>(s)).data(), XII_SOURCE_FILE, XII_SOURCE_LINE); \
      return s;                                                                                                                                                                   \
    }                                                                                                                                                                             \
  } while (false)

#define VK_SUCCEED_OR_RETURN(code)                                                                                                                                                \
  do                                                                                                                                                                              \
  {                                                                                                                                                                               \
    auto s = (code);                                                                                                                                                              \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                                       \
    {                                                                                                                                                                             \
      xiiLog::Error("Vulkan call '{0}' failed with: {1} in {2}:{3}", XII_PP_STRINGIFY(code), vk::to_string(static_cast<vk::Result>(s)).data(), XII_SOURCE_FILE, XII_SOURCE_LINE); \
      return;                                                                                                                                                                     \
    }                                                                                                                                                                             \
  } while (false)

#define VK_SUCCEED_OR_RETURN_XII_FAILURE(code)                                                                                                                                    \
  do                                                                                                                                                                              \
  {                                                                                                                                                                               \
    auto s = (code);                                                                                                                                                              \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                                       \
    {                                                                                                                                                                             \
      xiiLog::Error("Vulkan call '{0}' failed with: {1} in {2}:{3}", XII_PP_STRINGIFY(code), vk::to_string(static_cast<vk::Result>(s)).data(), XII_SOURCE_FILE, XII_SOURCE_LINE); \
      return XII_FAILURE;                                                                                                                                                         \
    }                                                                                                                                                                             \
  } while (false)

#define XII_SUCCEED_OR_RETURN_FAILURE(expression, ...) \
  do                                                   \
  {                                                    \
    XII_ASSERT_DEV((expression), __VA_ARGS__);         \
    if (!(expression)) { return XII_FAILURE; }         \
  } while (false)

////////// Forward Declarations //////////

class xiiGALCommandListVulkan;
class xiiGALCommandQueueVulkan;
class xiiGALDeviceVulkan;
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
class xiiGALFencePoolVulkan;
class xiiGALSemaphorePoolVulkan;
class xiiGALQueryPoolVulkan;
class xiiGALDescriptorSetPoolVulkan;
class xiiGALStagingBufferPoolVulkan;

struct xiiGALQueueInformationVulkan
{
  XII_DECLARE_POD_TYPE();

  vk::Queue m_vkQueue            = VK_NULL_HANDLE;
  xiiUInt32 m_uiQueueFamilyIndex = xiiInvalidIndex;
  xiiUInt32 m_uiQueueIndex       = 0U;
};

struct xiiGALDynamicBufferAllocationVulkan
{
  XII_DECLARE_POD_TYPE();

  vk::Buffer              m_vkBuffer;
  struct VmaAllocation_T* m_VmaAllocation;
  xiiUInt64               m_uiOffset;
};

struct xiiGALStagingBufferAllocationVulkan
{
  XII_DECLARE_POD_TYPE();

  vk::Buffer              m_vkBuffer;
  struct VmaAllocation_T* m_VmaAllocation;
  xiiUInt64               m_uiOffset;
};
