#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/States/BlendState.h>
#include <GraphicsFoundation/States/DepthStencilState.h>
#include <GraphicsFoundation/States/PipelineResourceSignature.h>
#include <GraphicsFoundation/States/RasterizerState.h>

class XII_GRAPHICSVULKAN_DLL xiiVulkanTypeConversions
{
public:
  /// \brief Helper function to hash Vulkan enumerations.
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

  static vk::BlendOp     GetBlendOp(xiiGALBlendOperation::Enum e);
  static vk::BlendFactor GetBlendFactor(xiiGALBlendFactor::Enum e);
  static vk::LogicOp     GetLogicOp(xiiGALLogicOperation::Enum e);

  static vk::CompareOp GetCompareOp(xiiGALComparisonFunction::Enum e);
  static vk::StencilOp GetStencilOp(xiiGALStencilOperation::Enum e);

  static vk::PolygonMode      GetPolygonMode(xiiGALFillMode::Enum e);
  static vk::CullModeFlagBits GetCullMode(xiiGALCullMode::Enum e);

  static vk::ColorComponentFlags GetColorWriteMask(xiiBitflags<xiiGALColorMask> e);

  static vk::Format                 GetFormat(xiiGALResourceFormat::Enum e);
  static xiiGALResourceFormat::Enum GetGALFormat(vk::Format e);

  static vk::ShaderStageFlags          GetShaderStageFlags(xiiBitflags<xiiGALShaderType> e);
  static xiiBitflags<xiiGALShaderType> GetGALShaderStageFlags(vk::ShaderStageFlags e);

  static vk::Extent2D                        ShadingRateToFragmentSize(xiiBitflags<xiiGALShadingRateFlags> e);
  static xiiBitflags<xiiGALShadingRateFlags> FragmentSizeToShadingRate(vk::Extent2D e);

  static xiiBitflags<xiiGALCommandQueueType> GetGALCommandQueueType(vk::QueueFlags e);

  static vk::SurfaceTransformFlagsKHR GetSurfaceTransform(xiiGALSurfaceTransform::Enum e);
  static xiiGALSurfaceTransform::Enum GetGALSurfaceTransform(vk::SurfaceTransformFlagsKHR e);

  static vk::Filter             GetFilter(xiiGALFilterType::Enum e);
  static vk::SamplerMipmapMode  GetSamplerMipMapMode(xiiGALFilterType::Enum e);
  static vk::SamplerAddressMode GetSamplerAddressMode(xiiGALTextureAddressMode::Enum e);
  static vk::BorderColor        GetBorderColor(const xiiColor& c);

  static vk::VertexInputRate GetFrequency(xiiGALInputElementFrequency::Enum e);

  static vk::AttachmentLoadOp  GetAttachmentLoadOperation(xiiGALAttachmentLoadOperation::Enum e);
  static vk::AttachmentStoreOp GetAttachmentStoreOperation(xiiGALAttachmentStoreOperation::Enum e);

  static vk::ImageLayout                       GetImageLayout(xiiBitflags<xiiGALResourceStateFlags> e, bool bIsInsideRenderPass = false, bool bFragDensityMapInsteadOfShadingRate = false);
  static xiiBitflags<xiiGALResourceStateFlags> GetResourceState(vk::ImageLayout e);

  static vk::PipelineStageFlags                GetPipelineStageFlags(xiiBitflags<xiiGALPipelineStageFlags> e);
  static vk::PipelineStageFlags                GetPipelineStageFlags(xiiBitflags<xiiGALResourceStateFlags> e);
  static vk::AccessFlags                       GetAccessFlags(xiiBitflags<xiiGALAccessFlags> e);
  static vk::AccessFlags                       GetAccessFlags(xiiBitflags<xiiGALResourceStateFlags> e);
  static xiiBitflags<xiiGALResourceStateFlags> GetResourceState(vk::AccessFlags e);

  static vk::ComponentSwizzle GetComponentSwizzle(xiiGALTextureComponentSwizzle::Enum e);
  static vk::ComponentMapping GetComponentMapping(const xiiGALTextureComponentMapping& mapping);

  static xiiBitflags<xiiGALSparseTextureFlags> GetSparseTextureFlags(vk::SparseImageFormatFlags e);

  static vk::ImageUsageFlags GetImageUsageFlags(xiiBitflags<xiiGALBindFlags> bindFlags, bool bIsMemoryless, bool bFragmentDensityMapInsteadOfShadingRate);

  static vk::DescriptorType GetDescriptorType(const xiiGALPipelineResourceDescription& resourceDescription);
};

#include <GraphicsVulkan/Utilities/Implementation/VulkanTypeConversions_inl.h>
