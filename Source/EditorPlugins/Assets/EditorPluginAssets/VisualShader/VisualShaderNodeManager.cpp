/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>

//////////////////////////////////////////////////////////////////////////
// xiiVisualShaderPin
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualShaderPin, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiVisualShaderPin::xiiVisualShaderPin(Type type, const xiiVisualShaderPinDescriptor* pDescriptor, const xiiDocumentObject* pObject) :
  xiiPin(type, pDescriptor->m_sName, pDescriptor->m_Color, pObject)
{
  m_pDescriptor = pDescriptor;
}

const xiiRTTI* xiiVisualShaderPin::GetDataType() const
{
  return m_pDescriptor->m_pDataType;
}

const xiiString& xiiVisualShaderPin::GetTooltip() const
{
  return m_pDescriptor->m_sTooltip;
}

//////////////////////////////////////////////////////////////////////////
// xiiVisualShaderNodeManager
//////////////////////////////////////////////////////////////////////////

bool xiiVisualShaderNodeManager::InternalIsNode(const xiiDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(xiiVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType());
}

void xiiVisualShaderNodeManager::InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& ref_node)
{
  const auto* pDesc = xiiVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType());

  if (pDesc == nullptr)
    return;

  ref_node.m_Inputs.Reserve(pDesc->m_InputPins.GetCount());
  ref_node.m_Outputs.Reserve(pDesc->m_OutputPins.GetCount());

  for (const auto& pin : pDesc->m_InputPins)
  {
    auto pPin = XII_DEFAULT_NEW(xiiVisualShaderPin, xiiPin::Type::Input, &pin, pObject);
    ref_node.m_Inputs.PushBack(pPin);
  }

  for (const auto& pin : pDesc->m_OutputPins)
  {
    auto pPin = XII_DEFAULT_NEW(xiiVisualShaderPin, xiiPin::Type::Output, &pin, pObject);
    ref_node.m_Outputs.PushBack(pPin);
  }
}

void xiiVisualShaderNodeManager::GetNodeCreationTemplates(xiiDynamicArray<xiiNodeCreationTemplate>& out_templates) const
{
  const xiiRTTI* pNodeBaseType = xiiVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

  xiiRTTI::ForEachDerivedType(
    pNodeBaseType,
    [&](const xiiRTTI* pRtti) {
      auto& nodeTemplate   = out_templates.ExpandAndGetRef();
      nodeTemplate.m_pType = pRtti;

      if (const xiiVisualShaderNodeDescriptor* pDesc = xiiVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pRtti))
      {
        nodeTemplate.m_sCategory = pDesc->m_sCategory;
      }
    },
    xiiRTTI::ForEachOptions::ExcludeAbstract);
}

xiiStatus xiiVisualShaderNodeManager::InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_result) const
{
  const xiiVisualShaderPin& pinSource = xiiStaticCast<const xiiVisualShaderPin&>(source);
  const xiiVisualShaderPin& pinTarget = xiiStaticCast<const xiiVisualShaderPin&>(target);

  const xiiRTTI* pSamplerType = xiiVisualShaderTypeRegistry::GetSingleton()->GetPinSamplerType();
  const xiiRTTI* pStringType  = xiiGetStaticRTTI<xiiString>();

  if ((pinSource.GetDataType() == pSamplerType && pinTarget.GetDataType() != pSamplerType) || (pinSource.GetDataType() != pSamplerType && pinTarget.GetDataType() == pSamplerType))
  {
    out_result = CanConnectResult::ConnectNever;
    return xiiStatus("Pin of type 'sampler' cannot be connected with a pin of a different type.");
  }

  if ((pinSource.GetDataType() == pStringType && pinTarget.GetDataType() != pStringType) || (pinSource.GetDataType() != pStringType && pinTarget.GetDataType() == pStringType))
  {
    out_result = CanConnectResult::ConnectNever;
    return xiiStatus("Pin of type 'string' cannot be connected with a pin of a different type.");
  }

  if (WouldConnectionCreateCircle(source, target))
  {
    out_result = CanConnectResult::ConnectNever;
    return xiiStatus("Connecting these pins would create a circle in the shader graph.");
  }

  out_result = CanConnectResult::ConnectNto1;
  return XII_SUCCESS;
}


xiiStatus xiiVisualShaderNodeManager::InternalCanAdd(const xiiRTTI* pRtti, const xiiDocumentObject* pParent, xiiStringView sParentProperty, const xiiVariant& index) const
{
  auto pDesc = xiiVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pRtti);

  if (pDesc)
  {
    if (pDesc->m_NodeType == xiiVisualShaderNodeType::Main && CountNodesOfType(xiiVisualShaderNodeType::Main) > 0)
    {
      return xiiStatus("The shader may only contain a single output node");
    }

    /// \todo This is an arbitrary limit and it does not count how many nodes reference the same texture
    static constexpr xiiUInt32 uiMaxTextures = 16;
    if (pDesc->m_NodeType == xiiVisualShaderNodeType::Texture && CountNodesOfType(xiiVisualShaderNodeType::Texture) >= uiMaxTextures)
    {
      return xiiStatus(xiiFmt("The maximum number of texture nodes is {0}", uiMaxTextures));
    }
  }

  return XII_SUCCESS;
}

xiiUInt32 xiiVisualShaderNodeManager::CountNodesOfType(xiiVisualShaderNodeType::Enum type) const
{
  xiiUInt32 count = 0;

  const xiiVisualShaderTypeRegistry* pRegistry = xiiVisualShaderTypeRegistry::GetSingleton();

  const auto& children = GetRootObject()->GetChildren();
  for (xiiUInt32 i = 0; i < children.GetCount(); ++i)
  {
    auto pDesc = pRegistry->GetDescriptorForType(children[i]->GetType());

    if (pDesc && pDesc->m_NodeType == type)
    {
      ++count;
    }
  }

  return count;
}
