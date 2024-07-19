#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderPipelineNode.h>

// static_assert(sizeof(xiiRenderPipelineNodePin) == 4);

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineNode, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodePin, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_ATTRIBUTES
  {
   new xiiHiddenAttribute(),
  }
  XII_END_ATTRIBUTES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeInputPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodeOutputPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineNodePassThrougPin, xiiRenderPipelineNodePin, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

void xiiRenderPipelineNode::InitializePins()
{
  m_InputPins.Clear();
  m_OutputPins.Clear();
  m_NameToPin.Clear();

  const xiiRTTI* pType = GetDynamicRTTI();

  xiiHybridArray<const xiiAbstractProperty*, 32U> properties;
  pType->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() != xiiPropertyCategory::Member || !pProp->GetSpecificType()->IsDerivedFrom(xiiGetStaticRTTI<xiiRenderPipelineNodePin>()))
      continue;

    auto                      pPinProp = static_cast<const xiiAbstractMemberProperty*>(pProp);
    xiiRenderPipelineNodePin* pPin     = static_cast<xiiRenderPipelineNodePin*>(pPinProp->GetPropertyPointer(this));

    pPin->m_pParent = this;
    if (pPin->m_Type == xiiRenderPipelineNodePin::Type::Unknown)
    {
      XII_REPORT_FAILURE("Pin '{0}' has an invalid type. Do not use xiiRenderPipelineNodePin directly as member but one of its derived types", pProp->GetPropertyName());
      continue;
    }

    if (pPin->m_Type == xiiRenderPipelineNodePin::Type::Input || pPin->m_Type == xiiRenderPipelineNodePin::Type::PassThrough)
    {
      pPin->m_uiInputIndex = static_cast<xiiUInt8>(m_InputPins.GetCount());
      m_InputPins.PushBack(pPin);
    }
    if (pPin->m_Type == xiiRenderPipelineNodePin::Type::Output || pPin->m_Type == xiiRenderPipelineNodePin::Type::PassThrough)
    {
      pPin->m_uiOutputIndex = static_cast<xiiUInt8>(m_OutputPins.GetCount());
      m_OutputPins.PushBack(pPin);
    }

    xiiHashedString sHashedName;
    sHashedName.Assign(pProp->GetPropertyName());
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
  const xiiRenderPipelineNodePin* pin;
  if (m_NameToPin.TryGetValue(sName, pin))
  {
    return pin;
  }
  return nullptr;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelineNode);
