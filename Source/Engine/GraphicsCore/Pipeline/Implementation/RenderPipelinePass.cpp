/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Reflection/ReflectionUtils.h>
#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelinePass, 2, xiiRTTINoAllocator)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MAP_MEMBER_PROPERTY("ParameterOverrides", m_ParameterOverrides),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiResult xiiRenderPipelinePass::Configure(const xiiVariantDictionary& properties)
{
  for (auto it = properties.GetIterator(); it.IsValid(); ++it)
  {
    const auto* property = GetDynamicRTTI()->FindPropertyByName(it.Key());
    if (!property || property->GetCategory() != xiiPropertyCategory::Member || property->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
      return XII_FAILURE;
    const auto current   = xiiReflectionUtils::GetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(property), this);
    xiiVariant converted = it.Value();
    if (current.IsValid())
    {
      xiiResult conversion = XII_FAILURE;
      converted            = it.Value().ConvertTo(current.GetType(), &conversion);
      XII_SUCCEED_OR_RETURN(conversion);
    }
    xiiReflectionUtils::SetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(property), this, converted);
  }
  return XII_SUCCESS;
}

xiiUInt32 xiiRenderPipelinePassContext::FindPin(xiiStringView name, bool input) const
{
  for (xiiUInt32 i = 0; i < m_Pins.GetCount(); ++i)
    if (m_Pins[i].m_bInput == input && m_Pins[i].m_sName == name)
      return i;
  return xiiInvalidIndex;
}
xiiRenderGraphTextureHandle xiiRenderPipelinePassContext::GetInputTexture(xiiStringView name) const
{
  const auto i = FindPin(name, true);
  return i == xiiInvalidIndex || m_Pins[i].m_Type != xiiRenderPipelinePin::Texture ? xiiRenderGraphTextureHandle() : m_Bindings[i].m_hTexture;
}
xiiRenderGraphBufferHandle xiiRenderPipelinePassContext::GetInputBuffer(xiiStringView name) const
{
  const auto i = FindPin(name, true);
  return i == xiiInvalidIndex || m_Pins[i].m_Type != xiiRenderPipelinePin::Buffer ? xiiRenderGraphBufferHandle() : m_Bindings[i].m_hBuffer;
}
xiiResult xiiRenderPipelinePassContext::SetOutput(xiiStringView name, xiiRenderGraphTextureHandle texture)
{
  const auto i = FindPin(name, false);
  if (i == xiiInvalidIndex || m_Pins[i].m_Type != xiiRenderPipelinePin::Texture)
    return XII_FAILURE;
  const auto* description = m_Graph.GetTextureDescription(texture);
  if (!description || (m_Pins[i].m_Format != xiiGALResourceFormat::Unknown && description->m_Format != m_Pins[i].m_Format))
    return XII_FAILURE;
  m_Bindings[i].m_hTexture = texture;
  return XII_SUCCESS;
}
xiiResult xiiRenderPipelinePassContext::SetOutput(xiiStringView name, xiiRenderGraphBufferHandle buffer)
{
  const auto i = FindPin(name, false);
  if (i == xiiInvalidIndex || m_Pins[i].m_Type != xiiRenderPipelinePin::Buffer)
    return XII_FAILURE;
  if (!m_Graph.GetBufferDescription(buffer))
    return XII_FAILURE;
  m_Bindings[i].m_hBuffer = buffer;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelinePass);
