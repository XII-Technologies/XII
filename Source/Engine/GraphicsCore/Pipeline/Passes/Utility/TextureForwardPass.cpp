/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <GraphicsCore/Pipeline/Passes/Utility/TextureForwardPass.h>
#include <GraphicsCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureForwardPass, 1, xiiRTTIDefaultAllocator<xiiTextureForwardPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ARRAY_MEMBER_PROPERTY("EnabledViewModes", m_EnabledViewModes),
    XII_MEMBER_PROPERTY("RequireInput", m_bRequireInput)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("Input", m_Input)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Output", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Utility") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiTextureForwardPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  m_ResolvedViewModes.Clear();
  for (const auto& name : m_EnabledViewModes)
  {
    xiiEnum<xiiViewRenderMode> mode;
    if (!xiiReflectionUtils::StringToEnumeration(name, mode) || !xiiViewRenderMode::IsSupportedVisualizationMode(mode))
      return XII_FAILURE;
    m_ResolvedViewModes.PushBack(mode);
  }
  return XII_SUCCESS;
}

void xiiTextureForwardPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const
{
  const auto first = out_pins.GetCount();
  xiiRenderPipelinePass::GetPins(out_pins);
  for (xiiUInt32 i = first; i < out_pins.GetCount(); ++i)
    if (out_pins[i].m_bInput && out_pins[i].m_sName == "Input")
      out_pins[i].m_bOptional = !m_bEnabled || !m_bRequireInput;
}

xiiResult xiiTextureForwardPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!m_bEnabled && (!context.m_pView || !m_ResolvedViewModes.Contains(context.m_pView->GetViewRenderMode())))
    return XII_SUCCESS;
  const auto input = context.GetInputTexture(m_Input);
  if (!input.IsValid() && m_bRequireInput)
    return context.ReportFailure("Enabled texture forwarding requires its authored input.");
  return input.IsValid() ? context.SetOutput(m_Output, input) : XII_SUCCESS;
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Utility_TextureForwardPass);
