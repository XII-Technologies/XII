#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <GraphicsFoundation/States/PipelineResourceSignature.h>
#include <GraphicsFoundation/States/PipelineState.h>
#include <GraphicsFoundation/Utilities/DescriptorHash.h>

xiiUInt32 xiiGALDescriptorHash::Hash(const xiiGALPipelineStateCreationDescription& description)
{
  xiiHashStreamWriter32 writer;

  writer << description.m_PipelineType;
  writer << description.m_uiNodeMask;

  // Graphics pipeline state.
  switch (description.m_PipelineType)
  {
    case xiiGALPipelineType::Graphics:
    case xiiGALPipelineType::Mesh:
    {
      const auto& graphicsPipeline = description.m_GraphicsPipeline;

      writer << graphicsPipeline.m_pVertexShader;
      writer << graphicsPipeline.m_pPixelShader;
      writer << graphicsPipeline.m_pDomainShader;
      writer << graphicsPipeline.m_pHullShader;
      writer << graphicsPipeline.m_pGeometryShader;
      writer << graphicsPipeline.m_pAmplificationShader;
      writer << graphicsPipeline.m_pMeshShader;
      writer << graphicsPipeline.m_pBlendState;
      writer << graphicsPipeline.m_pRasterizerState;
      writer << graphicsPipeline.m_pDepthStencilState;
      writer << graphicsPipeline.m_pInputLayout;
      writer << graphicsPipeline.m_pRenderPass;
      writer << graphicsPipeline.m_uiSampleMask;
      writer << graphicsPipeline.m_PrimitiveTopology;
      writer << graphicsPipeline.m_uiViewportCount;
      writer << graphicsPipeline.m_uiSubpassIndex;
      writer << graphicsPipeline.m_ShadingRateFlags;
      writer << graphicsPipeline.m_SampleDescription.m_uiCount;
      writer << graphicsPipeline.m_SampleDescription.m_uiQuality;
    }
    break;
    case xiiGALPipelineType::Compute:
    {
      const auto& computePipeline = description.m_ComputePipeline;

      writer << computePipeline.m_pComputeShader;
    }
    break;
    case xiiGALPipelineType::RayTracing:
    {
      const auto& rayTracingPipeline = description.m_RayTracingPipeline;

      writer << rayTracingPipeline.m_sShaderRecordName;
      writer << rayTracingPipeline.m_uiMaximumAttributeSize;
      writer << rayTracingPipeline.m_uiMaximumPayloadSize;
      writer << rayTracingPipeline.m_uiShaderRecordSize;
      writer << rayTracingPipeline.m_uiMaxRecursionDepth;

      writer << rayTracingPipeline.m_GeneralShaders.GetCount();
      for (xiiUInt32 i = 0; i < rayTracingPipeline.m_GeneralShaders.GetCount(); ++i)
      {
        const auto& generalShaderDescription = rayTracingPipeline.m_GeneralShaders[i];

        writer << generalShaderDescription.m_sName;
        writer << generalShaderDescription.m_pShader;
      }

      writer << rayTracingPipeline.m_TriangleHitShaders.GetCount();
      for (xiiUInt32 i = 0; i < rayTracingPipeline.m_TriangleHitShaders.GetCount(); ++i)
      {
        const auto& triangleHitShaderDescription = rayTracingPipeline.m_TriangleHitShaders[i];

        writer << triangleHitShaderDescription.m_sName;
        writer << triangleHitShaderDescription.m_pClosestHitShader;
        writer << triangleHitShaderDescription.m_pAnyHitShader;
      }

      writer << rayTracingPipeline.m_ProceduralHitShaders.GetCount();
      for (xiiUInt32 i = 0; i < rayTracingPipeline.m_ProceduralHitShaders.GetCount(); ++i)
      {
        const auto& proceduralHitShaderDescription = rayTracingPipeline.m_ProceduralHitShaders[i];

        writer << proceduralHitShaderDescription.m_sName;
        writer << proceduralHitShaderDescription.m_pIntersectionShader;
        writer << proceduralHitShaderDescription.m_pClosestHitShader;
        writer << proceduralHitShaderDescription.m_pAnyHitShader;
      }
    }
    break;
    case xiiGALPipelineType::Tile:
    {
      const auto& tilePipeline = description.m_TilePipeline;

      writer << tilePipeline.m_SampleCount;
      writer << tilePipeline.m_RenderTargetFormats.GetCount();

      for (xiiUInt32 i = 0; i < tilePipeline.m_RenderTargetFormats.GetCount(); ++i)
      {
        writer << tilePipeline.m_RenderTargetFormats[i];
      }
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return writer.GetHashValue();
}

bool xiiGALDescriptorHash::Equal(const xiiGALPipelineStateCreationDescription& a, const xiiGALPipelineStateCreationDescription& b)
{
  return a == b;
}

xiiUInt32 xiiGALDescriptorHash::Hash(const xiiGALPipelineResourceSignatureCreationDescription& description)
{
  xiiHashStreamWriter32 writer;

  writer << description.m_uiBindingIndex;

  writer << description.m_Resources.GetCount();
  for (xiiUInt32 i = 0; i < description.m_Resources.GetCount(); ++i)
  {
    const auto& resource = description.m_Resources[i];

    writer << i;
    writer << resource.m_sName;
    writer << resource.m_ShaderStages;
    writer << resource.m_uiArraySize;
    writer << resource.m_ResourceType;
    writer << resource.m_PipelineResourceFlags;
  }

  writer << description.m_ImmutableSamplers.GetCount();
  for (xiiUInt32 i = 0; i < description.m_ImmutableSamplers.GetCount(); ++i)
  {
    const auto& sampler = description.m_ImmutableSamplers[i];

    writer << i;
    writer << sampler.m_SamplerOrTextureName;
    writer << sampler.m_ShaderStages;
    writer << sampler.m_SamplerDescription.CalculateHash();
  }

  return writer.GetHashValue();
}

bool xiiGALDescriptorHash::Equal(const xiiGALPipelineResourceSignatureCreationDescription& a, const xiiGALPipelineResourceSignatureCreationDescription& b)
{
  return a == b;
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Utilities_Implementation_DescriptorHash);
