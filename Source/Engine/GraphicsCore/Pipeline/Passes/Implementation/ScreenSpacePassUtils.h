/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/Passes/Composition/CompositionPassUtils.h>
namespace xiiScreenSpacePassUtils
{
  inline const xiiGALTextureCreationDescription* Image(xiiRenderPipelinePassContext& context, xiiRenderGraphTextureHandle image, xiiUInt32 channels, const xiiSizeU32* size = nullptr)
  {
    const auto* d = context.m_Graph.GetTextureDescription(image);
    if (!d || !d->m_Size.HasNonZeroArea() || (size && d->m_Size != *size) || d->m_Type != xiiGALResourceDimension::Texture2D || d->m_uiArraySizeOrDepth != 1 || d->m_uiSampleCount != 1 || !d->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) || xiiGALTextureUtilities::GetComponentCount(d->m_Format) < channels || (channels > 1 && xiiGALResourceFormat::IsDepthFormat(d->m_Format)))
      return nullptr;
    const auto& p = xiiGALTextureUtilities::GetResourceFormatProperties(d->m_Format);
    return !p.IsTypeless() && (p.m_ComponentType == xiiGALResourceFormatComponentType::Float || p.IsNormalized() || p.m_ComponentType == xiiGALResourceFormatComponentType::UnsignedNormalizedSRGB || p.m_ComponentType == xiiGALResourceFormatComponentType::Depth || p.m_ComponentType == xiiGALResourceFormatComponentType::DepthStencil) ? d : nullptr;
  }
  inline bool Uniform(xiiRenderPipelinePassContext& context, xiiRenderGraphBufferHandle buffer, xiiUInt32 bytes)
  {
    const auto* d = context.m_Graph.GetBufferDescription(buffer);
    return d && d->m_uiSize == bytes && d->m_BindFlags.IsSet(xiiGALBindFlags::UniformBuffer);
  }
} // namespace xiiScreenSpacePassUtils
