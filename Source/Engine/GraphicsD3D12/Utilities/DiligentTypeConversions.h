#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/BufferView.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Resources/Query.h>
#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/TextureView.h>
#include <GraphicsFoundation/Resources/TopLevelAS.h>
#include <GraphicsFoundation/Shader/InputLayout.h>
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

  static Diligent::RESOURCE_DIMENSION GetResourceDimension(xiiEnum<xiiGALResourceDimension> e);
  static Diligent::TEXTURE_FORMAT     GetTextureFormat(xiiEnum<xiiGALTextureFormat> e);
  static xiiEnum<xiiGALTextureFormat> GetGALTextureFormat(Diligent::TEXTURE_FORMAT e);
  static Diligent::MISC_TEXTURE_FLAGS GetMiscTextureFlags(xiiBitflags<xiiGALMiscTextureFlags> e);

  static Diligent::FENCE_TYPE GetFenceType(xiiEnum<xiiGALFenceType> e);

  static Diligent::QUERY_TYPE GetQueryType(xiiEnum<xiiGALQueryType> e);

  static Diligent::ATTACHMENT_LOAD_OP  GetLoadOperation(xiiEnum<xiiGALAttachmentLoadOperation> e);
  static Diligent::ATTACHMENT_STORE_OP GetStoreOperation(xiiEnum<xiiGALAttachmentStoreOperation> e);

  static Diligent::BUFFER_VIEW_TYPE GetBufferViewType(xiiEnum<xiiGALBufferViewType> e);

  static Diligent::TEXTURE_VIEW_TYPE GetTextureViewType(xiiEnum<xiiGALTextureViewType> e);

  static Diligent::TEXTURE_ADDRESS_MODE GetTextureAddress(xiiEnum<xiiGALTextureAddressMode> e);

  static Diligent::VALUE_TYPE GetValueType(xiiEnum<xiiGALValueType> e);

  static Diligent::UAV_ACCESS_FLAG           GetUAVAccessFlags(xiiBitflags<xiiGALUnorderedAccessViewFlags> e);
  static Diligent::TEXTURE_VIEW_FLAGS        GetTextureViewFlags(xiiBitflags<xiiGALTextureViewFlags> e);
  static Diligent::TEXTURE_COMPONENT_SWIZZLE GetComponentSwizzle(xiiEnum<xiiGALTextureComponentSwizzle> e);

  static Diligent::RAYTRACING_BUILD_AS_FLAGS GetRayTracingBuildASFlags(xiiBitflags<xiiGALRayTracingBuildASFlags> e);
  static Diligent::HIT_GROUP_BINDING_MODE    GetHitGroupBindingMode(xiiEnum<xiiGALHitGroupBindingMode> e);
  static xiiEnum<xiiGALHitGroupBindingMode>  GetGALHitGroupBindingMode(Diligent::HIT_GROUP_BINDING_MODE e);

  static Diligent::ADAPTER_TYPE           GetAdapterType(xiiEnum<xiiGALDeviceAdapterType> e);
  static xiiEnum<xiiGALDeviceAdapterType> GetGALAdapterType(Diligent::ADAPTER_TYPE e);
  static Diligent::DEVICE_FEATURE_STATE   GetDeviceFeatureState(xiiEnum<xiiGALDeviceFeatureState> e);
  static Diligent::VALIDATION_LEVEL       GetDeviceValidationLevel(xiiEnum<xiiGALDeviceValidationLevel> e);

  static Diligent::SHADER_TYPE GetShaderTypeFlags(xiiBitflags<xiiGALShaderStage> e);

  static Diligent::INPUT_ELEMENT_FREQUENCY GetElementFrequency(xiiEnum<xiiGALInputElementFrequency> e);

  static Diligent::VALUE_TYPE GetDiligentValueType(Diligent::TEXTURE_FORMAT e);
  static bool                 GetFormatNormalized(Diligent::TEXTURE_FORMAT e);

  static Diligent::SCALING_MODE      GetScalingMode(xiiEnum<xiiGALScalingMode> e);
  static Diligent::SCANLINE_ORDER    GetScanLineOrder(xiiEnum<xiiGALScanLineOrder> e);
  static Diligent::SURFACE_TRANSFORM GetSurfaceTransform(xiiEnum<xiiGALSurfaceTransform> e);

  static Diligent::SWAP_CHAIN_USAGE_FLAGS GetSwapChainUsageFlags(xiiBitflags<xiiGALSwapChainUsageFlags> e);

  static xiiBitflags<xiiGALBindFlags>        GetGALBindFlags(Diligent::BIND_FLAGS e);
  static xiiEnum<xiiGALResourceUsage>        GetGALUsage(Diligent::USAGE e);
  static xiiBitflags<xiiGALCPUAccessFlag>    GetGALCPUAccessFlags(Diligent::CPU_ACCESS_FLAGS e);
  static xiiBitflags<xiiGALMiscTextureFlags> GetGALMiscTextureFlags(Diligent::MISC_TEXTURE_FLAGS e);
};

#include <GraphicsD3D12/Utilities/Implementation/DiligentTypeConversions_inl.h>
