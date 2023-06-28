#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraph.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptTypeRegistry.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////
// xiiVisualScriptPin
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptPin_Legacy, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiVisualScriptPin_Legacy::xiiVisualScriptPin_Legacy(Type type, const xiiVisualScriptPinDescriptor* pDescriptor, const xiiDocumentObject* pObject) :
  xiiPin(type, pDescriptor->m_sName, pDescriptor->m_Color, pObject)
{
  m_pDescriptor = pDescriptor;
  if (pDescriptor->m_PinType == xiiVisualScriptPinDescriptor::PinType::Data)
  {
    m_Shape = Shape::Rect;
  }
}

const xiiString& xiiVisualScriptPin_Legacy::GetTooltip() const
{
  return m_pDescriptor->m_sTooltip;
}

//////////////////////////////////////////////////////////////////////////
// xiiVisualScriptConnection
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptConnection_Legacy, 2, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////
// xiiVisualScriptNodeManager
//////////////////////////////////////////////////////////////////////////

bool xiiVisualScriptNodeManager_Legacy::InternalIsNode(const xiiDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(xiiVisualScriptTypeRegistry::GetSingleton()->GetNodeBaseType());
}

void xiiVisualScriptNodeManager_Legacy::InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& ref_node)
{
  const auto* pDesc = xiiVisualScriptTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType());

  if (pDesc == nullptr)
    return;

  ref_node.m_Inputs.Reserve(pDesc->m_InputPins.GetCount());
  ref_node.m_Outputs.Reserve(pDesc->m_OutputPins.GetCount());

  for (const auto& pinDesc : pDesc->m_InputPins)
  {
    auto pPin = XII_DEFAULT_NEW(xiiVisualScriptPin_Legacy, xiiPin::Type::Input, &pinDesc, pObject);
    ref_node.m_Inputs.PushBack(pPin);
  }

  for (const auto& pinDesc : pDesc->m_OutputPins)
  {
    auto pPin = XII_DEFAULT_NEW(xiiVisualScriptPin_Legacy, xiiPin::Type::Output, &pinDesc, pObject);
    ref_node.m_Outputs.PushBack(pPin);
  }
}

void xiiVisualScriptNodeManager_Legacy::GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& ref_types) const
{
  const xiiRTTI* pNodeBaseType = xiiVisualScriptTypeRegistry::GetSingleton()->GetNodeBaseType();

  for (auto it = xiiRTTI::GetFirstInstance(); it != nullptr; it = it->GetNextInstance())
  {
    if (it->IsDerivedFrom(pNodeBaseType) && !it->GetTypeFlags().IsSet(xiiTypeFlags::Abstract))
      ref_types.PushBack(it);
  }
}

const xiiRTTI* xiiVisualScriptNodeManager_Legacy::GetConnectionType() const
{
  return xiiGetStaticRTTI<xiiVisualScriptConnection_Legacy>();
}

xiiStatus xiiVisualScriptNodeManager_Legacy::InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_result) const
{
  const xiiVisualScriptPin_Legacy& pinSource = xiiStaticCast<const xiiVisualScriptPin_Legacy&>(source);
  const xiiVisualScriptPin_Legacy& pinTarget = xiiStaticCast<const xiiVisualScriptPin_Legacy&>(target);

  if (pinSource.GetDescriptor()->m_PinType != pinTarget.GetDescriptor()->m_PinType)
  {
    out_result = CanConnectResult::ConnectNever;
    return xiiStatus("Cannot connect data pins with execution pins.");
  }

  if (pinSource.GetDescriptor()->m_PinType == xiiVisualScriptPinDescriptor::PinType::Data &&
      pinSource.GetDescriptor()->m_DataType != pinTarget.GetDescriptor()->m_DataType)
  {
    xiiVisualScriptInstance::SetupPinDataTypeConversions();

    if (xiiVisualScriptInstance::FindDataPinAssignFunction(pinSource.GetDescriptor()->m_DataType, pinTarget.GetDescriptor()->m_DataType) ==
        nullptr)
    {
      out_result = CanConnectResult::ConnectNever;
      return xiiStatus(xiiFmt("The pin data types are incompatible."));
    }
  }

  if (WouldConnectionCreateCircle(source, target))
  {
    out_result = CanConnectResult::ConnectNever;
    return xiiStatus("Connecting these pins would create a circle in the graph.");
  }

  // only one connection is allowed on DATA input pins, execution input pins may have multiple incoming connections
  if (pinTarget.GetDescriptor()->m_PinType == xiiVisualScriptPinDescriptor::PinType::Data && HasConnections(pinTarget))
  {
    out_result = CanConnectResult::ConnectNto1;
    return xiiStatus(XII_FAILURE);
  }

  // only one outgoing connection is allowed on EXECUTION pins, data pins may have multiple outgoing connections
  if (pinSource.GetDescriptor()->m_PinType == xiiVisualScriptPinDescriptor::PinType::Execution && HasConnections(pinSource))
  {
    out_result = CanConnectResult::Connect1toN;
    return xiiStatus(XII_FAILURE);
  }

  out_result = CanConnectResult::ConnectNtoN;
  return xiiStatus(XII_SUCCESS);
}

const char* xiiVisualScriptNodeManager_Legacy::GetTypeCategory(const xiiRTTI* pRtti) const
{
  const xiiVisualScriptNodeDescriptor* pDesc = xiiVisualScriptTypeRegistry::GetSingleton()->GetDescriptorForType(pRtti);

  if (pDesc == nullptr)
    return nullptr;

  return pDesc->m_sCategory;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiVisualScriptConnectionPatch_1_2 : public xiiGraphPatch
{
public:
  xiiVisualScriptConnectionPatch_1_2() :
    xiiGraphPatch("xiiVisualScriptConnection", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("xiiVisualScriptConnection_Legacy");
  }
};

xiiVisualScriptConnectionPatch_1_2 g_xiiVisualScriptConnectionPatch_1_2;
