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

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

#define VK_ENABLE_BETA_EXTENSIONS
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  define VK_USE_PLATFORM_WIN32_KHR
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  define VK_USE_PLATFORM_XCB_KHR
#elif XII_ENABLED(XII_PLATFORM_ANDROID)
#  define VK_USE_PLATFORM_ANDROID_KHR
#endif

#include <vulkan/vulkan.hpp>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <vulkan/vulkan_win32.h>
#elif XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <vulkan/vulkan_android.h>
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

#define VK_ASSERT_DEBUG(code)                                                                                                                                                                                                         \
  do                                                                                                                                                                                                                                  \
  {                                                                                                                                                                                                                                   \
    auto s = (code);                                                                                                                                                                                                                  \
    XII_ASSERT_DEBUG(static_cast<vk::Result>(s) == vk::Result::eSuccess, "Vukan call '{0}' failed with: {1} in {2}:{3}", XII_PP_STRINGIFY(code), vk::to_string(static_cast<vk::Result>(s)).data(), XII_SOURCE_FILE, XII_SOURCE_LINE); \
  } while (false)

#define VK_ASSERT_DEV(code)                                                                                                                                                                                                         \
  do                                                                                                                                                                                                                                \
  {                                                                                                                                                                                                                                 \
    auto s = (code);                                                                                                                                                                                                                \
    XII_ASSERT_DEV(static_cast<vk::Result>(s) == vk::Result::eSuccess, "Vukan call '{0}' failed with: {1} in {2}:{3}", XII_PP_STRINGIFY(code), vk::to_string(static_cast<vk::Result>(s)).data(), XII_SOURCE_FILE, XII_SOURCE_LINE); \
  } while (false)

#define VK_LOG_ERROR(code)                                                                                                                                                       \
  do                                                                                                                                                                             \
  {                                                                                                                                                                              \
    auto s = (code);                                                                                                                                                             \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                                      \
    {                                                                                                                                                                            \
      xiiLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", XII_PP_STRINGIFY(code), vk::to_string(static_cast<vk::Result>(s)).data(), XII_SOURCE_FILE, XII_SOURCE_LINE); \
    }                                                                                                                                                                            \
  } while (false)

#define VK_SUCCEED_OR_RETURN_LOG(code)                                                                                                                                           \
  do                                                                                                                                                                             \
  {                                                                                                                                                                              \
    auto s = (code);                                                                                                                                                             \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                                      \
    {                                                                                                                                                                            \
      xiiLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", XII_PP_STRINGIFY(code), vk::to_string(static_cast<vk::Result>(s)).data(), XII_SOURCE_FILE, XII_SOURCE_LINE); \
      return s;                                                                                                                                                                  \
    }                                                                                                                                                                            \
  } while (false)

#define VK_SUCCEED_OR_RETURN_XII_FAILURE(code)                                                                                                                                   \
  do                                                                                                                                                                             \
  {                                                                                                                                                                              \
    auto s = (code);                                                                                                                                                             \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                                      \
    {                                                                                                                                                                            \
      xiiLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", XII_PP_STRINGIFY(code), vk::to_string(static_cast<vk::Result>(s)).data(), XII_SOURCE_FILE, XII_SOURCE_LINE); \
      return XII_FAILURE;                                                                                                                                                        \
    }                                                                                                                                                                            \
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

XII_DEFINE_AS_POD_TYPE(vk::Format);
