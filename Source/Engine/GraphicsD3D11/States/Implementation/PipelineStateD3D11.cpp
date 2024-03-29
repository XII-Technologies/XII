#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <Diligent/Graphics/GraphicsEngine/interface/PipelineState.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Resources/RenderPassD3D11.h>
#include <GraphicsD3D11/Shader/InputLayoutD3D11.h>
#include <GraphicsD3D11/Shader/ShaderD3D11.h>
#include <GraphicsD3D11/States/BlendStateD3D11.h>
#include <GraphicsD3D11/States/DepthStencilStateD3D11.h>
#include <GraphicsD3D11/States/PipelineResourceSignatureD3D11.h>
#include <GraphicsD3D11/States/PipelineStateD3D11.h>
#include <GraphicsD3D11/States/RasterizerStateD3D11.h>

xiiGALPipelineStateD3D11::xiiGALPipelineStateD3D11(const xiiGALPipelineStateCreationDescription& creationDescription) :
  xiiGALPipelineState(creationDescription)
{
}

xiiGALPipelineStateD3D11::~xiiGALPipelineStateD3D11() = default;

xiiResult xiiGALPipelineStateD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  xiiGALShaderD3D11*                    pShaderD3D11                    = static_cast<xiiGALShaderD3D11*>(pDeviceD3D11->GetShader(m_Description.m_hShader));
  xiiGALPipelineResourceSignatureD3D11* pPipelineResourceSignatureD3D11 = static_cast<xiiGALPipelineResourceSignatureD3D11*>(pDeviceD3D11->GetPipelineResourceSignature(m_Description.m_hPipelineResourceSignature));
  Diligent::IPipelineResourceSignature* ppPipelineSignatures            = {pPipelineResourceSignatureD3D11->GetPipelineResourceSignature()};

  switch (m_Description.m_PipelineType)
  {
    case xiiGALPipelineType::Graphics:
    case xiiGALPipelineType::Mesh:
    {
      xiiGALRenderPassD3D11*        pRenderPassD3D11        = static_cast<xiiGALRenderPassD3D11*>(pDeviceD3D11->GetRenderPass(m_Description.m_GraphicsPipeline.m_hRenderPass));
      xiiGALBlendStateD3D11*        pBlendStateD3D11        = static_cast<xiiGALBlendStateD3D11*>(pDeviceD3D11->GetBlendState(m_Description.m_GraphicsPipeline.m_hBlendState));
      xiiGALInputLayoutD3D11*       pInputLayoutD3D11       = static_cast<xiiGALInputLayoutD3D11*>(pDeviceD3D11->GetInputLayout(m_Description.m_GraphicsPipeline.m_hInputLayout));
      xiiGALRasterizerStateD3D11*   pRasterizerStateD3D11   = static_cast<xiiGALRasterizerStateD3D11*>(pDeviceD3D11->GetRasterizerState(m_Description.m_GraphicsPipeline.m_hRasterizerState));
      xiiGALDepthStencilStateD3D11* pDepthStencilStateD3D11 = static_cast<xiiGALDepthStencilStateD3D11*>(pDeviceD3D11->GetDepthStencilState(m_Description.m_GraphicsPipeline.m_hDepthStencilState));

      Diligent::GraphicsPipelineStateCreateInfo graphicsPipelineStateDescription = {};
      graphicsPipelineStateDescription.PSODesc.PipelineType                      = (m_Description.m_PipelineType == xiiGALPipelineType::Graphics) ? Diligent::PIPELINE_TYPE_GRAPHICS : Diligent::PIPELINE_TYPE_MESH;
      graphicsPipelineStateDescription.Flags                                     = Diligent::PSO_CREATE_FLAG_NONE;
      graphicsPipelineStateDescription.pVS                                       = pShaderD3D11->GetVertexShader();
      graphicsPipelineStateDescription.pPS                                       = pShaderD3D11->GetPixelShader();
      graphicsPipelineStateDescription.pDS                                       = pShaderD3D11->GetDomainShader();
      graphicsPipelineStateDescription.pHS                                       = pShaderD3D11->GetHullShader();
      graphicsPipelineStateDescription.pGS                                       = pShaderD3D11->GetGeometryShader();
      graphicsPipelineStateDescription.pAS                                       = pShaderD3D11->GetAmplificationShader();
      graphicsPipelineStateDescription.pMS                                       = pShaderD3D11->GetMeshShader();
      graphicsPipelineStateDescription.ppResourceSignatures                      = &ppPipelineSignatures;
      graphicsPipelineStateDescription.ResourceSignaturesCount                   = 1U;

      const auto& sourceGraphicsPipeline = m_Description.m_GraphicsPipeline;

      Diligent::GraphicsPipelineDesc& graphicsPipelineDescription = graphicsPipelineStateDescription.GraphicsPipeline;
      graphicsPipelineDescription.pRenderPass                     = pRenderPassD3D11->GetRenderPass();
      graphicsPipelineDescription.BlendDesc                       = *pBlendStateD3D11->GetBlendState();
      graphicsPipelineDescription.SampleMask                      = sourceGraphicsPipeline.m_uiSampleMask;
      graphicsPipelineDescription.RasterizerDesc                  = *pRasterizerStateD3D11->GetRasterizerState();
      graphicsPipelineDescription.DepthStencilDesc                = *pDepthStencilStateD3D11->GetDepthStencilState();
      graphicsPipelineDescription.InputLayout                     = *pInputLayoutD3D11->GetLayout();
      graphicsPipelineDescription.PrimitiveTopology               = xiiDiligentTypeConversions::GetPrimitiveTopology(m_Description.m_GraphicsPipeline.m_PrimitiveTopology);
      graphicsPipelineDescription.NumViewports                    = m_Description.m_GraphicsPipeline.m_uiViewportCount;
      graphicsPipelineDescription.SubpassIndex                    = sourceGraphicsPipeline.m_uiSubpassIndex;
      graphicsPipelineDescription.ShadingRateFlags                = xiiDiligentTypeConversions::GetPipelineShadingRateFlags(sourceGraphicsPipeline.m_ShadingRateFlags);
      graphicsPipelineDescription.SmplDesc.Count                  = sourceGraphicsPipeline.m_SampleDescription.m_uiCount;
      graphicsPipelineDescription.SmplDesc.Quality                = sourceGraphicsPipeline.m_SampleDescription.m_uiQuality;

      pDeviceD3D11->GetDevice()->CreateGraphicsPipelineState(graphicsPipelineStateDescription, &m_pPipelineState);
    }
    break;
    case xiiGALPipelineType::Compute:
    {
      Diligent::ComputePipelineStateCreateInfo computePipelineStateDescription = {};
      computePipelineStateDescription.PSODesc.PipelineType                     = Diligent::PIPELINE_TYPE_COMPUTE;
      computePipelineStateDescription.Flags                                    = Diligent::PSO_CREATE_FLAG_NONE;
      computePipelineStateDescription.pCS                                      = pShaderD3D11->GetComputeShader();
      computePipelineStateDescription.ppResourceSignatures                     = &ppPipelineSignatures;
      computePipelineStateDescription.ResourceSignaturesCount                  = 1U;

      pDeviceD3D11->GetDevice()->CreateComputePipelineState(computePipelineStateDescription, &m_pPipelineState);
    }
    break;
    case xiiGALPipelineType::RayTracing:
    {
      Diligent::RayTracingPipelineStateCreateInfo rayTracingPipelineStateDescription = {};
      rayTracingPipelineStateDescription.PSODesc.PipelineType                        = Diligent::PIPELINE_TYPE_RAY_TRACING;
      rayTracingPipelineStateDescription.Flags                                       = Diligent::PSO_CREATE_FLAG_NONE;
      rayTracingPipelineStateDescription.ppResourceSignatures                        = &ppPipelineSignatures;
      rayTracingPipelineStateDescription.ResourceSignaturesCount                     = 1U;

      /// \todo GraphicsD3D11: Implement ray tracing pipeline creation.

      pDeviceD3D11->GetDevice()->CreateRayTracingPipelineState(rayTracingPipelineStateDescription, &m_pPipelineState);
    }
    break;
    case xiiGALPipelineType::Tile:
    {
      Diligent::TilePipelineStateCreateInfo tilePipelineStateDescription = {};
      tilePipelineStateDescription.pTS                                   = pShaderD3D11->GetTileShader();
      tilePipelineStateDescription.ppResourceSignatures                  = &ppPipelineSignatures;
      tilePipelineStateDescription.ResourceSignaturesCount               = 1U;

      tilePipelineStateDescription.TilePipeline.NumRenderTargets = m_Description.m_TilePipeline.m_RenderTargetFormats.GetCount();
      tilePipelineStateDescription.TilePipeline.SampleCount      = m_Description.m_TilePipeline.m_SampleCount;
      for (xiiUInt32 i = 0; i < m_Description.m_TilePipeline.m_RenderTargetFormats.GetCount(); ++i)
      {
        tilePipelineStateDescription.TilePipeline.RTVFormats[i] = xiiDiligentTypeConversions::GetTextureFormat(m_Description.m_TilePipeline.m_RenderTargetFormats[i]);
      }

      pDeviceD3D11->GetDevice()->CreateTilePipelineState(tilePipelineStateDescription, &m_pPipelineState);
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  m_pPipelineState->CreateShaderResourceBinding(&m_pShaderResourceBinding, true);

  XII_ASSERT_DEV(m_pShaderResourceBinding != nullptr, "Failed to create shader resource binding.");

  return (m_pPipelineState != nullptr && m_pShaderResourceBinding != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALPipelineStateD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_D3D11_RELEASE(m_pPipelineState);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_States_Implementation_PipelineStateD3D11);
