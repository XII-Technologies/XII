/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Shaders/Platform/Platform.h>

// HLSL float component types do not describe the storage precision. Vulkan requires
// the SPIR-V image format to agree with the image view bound by the render graph.
#if XII_ENABLED(XII_SHADER_PLATFORM_VULKAN)
#  define XII_STORAGE_IMAGE_FORMAT(Format) [[vk::image_format(Format)]]
#else
#  define XII_STORAGE_IMAGE_FORMAT(Format)
#endif
