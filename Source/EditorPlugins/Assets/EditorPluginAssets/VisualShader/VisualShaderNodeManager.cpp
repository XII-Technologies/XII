/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <Foundation/Types/ScopeExit.h>

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

namespace
{
  xiiUInt32 NumericWidth(const xiiRTTI* type)
  {
    if (type == xiiGetStaticRTTI<float>())
      return 1;
    if (type == xiiGetStaticRTTI<xiiVec2>())
      return 2;
    if (type == xiiGetStaticRTTI<xiiVec3>())
      return 3;
    if (type == xiiGetStaticRTTI<xiiVec4>() || type == xiiGetStaticRTTI<xiiColor>())
      return 4;
    return 0;
  }

  const xiiRTTI* ResolvePinType(const xiiVisualShaderPin& pin, xiiHybridArray<const xiiPin*, 32>& stack)
  {
    if (!pin.GetDescriptor()->m_bDynamicNumeric)
      return pin.GetDescriptor()->m_pDataType;
    if (stack.Contains(&pin) || stack.GetCount() >= 256)
      return nullptr;
    stack.PushBack(&pin);
    XII_SCOPE_EXIT(stack.PopBack());
    const auto*    manager = static_cast<const xiiVisualShaderNodeManager*>(pin.GetParent()->GetDocumentObjectManager());
    const xiiRTTI* result  = xiiGetStaticRTTI<float>();
    for (const auto& input : manager->GetInputPins(pin.GetParent()))
    {
      const auto* typed = static_cast<const xiiVisualShaderPin*>(input.Borrow());
      if (!typed->GetDescriptor()->m_bDynamicNumeric)
        continue;
      for (const auto* connection : manager->GetConnections(*input))
      {
        const auto* sourceType = ResolvePinType(static_cast<const xiiVisualShaderPin&>(connection->GetSourcePin()), stack);
        const auto  width      = NumericWidth(sourceType);
        if (width == 0 || (NumericWidth(result) > 1 && width > 1 && NumericWidth(result) != width))
          return nullptr;
        if (width > NumericWidth(result))
          result = sourceType;
      }
    }
    return result;
  }
} // namespace

const xiiRTTI* xiiVisualShaderPin::GetDataType() const
{
  xiiHybridArray<const xiiPin*, 32> stack;
  return ResolvePinType(*this, stack);
}

bool xiiVisualShaderPin::CanConvert(const xiiVisualShaderPin& source, const xiiVisualShaderPin& target)
{
  const auto* sourceType = source.GetDataType();
  const auto* targetType = target.GetDataType();
  if (sourceType == nullptr || targetType == nullptr)
    return false;
  if (sourceType == targetType)
    return true;
  const auto sourceWidth = NumericWidth(sourceType), targetWidth = NumericWidth(targetType);
  if (sourceWidth == 1 && targetWidth > 1)
    return true; // Scalar splat preserves the scalar value in every component.
  if (target.GetDescriptor()->m_bDynamicNumeric && sourceWidth > 0 && targetWidth == 1)
    return true; // First vector connection determines the arithmetic node's shape.
  if (sourceWidth == 4 && targetWidth == 4)
    return true; // Linear RGBA and float4 share the same representation.
  return sourceType == xiiGetStaticRTTI<xiiColor>() && targetType == xiiGetStaticRTTI<xiiVec3>();
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

  xiiRTTI::ForEachDerivedType(pNodeBaseType, [&](const xiiRTTI* pRtti) {
      auto& nodeTemplate   = out_templates.ExpandAndGetRef();
      nodeTemplate.m_pType = pRtti;

      if (const xiiVisualShaderNodeDescriptor* pDesc = xiiVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pRtti))
      {
        nodeTemplate.m_sCategory = pDesc->m_sCategory;
      } }, xiiRTTI::ForEachOptions::ExcludeAbstract);
}

xiiStatus xiiVisualShaderNodeManager::InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_result) const
{
  const xiiVisualShaderPin& pinSource = xiiStaticCast<const xiiVisualShaderPin&>(source);
  const xiiVisualShaderPin& pinTarget = xiiStaticCast<const xiiVisualShaderPin&>(target);

  if (!xiiVisualShaderPin::CanConvert(pinSource, pinTarget))
  {
    out_result = CanConnectResult::ConnectNever;
    return xiiStatus(xiiFmt("Incompatible Material Graph pins '{}' and '{}'. Use an explicit conversion node for component extraction or padding.", source.GetName(), target.GetName()));
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

    // The shader compiler validates the selected backend's actual resources.
    // Disconnected nodes and repeated sampling do not consume extra bindings.
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
