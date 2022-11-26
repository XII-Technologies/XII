#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

#include <vulkan/vulkan.hpp>

XII_DEFINE_AS_POD_TYPE(vk::PresentModeKHR);

/// \brief Helper functions to convert and extract Vulkan objects from XII objects.
class XII_RENDERERVULKAN_DLL xiiConversionUtilsVulkan
{
public:
  /// \brief Helper function to hash vk enums.
  template <typename T, typename R = typename std::underlying_type<T>::type>
  static R GetUnderlyingValue(T value)
  {
    return static_cast<typename std::underlying_type<T>::type>(value);
  }

  /// \brief Helper function to hash vk flags.
  template <typename T>
  static auto GetUnderlyingFlagsValue(T value)
  {
    return static_cast<typename T::MaskType>(value);
  }

  static vk::SampleCountFlagBits   GetSamples(xiiEnum<xiiGALMSAASampleCount> samples);
  static vk::PresentModeKHR        GetPresentMode(xiiEnum<xiiGALPresentMode> presentMode, const xiiDynamicArray<vk::PresentModeKHR>& supportedModes);
  static vk::ImageSubresourceRange GetSubresourceRange(const xiiGALTextureCreationDescription& texDesc, const xiiGALRenderTargetViewCreationDescription& desc);
  static vk::ImageSubresourceRange GetSubresourceRange(const xiiGALTextureCreationDescription& texDesc, const xiiGALResourceViewCreationDescription& viewDesc);
  static vk::ImageSubresourceRange GetSubresourceRange(const xiiGALTextureCreationDescription& texDesc, const xiiGALUnorderedAccessViewCreationDescription& viewDesc);
  static vk::ImageSubresourceRange GetSubresourceRange(const vk::ImageSubresourceLayers& layers);
  static vk::ImageViewType         GetImageViewType(xiiEnum<xiiGALTextureType> texType, bool bIsArray);

  static bool                    IsDepthFormat(vk::Format format);
  static bool                    IsStencilFormat(vk::Format format);
  static vk::PrimitiveTopology   GetPrimitiveTopology(xiiEnum<xiiGALPrimitiveTopology> topology);
  static vk::ShaderStageFlagBits GetShaderStage(xiiGALShaderStage::Enum stage);
  static vk::PipelineStageFlags  GetPipelineStage(xiiGALShaderStage::Enum stage);
  static vk::PipelineStageFlags  GetPipelineStage(vk::ShaderStageFlags flags);
};

#include <RendererVulkan/Utils/Implementation/ConversionUtilsVulkan.inl.h>
