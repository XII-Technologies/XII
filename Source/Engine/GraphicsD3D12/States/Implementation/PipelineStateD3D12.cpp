#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <Diligent/Graphics/GraphicsEngine/interface/PipelineState.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/RenderPassD3D12.h>
#include <GraphicsD3D12/Shader/InputLayoutD3D12.h>
#include <GraphicsD3D12/Shader/ShaderD3D12.h>
#include <GraphicsD3D12/States/BlendStateD3D12.h>
#include <GraphicsD3D12/States/DepthStencilStateD3D12.h>
#include <GraphicsD3D12/States/PipelineResourceSignatureD3D12.h>
#include <GraphicsD3D12/States/PipelineStateD3D12.h>
#include <GraphicsD3D12/States/RasterizerStateD3D12.h>

xiiGALPipelineStateD3D12::xiiGALPipelineStateD3D12(const xiiGALPipelineStateCreationDescription& creationDescription) :
  xiiGALPipelineState(creationDescription)
{
}

xiiGALPipelineStateD3D12::~xiiGALPipelineStateD3D12() = default;

xiiResult xiiGALPipelineStateD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  xiiGALShaderD3D12*                    pShaderD3D12                    = static_cast<xiiGALShaderD3D12*>(pDeviceD3D12->GetShader(m_Description.m_hShader));
  xiiGALPipelineResourceSignatureD3D12* pPipelineResourceSignatureD3D12 = static_cast<xiiGALPipelineResourceSignatureD3D12*>(pDeviceD3D12->GetPipelineResourceSignature(m_Description.m_hPipelineResourceSignature));
  Diligent::IPipelineResourceSignature* ppPipelineSignatures            = {pPipelineResourceSignatureD3D12->GetPipelineResourceSignature()};

  switch (m_Description.m_PipelineType)
  {
    case xiiGALPipelineType::Graphics:
    case xiiGALPipelineType::Mesh:
    {
      xiiGALRenderPassD3D12*        pRenderPassD3D12        = static_cast<xiiGALRenderPassD3D12*>(pDeviceD3D12->GetRenderPass(m_Description.m_GraphicsPipeline.m_hRenderPass));
      xiiGALBlendStateD3D12*        pBlendStateD3D12        = static_cast<xiiGALBlendStateD3D12*>(pDeviceD3D12->GetBlendState(m_Description.m_GraphicsPipeline.m_hBlendState));
      xiiGALInputLayoutD3D12*       pInputLayoutD3D12       = static_cast<xiiGALInputLayoutD3D12*>(pDeviceD3D12->GetInputLayout(m_Description.m_GraphicsPipeline.m_hInputLayout));
      xiiGALRasterizerStateD3D12*   pRasterizerStateD3D12   = static_cast<xiiGALRasterizerStateD3D12*>(pDeviceD3D12->GetRasterizerState(m_Description.m_GraphicsPipeline.m_hRasterizerState));
      xiiGALDepthStencilStateD3D12* pDepthStencilStateD3D12 = static_cast<xiiGALDepthStencilStateD3D12*>(pDeviceD3D12->GetDepthStencilState(m_Description.m_GraphicsPipeline.m_hDepthStencilState));

      Diligent::GraphicsPipelineStateCreateInfo graphicsPipelineStateDescription = {};
      graphicsPipelineStateDescription.PSODesc.PipelineType                      = (m_Description.m_PipelineType == xiiGALPipelineType::Graphics) ? Diligent::PIPELINE_TYPE_GRAPHICS : Diligent::PIPELINE_TYPE_MESH;
      graphicsPipelineStateDescription.Flags                                     = Diligent::PSO_CREATE_FLAG_NONE;
      graphicsPipelineStateDescription.pVS                                       = pShaderD3D12->GetVertexShader();
      graphicsPipelineStateDescription.pPS                                       = pShaderD3D12->GetPixelShader();
      graphicsPipelineStateDescription.pDS                                       = pShaderD3D12->GetDomainShader();
      graphicsPipelineStateDescription.pHS                                       = pShaderD3D12->GetHullShader();
      graphicsPipelineStateDescription.pGS                                       = pShaderD3D12->GetGeometryShader();
      graphicsPipelineStateDescription.pAS                                       = pShaderD3D12->GetAmplificationShader();
      graphicsPipelineStateDescription.pMS                                       = pShaderD3D12->GetMeshShader();
      graphicsPipelineStateDescription.ppResourceSignatures                      = &ppPipelineSignatures;
      graphicsPipelineStateDescription.ResourceSignaturesCount                   = 1U;

      const auto& sourceGraphicsPipeline = m_Description.m_GraphicsPipeline;

      Diligent::GraphicsPipelineDesc& graphicsPipelineDescription = graphicsPipelineStateDescription.GraphicsPipeline;
      graphicsPipelineDescription.pRenderPass                     = pRenderPassD3D12->GetRenderPass();
      graphicsPipelineDescription.BlendDesc                       = *pBlendStateD3D12->GetBlendState();
      graphicsPipelineDescription.SampleMask                      = sourceGraphicsPipeline.m_uiSampleMask;
      graphicsPipelineDescription.RasterizerDesc                  = *pRasterizerStateD3D12->GetRasterizerState();
      graphicsPipelineDescription.DepthStencilDesc                = *pDepthStencilStateD3D12->GetDepthStencilState();
      graphicsPipelineDescription.InputLayout                     = *pInputLayoutD3D12->GetLayout();
      graphicsPipelineDescription.PrimitiveTopology               = xiiDiligentTypeConversions::GetPrimitiveTopology(m_Description.m_GraphicsPipeline.m_PrimitiveTopology);
      graphicsPipelineDescription.NumViewports                    = m_Description.m_GraphicsPipeline.m_uiViewportCount;
      graphicsPipelineDescription.SubpassIndex                    = sourceGraphicsPipeline.m_uiSubpassIndex;
      graphicsPipelineDescription.ShadingRateFlags                = xiiDiligentTypeConversions::GetPipelineShadingRateFlags(sourceGraphicsPipeline.m_ShadingRateFlags);
      graphicsPipelineDescription.SmplDesc.Count                  = sourceGraphicsPipeline.m_SampleDescription.m_uiCount;
      graphicsPipelineDescription.SmplDesc.Quality                = sourceGraphicsPipeline.m_SampleDescription.m_uiQuality;

      pDeviceD3D12->GetDevice()->CreateGraphicsPipelineState(graphicsPipelineStateDescription, &m_pPipelineState);
    }
    break;
    case xiiGALPipelineType::Compute:
    {
      Diligent::ComputePipelineStateCreateInfo computePipelineStateDescription = {};
      computePipelineStateDescription.PSODesc.PipelineType                     = Diligent::PIPELINE_TYPE_COMPUTE;
      computePipelineStateDescription.Flags                                    = Diligent::PSO_CREATE_FLAG_NONE;
      computePipelineStateDescription.pCS                                      = pShaderD3D12->GetComputeShader();
      computePipelineStateDescription.ppResourceSignatures                     = &ppPipelineSignatures;
      computePipelineStateDescription.ResourceSignaturesCount                  = 1U;

      pDeviceD3D12->GetDevice()->CreateComputePipelineState(computePipelineStateDescription, &m_pPipelineState);
    }
    break;
    case xiiGALPipelineType::RayTracing:
    {
      Diligent::RayTracingPipelineStateCreateInfo rayTracingPipelineStateDescription = {};
      rayTracingPipelineStateDescription.PSODesc.PipelineType                        = Diligent::PIPELINE_TYPE_RAY_TRACING;
      rayTracingPipelineStateDescription.Flags                                       = Diligent::PSO_CREATE_FLAG_NONE;
      rayTracingPipelineStateDescription.ppResourceSignatures                        = &ppPipelineSignatures;
      rayTracingPipelineStateDescription.ResourceSignaturesCount                     = 1U;

      /// \todo GraphicsD3D12: Implement ray tracing pipeline creation.

      pDeviceD3D12->GetDevice()->CreateRayTracingPipelineState(rayTracingPipelineStateDescription, &m_pPipelineState);
    }
    break;
    case xiiGALPipelineType::Tile:
    {
      Diligent::TilePipelineStateCreateInfo tilePipelineStateDescription = {};
      tilePipelineStateDescription.pTS                                   = pShaderD3D12->GetTileShader();
      tilePipelineStateDescription.ppResourceSignatures                  = &ppPipelineSignatures;
      tilePipelineStateDescription.ResourceSignaturesCount               = 1U;

      tilePipelineStateDescription.TilePipeline.NumRenderTargets = m_Description.m_TilePipeline.m_RenderTargetFormats.GetCount();
      tilePipelineStateDescription.TilePipeline.SampleCount      = m_Description.m_TilePipeline.m_SampleCount;
      for (xiiUInt32 i = 0; i < m_Description.m_TilePipeline.m_RenderTargetFormats.GetCount(); ++i)
      {
        tilePipelineStateDescription.TilePipeline.RTVFormats[i] = xiiDiligentTypeConversions::GetTextureFormat(m_Description.m_TilePipeline.m_RenderTargetFormats[i]);
      }

      pDeviceD3D12->GetDevice()->CreateTilePipelineState(tilePipelineStateDescription, &m_pPipelineState);
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  m_pPipelineState->CreateShaderResourceBinding(&m_pShaderResourceBinding, true);

  XII_ASSERT_DEV(m_pShaderResourceBinding != nullptr, "Failed to create shader resource binding.");

  return (m_pPipelineState != nullptr && m_pShaderResourceBinding != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALPipelineStateD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pPipelineState);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_States_Implementation_PipelineStateD3D12);
