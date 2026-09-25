/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
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

xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiGALRenderPassAttachmentDescription& value)
{
  ref_stream << value.m_Format;
  ref_stream << value.m_uiSampleCount;
  ref_stream << value.m_LoadOperation;
  ref_stream << value.m_StoreOperation;
  ref_stream << value.m_StencilLoadOperation;
  ref_stream << value.m_StencilStoreOperation;
  ref_stream << value.m_InitialStateFlags;
  ref_stream << value.m_FinalStateFlags;

  return ref_stream;
}

xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiGALAttachmentReferenceDescription& value)
{
  ref_stream << value.m_uiAttachmentIndex;
  ref_stream << value.m_ResourceStateFlags;

  return ref_stream;
}

xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiGALShadingRateAttachmentDescription& value)
{
  ref_stream << value.m_AttachmentReference;
  ref_stream << value.m_TileSize;

  return ref_stream;
}

xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiGALDepthResolveDescription& value)
{
  ref_stream << value.m_Attachment;
  ref_stream << value.m_DepthMode;
  ref_stream << value.m_StencilMode;

  return ref_stream;
}

xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiGALSubPassDescription& value)
{
  ref_stream << value.m_InputAttachments.GetCount();
  for (const auto& attachment : value.m_InputAttachments)
    ref_stream << attachment;

  ref_stream << value.m_RenderTargetAttachments.GetCount();
  for (const auto& attachment : value.m_RenderTargetAttachments)
    ref_stream << attachment;

  ref_stream << value.m_ResolveAttachments.GetCount();
  for (const auto& attachment : value.m_ResolveAttachments)
    ref_stream << attachment;

  ref_stream << value.m_DepthStencilAttachment.GetCount();
  for (const auto& attachment : value.m_DepthStencilAttachment)
    ref_stream << attachment;

  ref_stream << value.m_DepthResolveAttachment.GetCount();
  for (const auto& attachment : value.m_DepthResolveAttachment)
    ref_stream << attachment;

  ref_stream << value.m_PreserveAttachments.GetCount();
  for (const xiiUInt32 uiAttachment : value.m_PreserveAttachments)
    ref_stream << uiAttachment;

  ref_stream << value.m_ShadingRateAttachment.GetCount();
  for (const auto& attachment : value.m_ShadingRateAttachment)
    ref_stream << attachment;

  return ref_stream;
}

xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiGALSubPassDependencyDescription& value)
{
  ref_stream << value.m_uiSourceSubPass;
  ref_stream << value.m_uiDestinationSubPass;
  ref_stream << value.m_SourceStageFlags;
  ref_stream << value.m_DestinationStageFlags;
  ref_stream << value.m_SourceAccessFlags;
  ref_stream << value.m_DestinationAccessFlags;

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

xiiUInt32 xiiGALDescriptorHash::Hash(const xiiGALBlendStateCreationDescription& blendStateDescription)
{
  xiiHashStreamWriter32 writer;

  writer << blendStateDescription.m_bAlphaToCoverage;
  writer << blendStateDescription.m_bIndependentBlend;
  writer << blendStateDescription.m_LogicOperationEnable;
  writer << blendStateDescription.m_LogicOperation;

  writer << blendStateDescription.m_RenderTargets.GetCount();
  for (xiiUInt32 i = 0; i < blendStateDescription.m_RenderTargets.GetCount(); ++i)
  {
    const xiiGALRenderTargetBlendDescription& renderTargetBlend = blendStateDescription.m_RenderTargets[i];

    writer << renderTargetBlend.m_bBlendEnable;
    writer << renderTargetBlend.m_SourceBlend;
    writer << renderTargetBlend.m_DestinationBlend;
    writer << renderTargetBlend.m_BlendOperation;
    writer << renderTargetBlend.m_SourceBlendAlpha;
    writer << renderTargetBlend.m_DestinationBlendAlpha;
    writer << renderTargetBlend.m_BlendOperationAlpha;
    writer << renderTargetBlend.m_ColorMask;
  }

  return writer.GetHashValue();
}

bool xiiGALDescriptorHash::Equal(const xiiGALBlendStateCreationDescription& a, const xiiGALBlendStateCreationDescription& b)
{
  return a == b;
}

xiiUInt32 xiiGALDescriptorHash::Hash(const xiiGALRenderPassCreationDescription& renderPassDescription)
{
  xiiHashStreamWriter32 writer;

  writer << renderPassDescription.m_Attachments.GetCount();

  for (xiiUInt32 i = 0; i < renderPassDescription.m_Attachments.GetCount(); ++i)
  {
    writer << renderPassDescription.m_Attachments[i];
  }

  writer << renderPassDescription.m_SubPasses.GetCount();

  for (xiiUInt32 i = 0; i < renderPassDescription.m_SubPasses.GetCount(); ++i)
  {
    writer << renderPassDescription.m_SubPasses[i];
  }

  writer << renderPassDescription.m_Dependencies.GetCount();

  for (xiiUInt32 i = 0; i < renderPassDescription.m_Dependencies.GetCount(); ++i)
  {
    writer << renderPassDescription.m_Dependencies[i];
  }

  return writer.GetHashValue();
}

bool xiiGALDescriptorHash::Equal(const xiiGALRenderPassCreationDescription& a, const xiiGALRenderPassCreationDescription& b)
{
  return a == b;
}

xiiUInt32 xiiGALDescriptorHash::Hash(const xiiGALFramebufferCreationDescription& framebufferDescription)
{
  xiiHashStreamWriter32 writer;

  writer << framebufferDescription.m_pRenderPass;
  writer << framebufferDescription.m_FramebufferSize.width;
  writer << framebufferDescription.m_FramebufferSize.height;
  writer << framebufferDescription.m_uiArraySliceCount;
  writer << framebufferDescription.m_Attachments.GetCount();

  for (xiiUInt32 i = 0; i < framebufferDescription.m_Attachments.GetCount(); ++i)
  {
    writer << framebufferDescription.m_Attachments[i];
  }

  return writer.GetHashValue();
}

bool xiiGALDescriptorHash::Equal(const xiiGALFramebufferCreationDescription& a, const xiiGALFramebufferCreationDescription& b)
{
  return a == b;
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
