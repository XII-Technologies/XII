#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/BlendState.h>
#include <GraphicsFoundation/States/DepthStencilState.h>
#include <GraphicsFoundation/States/RasterizerState.h>

class XII_GRAPHICSD3D12_DLL xiiDiligentTypeConversions
{
public:
  static Diligent::RENDER_DEVICE_TYPE GetRenderDeviceType(const xiiEnum<xiiGALGraphicsDeviceType> e);

  static Diligent::BLEND_FACTOR    GetBlendFactor(xiiEnum<xiiGALBlendFactor> e);
  static Diligent::BLEND_OPERATION GetBlendOperation(xiiEnum<xiiGALBlendOperation> e);

  static Diligent::STENCIL_OP          GetStencilOperation(xiiEnum<xiiGALStencilOperation> e);
  static Diligent::COMPARISON_FUNCTION GetComparisonFunction(xiiEnum<xiiGALComparisonFunction> e);

  static Diligent::FILTER_TYPE GetFilter(xiiEnum<xiiGALFilterType> e);

  static Diligent::COLOR_MASK GetColorMask(xiiBitflags<xiiGALColorMask> mask);
};

#include <GraphicsD3D12/Utilities/Implementation/DiligentTypeConversions_inl.h>
