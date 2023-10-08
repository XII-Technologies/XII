#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/States/BlendState.h>
#include <GraphicsFoundation/States/DepthStencilState.h>
#include <GraphicsFoundation/States/RasterizerState.h>

class XII_GRAPHICSD3D12_DLL xiiDiligentTypeConversions
{
public:
  static Diligent::RENDER_DEVICE_TYPE GetRenderDeviceType(const xiiEnum<xiiGALGraphicsDeviceType> e);

  static Diligent::BLEND_FACTOR    GetBlendFactor(xiiEnum<xiiGALBlendFactor> e);
  static Diligent::BLEND_OPERATION GetBlendOp(xiiEnum<xiiGALBlendOperation> e);

  static Diligent::COMPARISON_FUNCTION GetComparisonFunc(xiiEnum<xiiGALComparisonFunction> e);
  static Diligent::STENCIL_OP          GetStencilOp(xiiEnum<xiiGALStencilOperation> e);

  static Diligent::FILL_MODE GetFillMode(xiiEnum<xiiGALFillMode> e);
  static Diligent::CULL_MODE GetCullMode(xiiEnum<xiiGALCullMode> e);

  static Diligent::FILTER_TYPE GetFilter(xiiEnum<xiiGALFilterType> e);

  static Diligent::COLOR_MASK GetColorMask(xiiBitflags<xiiGALColorMask> mask);

  static Diligent::BIND_FLAGS       GetBindFlags(xiiBitflags<xiiGALBindFlags> e);
  static Diligent::CPU_ACCESS_FLAGS GetCPUAccessFlags(xiiBitflags<xiiGALCPUAccessFlag> e);
  static Diligent::BUFFER_MODE      GetBufferMode(xiiEnum<xiiGALBufferMode> e);
  static Diligent::USAGE            GetUsage(xiiEnum<xiiGALResourceUsage> e);

  static Diligent::RESOURCE_STATE              GetResourceState(xiiBitflags<xiiGALResourceStateFlags> e);
  static xiiBitflags<xiiGALResourceStateFlags> GetResourceState(Diligent::RESOURCE_STATE e);
};

#include <GraphicsD3D12/Utilities/Implementation/DiligentTypeConversions_inl.h>
