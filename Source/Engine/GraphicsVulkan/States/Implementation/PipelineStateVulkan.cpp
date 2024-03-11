#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <Diligent/Graphics/GraphicsEngine/interface/PipelineState.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/RenderPassVulkan.h>
#include <GraphicsVulkan/Shader/InputLayoutVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>
#include <GraphicsVulkan/States/BlendStateVulkan.h>
#include <GraphicsVulkan/States/DepthStencilStateVulkan.h>
#include <GraphicsVulkan/States/PipelineResourceSignatureVulkan.h>
#include <GraphicsVulkan/States/PipelineStateVulkan.h>
#include <GraphicsVulkan/States/RasterizerStateVulkan.h>

xiiGALPipelineStateVulkan::xiiGALPipelineStateVulkan(const xiiGALPipelineStateCreationDescription& creationDescription) :
  xiiGALPipelineState(creationDescription)
{
}

xiiGALPipelineStateVulkan::~xiiGALPipelineStateVulkan() = default;

xiiResult xiiGALPipelineStateVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  xiiGALShaderVulkan*                    pShaderVulkan                    = static_cast<xiiGALShaderVulkan*>(pDeviceVulkan->GetShader(m_Description.m_hShader));
  xiiGALPipelineResourceSignatureVulkan* pPipelineResourceSignatureVulkan = static_cast<xiiGALPipelineResourceSignatureVulkan*>(pDeviceVulkan->GetPipelineResourceSignature(m_Description.m_hPipelineResourceSignature));
  Diligent::IPipelineResourceSignature*  ppPipelineSignatures             = {pPipelineResourceSignatureVulkan->GetPipelineResourceSignature()};

  switch (m_Description.m_PipelineType)
  {
    case xiiGALPipelineType::Graphics:
    case xiiGALPipelineType::Mesh:
    {
      xiiGALRenderPassVulkan*        pRenderPassVulkan        = static_cast<xiiGALRenderPassVulkan*>(pDeviceVulkan->GetRenderPass(m_Description.m_GraphicsPipeline.m_hRenderPass));
      xiiGALBlendStateVulkan*        pBlendStateVulkan        = static_cast<xiiGALBlendStateVulkan*>(pDeviceVulkan->GetBlendState(m_Description.m_GraphicsPipeline.m_hBlendState));
      xiiGALInputLayoutVulkan*       pInputLayoutVulkan       = static_cast<xiiGALInputLayoutVulkan*>(pDeviceVulkan->GetInputLayout(m_Description.m_GraphicsPipeline.m_hInputLayout));
      xiiGALRasterizerStateVulkan*   pRasterizerStateVulkan   = static_cast<xiiGALRasterizerStateVulkan*>(pDeviceVulkan->GetRasterizerState(m_Description.m_GraphicsPipeline.m_hRasterizerState));
      xiiGALDepthStencilStateVulkan* pDepthStencilStateVulkan = static_cast<xiiGALDepthStencilStateVulkan*>(pDeviceVulkan->GetDepthStencilState(m_Description.m_GraphicsPipeline.m_hDepthStencilState));

      Diligent::GraphicsPipelineStateCreateInfo graphicsPipelineStateDescription = {};
      graphicsPipelineStateDescription.PSODesc.PipelineType                      = (m_Description.m_PipelineType == xiiGALPipelineType::Graphics) ? Diligent::PIPELINE_TYPE_GRAPHICS : Diligent::PIPELINE_TYPE_MESH;
      graphicsPipelineStateDescription.Flags                                     = Diligent::PSO_CREATE_FLAG_NONE;
      graphicsPipelineStateDescription.pVS                                       = pShaderVulkan->GetVertexShader();
      graphicsPipelineStateDescription.pPS                                       = pShaderVulkan->GetPixelShader();
      graphicsPipelineStateDescription.pDS                                       = pShaderVulkan->GetDomainShader();
      graphicsPipelineStateDescription.pHS                                       = pShaderVulkan->GetHullShader();
      graphicsPipelineStateDescription.pGS                                       = pShaderVulkan->GetGeometryShader();
      graphicsPipelineStateDescription.pAS                                       = pShaderVulkan->GetAmplificationShader();
      graphicsPipelineStateDescription.pMS                                       = pShaderVulkan->GetMeshShader();
      graphicsPipelineStateDescription.ppResourceSignatures                      = &ppPipelineSignatures;
      graphicsPipelineStateDescription.ResourceSignaturesCount                   = 1U;

      const auto& sourceGraphicsPipeline = m_Description.m_GraphicsPipeline;

      Diligent::GraphicsPipelineDesc& graphicsPipelineDescription = graphicsPipelineStateDescription.GraphicsPipeline;
      graphicsPipelineDescription.pRenderPass                     = pRenderPassVulkan->GetRenderPass();
      graphicsPipelineDescription.BlendDesc                       = *pBlendStateVulkan->GetBlendState();
      graphicsPipelineDescription.SampleMask                      = sourceGraphicsPipeline.m_uiSampleMask;
      graphicsPipelineDescription.RasterizerDesc                  = *pRasterizerStateVulkan->GetRasterizerState();
      graphicsPipelineDescription.DepthStencilDesc                = *pDepthStencilStateVulkan->GetDepthStencilState();
      graphicsPipelineDescription.InputLayout                     = *pInputLayoutVulkan->GetLayout();
      graphicsPipelineDescription.PrimitiveTopology               = xiiDiligentTypeConversions::GetPrimitiveTopology(m_Description.m_GraphicsPipeline.m_PrimitiveTopology);
      graphicsPipelineDescription.NumViewports                    = m_Description.m_GraphicsPipeline.m_uiViewportCount;
      graphicsPipelineDescription.SubpassIndex                    = sourceGraphicsPipeline.m_uiSubpassIndex;
      graphicsPipelineDescription.ShadingRateFlags                = xiiDiligentTypeConversions::GetPipelineShadingRateFlags(sourceGraphicsPipeline.m_ShadingRateFlags);
      graphicsPipelineDescription.SmplDesc.Count                  = sourceGraphicsPipeline.m_SampleDescription.m_uiCount;
      graphicsPipelineDescription.SmplDesc.Quality                = sourceGraphicsPipeline.m_SampleDescription.m_uiQuality;

      pDeviceVulkan->GetDevice()->CreateGraphicsPipelineState(graphicsPipelineStateDescription, &m_pPipelineState);
    }
    break;
    case xiiGALPipelineType::Compute:
    {
      Diligent::ComputePipelineStateCreateInfo computePipelineStateDescription = {};
      computePipelineStateDescription.PSODesc.PipelineType                     = Diligent::PIPELINE_TYPE_COMPUTE;
      computePipelineStateDescription.Flags                                    = Diligent::PSO_CREATE_FLAG_NONE;
      computePipelineStateDescription.pCS                                      = pShaderVulkan->GetComputeShader();
      computePipelineStateDescription.ppResourceSignatures                     = &ppPipelineSignatures;
      computePipelineStateDescription.ResourceSignaturesCount                  = 1U;

      pDeviceVulkan->GetDevice()->CreateComputePipelineState(computePipelineStateDescription, &m_pPipelineState);
    }
    break;
    case xiiGALPipelineType::RayTracing:
    {
      Diligent::RayTracingPipelineStateCreateInfo rayTracingPipelineStateDescription = {};
      rayTracingPipelineStateDescription.PSODesc.PipelineType                        = Diligent::PIPELINE_TYPE_RAY_TRACING;
      rayTracingPipelineStateDescription.Flags                                       = Diligent::PSO_CREATE_FLAG_NONE;
      rayTracingPipelineStateDescription.ppResourceSignatures                        = &ppPipelineSignatures;
      rayTracingPipelineStateDescription.ResourceSignaturesCount                     = 1U;

      /// \todo GraphicsVulkan: Implement ray tracing pipeline creation.

      pDeviceVulkan->GetDevice()->CreateRayTracingPipelineState(rayTracingPipelineStateDescription, &m_pPipelineState);
    }
    break;
    case xiiGALPipelineType::Tile:
    {
      Diligent::TilePipelineStateCreateInfo tilePipelineStateDescription = {};
      tilePipelineStateDescription.pTS                                   = pShaderVulkan->GetTileShader();
      tilePipelineStateDescription.ppResourceSignatures                  = &ppPipelineSignatures;
      tilePipelineStateDescription.ResourceSignaturesCount               = 1U;

      tilePipelineStateDescription.TilePipeline.NumRenderTargets = m_Description.m_TilePipeline.m_RenderTargetFormats.GetCount();
      tilePipelineStateDescription.TilePipeline.SampleCount      = m_Description.m_TilePipeline.m_SampleCount;
      for (xiiUInt32 i = 0; i < m_Description.m_TilePipeline.m_RenderTargetFormats.GetCount(); ++i)
      {
        tilePipelineStateDescription.TilePipeline.RTVFormats[i] = xiiDiligentTypeConversions::GetTextureFormat(m_Description.m_TilePipeline.m_RenderTargetFormats[i]);
      }

      pDeviceVulkan->GetDevice()->CreateTilePipelineState(tilePipelineStateDescription, &m_pPipelineState);
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  m_pPipelineState->CreateShaderResourceBinding(&m_pShaderResourceBinding, true);

  XII_ASSERT_DEV(m_pShaderResourceBinding != nullptr, "Failed to create shader resource binding.");

  return (m_pPipelineState != nullptr && m_pShaderResourceBinding != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALPipelineStateVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pPipelineState);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_PipelineStateVulkan);
