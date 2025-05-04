#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <GraphicsFoundation/States/PipelineResourceSignature.h>
#include <GraphicsFoundation/States/PipelineState.h>
#include <GraphicsFoundation/Utilities/DescriptorHash.h>

template <typename T>
xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, xiiSharedPtr<T>& pPtr)
{
  ref_stream << reinterpret_cast<const xiiUInt64&>(pPtr.Borrow());
  return ref_stream;
}

xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiGALSampleDescription& value)
{
  ref_stream << value.m_uiCount;
  ref_stream << value.m_uiQuality;
  return ref_stream;
}

xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiGALGraphicsPipelineDescription& value)
{
  ref_stream << value.m_pBlendState;
  ref_stream << value.m_pRasterizerState;
  ref_stream << value.m_pDepthStencilState;
  ref_stream << value.m_pInputLayout;
  ref_stream << value.m_pRenderPass;
  ref_stream << value.m_uiSampleMask;
  ref_stream << value.m_PrimitiveTopology;
  ref_stream << value.m_uiViewportCount;
  ref_stream << value.m_uiSubpassIndex;
  ref_stream << value.m_ShadingRateFlags;
  ref_stream << value.m_SampleDescription;
  return ref_stream;
}

xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiGALRayTracingPipelineDescription& value)
{
  ref_stream << value.m_uiShaderRecordSize;
  ref_stream << value.m_uiMaxRecursionDepth;
  return ref_stream;
}

xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiGALRayTracingGeneralShaderGroupDescription& value)
{
  ref_stream << value.m_sName;
  ref_stream << value.m_pShader;
  return ref_stream;
}

xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiGALRayTracingTriangleHitShaderGroupDescription& value)
{
  ref_stream << value.m_sName;
  ref_stream << value.m_pClosestHitShader;
  ref_stream << value.m_pAnyHitShader;
  return ref_stream;
}

xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiGALRayTracingProceduralHitShaderGroupDescription& value)
{
  ref_stream << value.m_sName;
  ref_stream << value.m_pIntersectionShader;
  ref_stream << value.m_pClosestHitShader;
  ref_stream << value.m_pAnyHitShader;
  return ref_stream;
}

xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiGALTilePipelineDescription& value)
{
  ref_stream << value.m_SampleCount;
  ref_stream << value.m_RenderTargetFormats.GetCount();

  for (xiiUInt32 i = 0; i < value.m_RenderTargetFormats.GetCount(); ++i)
  {
    ref_stream << value.m_RenderTargetFormats[i];
  }
  return ref_stream;
}

xiiUInt32 xiiGALDescriptorHash::Hash(const xiiGALPipelineStateCreationDescription& description)
{
  xiiHashStreamWriter32 writer;

  writer << description.m_PipelineType;
  writer << description.m_pPipelineResourceSignature;
  return writer.GetHashValue();
}

bool xiiGALDescriptorHash::Equal(const xiiGALPipelineStateCreationDescription& a, const xiiGALPipelineStateCreationDescription& b)
{
  return a == b;
}

xiiUInt32 xiiGALDescriptorHash::Hash(const xiiGALGraphicsPipelineStateCreationDescription& description)
{
  xiiHashStreamWriter32 writer;

  writer << description.m_PipelineType;
  writer << description.m_pPipelineResourceSignature;
  writer << description.m_GraphicsPipeline;
  writer << description.m_pVertexShader;
  writer << description.m_pPixelShader;
  writer << description.m_pDomainShader;
  writer << description.m_pHullShader;
  writer << description.m_pGeometryShader;
  writer << description.m_pAmplificationShader;
  writer << description.m_pMeshShader;

  return writer.GetHashValue();
}

bool xiiGALDescriptorHash::Equal(const xiiGALGraphicsPipelineStateCreationDescription& a, const xiiGALGraphicsPipelineStateCreationDescription& b)
{
  return a == b;
}

xiiUInt32 xiiGALDescriptorHash::Hash(const xiiGALComputePipelineStateCreationDescription& description)
{
  xiiHashStreamWriter32 writer;

  writer << description.m_PipelineType;
  writer << description.m_pPipelineResourceSignature;
  writer << description.m_pComputeShader;

  return writer.GetHashValue();
}

bool xiiGALDescriptorHash::Equal(const xiiGALComputePipelineStateCreationDescription& a, const xiiGALComputePipelineStateCreationDescription& b)
{
  return a == b;
}

xiiUInt32 xiiGALDescriptorHash::Hash(const xiiGALRayTracingPipelineStateCreationDescription& description)
{
  xiiHashStreamWriter32 writer;

  writer << description.m_PipelineType;
  writer << description.m_pPipelineResourceSignature;
  writer << description.m_RayTracingPipeline;
  writer << description.m_sShaderRecordName;

  writer << description.m_GeneralShaders.GetCount();
  for (xiiUInt32 i = 0; i < description.m_GeneralShaders.GetCount(); ++i)
  {
    writer << description.m_GeneralShaders[i];
  }

  writer << description.m_TriangleHitShaders.GetCount();
  for (xiiUInt32 i = 0; i < description.m_TriangleHitShaders.GetCount(); ++i)
  {
    writer << description.m_TriangleHitShaders[i];
  }

  writer << description.m_ProceduralHitShaders.GetCount();
  for (xiiUInt32 i = 0; i < description.m_ProceduralHitShaders.GetCount(); ++i)
  {
    writer << description.m_ProceduralHitShaders[i];
  }

  writer << description.m_uiMaximumAttributeSize;
  writer << description.m_uiMaximumPayloadSize;

  return writer.GetHashValue();
}

bool xiiGALDescriptorHash::Equal(const xiiGALRayTracingPipelineStateCreationDescription& a, const xiiGALRayTracingPipelineStateCreationDescription& b)
{
  return a == b;
}

xiiUInt32 xiiGALDescriptorHash::Hash(const xiiGALTilePipelineStateCreationDescription& description)
{
  xiiHashStreamWriter32 writer;

  writer << description.m_PipelineType;
  writer << description.m_pPipelineResourceSignature;
  writer << description.m_TilePipeline;
  writer << description.m_pTileShader;

  return writer.GetHashValue();
}

bool xiiGALDescriptorHash::Equal(const xiiGALTilePipelineStateCreationDescription& a, const xiiGALTilePipelineStateCreationDescription& b)
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
