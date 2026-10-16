/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelinePass, 3, xiiRTTINoAllocator)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MAP_MEMBER_PROPERTY("ParameterOverrides", m_ParameterOverrides),
      XII_ARRAY_MEMBER_PROPERTY("PinConfigurations", m_PinConfigurations)->AddFlags(xiiPropertyFlags::PointerOwner)->AddAttributes(new xiiContainerAttribute(false, false, false)),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiRenderPipelinePass::~xiiRenderPipelinePass()
{
  for (auto* configuration : m_PinConfigurations)
    if (configuration)
      configuration->GetDynamicRTTI()->GetAllocator()->Deallocate(configuration);
}

xiiResult xiiRenderPipelinePass::GetConfiguredPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins, xiiStringBuilder* out_error) const
{
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  GetPins(pins);
  if (m_PinConfigurations.GetCount() > 256)
    return XII_FAILURE;
  xiiHashSet<xiiString> configured;
  xiiHashSet<xiiUuid>   identities;
  for (const auto* configuration : m_PinConfigurations)
  {
    if (!configuration)
      return XII_FAILURE;
    if (identities.Contains(configuration->m_Id))
    {
      if (out_error)
        out_error->SetFormat("Pin '{}': duplicate persistent identity.", configuration->m_Metadata.m_sName);
      return XII_FAILURE;
    }
    identities.Insert(configuration->m_Id);
    const auto&      metadata = configuration->m_Metadata;
    xiiStringBuilder key(metadata.m_bInput ? "Input/" : "Output/", metadata.m_sName);
    if (configured.Contains(key))
    {
      if (out_error)
        out_error->SetFormat("Pin '{}': duplicate configuration.", metadata.m_sName);
      return XII_FAILURE;
    }
    configured.Insert(key);
    bool found = false;
    for (auto& pin : pins)
      if (pin.m_sName == metadata.m_sName && pin.m_bInput == metadata.m_bInput)
      {
        XII_SUCCEED_OR_RETURN(pin.Apply(*configuration, out_error));
        found = true;
        break;
      }
    if (!found)
    {
      if (out_error)
        out_error->SetFormat("Pin '{}': configuration references an unknown native pin or direction.", metadata.m_sName);
      return XII_FAILURE;
    }
  }
  out_pins = std::move(pins);
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
  if (m_Pins[i].m_bTemporalHistoryOutput != m_bTemporalHistoryPhase)
    return XII_FAILURE;
  const auto* description = m_Graph.GetTextureDescription(texture);
  if (!description || !m_Pins[i].Accepts(*description))
    return XII_FAILURE;
  m_Bindings[i].m_hTexture = texture;
  return XII_SUCCESS;
}
xiiResult xiiRenderPipelinePassContext::SetOutput(xiiStringView name, xiiRenderGraphBufferHandle buffer)
{
  const auto i = FindPin(name, false);
  if (i == xiiInvalidIndex || m_Pins[i].m_Type != xiiRenderPipelinePin::Buffer)
    return XII_FAILURE;
  if (m_Pins[i].m_bTemporalHistoryOutput != m_bTemporalHistoryPhase)
    return XII_FAILURE;
  const auto* description = m_Graph.GetBufferDescription(buffer);
  if (!description || !m_Pins[i].Accepts(*description))
    return XII_FAILURE;
  m_Bindings[i].m_hBuffer = buffer;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelinePass);
