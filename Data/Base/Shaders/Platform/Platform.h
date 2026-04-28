/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Basics/PreprocessorUtilities.h>

/// Auto-detect shader platform from known shader-model macros (e.g. VK_SM68, D3D_SM67).
#define XII_SHADER_PLATFORM        XII_OFF
#define XII_SHADER_PLATFORM_VULKAN XII_OFF
#define XII_SHADER_PLATFORM_D3D12  XII_OFF
#define XII_SHADER_PLATFORM_METAL  XII_OFF

/// Detect D3D12 shader model macros.
#if defined(D3D_SM69) || defined(D3D_SM68) || defined(D3D_SM67) || defined(D3D_SM66) || defined(D3D_SM65) || \
  defined(D3D_SM64) || defined(D3D_SM63) || defined(D3D_SM62) || defined(D3D_SM61) || defined(D3D_SM60) || defined(D3D_SM51)
#  undef XII_SHADER_PLATFORM_D3D12
#  define XII_SHADER_PLATFORM_D3D12 XII_ON

/// Detect Vulkan shader model macros.
#elif defined(VK_SM69) || defined(VK_SM68) || defined(VK_SM67) || defined(VK_SM66) || defined(VK_SM65) || \
  defined(VK_SM64) || defined(VK_SM63) || defined(VK_SM62) || defined(VK_SM61) || defined(VK_SM60)
#  undef XII_SHADER_PLATFORM_VULKAN
#  define XII_SHADER_PLATFORM_VULKAN XII_ON

/// Detect Metal shader model version.
#elif defined(METAL_SM10) || defined(METAL_SM11) || defined(METAL_SM12) || defined(METAL_SM13) || defined(METAL_SM20) || \
  defined(METAL_SM21) || defined(METAL_SM22) || defined(METAL_SM23) || defined(METAL_SM30) || defined(METAL_SM31) ||     \
  defined(METAL_SM32) || defined(METAL_SM33) || defined(METAL_SM40) || defined(METAL_SM41) || defined(METAL_SM42)
#  undef XII_SHADER_PLATFORM_METAL
#  define XII_SHADER_PLATFORM_METAL XII_ON

#else
#  if !defined(__cplusplus)
#    error "Could not auto-detect shader platform. Please define XII_SHADER_PLATFORM_VULKAN, XII_SHADER_PLATFORM_D3D12, or XII_SHADER_PLATFORM_METAL."
#  endif
#endif

#if !defined(__cplusplus)
#  if XII_ENABLED(XII_SHADER_PLATFORM_VULKAN)
#    include "Platform_Vulkan.h"
#  elif XII_ENABLED(XII_SHADER_PLATFORM_D3D12)
#    include "Platform_D3D12.h"
#  elif XII_ENABLED(XII_SHADER_PLATFORM_METAL)
#    include "Platform_Metal.h"
#  else
#    error "Unsupported shader platform: define XII_SHADER_PLATFORM_VULKAN, XII_SHADER_PLATFORM_D3D12, or XII_SHADER_PLATFORM_METAL."
#  endif
#endif
