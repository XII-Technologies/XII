#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERVULKAN_LIB
#    define XII_RENDERERVULKAN_DLL XII_DECL_EXPORT
#  else
#    define XII_RENDERERVULKAN_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_RENDERERVULKAN_DLL
#endif

// Uncomment to log all layout transitions.
//#define VK_LOG_LAYOUT_CHANGES

#define XII_GAL_VULKAN_RELEASE(vulkanObj) \
  do                                      \
  {                                       \
    if ((vulkanObj) != nullptr)           \
    {                                     \
      (vulkanObj)->Release();             \
      (vulkanObj) = nullptr;              \
    }                                     \
  } while (0)

#define VK_ASSERT_DEBUG(code)                                                                                                  \
  do                                                                                                                           \
  {                                                                                                                            \
    auto s = (code);                                                                                                           \
    XII_ASSERT_DEBUG(static_cast<vk::Result>(s) == vk::Result::eSuccess, "Vukan call '{0}' failed with: {1} in {2}:{3}",       \
                     XII_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), XII_SOURCE_FILE, XII_SOURCE_LINE); \
  } while (false)

#define VK_ASSERT_DEV(code)                                                                                                  \
  do                                                                                                                         \
  {                                                                                                                          \
    auto s = (code);                                                                                                         \
    XII_ASSERT_DEV(static_cast<vk::Result>(s) == vk::Result::eSuccess, "Vukan call '{0}' failed with: {1} in {2}:{3}",       \
                   XII_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), XII_SOURCE_FILE, XII_SOURCE_LINE); \
  } while (false)

#define VK_LOG_ERROR(code)                                                                                                                                                    \
  do                                                                                                                                                                          \
  {                                                                                                                                                                           \
    auto s = (code);                                                                                                                                                          \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                                   \
    {                                                                                                                                                                         \
      xiiLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", XII_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), XII_SOURCE_FILE, XII_SOURCE_LINE); \
    }                                                                                                                                                                         \
  } while (false)

#define VK_SUCCEED_OR_RETURN_LOG(code)                                                                                                                                        \
  do                                                                                                                                                                          \
  {                                                                                                                                                                           \
    auto s = (code);                                                                                                                                                          \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                                   \
    {                                                                                                                                                                         \
      xiiLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", XII_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), XII_SOURCE_FILE, XII_SOURCE_LINE); \
      return s;                                                                                                                                                               \
    }                                                                                                                                                                         \
  } while (false)

#define VK_SUCCEED_OR_RETURN_XII_FAILURE(code)                                                                                                                                \
  do                                                                                                                                                                          \
  {                                                                                                                                                                           \
    auto s = (code);                                                                                                                                                          \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                                   \
    {                                                                                                                                                                         \
      xiiLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", XII_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), XII_SOURCE_FILE, XII_SOURCE_LINE); \
      return XII_FAILURE;                                                                                                                                                     \
    }                                                                                                                                                                         \
  } while (false)
