#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/BlendState.h>
#include <GraphicsFoundation/States/DepthStencilState.h>
#include <GraphicsFoundation/States/RasterizerState.h>

#include <d3d12.h>

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
};

#include <GraphicsD3D12/Utilities/Implementation/D3D12TypeConversions_inl.h>
