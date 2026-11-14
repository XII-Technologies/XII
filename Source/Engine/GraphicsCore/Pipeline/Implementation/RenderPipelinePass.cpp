/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Texture.h>

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
  for (const auto& pin : pins)
  {
    if (pin.m_Resolution.Validate().Failed() || (pin.m_Type != xiiRenderPipelinePin::Texture && !pin.m_Resolution.IsDefault()))
      return XII_FAILURE;
    if (pin.m_Resolution.m_Mode == xiiRenderPipelineTextureResolutionMode::Input && !pin.m_Resolution.m_sInput.IsEmpty())
    {
      bool found = false;
      for (const auto& input : pins)
        found |= input.m_bInput && input.m_Type == xiiRenderPipelinePin::Texture && input.m_sName == pin.m_Resolution.m_sInput;
      if (!found)
      {
        if (out_error)
          out_error->SetFormat("Pin '{}': resolution reference '{}' is not a texture input on this pass.", pin.m_sName, pin.m_Resolution.m_sInput);
        return XII_FAILURE;
      }
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

xiiResult xiiRenderPipelinePassContext::ResolveTextureSize(const xiiRenderPipelineTextureOutputPin& pin, xiiRenderGraphTextureHandle reference, xiiSizeU32& out_size) const
{
  const auto index = pin.GetBindingIndex();
  if (index >= m_Pins.GetCount() || m_Pins[index].m_bInput)
    return ReportFailure("Texture sizing requires a configured output pin.");
  return ResolveTextureSize(m_Pins[index], reference, out_size);
}

xiiResult xiiRenderPipelinePassContext::ResolveTextureSize(const xiiRenderPipelinePin& pin, xiiRenderGraphTextureHandle reference, xiiSizeU32& out_size) const
{
  using Mode             = xiiRenderPipelineTextureResolutionMode;
  out_size               = {0, 0};
  const auto& resolution = pin.m_Resolution;
  if (pin.m_Type != xiiRenderPipelinePin::Texture || resolution.Validate().Failed())
    return ReportFailure("Invalid texture resolution policy.");
  if (!resolution.m_sInput.IsEmpty())
    reference = GetInputTexture(resolution.m_sInput);
  const auto mode = resolution.m_Mode.GetValue();
  if (!reference.IsValid() && resolution.m_sInput.IsEmpty() && mode == Mode::Input)
    for (xiiUInt32 i = 0; i < m_Pins.GetCount(); ++i)
      if (m_Pins[i].m_bInput && m_Pins[i].m_Type == xiiRenderPipelinePin::Texture && m_Bindings[i].m_hTexture.IsValid())
      {
        reference = m_Bindings[i].m_hTexture;
        break;
      }
  const auto* input = m_Graph.GetTextureDescription(reference);
  xiiSizeU32  size(0, 0);
  if (mode == Mode::Input && input)
    size = input->m_Size;
  else if (mode == Mode::Fixed)
    size = {resolution.m_uiWidth, resolution.m_uiHeight};
  else if (m_pView && (mode == Mode::Viewport || mode == Mode::RenderResolution))
  {
    const auto& viewport = m_pView->GetViewport();
    if (!xiiMath::IsFinite(viewport.width) || !xiiMath::IsFinite(viewport.height) || viewport.width <= 0 || viewport.height <= 0 || viewport.width > 65536 || viewport.height > 65536)
      return ReportFailure("Texture sizing requires a finite, nonempty viewport of at most 65536 texels per dimension.");
    size = mode == Mode::Viewport ? xiiSizeU32(static_cast<xiiUInt32>(viewport.width), static_cast<xiiUInt32>(viewport.height)) :
                                    xiiSizeU32(m_pView->GetRenderResolutionWidth(), m_pView->GetRenderResolutionHeight());
  }
  const double width  = xiiMath::Ceil(static_cast<double>(size.width) * resolution.m_fScale);
  const double height = xiiMath::Ceil(static_cast<double>(size.height) * resolution.m_fScale);
  if (!size.width || !size.height || width > 65536 || height > 65536)
    return ReportFailure("Texture resolution requires a view, an available input or explicit dimensions, and must fit within 65536 texels per dimension.");
  if (xiiGALDevice::HasDefaultDevice())
  {
    const auto& limits  = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_TextureProperties;
    const auto  maximum = pin.m_Constraints.m_Dimension == xiiGALResourceDimension::Texture3D                                                             ? limits.m_uiMaxTexture3DDimension :
      pin.m_Constraints.m_Dimension == xiiGALResourceDimension::TextureCube || pin.m_Constraints.m_Dimension == xiiGALResourceDimension::TextureCubeArray ? limits.m_uiMaxTextureCubeDimension :
                                                                                                                                                            limits.m_uiMaxTexture2DDimension;
    if (maximum && (width > maximum || height > maximum))
      return ReportFailure("Resolved texture extent exceeds the device's texture dimension limit.");
  }
  out_size = {xiiMath::Max(1U, static_cast<xiiUInt32>(width)), xiiMath::Max(1U, static_cast<xiiUInt32>(height))};
  return XII_SUCCESS;
}

xiiResult xiiRenderPipelinePassContext::ValidateTexture(const xiiRenderPipelinePin& pin, xiiRenderGraphTextureHandle texture) const
{
  const auto* description = m_Graph.GetTextureDescription(texture);
  if (!description || !pin.Accepts(*description))
    return XII_FAILURE;
  const auto mode = pin.m_Resolution.m_Mode;
  if (pin.m_bConstrainResolution &&
      !(mode == xiiRenderPipelineTextureResolutionMode::Input && m_bTemporalHistoryPhase))
  {
    xiiSizeU32 expected;
    XII_SUCCEED_OR_RETURN(ResolveTextureSize(pin, {}, expected));
    if (expected != description->m_Size)
      return ReportFailure("Texture extent does not match the pin's resolution contract.");
  }
  return XII_SUCCESS;
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
  if (!description || ValidateTexture(m_Pins[i], texture).Failed())
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
  if (!description || ValidateTexture(m_Pins[i], texture).Failed())
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
