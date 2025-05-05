#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <Foundation/Containers/IterateBits.h>

#include <GraphicsD3D11/CommandEncoder/CommandListD3D11.h>
#include <GraphicsD3D11/Resources/BufferD3D11.h>
#include <GraphicsD3D11/Resources/BufferViewD3D11.h>
#include <GraphicsD3D11/Resources/RenderPassD3D11.h>
#include <GraphicsD3D11/Resources/SamplerD3D11.h>
#include <GraphicsD3D11/Resources/TextureD3D11.h>
#include <GraphicsD3D11/Resources/TextureViewD3D11.h>
#include <GraphicsD3D11/Shader/InputLayoutD3D11.h>
#include <GraphicsD3D11/Shader/ShaderD3D11.h>
#include <GraphicsD3D11/States/PipelineResourceSignatureD3D11.h>
#include <GraphicsD3D11/States/PipelineStateD3D11.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALGraphicsPipelineStateD3D11, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALComputePipelineStateD3D11, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALRayTracingPipelineStateD3D11, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTilePipelineStateD3D11, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALGraphicsPipelineStateD3D11::xiiGALGraphicsPipelineStateD3D11(xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11, const xiiGALGraphicsPipelineStateCreationDescription& creationDescription) :
  xiiGALGraphicsPipelineState(pDeviceD3D11, creationDescription)
{
}

xiiGALGraphicsPipelineStateD3D11::~xiiGALGraphicsPipelineStateD3D11() = default;

xiiResult xiiGALGraphicsPipelineStateD3D11::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11 = m_pDevice.Downcast<xiiGALDeviceD3D11>();

  m_pPipelineResourceSignatureD3D11 = m_Description.m_pPipelineResourceSignature.Downcast<xiiGALPipelineResourceSignatureD3D11>();

  m_pVertexShaderD3D11   = m_Description.m_pVertexShader.Downcast<xiiGALShaderD3D11>();
  m_pPixelShaderD3D11    = m_Description.m_pPixelShader.Downcast<xiiGALShaderD3D11>();
  m_pDomainShaderD3D11   = m_Description.m_pDomainShader.Downcast<xiiGALShaderD3D11>();
  m_pHullShaderD3D11     = m_Description.m_pHullShader.Downcast<xiiGALShaderD3D11>();
  m_pGeometryShaderD3D11 = m_Description.m_pGeometryShader.Downcast<xiiGALShaderD3D11>();

  m_pRenderPassD3D11        = m_Description.m_GraphicsPipeline.m_pRenderPass.Downcast<xiiGALRenderPassD3D11>();
  m_pBlendStateD3D11        = m_Description.m_GraphicsPipeline.m_pBlendState.Downcast<xiiGALBlendStateD3D11>();
  m_pInputLayoutD3D11       = m_Description.m_GraphicsPipeline.m_pInputLayout.Downcast<xiiGALInputLayoutD3D11>();
  m_pRasterizerStateD3D11   = m_Description.m_GraphicsPipeline.m_pRasterizerState.Downcast<xiiGALRasterizerStateD3D11>();
  m_pDepthStencilStateD3D11 = m_Description.m_GraphicsPipeline.m_pDepthStencilState.Downcast<xiiGALDepthStencilStateD3D11>();

  return XII_SUCCESS;
}

xiiGALGraphicsPipelineStateD3D11::ShaderType::Enum xiiGALGraphicsPipelineStateD3D11::ShaderType::GetIndex(xiiBitflags<xiiGALShaderType> type)
{
  switch (type.GetValue())
  {
    case xiiGALShaderType::Vertex:
      return ShaderType::Vertex;
    case xiiGALShaderType::Pixel:
      return ShaderType::Pixel;
    case xiiGALShaderType::Geometry:
      return ShaderType::Geometry;
    case xiiGALShaderType::Hull:
      return ShaderType::Hull;
    case xiiGALShaderType::Domain:
      return ShaderType::Domain;
    case xiiGALShaderType::Compute:
      return ShaderType::Compute;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return ShaderType::Unknown;
}

///////////////////////////////////////////////////////////////////

xiiGALComputePipelineStateD3D11::xiiGALComputePipelineStateD3D11(xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11, const xiiGALComputePipelineStateCreationDescription& creationDescription) :
  xiiGALComputePipelineState(pDeviceD3D11, creationDescription)
{
}

xiiGALComputePipelineStateD3D11::~xiiGALComputePipelineStateD3D11() = default;

xiiResult xiiGALComputePipelineStateD3D11::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11 = m_pDevice.Downcast<xiiGALDeviceD3D11>();

  m_pPipelineResourceSignatureD3D11 = m_Description.m_pPipelineResourceSignature.Downcast<xiiGALPipelineResourceSignatureD3D11>();
  m_pComputeShaderD3D11             = m_Description.m_pComputeShader.Downcast<xiiGALShaderD3D11>();

  return XII_SUCCESS;
}

///////////////////////////////////////////////////////////////////

xiiGALRayTracingPipelineStateD3D11::xiiGALRayTracingPipelineStateD3D11(xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11, const xiiGALRayTracingPipelineStateCreationDescription& creationDescription) :
  xiiGALRayTracingPipelineState(pDeviceD3D11, creationDescription)
{
}

xiiGALRayTracingPipelineStateD3D11::~xiiGALRayTracingPipelineStateD3D11() = default;

xiiResult xiiGALRayTracingPipelineStateD3D11::InitPlatform()
{
  return XII_FAILURE;
}

///////////////////////////////////////////////////////////////////

xiiGALTilePipelineStateD3D11::xiiGALTilePipelineStateD3D11(xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11, const xiiGALTilePipelineStateCreationDescription& creationDescription) :
  xiiGALTilePipelineState(pDeviceD3D11, creationDescription)
{
}

xiiGALTilePipelineStateD3D11::~xiiGALTilePipelineStateD3D11() = default;

xiiResult xiiGALTilePipelineStateD3D11::InitPlatform()
{
  return XII_FAILURE;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_States_Implementation_PipelineStateD3D11);
