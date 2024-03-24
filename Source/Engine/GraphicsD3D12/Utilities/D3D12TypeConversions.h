#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>
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

  static D3D12_BLEND    GetD3D12BlendFactor(xiiEnum<xiiGALBlendFactor> e);
  static D3D12_BLEND_OP GetD3D12BlendOp(xiiEnum<xiiGALBlendOperation> e);

  static D3D12_COMPARISON_FUNC GetD3D12ComparisonFunc(xiiEnum<xiiGALComparisonFunction> e);
  static D3D12_STENCIL_OP      GetD3D12StencilOp(xiiEnum<xiiGALStencilOperation> e);

  static D3D12_FILL_MODE GetD3D12FillMode(xiiEnum<xiiGALFillMode> e);
  static D3D12_CULL_MODE GetD3D12CullMode(xiiEnum<xiiGALCullMode> e);

  static xiiUInt8 GetColorWriteMask(xiiBitflags<xiiGALColorMask> e);

  static DXGI_FORMAT                  GetD3D12Format(xiiEnum<xiiGALTextureFormat> e);
  static xiiEnum<xiiGALTextureFormat> GetGALFormat(DXGI_FORMAT e);

  static DXGI_MODE_SCALING            GetD3D12ScalingMode(xiiEnum<xiiGALScalingMode> e);
  static xiiEnum<xiiGALScalingMode>   GetGALScalingMode(DXGI_MODE_SCALING e);
  static DXGI_MODE_SCANLINE_ORDER     GetD3D12ScanLineOrder(xiiEnum<xiiGALScanLineOrder> e);
  static xiiEnum<xiiGALScanLineOrder> GetGALScanLineOrder(DXGI_MODE_SCANLINE_ORDER e);

  static D3D12_FILTER GetD3D12Filter(xiiEnum<xiiGALFilterType> minFilter, xiiEnum<xiiGALFilterType> magFilter, xiiEnum<xiiGALFilterType> mipFilter);
  static D3D12_TEXTURE_ADDRESS_MODE GetD3D12TextureAddressMode(xiiEnum<xiiGALTextureAddressMode> e);

  static D3D12_QUERY_HEAP_TYPE GetD3D12QueryType(xiiEnum<xiiGALQueryType> e);
};

#include <GraphicsD3D12/Utilities/Implementation/D3D12TypeConversions_inl.h>
