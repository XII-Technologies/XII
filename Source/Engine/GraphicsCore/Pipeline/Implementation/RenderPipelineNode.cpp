#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderPipelineNode.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiRenderPipelineNodePinFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiRenderPipelineNodePinFlags::Unknown),
  XII_BITFLAGS_CONSTANT(xiiRenderPipelineNodePinFlags::Input),
  XII_BITFLAGS_CONSTANT(xiiRenderPipelineNodePinFlags::Output),
  XII_BITFLAGS_CONSTANT(xiiRenderPipelineNodePinFlags::PassThrough),
  XII_BITFLAGS_CONSTANT(xiiRenderPipelineNodePinFlags::ResourceProvider),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRenderPipelineNodePinResourceType, 1)
  XII_ENUM_CONSTANT(xiiRenderPipelineNodePinResourceType::Unknown),
  XII_ENUM_CONSTANT(xiiRenderPipelineNodePinResourceType::Buffer),
  XII_ENUM_CONSTANT(xiiRenderPipelineNodePinResourceType::ColourAttachment),
  XII_ENUM_CONSTANT(xiiRenderPipelineNodePinResourceType::DepthAttachment),
  XII_ENUM_CONSTANT(xiiRenderPipelineNodePinResourceType::Sampler),
  XII_ENUM_CONSTANT(xiiRenderPipelineNodePinResourceType::AccelerationStructure),
  XII_ENUM_CONSTANT(xiiRenderPipelineNodePinResourceType::StorageImage),
  XII_ENUM_CONSTANT(xiiRenderPipelineNodePinResourceType::ReadWriteBuffer),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodePin, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_ATTRIBUTES
  {
   new xiiHiddenAttribute(),
  }
  XII_END_ATTRIBUTES;
}
XII_END_STATIC_REFLECTED_TYPE;

// Input Pins.
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeInputBufferPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeInputColourAttachmentPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeInputDepthAttachmentPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeInputSamplerPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeInputAccelerationStructurePin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;


// Output Pins.
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeOutputBufferPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeOutputColourAttachmentPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
    XII_ENUM_MEMBER_PROPERTY("Format", xiiSourceFormat, m_Format)->AddAttributes(new xiiDefaultValueAttribute(xiiSourceFormat::Color4Channel8BitNormalized_sRGB)),
    XII_ENUM_MEMBER_PROPERTY("SampleCount", xiiGALMSAASampleCount, m_SampleCount)->AddAttributes(new xiiDefaultValueAttribute(xiiGALMSAASampleCount::OneSample)),
    XII_ENUM_MEMBER_PROPERTY("LoadOperation", xiiGALAttachmentLoadOperation, m_AttachmentLoadOperation)->AddAttributes(new xiiDefaultValueAttribute(xiiGALAttachmentLoadOperation::Load)),
    XII_ENUM_MEMBER_PROPERTY("StoreOperation", xiiGALAttachmentStoreOperation, m_AttachmentStoreOperation)->AddAttributes(new xiiDefaultValueAttribute(xiiGALAttachmentStoreOperation::Store)),
    XII_MEMBER_PROPERTY("ClearColor", m_ClearColor)->AddAttributes(new xiiExposeColorAlphaAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeOutputDepthAttachmentPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
    XII_ENUM_MEMBER_PROPERTY("Format", xiiSourceFormat, m_Format)->AddAttributes(new xiiDefaultValueAttribute(xiiSourceFormat::Color4Channel8BitNormalized_sRGB)),
    XII_ENUM_MEMBER_PROPERTY("SampleCount", xiiGALMSAASampleCount, m_SampleCount)->AddAttributes(new xiiDefaultValueAttribute(xiiGALMSAASampleCount::OneSample)),
    XII_ENUM_MEMBER_PROPERTY("LoadOperation", xiiGALAttachmentLoadOperation, m_AttachmentLoadOperation)->AddAttributes(new xiiDefaultValueAttribute(xiiGALAttachmentLoadOperation::Load)),
    XII_ENUM_MEMBER_PROPERTY("StoreOperation", xiiGALAttachmentStoreOperation, m_AttachmentStoreOperation)->AddAttributes(new xiiDefaultValueAttribute(xiiGALAttachmentStoreOperation::Store)),
    XII_ENUM_MEMBER_PROPERTY("StencilLoadOperation", xiiGALAttachmentLoadOperation, m_AttachmentStencilLoadOperation)->AddAttributes(new xiiDefaultValueAttribute(xiiGALAttachmentLoadOperation::Load)),
    XII_ENUM_MEMBER_PROPERTY("StencilStoreOperation", xiiGALAttachmentStoreOperation, m_AttachmentStencilStoreOperation)->AddAttributes(new xiiDefaultValueAttribute(xiiGALAttachmentStoreOperation::Store)),
    XII_MEMBER_PROPERTY("DepthClearValue", m_fDepthClearValue)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("StencilClearValue", m_uiStencilClearValue)->AddAttributes(new xiiDefaultValueAttribute(0U)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeOutputSamplerPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeOutputAccelerationStructurePin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;


// Pass-Through Pins.
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodePassThroughBufferPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodePassThroughColourAttachmentPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodePassThroughDepthAttachmentPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodePassThroughSamplerPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodePassThroughAccelerationStructurePin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;


// Input-Provider Pins.
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeInputBufferProviderPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeInputColourAttachmentProviderPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeInputDepthAttachmentProviderPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeInputSamplerProviderPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeInputAccelerationStructureProviderPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;


// Output-Provider Pins.
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeOutputBufferProviderPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeOutputColourAttachmentProviderPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeOutputDepthAttachmentProviderPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeOutputSamplerProviderPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeOutputAccelerationStructureProviderPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY("Flags", xiiRenderPipelineNodePinFlags, GetFlags),
    XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY("ResourceType", xiiRenderPipelineNodePinResourceType, GetResourceType),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;


XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineNode, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

///////////////////////////////////////////////////////////////////////////////

xiiResult xiiRenderPipelineNodeOutputColourAttachmentPin::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_Format;
  inout_stream << m_SampleCount;
  inout_stream << m_AttachmentLoadOperation;
  inout_stream << m_AttachmentStoreOperation;
  inout_stream << m_ClearColor;

  return XII_SUCCESS;
}

xiiResult xiiRenderPipelineNodeOutputColourAttachmentPin::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_Format;
  inout_stream >> m_SampleCount;
  inout_stream >> m_AttachmentLoadOperation;
  inout_stream >> m_AttachmentStoreOperation;
  inout_stream >> m_ClearColor;

  return XII_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////

xiiResult xiiRenderPipelineNodeOutputDepthAttachmentPin::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_Format;
  inout_stream << m_SampleCount;
  inout_stream << m_AttachmentLoadOperation;
  inout_stream << m_AttachmentStoreOperation;
  inout_stream << m_AttachmentStencilLoadOperation;
  inout_stream << m_AttachmentStencilStoreOperation;
  inout_stream << m_fDepthClearValue;
  inout_stream << m_uiStencilClearValue;

  return XII_SUCCESS;
}

xiiResult xiiRenderPipelineNodeOutputDepthAttachmentPin::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_Format;
  inout_stream >> m_SampleCount;
  inout_stream >> m_AttachmentLoadOperation;
  inout_stream >> m_AttachmentStoreOperation;
  inout_stream >> m_AttachmentStencilLoadOperation;
  inout_stream >> m_AttachmentStencilStoreOperation;
  inout_stream >> m_fDepthClearValue;
  inout_stream >> m_uiStencilClearValue;

  return XII_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////

void xiiRenderPipelineNode::InitializePins()
{
  m_InputPins.Clear();
  m_OutputPins.Clear();
  m_NameToPin.Clear();

  const xiiRTTI* pType = GetDynamicRTTI();

  xiiHybridArray<const xiiAbstractProperty*, 32U> properties;
  pType->GetAllProperties(properties);

  for (auto pProperty : properties)
  {
    if (pProperty->GetCategory() != xiiPropertyCategory::Member || !pProperty->GetSpecificType()->IsDerivedFrom(xiiGetStaticRTTI<xiiRenderPipelineNodePin>()))
      continue;

    auto                      pPinProperty = static_cast<const xiiAbstractMemberProperty*>(pProperty);
    xiiRenderPipelineNodePin* pPin         = static_cast<xiiRenderPipelineNodePin*>(pPinProperty->GetPropertyPointer(this));

    pPin->m_pParent                   = this;
    const bool bMoreThanOneType       = ((xiiInt32)pPin->m_Flags.IsSet(xiiRenderPipelineNodePinFlags::PassThrough) + (xiiInt32)pPin->m_Flags.IsSet(xiiRenderPipelineNodePinFlags::Input) + (xiiInt32)pPin->m_Flags.IsSet(xiiRenderPipelineNodePinFlags::Output)) > 1;
    const bool bProviderOnPassThrough = pPin->m_Flags.IsSet(xiiRenderPipelineNodePinFlags::PassThrough) && pPin->m_Flags.IsSet(xiiRenderPipelineNodePinFlags::ResourceProvider);
    if (bMoreThanOneType || bProviderOnPassThrough)
    {
      XII_REPORT_FAILURE("Pin '{0}' has an invalid type. Do not use xiiRenderPipelineNodePin directly as member but one of its derived types.", pProperty->GetPropertyName());
      continue;
    }

    if (pPin->m_Flags.IsAnySet(xiiRenderPipelineNodePinFlags::Input | xiiRenderPipelineNodePinFlags::PassThrough))
    {
      pPin->m_uiInputIndex = static_cast<xiiUInt8>(m_InputPins.GetCount());
      m_InputPins.PushBack(pPin);
    }
    if (pPin->m_Flags.IsAnySet(xiiRenderPipelineNodePinFlags::Output | xiiRenderPipelineNodePinFlags::PassThrough))
    {
      pPin->m_uiOutputIndex = static_cast<xiiUInt8>(m_OutputPins.GetCount());
      m_OutputPins.PushBack(pPin);
    }

    xiiHashedString sHashedName;
    sHashedName.Assign(pProperty->GetPropertyName());
    m_NameToPin.Insert(sHashedName, pPin);
  }
}

xiiHashedString xiiRenderPipelineNode::GetPinName(const xiiRenderPipelineNodePin* pPin) const
{
  for (auto it = m_NameToPin.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value() == pPin)
    {
      return it.Key();
    }
  }
  return xiiHashedString();
}

const xiiRenderPipelineNodePin* xiiRenderPipelineNode::GetPinByName(xiiStringView sName) const
{
  xiiHashedString sHashedName;
  sHashedName.Assign(sName);
  return GetPinByName(sHashedName);
}

const xiiRenderPipelineNodePin* xiiRenderPipelineNode::GetPinByName(xiiHashedString sName) const
{
  const xiiRenderPipelineNodePin* pRenderPipelineNodePin;
  if (m_NameToPin.TryGetValue(sName, pRenderPipelineNodePin))
  {
    return pRenderPipelineNodePin;
  }
  return nullptr;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelineNode);
