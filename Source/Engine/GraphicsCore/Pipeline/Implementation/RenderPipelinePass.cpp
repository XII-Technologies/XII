/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineFrameData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

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

void xiiRenderPipelinePass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const
{
  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  GetDynamicRTTI()->GetAllProperties(properties);
  for (const auto* property : properties)
  {
    if (property->GetCategory() != xiiPropertyCategory::Member || property->GetFlags().IsSet(xiiPropertyFlags::Pointer) || !property->GetSpecificType()->IsDerivedFrom<xiiRenderPipelinePinHandle>())
      continue;
    const auto* member = static_cast<const xiiAbstractMemberProperty*>(property);
    const auto* handle = static_cast<const xiiRenderPipelinePinHandle*>(member->GetPropertyPointer(this));
    XII_ASSERT_DEV(handle, "Pin members must provide direct reflected storage.");
    if (const auto* enabled = property->GetAttributeByType<xiiRenderPipelinePinEnabledAttribute>())
    {
      const auto* flag = GetDynamicRTTI()->FindPropertyByName(enabled->m_sProperty);
      XII_ASSERT_DEV(flag && flag->GetCategory() == xiiPropertyCategory::Member && flag->GetSpecificType() == xiiGetStaticRTTI<bool>(), "Pin presence must reference a boolean member property.");
      if (!flag || flag->GetCategory() != xiiPropertyCategory::Member || flag->GetSpecificType() != xiiGetStaticRTTI<bool>() || !xiiReflectionUtils::GetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(flag), this).Get<bool>())
        continue;
    }
    auto        pin  = handle->m_Declaration;
    const auto* name = property->GetAttributeByType<xiiRenderPipelinePinNameAttribute>();
    pin.m_sName      = name ? name->m_sName.GetView() : property->GetPropertyName();
    if (const auto* optional = property->GetAttributeByType<xiiRenderPipelinePinOptionalAttribute>())
    {
      const auto* flag = GetDynamicRTTI()->FindPropertyByName(optional->m_sProperty);
      XII_ASSERT_DEV(flag && flag->GetCategory() == xiiPropertyCategory::Member && flag->GetSpecificType() == xiiGetStaticRTTI<bool>(), "Pin optionality must reference a boolean member property.");
      if (flag && flag->GetCategory() == xiiPropertyCategory::Member && flag->GetSpecificType() == xiiGetStaticRTTI<bool>())
        pin.m_bOptional = xiiReflectionUtils::GetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(flag), this).Get<bool>();
    }
    out_pins.PushBack(std::move(pin));
  }
}

xiiResult xiiRenderPipelinePass::InitializePinBindings(xiiArrayPtr<const xiiRenderPipelinePin> pins) const
{
  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  GetDynamicRTTI()->GetAllProperties(properties);
  for (const auto* property : properties)
    if (property->GetCategory() == xiiPropertyCategory::Member && !property->GetFlags().IsSet(xiiPropertyFlags::Pointer) && property->GetSpecificType()->IsDerivedFrom<xiiRenderPipelinePinHandle>())
      static_cast<const xiiRenderPipelinePinHandle*>(static_cast<const xiiAbstractMemberProperty*>(property)->GetPropertyPointer(this))->m_uiBindingIndex = xiiInvalidIndex;
  xiiDynamicArray<xiiRenderPipelinePin> native;
  GetPins(native);
  if (native.GetCount() != pins.GetCount())
    return XII_FAILURE;
  for (xiiUInt32 i = 0; i < native.GetCount(); ++i)
    if (native[i].m_sName != pins[i].m_sName || native[i].m_Type != pins[i].m_Type || native[i].m_bInput != pins[i].m_bInput)
      return XII_FAILURE;
  for (const auto* property : properties)
  {
    if (property->GetCategory() != xiiPropertyCategory::Member || property->GetFlags().IsSet(xiiPropertyFlags::Pointer) || !property->GetSpecificType()->IsDerivedFrom<xiiRenderPipelinePinHandle>())
      continue;
    const auto*         handle = static_cast<const xiiRenderPipelinePinHandle*>(static_cast<const xiiAbstractMemberProperty*>(property)->GetPropertyPointer(this));
    const auto*         alias  = property->GetAttributeByType<xiiRenderPipelinePinNameAttribute>();
    const xiiStringView name   = alias ? alias->m_sName.GetView() : property->GetPropertyName();
    for (xiiUInt32 i = 0; i < pins.GetCount(); ++i)
      if (pins[i].m_sName == name && pins[i].m_bInput == handle->m_Declaration.m_bInput && pins[i].m_Type == handle->m_Declaration.m_Type && pins[i].m_pDataType == handle->m_Declaration.m_pDataType)
      {
        handle->m_uiBindingIndex = i;
        break;
      }
  }
  return XII_SUCCESS;
}

xiiResult xiiRenderPipelinePass::GetConfiguredPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins, xiiStringBuilder* out_error) const
{
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  GetPins(pins);
  return GetConfiguredPins(pins.GetArrayPtr(), out_pins, out_error);
}

xiiResult xiiRenderPipelinePass::GetConfiguredPins(xiiArrayPtr<const xiiRenderPipelinePin> declarations, xiiDynamicArray<xiiRenderPipelinePin>& out_pins, xiiStringBuilder* out_error) const
{
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  pins = declarations;
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
xiiRenderGraphTextureHandle xiiRenderPipelinePassContext::GetInputTexture(const xiiRenderPipelineTextureInputPin& pin) const
{
  const auto i = pin.GetBindingIndex();
  return i >= m_Pins.GetCount() || i >= m_Bindings.GetCount() || !m_Pins[i].m_bInput || m_Pins[i].m_Type != xiiRenderPipelinePin::Texture ? xiiRenderGraphTextureHandle() : m_Bindings[i].m_hTexture;
}
xiiRenderGraphBufferHandle xiiRenderPipelinePassContext::GetInputBuffer(const xiiRenderPipelineBufferInputPin& pin) const
{
  const auto i = pin.GetBindingIndex();
  return i >= m_Pins.GetCount() || i >= m_Bindings.GetCount() || !m_Pins[i].m_bInput || m_Pins[i].m_Type != xiiRenderPipelinePin::Buffer ? xiiRenderGraphBufferHandle() : m_Bindings[i].m_hBuffer;
}
xiiResult xiiRenderPipelinePassContext::SetOutput(const xiiRenderPipelineTextureOutputPin& pin, xiiRenderGraphTextureHandle texture)
{
  const auto i = pin.GetBindingIndex();
  if (i >= m_Pins.GetCount() || i >= m_Bindings.GetCount() || m_Pins[i].m_bInput || m_Pins[i].m_Type != xiiRenderPipelinePin::Texture || m_Pins[i].m_bTemporalHistoryOutput != m_bTemporalHistoryPhase)
    return XII_FAILURE;
  const auto* description = m_Graph.GetTextureDescription(texture);
  if (!description || !m_Pins[i].Accepts(*description))
    return XII_FAILURE;
  m_Bindings[i].m_hTexture = texture;
  return XII_SUCCESS;
}
xiiResult xiiRenderPipelinePassContext::SetOutput(const xiiRenderPipelineBufferOutputPin& pin, xiiRenderGraphBufferHandle buffer)
{
  const auto i = pin.GetBindingIndex();
  if (i >= m_Pins.GetCount() || i >= m_Bindings.GetCount() || m_Pins[i].m_bInput || m_Pins[i].m_Type != xiiRenderPipelinePin::Buffer || m_Pins[i].m_bTemporalHistoryOutput != m_bTemporalHistoryPhase)
    return XII_FAILURE;
  const auto* description = m_Graph.GetBufferDescription(buffer);
  if (!description || !m_Pins[i].Accepts(*description))
    return XII_FAILURE;
  m_Bindings[i].m_hBuffer = buffer;
  return XII_SUCCESS;
}
xiiRenderPipelineAccelerationStructureBinding xiiRenderPipelinePassContext::GetInputAccelerationStructure(const xiiRenderPipelineAccelerationStructureInputPin& pin) const
{
  const auto i = pin.GetBindingIndex();
  return i >= m_Pins.GetCount() || i >= m_Bindings.GetCount() || !m_Pins[i].m_bInput || m_Pins[i].m_Type != xiiRenderPipelinePin::AccelerationStructure ? xiiRenderPipelineAccelerationStructureBinding() : m_Bindings[i].m_AccelerationStructure;
}
xiiResult xiiRenderPipelinePassContext::SetOutput(const xiiRenderPipelineAccelerationStructureOutputPin& pin, const xiiRenderPipelineAccelerationStructureBinding& structure)
{
  const auto i = pin.GetBindingIndex();
  if (i >= m_Pins.GetCount() || i >= m_Bindings.GetCount() || m_Pins[i].m_bInput || m_Pins[i].m_Type != xiiRenderPipelinePin::AccelerationStructure || m_bTemporalHistoryPhase)
    return ReportFailure("Cannot publish an acceleration structure through this pin or temporal phase.");
  if (!structure.IsValid())
    return ReportFailure("Acceleration-structure output requires both a TLAS and its graph build dependency.");
  if (!m_Graph.GetBufferDescription(structure.m_Dependency))
    return ReportFailure("Acceleration-structure output references a missing or stale graph build dependency.");
  m_Bindings[i].m_AccelerationStructure = structure;
  return XII_SUCCESS;
}
xiiSharedPtr<const xiiRenderPipelineFrameData> xiiRenderPipelinePassContext::GetInputData(const xiiRenderPipelineDataInputPin& pin) const
{
  const auto i = pin.GetBindingIndex();
  if (i >= m_Pins.GetCount() || i >= m_Bindings.GetCount() || !m_Pins[i].m_bInput || m_Pins[i].m_Type != xiiRenderPipelinePin::FrameData)
    return nullptr;
  const auto& value = m_Bindings[i].m_pFrameData;
  return value && value->GetSetupSerial() && !m_Bindings[i].m_hTexture.IsValid() && !m_Bindings[i].m_hBuffer.IsValid() && m_Bindings[i].m_AccelerationStructure.IsEmpty() && m_Pins[i].m_pDataType && value->GetSetupSerial() == m_Graph.GetSetupSerial() && value->GetDynamicRTTI()->IsDerivedFrom(m_Pins[i].m_pDataType) ? value : xiiSharedPtr<const xiiRenderPipelineFrameData>();
}
xiiResult xiiRenderPipelinePassContext::SetOutput(const xiiRenderPipelineDataOutputPin& pin, xiiSharedPtr<const xiiRenderPipelineFrameData> data)
{
  const auto i = pin.GetBindingIndex();
  if (i >= m_Pins.GetCount() || i >= m_Bindings.GetCount() || m_Pins[i].m_bInput || m_Pins[i].m_Type != xiiRenderPipelinePin::FrameData || m_bTemporalHistoryPhase)
    return ReportFailure("Cannot publish frame data through this pin or temporal phase.");
  if (!data || !data->GetSetupSerial() || !m_Pins[i].m_pDataType || data->GetSetupSerial() != m_Graph.GetSetupSerial() || !data->GetDynamicRTTI()->IsDerivedFrom(m_Pins[i].m_pDataType))
    return ReportFailure("Frame data is missing, belongs to another setup or violates the native RTTI contract.");
  m_Bindings[i]              = {};
  m_Bindings[i].m_pFrameData = std::move(data);
  return XII_SUCCESS;
}
xiiResult xiiRenderPipelinePassContext::ReportFailure(xiiStringView reason) const
{
  if (m_pBuildError)
    m_pBuildError->Set(reason);
  return XII_FAILURE;
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
