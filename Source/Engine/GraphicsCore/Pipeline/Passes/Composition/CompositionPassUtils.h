/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>
namespace xiiCompositionPassUtils
{
  inline const xiiGALTextureCreationDescription* Image(xiiRenderPipelinePassContext& context, xiiRenderGraphTextureHandle image)
  {
    const auto* d = context.m_Graph.GetTextureDescription(image);
    if (!d || !d->m_Size.HasNonZeroArea() || d->m_Type != xiiGALResourceDimension::Texture2D || d->m_uiArraySizeOrDepth != 1 || d->m_uiSampleCount != 1 || !d->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) || xiiGALTextureUtilities::GetComponentCount(d->m_Format) < 4)
      return nullptr;
    const auto& properties = xiiGALTextureUtilities::GetResourceFormatProperties(d->m_Format);
    return !properties.IsTypeless() && (properties.m_ComponentType == xiiGALResourceFormatComponentType::Float || properties.IsNormalized() || properties.m_ComponentType == xiiGALResourceFormatComponentType::UnsignedNormalizedSRGB) ? d : nullptr;
  }
  inline xiiRenderGraphTextureHandle Output(xiiRenderGraphBuilder& builder, xiiSizeU32 size, xiiGALResourceFormat::Enum format)
  {
    xiiGALTextureCreationDescription d;
    d.m_Type      = xiiGALResourceDimension::Texture2D;
    d.m_Size      = size;
    d.m_Format    = format;
    d.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    return builder.WriteTexture("Radiance", d, xiiGALResourceStateFlags::UnorderedAccess);
  }
} // namespace xiiCompositionPassUtils
