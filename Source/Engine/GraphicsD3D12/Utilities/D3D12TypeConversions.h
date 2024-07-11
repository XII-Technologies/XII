#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/States/BlendState.h>
#include <GraphicsFoundation/States/DepthStencilState.h>
#include <GraphicsFoundation/States/RasterizerState.h>

#include <d3d12.h>
#include <dxgitype.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

class XII_GRAPHICSD3D12_DLL xiiD3D12TypeConversions
{
public:
  /// \brief Helper function to hash D3D12 enumerations.
  template <typename T, typename R = typename std::underlying_type<T>::type>
  static R GetUnderlyingValue(T value)
  {
    return static_cast<typename std::underlying_type<T>::type>(value);
  }

  /// \brief Helper function to hash D3D12 flags.
  template <typename T>
  static auto GetUnderlyingFlagsValue(T value)
  {
    return static_cast<typename T::MaskType>(value);
  }

  static D3D12_BLEND    GetBlendFactor(xiiGALBlendFactor::Enum e);
  static D3D12_BLEND_OP GetBlendOp(xiiGALBlendOperation::Enum e);

  static D3D12_COMPARISON_FUNC GetComparisonFunc(xiiGALComparisonFunction::Enum e);
  static D3D12_STENCIL_OP      GetStencilOp(xiiGALStencilOperation::Enum e);

  static D3D12_FILL_MODE GetFillMode(xiiGALFillMode::Enum e);
  static D3D12_CULL_MODE GetCullMode(xiiGALCullMode::Enum e);

  static xiiUInt8 GetColorWriteMask(xiiBitflags<xiiGALColorMask> e);

  static DXGI_FORMAT               GetFormat(xiiGALTextureFormat::Enum e);
  static xiiGALTextureFormat::Enum GetGALFormat(DXGI_FORMAT e);

  static DXGI_MODE_SCALING         GetScalingMode(xiiGALScalingMode::Enum e);
  static xiiGALScalingMode::Enum   GetGALScalingMode(DXGI_MODE_SCALING e);
  static DXGI_MODE_SCANLINE_ORDER  GetScanLineOrder(xiiGALScanLineOrder::Enum e);
  static xiiGALScanLineOrder::Enum GetGALScanLineOrder(DXGI_MODE_SCANLINE_ORDER e);

  static D3D12_FILTER               GetFilter(xiiGALFilterType::Enum minFilter, xiiGALFilterType::Enum magFilter, xiiGALFilterType::Enum mipFilter);
  static D3D12_TEXTURE_ADDRESS_MODE GetTextureAddressMode(xiiGALTextureAddressMode::Enum e);

  static D3D12_QUERY_HEAP_TYPE GetQueryType(xiiGALQueryType::Enum e);

  static DXGI_FORMAT GetDXGIFormatFromType(xiiGALValueType::Enum e, xiiUInt32 uiComponentCount, bool bIsNormalized);

  static D3D_PRIMITIVE_TOPOLOGY GetPrimitiveTopology(xiiGALPrimitiveTopology::Enum e);

  static D3D12_INPUT_CLASSIFICATION GetElementFrequency(xiiGALInputElementFrequency::Enum e);

  static xiiUInt32 CalculateSubResourceIndex(xiiUInt32 uiMipSlice, xiiUInt32 uiArraySlice, xiiUInt32 uiMipLevelCount);
};

#include <GraphicsD3D12/Utilities/Implementation/D3D12TypeConversions_inl.h>
