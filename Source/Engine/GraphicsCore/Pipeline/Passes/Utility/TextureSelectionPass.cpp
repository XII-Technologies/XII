/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Utility/TextureSelectionPass.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureSelectionPass, 1, xiiRTTIDefaultAllocator<xiiTextureSelectionPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Primary", m_InputPrimary)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Fallback", m_InputFallback)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Texture", m_OutputTexture)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PreferPrimary", m_bPreferPrimary)->AddAttributes(new xiiDefaultValueAttribute(true)),
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

xiiResult xiiTextureSelectionPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto primary = context.GetInputTexture(m_InputPrimary);
  return context.SetOutput(m_OutputTexture, m_bPreferPrimary && primary.IsValid() ? primary : context.GetInputTexture(m_InputFallback));
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Utility_TextureSelectionPass);
