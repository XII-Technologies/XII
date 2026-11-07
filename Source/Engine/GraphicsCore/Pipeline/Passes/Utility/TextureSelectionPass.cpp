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
    XII_MEMBER_PROPERTY("AllowAbsentFallback", m_bAllowAbsentFallback)->AddAttributes(new xiiDefaultValueAttribute(false)),
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

void xiiTextureSelectionPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const
{
  const auto first = out_pins.GetCount();
  xiiRenderPipelinePass::GetPins(out_pins);
  for (xiiUInt32 i = first; i < out_pins.GetCount(); ++i)
    if (out_pins[i].m_sName == "Fallback" || out_pins[i].m_sName == "Texture")
      out_pins[i].m_bOptional = m_bAllowAbsentFallback;
}
xiiResult xiiTextureSelectionPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto primary = context.GetInputTexture(m_InputPrimary);
  const auto selected = m_bPreferPrimary && primary.IsValid() ? primary : context.GetInputTexture(m_InputFallback);
  return !selected.IsValid() && m_bAllowAbsentFallback ? XII_SUCCESS : context.SetOutput(m_OutputTexture, selected);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Utility_TextureSelectionPass);
