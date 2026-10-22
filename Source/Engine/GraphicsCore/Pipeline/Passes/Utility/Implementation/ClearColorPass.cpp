/// Copyright (c) Theophilus Eriata. All Rights Reserved.


#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Utility/ClearColorPass.h>
#include <GraphicsFoundation/Resources/Texture.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClearColorPass, 1, xiiRTTIDefaultAllocator<xiiClearColorPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("TextureInput", m_InputTexture)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("Texture")),
    XII_MEMBER_PROPERTY("TextureOutput", m_OutputTexture)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("Texture")),
    XII_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new xiiDefaultValueAttribute(xiiColor::Black)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Utility")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiClearColorPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_Color.IsValid() ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiClearColorPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  hInput       = context.GetInputTexture(m_InputTexture);
  const auto* pDescription = context.m_Graph.GetTextureDescription(hInput);
  if (!pDescription || pDescription->m_Type != xiiGALResourceDimension::Texture2D || pDescription->m_uiArraySizeOrDepth != 1 ||
      pDescription->m_uiSampleCount != 1 || pDescription->m_uiMipLevels != 1 || !pDescription->m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget) || xiiGALResourceFormat::IsDepthFormat(pDescription->m_Format))
    return context.ReportFailure("Color clearing requires a single-sample 2D color render target.");

  struct PassData
  {
    xiiRenderGraphTextureHandle m_hTexture;
    xiiColor                    m_Color;
  };

  const auto emitted = context.m_Graph.AddPass<PassData>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](PassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hTexture = builder.WriteTexture(hInput, xiiGALResourceStateFlags::RenderTarget);
      data.m_Color = m_Color; }, [](const PassData& data, xiiRenderGraphPassContext& execute) { xiiRenderPassUtils::ClearRenderTargetPreservingState(execute.GetCommandList(), execute.GetTexture(data.m_hTexture)->GetDefaultView(xiiGALTextureViewType::RenderTarget), data.m_Color); });

  return context.SetOutput(m_OutputTexture, emitted.first->m_hTexture);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Utility_Implementation_ClearColorPass);
