#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/BlendState.h>
#include <GraphicsFoundation/States/DepthStencilState.h>
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

  static vk::Format GetFormat(xiiGALTextureFormat::Enum e);
};

#include <GraphicsVulkan/Utilities/Implementation/VulkanTypeConversions_inl.h>
