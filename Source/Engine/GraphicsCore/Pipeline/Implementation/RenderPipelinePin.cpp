/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <GraphicsCore/Pipeline/RenderPipelineFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePin.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>

xiiStringView xiiRenderPipelinePinMetadata::GetDataTypeName() const
{
  return m_pDataType ? m_pDataType->GetTypeName() : "";
}

void xiiRenderPipelinePinMetadata::SetDataTypeName(xiiStringView sTypeName)
{
  m_pDataType = xiiRTTI::FindTypeByName(sTypeName);
}

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRenderPipelinePinType, 1)
  XII_ENUM_CONSTANTS(xiiRenderPipelinePinType::Texture, xiiRenderPipelinePinType::Buffer, xiiRenderPipelinePinType::Order, xiiRenderPipelinePinType::AccelerationStructure, xiiRenderPipelinePinType::FrameData)
XII_END_STATIC_REFLECTED_ENUM;
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRenderPipelinePinAccess, 1)
  XII_ENUM_CONSTANTS(xiiRenderPipelinePinAccess::Automatic, xiiRenderPipelinePinAccess::Read, xiiRenderPipelinePinAccess::Write, xiiRenderPipelinePinAccess::ReadWrite, xiiRenderPipelinePinAccess::Ordering)
XII_END_STATIC_REFLECTED_ENUM;
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRenderPipelinePinCardinality, 1)
  XII_ENUM_CONSTANTS(xiiRenderPipelinePinCardinality::Single, xiiRenderPipelinePinCardinality::Multiple)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelinePinConstraints, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRenderPipelinePinConstraints>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_ENUM_MEMBER_PROPERTY("Dimension", xiiGALResourceDimension, m_Dimension),
      XII_MEMBER_PROPERTY("Samples", m_uiSamples)->AddAttributes(new xiiClampValueAttribute(0U, 64U)),
      XII_MEMBER_PROPERTY("Width", m_uiWidth),
      XII_MEMBER_PROPERTY("Height", m_uiHeight),
      XII_BITFLAGS_MEMBER_PROPERTY("RequiredBindFlags", xiiGALBindFlags, m_RequiredBindFlags),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelinePinMetadata, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRenderPipelinePinMetadata>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiReadOnlyAttribute()),
      XII_MEMBER_PROPERTY("Input", m_bInput)->AddAttributes(new xiiReadOnlyAttribute()),
      XII_ENUM_MEMBER_PROPERTY("ResourceType", xiiRenderPipelinePinType, m_Type)->AddAttributes(new xiiReadOnlyAttribute()),
      XII_ENUM_MEMBER_PROPERTY("Access", xiiRenderPipelinePinAccess, m_Access)->AddAttributes(new xiiReadOnlyAttribute()),
      XII_ENUM_MEMBER_PROPERTY("Cardinality", xiiRenderPipelinePinCardinality, m_Cardinality)->AddAttributes(new xiiReadOnlyAttribute()),
      XII_MEMBER_PROPERTY("Optional", m_bOptional)->AddAttributes(new xiiReadOnlyAttribute()),
      XII_MEMBER_PROPERTY("TemporalHistory", m_bTemporalHistory)->AddAttributes(new xiiReadOnlyAttribute()),
      XII_ENUM_MEMBER_PROPERTY("DeclaredFormat", xiiGALResourceFormat, m_DeclaredFormat)->AddAttributes(new xiiReadOnlyAttribute()),
      XII_ACCESSOR_PROPERTY("DataType", GetDataTypeName, SetDataTypeName)->AddAttributes(new xiiReadOnlyAttribute()),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelinePinConfiguration, 1, xiiRTTIDefaultAllocator<xiiRenderPipelinePinConfiguration>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Id", m_Id)->AddAttributes(new xiiReadOnlyAttribute(), new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("Metadata", m_Metadata)->AddAttributes(new xiiReadOnlyAttribute(), new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("DisplayName", m_sDisplayName),
      XII_ENUM_MEMBER_PROPERTY("RequiredFormat", xiiGALResourceFormat, m_Format),
      XII_MEMBER_PROPERTY("Requirements", m_Constraints),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;


// Native pin members contain no serialized runtime handles. Persistent authoring
// contracts are applied after RTTI schema discovery.
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelinePinHandle, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineTextureInputPin, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineTextureInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineTextureOutputPin, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineTextureOutputPin>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_ENUM_ACCESSOR_PROPERTY("Format", xiiGALResourceFormat, GetFormat, SetFormat),
      XII_ACCESSOR_PROPERTY("Requirements", GetRequirements, SetRequirements),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineBufferInputPin, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineBufferInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineBufferOutputPin, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineBufferOutputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineAccelerationStructureInputPin, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineAccelerationStructureInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineAccelerationStructureOutputPin, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineAccelerationStructureOutputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineOrderInputPin, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineOrderInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineDataInputPin, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineDataInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineDataOutputPin, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineDataOutputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineOrderOutputPin, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineOrderOutputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelinePinNameAttribute, 1, xiiRTTIDefaultAllocator<xiiRenderPipelinePinNameAttribute>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Name", m_sName),
    } XII_END_PROPERTIES;
    XII_BEGIN_FUNCTIONS
    {
      XII_CONSTRUCTOR_PROPERTY(const char*),
    } XII_END_FUNCTIONS;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelinePinOptionalAttribute, 1, xiiRTTIDefaultAllocator<xiiRenderPipelinePinOptionalAttribute>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Property", m_sProperty),
    } XII_END_PROPERTIES;
    XII_BEGIN_FUNCTIONS
    {
      XII_CONSTRUCTOR_PROPERTY(const char*),
    } XII_END_FUNCTIONS;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelinePinEnabledAttribute, 1, xiiRTTIDefaultAllocator<xiiRenderPipelinePinEnabledAttribute>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Property", m_sProperty),
    } XII_END_PROPERTIES;
    XII_BEGIN_FUNCTIONS
    {
      XII_CONSTRUCTOR_PROPERTY(const char*),
    } XII_END_FUNCTIONS;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiRenderPipelinePinHandle::xiiRenderPipelinePinHandle(xiiRenderPipelinePin::Type type, bool input, bool optional, xiiGALResourceFormat::Enum format, xiiBitflags<xiiGALBindFlags> bindings, xiiGALResourceDimension::Enum dimension, xiiUInt32 samples, xiiRenderPipelinePinAccess::Enum access, bool temporal)
{
  m_Declaration.m_Type                            = type;
  m_Declaration.m_bInput                          = input;
  m_Declaration.m_bOptional                       = optional;
  m_Declaration.m_Format                          = format;
  m_Declaration.m_Constraints.m_RequiredBindFlags = bindings;
  m_Declaration.m_Constraints.m_Dimension         = dimension;
  m_Declaration.m_Constraints.m_uiSamples         = samples;
  m_Declaration.m_Access                          = access;
  m_Declaration.m_bTemporalHistoryOutput          = temporal;
}
xiiRenderPipelineTextureInputPin::xiiRenderPipelineTextureInputPin(xiiGALResourceFormat::Enum format, bool optional, xiiBitflags<xiiGALBindFlags> bindings, xiiGALResourceDimension::Enum dimension, xiiUInt32 samples) :
  xiiRenderPipelinePinHandle(xiiRenderPipelinePin::Texture, true, optional, format, bindings, dimension, samples, xiiRenderPipelinePinAccess::Automatic, false) {}
xiiRenderPipelineTextureOutputPin::xiiRenderPipelineTextureOutputPin(xiiGALResourceFormat::Enum format, bool optional, xiiBitflags<xiiGALBindFlags> bindings, xiiGALResourceDimension::Enum dimension, xiiUInt32 samples, xiiRenderPipelinePinAccess::Enum access, bool temporal) :
  xiiRenderPipelinePinHandle(xiiRenderPipelinePin::Texture, false, optional, format, bindings, dimension, samples, access, temporal) {}
xiiRenderPipelineBufferInputPin::xiiRenderPipelineBufferInputPin(bool optional, xiiBitflags<xiiGALBindFlags> bindings) :
  xiiRenderPipelinePinHandle(xiiRenderPipelinePin::Buffer, true, optional, xiiGALResourceFormat::Unknown, bindings, xiiGALResourceDimension::Undefined, 0, xiiRenderPipelinePinAccess::Automatic, false) {}
xiiRenderPipelineBufferOutputPin::xiiRenderPipelineBufferOutputPin(bool optional, xiiBitflags<xiiGALBindFlags> bindings, xiiRenderPipelinePinAccess::Enum access, bool temporal) :
  xiiRenderPipelinePinHandle(xiiRenderPipelinePin::Buffer, false, optional, xiiGALResourceFormat::Unknown, bindings, xiiGALResourceDimension::Undefined, 0, access, temporal) {}
xiiRenderPipelineAccelerationStructureInputPin::xiiRenderPipelineAccelerationStructureInputPin(bool optional) :
  xiiRenderPipelinePinHandle(xiiRenderPipelinePin::AccelerationStructure, true, optional, xiiGALResourceFormat::Unknown, {}, xiiGALResourceDimension::Undefined, 0, xiiRenderPipelinePinAccess::Read, false) {}
xiiRenderPipelineAccelerationStructureOutputPin::xiiRenderPipelineAccelerationStructureOutputPin(bool optional, xiiRenderPipelinePinAccess::Enum access) :
  xiiRenderPipelinePinHandle(xiiRenderPipelinePin::AccelerationStructure, false, optional, xiiGALResourceFormat::Unknown, {}, xiiGALResourceDimension::Undefined, 0, access, false) {}
xiiRenderPipelineDataInputPin::xiiRenderPipelineDataInputPin(const xiiRTTI* type, bool optional) :
  xiiRenderPipelinePinHandle(xiiRenderPipelinePin::FrameData, true, optional, xiiGALResourceFormat::Unknown, {}, xiiGALResourceDimension::Undefined, 0, xiiRenderPipelinePinAccess::Read, false) { SetDataType(type); }
xiiRenderPipelineDataOutputPin::xiiRenderPipelineDataOutputPin(const xiiRTTI* type, bool optional) :
  xiiRenderPipelinePinHandle(xiiRenderPipelinePin::FrameData, false, optional, xiiGALResourceFormat::Unknown, {}, xiiGALResourceDimension::Undefined, 0, xiiRenderPipelinePinAccess::Read, false) { SetDataType(type); }
xiiRenderPipelineOrderInputPin::xiiRenderPipelineOrderInputPin() :
  xiiRenderPipelinePinHandle(xiiRenderPipelinePin::Order, true, true, xiiGALResourceFormat::Unknown, {}, xiiGALResourceDimension::Undefined, 0, xiiRenderPipelinePinAccess::Automatic, false) {}
xiiRenderPipelineOrderOutputPin::xiiRenderPipelineOrderOutputPin() :
  xiiRenderPipelinePinHandle(xiiRenderPipelinePin::Order, false, true, xiiGALResourceFormat::Unknown, {}, xiiGALResourceDimension::Undefined, 0, xiiRenderPipelinePinAccess::Automatic, false) {}

xiiRenderPipelinePinMetadata xiiRenderPipelinePin::GetMetadata() const
{
  xiiRenderPipelinePinMetadata metadata;
  metadata.m_sName  = m_sName;
  metadata.m_bInput = m_bInput;
  metadata.m_Type   = m_Type;
  metadata.m_Access = m_Access;
  if (metadata.m_Access == xiiRenderPipelinePinAccess::Automatic)
    metadata.m_Access = m_Type == Order ? xiiRenderPipelinePinAccess::Ordering : m_bInput || m_bTemporalHistoryOutput ? xiiRenderPipelinePinAccess::Read :
                                                                                                                        xiiRenderPipelinePinAccess::Write;
  metadata.m_Cardinality      = !m_bInput || m_Type == Order ? xiiRenderPipelinePinCardinality::Multiple : xiiRenderPipelinePinCardinality::Single;
  metadata.m_bOptional        = m_bOptional;
  metadata.m_bTemporalHistory = m_bTemporalHistoryOutput;
  metadata.m_DeclaredFormat   = m_Format;
  metadata.m_pDataType        = m_pDataType;
  return metadata;
}

xiiResult xiiRenderPipelinePin::Apply(const xiiRenderPipelinePinConfiguration& configuration, xiiStringBuilder* out_error)
{
  const auto fail = [&](xiiStringView message) {
    if (out_error)
      out_error->SetFormat("Pin '{}': {}", configuration.m_Metadata.m_sName, message);
    return XII_FAILURE;
  };
  const auto& limits = configuration.m_Constraints;
  if (!configuration.m_Id.IsValid())
    return fail("configuration has no persistent document identity.");
  if (configuration.m_Format.GetValue() >= xiiGALResourceFormat::ENUM_COUNT || limits.m_Dimension.GetValue() >= xiiGALResourceDimension::ENUM_COUNT || (limits.m_uiSamples && (limits.m_uiSamples > 64 || !xiiMath::IsPowerOf2(limits.m_uiSamples))))
    return fail("invalid format, dimension or sample count.");
  if (m_Type != Texture && (configuration.m_Format != xiiGALResourceFormat::Unknown || limits.m_Dimension != xiiGALResourceDimension::Undefined || limits.m_uiSamples || limits.m_uiWidth || limits.m_uiHeight))
    return fail("texture requirements are not valid on this resource kind.");
  if ((m_Type == Order || m_Type == AccelerationStructure || m_Type == FrameData) && limits.m_RequiredBindFlags.IsAnyFlagSet())
    return fail("ordering, acceleration-structure and frame-data pins cannot require texture/buffer binding flags.");
  if (m_Type == Texture && limits.m_Dimension == xiiGALResourceDimension::Buffer)
    return fail("a texture pin cannot require a buffer dimension.");
  if (xiiReflectionUtils::MakeEnumerationValid(xiiGetStaticRTTI<xiiGALBindFlags>(), limits.m_RequiredBindFlags.GetValue()) != limits.m_RequiredBindFlags.GetValue())
    return fail("unknown required resource binding flags.");
  auto combined = *this;
  if (configuration.m_Format != xiiGALResourceFormat::Unknown)
  {
    if (combined.m_Format != xiiGALResourceFormat::Unknown && combined.m_Format != configuration.m_Format)
      return fail("required format conflicts with the producer or native schema.");
    combined.m_Format = configuration.m_Format;
  }
  if (limits.m_Dimension != xiiGALResourceDimension::Undefined)
  {
    if (combined.m_Constraints.m_Dimension != xiiGALResourceDimension::Undefined && combined.m_Constraints.m_Dimension != limits.m_Dimension)
      return fail("required dimensions conflict.");
    combined.m_Constraints.m_Dimension = limits.m_Dimension;
  }
  const auto merge = [](xiiUInt32& current, xiiUInt32 required) {
    if (current && required && current != required)
      return false;
    if (required)
      current = required;
    return true;
  };
  if (!merge(combined.m_Constraints.m_uiSamples, limits.m_uiSamples) || !merge(combined.m_Constraints.m_uiWidth, limits.m_uiWidth) || !merge(combined.m_Constraints.m_uiHeight, limits.m_uiHeight))
    return fail("required samples or extent conflict.");
  combined.m_Constraints.m_RequiredBindFlags.Add(limits.m_RequiredBindFlags);
  combined.m_SourceId = configuration.m_Id;
  combined.m_Id       = configuration.m_Id;
  if (!configuration.m_sDisplayName.IsEmpty())
    combined.m_sDisplayName = configuration.m_sDisplayName;
  *this = std::move(combined);
  return XII_SUCCESS;
}

bool xiiRenderPipelinePin::Accepts(const xiiGALTextureCreationDescription& description) const
{
  return m_Type == Texture && (m_Format == xiiGALResourceFormat::Unknown || m_Format == description.m_Format) &&
    (m_Constraints.m_Dimension == xiiGALResourceDimension::Undefined || m_Constraints.m_Dimension == description.m_Type) &&
    (!m_Constraints.m_uiSamples || m_Constraints.m_uiSamples == description.m_uiSampleCount) &&
    (!m_Constraints.m_uiWidth || m_Constraints.m_uiWidth == description.m_Size.width) &&
    (!m_Constraints.m_uiHeight || m_Constraints.m_uiHeight == description.m_Size.height) &&
    description.m_BindFlags.AreAllSet(m_Constraints.m_RequiredBindFlags);
}

bool xiiRenderPipelinePin::Accepts(const xiiGALBufferCreationDescription& description) const
{
  return m_Type == Buffer && description.m_BindFlags.AreAllSet(m_Constraints.m_RequiredBindFlags);
}

bool xiiRenderPipelinePin::IsCompatibleWith(const xiiRenderPipelinePin& other) const
{
  const auto compatible = [](auto a, auto b, auto unknown) {
    return a == unknown || b == unknown || a == b;
  };
  return m_Type == other.m_Type && (m_Type != FrameData || (m_pDataType && m_pDataType == other.m_pDataType)) && compatible(m_Format, other.m_Format, xiiGALResourceFormat::Unknown) &&
    compatible(m_Constraints.m_Dimension.GetValue(), other.m_Constraints.m_Dimension.GetValue(), xiiGALResourceDimension::Undefined) &&
    compatible(m_Constraints.m_uiSamples, other.m_Constraints.m_uiSamples, 0U) &&
    compatible(m_Constraints.m_uiWidth, other.m_Constraints.m_uiWidth, 0U) &&
    compatible(m_Constraints.m_uiHeight, other.m_Constraints.m_uiHeight, 0U);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelinePin);
