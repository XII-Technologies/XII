#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationControllerAsset/AnimationControllerAsset.h>
#include <EditorPluginAssets/AnimationControllerAsset/AnimationControllerGraphQt.h>
#include <Foundation/Math/ColorScheme.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationControllerAssetDocument, 3, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationControllerNodePin, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, AnimationController)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiQtNodeScene::GetNodeFactory().RegisterCreator(xiiGetStaticRTTI<xiiAnimGraphNode>(), [](const xiiRTTI* pRtti)->xiiQtNode* { return new xiiQtAnimationControllerNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiQtNodeScene::GetNodeFactory().UnregisterCreator(xiiGetStaticRTTI<xiiAnimGraphNode>());
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

bool xiiAnimationControllerNodeManager::InternalIsNode(const xiiDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom<xiiAnimGraphNode>();
}

void xiiAnimationControllerNodeManager::InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& node)
{
  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom<xiiAnimGraphNode>())
    return;

  xiiHybridArray<xiiAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  const xiiColor triggerPinColor   = xiiColorScheme::DarkUI(xiiColorScheme::Grape);
  const xiiColor numberPinColor    = xiiColorScheme::DarkUI(xiiColorScheme::Lime);
  const xiiColor weightPinColor    = xiiColorScheme::DarkUI(xiiColorScheme::Teal);
  const xiiColor localPosePinColor = xiiColorScheme::DarkUI(xiiColorScheme::Blue);
  const xiiColor modelPosePinColor = xiiColorScheme::DarkUI(xiiColorScheme::Violet);
  // EXTEND THIS if a new type is introduced

  for (xiiAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != xiiPropertyCategory::Member || !pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphPin>())
      continue;

    if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphTriggerInputPin>())
    {
      auto pPin        = XII_DEFAULT_NEW(xiiAnimationControllerNodePin, xiiPin::Type::Input, pProp->GetPropertyName(), triggerPinColor, pObject);
      pPin->m_DataType = xiiAnimGraphPin::Trigger;
      node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphTriggerOutputPin>())
    {
      auto pPin        = XII_DEFAULT_NEW(xiiAnimationControllerNodePin, xiiPin::Type::Output, pProp->GetPropertyName(), triggerPinColor, pObject);
      pPin->m_DataType = xiiAnimGraphPin::Trigger;
      node.m_Outputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphNumberInputPin>())
    {
      auto pPin        = XII_DEFAULT_NEW(xiiAnimationControllerNodePin, xiiPin::Type::Input, pProp->GetPropertyName(), numberPinColor, pObject);
      pPin->m_DataType = xiiAnimGraphPin::Number;
      node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphNumberOutputPin>())
    {
      auto pPin        = XII_DEFAULT_NEW(xiiAnimationControllerNodePin, xiiPin::Type::Output, pProp->GetPropertyName(), numberPinColor, pObject);
      pPin->m_DataType = xiiAnimGraphPin::Number;
      node.m_Outputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphBoneWeightsInputPin>())
    {
      auto pPin        = XII_DEFAULT_NEW(xiiAnimationControllerNodePin, xiiPin::Type::Input, pProp->GetPropertyName(), weightPinColor, pObject);
      pPin->m_DataType = xiiAnimGraphPin::BoneWeights;
      node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphBoneWeightsOutputPin>())
    {
      auto pPin        = XII_DEFAULT_NEW(xiiAnimationControllerNodePin, xiiPin::Type::Output, pProp->GetPropertyName(), weightPinColor, pObject);
      pPin->m_DataType = xiiAnimGraphPin::BoneWeights;
      node.m_Outputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphLocalPoseInputPin>())
    {
      auto pPin        = XII_DEFAULT_NEW(xiiAnimationControllerNodePin, xiiPin::Type::Input, pProp->GetPropertyName(), localPosePinColor, pObject);
      pPin->m_DataType = xiiAnimGraphPin::LocalPose;
      node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphLocalPoseMultiInputPin>())
    {
      auto pPin              = XII_DEFAULT_NEW(xiiAnimationControllerNodePin, xiiPin::Type::Input, pProp->GetPropertyName(), localPosePinColor, pObject);
      pPin->m_DataType       = xiiAnimGraphPin::LocalPose;
      pPin->m_bMultiInputPin = true;
      pPin->m_Shape          = xiiPin::Shape::RoundRect;
      node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphLocalPoseOutputPin>())
    {
      auto pPin        = XII_DEFAULT_NEW(xiiAnimationControllerNodePin, xiiPin::Type::Output, pProp->GetPropertyName(), localPosePinColor, pObject);
      pPin->m_DataType = xiiAnimGraphPin::LocalPose;
      node.m_Outputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphModelPoseInputPin>())
    {
      auto pPin        = XII_DEFAULT_NEW(xiiAnimationControllerNodePin, xiiPin::Type::Input, pProp->GetPropertyName(), modelPosePinColor, pObject);
      pPin->m_DataType = xiiAnimGraphPin::ModelPose;
      node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphModelPoseOutputPin>())
    {
      auto pPin        = XII_DEFAULT_NEW(xiiAnimationControllerNodePin, xiiPin::Type::Output, pProp->GetPropertyName(), modelPosePinColor, pObject);
      pPin->m_DataType = xiiAnimGraphPin::ModelPose;
      node.m_Outputs.PushBack(pPin);
    }
    else
    {
      // EXTEND THIS if a new type is introduced
      XII_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

void xiiAnimationControllerNodeManager::GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& Types) const
{
  xiiSet<const xiiRTTI*> typeSet;
  xiiReflectionUtils::GatherTypesDerivedFromClass(xiiGetStaticRTTI<xiiAnimGraphNode>(), typeSet, false);

  Types.Clear();
  for (auto pType : typeSet)
  {
    if (pType->GetTypeFlags().IsAnySet(xiiTypeFlags::Abstract))
      continue;

    Types.PushBack(pType);
  }
}

xiiStatus xiiAnimationControllerNodeManager::InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_Result) const
{
  const xiiAnimationControllerNodePin& sourcePin = xiiStaticCast<const xiiAnimationControllerNodePin&>(source);
  const xiiAnimationControllerNodePin& targetPin = xiiStaticCast<const xiiAnimationControllerNodePin&>(target);

  out_Result = CanConnectResult::ConnectNever;

  if (sourcePin.m_DataType != targetPin.m_DataType)
    return xiiStatus("Can't connect pins of different data types");

  if (sourcePin.GetType() == targetPin.GetType())
    return xiiStatus("Can only connect input pins with output pins.");

  switch (sourcePin.m_DataType)
  {
    case xiiAnimGraphPin::Trigger:
      out_Result = CanConnectResult::ConnectNtoN;
      break;

    case xiiAnimGraphPin::Number:
      out_Result = CanConnectResult::ConnectNto1;
      break;

    case xiiAnimGraphPin::BoneWeights:
      out_Result = CanConnectResult::ConnectNto1;
      break;

    case xiiAnimGraphPin::LocalPose:
      if (targetPin.m_bMultiInputPin)
        out_Result = CanConnectResult::ConnectNtoN;
      else
        out_Result = CanConnectResult::ConnectNto1;
      break;

    case xiiAnimGraphPin::ModelPose:
      out_Result = CanConnectResult::ConnectNto1;
      break;

      // EXTEND THIS if a new type is introduced
      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return xiiStatus(XII_SUCCESS);
}

xiiAnimationControllerAssetDocument::xiiAnimationControllerAssetDocument(const char* szDocumentPath) :
  xiiAssetDocument(szDocumentPath, XII_DEFAULT_NEW(xiiAnimationControllerNodeManager), xiiAssetDocEngineConnection::None)
{
}

xiiTransformStatus xiiAnimationControllerAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  const auto* pNodeManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());

  xiiDynamicArray<const xiiDocumentObject*> allNodes;
  xiiMap<xiiUInt8, PinCount>                pinCounts;
  CountPinTypes(pNodeManager, allNodes, pinCounts);

  // if the asset is entirely empty, don't complain
  if (allNodes.IsEmpty())
    return xiiStatus(XII_SUCCESS);

  SortNodesByPriority(allNodes);

  if (allNodes.IsEmpty())
  {
    return xiiStatus("Animation controller graph doesn't have any output nodes.");
  }

  xiiAnimGraph animController;
  animController.m_TriggerInputPinStates.SetCount(pinCounts[xiiAnimGraphPin::Trigger].m_uiInputCount);
  animController.m_NumberInputPinStates.SetCount(pinCounts[xiiAnimGraphPin::Number].m_uiInputCount);
  animController.m_BoneWeightInputPinStates.SetCount(pinCounts[xiiAnimGraphPin::BoneWeights].m_uiInputCount);
  animController.m_LocalPoseInputPinStates.SetCount(pinCounts[xiiAnimGraphPin::LocalPose].m_uiInputCount);
  animController.m_ModelPoseInputPinStates.SetCount(pinCounts[xiiAnimGraphPin::ModelPose].m_uiInputCount);
  // EXTEND THIS if a new type is introduced

  for (xiiUInt32 i = 0; i < xiiAnimGraphPin::ENUM_COUNT; ++i)
  {
    animController.m_OutputPinToInputPinMapping[i].SetCount(pinCounts[i].m_uiOutputCount);
  }

  auto pIdxProperty = static_cast<xiiAbstractMemberProperty*>(xiiAnimGraphPin::GetStaticRTTI()->FindPropertyByName("PinIdx", false));
  XII_ASSERT_DEBUG(pIdxProperty, "Missing PinIdx property");
  auto pNumProperty = static_cast<xiiAbstractMemberProperty*>(xiiAnimGraphPin::GetStaticRTTI()->FindPropertyByName("NumConnections", false));
  XII_ASSERT_DEBUG(pNumProperty, "Missing NumConnections property");

  xiiDynamicArray<xiiAnimGraphNode*> newNodes;
  newNodes.Reserve(allNodes.GetCount());

  CreateOutputGraphNodes(allNodes, animController, newNodes);

  xiiMap<const xiiPin*, xiiUInt16> inputPinIndices;
  SetInputPinIndices(newNodes, allNodes, pNodeManager, pinCounts, inputPinIndices, pIdxProperty, pNumProperty);
  SetOutputPinIndices(newNodes, allNodes, pNodeManager, pinCounts, animController, pIdxProperty, inputPinIndices);

  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  XII_SUCCEED_OR_RETURN(animController.Serialize(writer));

  stream << storage.GetStorageSize32();
  return storage.CopyToStream(stream);
}

static void AssignNodePriority(const xiiDocumentObject* pNode, xiiUInt16 curPrio, xiiMap<const xiiDocumentObject*, xiiUInt16>& prios, const xiiDocumentNodeManager* pNodeManager)
{
  prios[pNode] = xiiMath::Min(prios[pNode], curPrio);

  const auto inputPins = pNodeManager->GetInputPins(pNode);

  for (auto& pPin : inputPins)
  {
    for (auto pConnection : pNodeManager->GetConnections(*pPin))
    {
      AssignNodePriority(pConnection->GetSourcePin().GetParent(), curPrio - 1, prios, pNodeManager);
    }
  }
}

void xiiAnimationControllerAssetDocument::SortNodesByPriority(xiiDynamicArray<const xiiDocumentObject*>& allNodes)
{
  // starts at output nodes (which have no output pins) and walks back recursively over the connections on their input nodes
  // until it reaches the end of the graph
  // assigns decreasing priorities to the nodes that it finds
  // thus it generates a weak order in which the nodes should be stepped at runtime

  const auto* pNodeManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());

  xiiMap<const xiiDocumentObject*, xiiUInt16> prios;
  for (const xiiDocumentObject* pNode : allNodes)
  {
    prios[pNode] = 0xFFFF;
  }

  for (const xiiDocumentObject* pNode : allNodes)
  {
    // only look at the final nodes in the graph
    if (pNodeManager->GetOutputPins(pNode).IsEmpty())
    {
      AssignNodePriority(pNode, 0xFFFE, prios, pNodeManager);
    }
  }

  // remove unreachable nodes
  for (xiiUInt32 i = allNodes.GetCount(); i > 0; --i)
  {
    if (prios[allNodes[i - 1]] == 0xFFFF)
    {
      allNodes.RemoveAtAndSwap(i - 1);
    }
  }

  allNodes.Sort([&](auto lhs, auto rhs) -> bool { return prios[lhs] < prios[rhs]; });
}

void xiiAnimationControllerAssetDocument::SetOutputPinIndices(const xiiDynamicArray<xiiAnimGraphNode*>& newNodes, const xiiDynamicArray<const xiiDocumentObject*>& allNodes, const xiiDocumentNodeManager* pNodeManager, xiiMap<xiiUInt8, PinCount>& pinCounts, xiiAnimGraph& animController, xiiAbstractMemberProperty* pIdxProperty, const xiiMap<const xiiPin*, xiiUInt16>& inputPinIndices) const
{
  // this function is generic and doesn't need to be extended for new types

  for (xiiUInt32 nodeIdx = 0; nodeIdx < newNodes.GetCount(); ++nodeIdx)
  {
    const xiiDocumentObject* pNode      = allNodes[nodeIdx];
    auto*                    pNewNode   = newNodes[nodeIdx];
    const auto               outputPins = pNodeManager->GetOutputPins(pNode);

    for (auto& pPin : outputPins)
    {
      auto connections = pNodeManager->GetConnections(*pPin);
      if (connections.IsEmpty())
        continue;

      const xiiAnimationControllerNodePin& ctrlPin = xiiStaticCast<const xiiAnimationControllerNodePin&>(*pPin);
      const xiiUInt8                       pinType = ctrlPin.m_DataType;

      const xiiUInt32 idx = pinCounts[pinType].m_uiOutputIdx++;

      animController.m_OutputPinToInputPinMapping[pinType][idx].Reserve(connections.GetCount());

      auto pPinProp = static_cast<xiiAbstractMemberProperty*>(pNewNode->GetDynamicRTTI()->FindPropertyByName(pPin->GetName()));
      XII_ASSERT_DEBUG(pPinProp, "Pin with name '{}' has no equally named property", pPin->GetName());

      // set the output index to use by this pin
      xiiReflectionUtils::SetMemberPropertyValue(pIdxProperty, pPinProp->GetPropertyPointer(pNewNode), idx);

      // set output pin to input pin mapping

      for (const auto pCon : connections)
      {
        const xiiUInt16 uiTargetIdx = inputPinIndices.GetValueOrDefault(&pCon->GetTargetPin(), 0xFFFF);

        if (uiTargetIdx != 0xFFFF)
        {
          animController.m_OutputPinToInputPinMapping[pinType][idx].PushBack(uiTargetIdx);
        }
      }
    }
  }
}

void xiiAnimationControllerAssetDocument::SetInputPinIndices(const xiiDynamicArray<xiiAnimGraphNode*>& newNodes, const xiiDynamicArray<const xiiDocumentObject*>& allNodes, const xiiDocumentNodeManager* pNodeManager, xiiMap<xiiUInt8, PinCount>& pinCounts, xiiMap<const xiiPin*, xiiUInt16>& inputPinIndices, xiiAbstractMemberProperty* pIdxProperty, xiiAbstractMemberProperty* pNumProperty) const
{
  // this function is generic and doesn't need to be extended for new types

  for (xiiUInt32 nodeIdx = 0; nodeIdx < newNodes.GetCount(); ++nodeIdx)
  {
    const xiiDocumentObject* pNode    = allNodes[nodeIdx];
    auto*                    pNewNode = newNodes[nodeIdx];

    const auto inputPins = pNodeManager->GetInputPins(pNode);

    for (auto& pPin : inputPins)
    {
      auto connections = pNodeManager->GetConnections(*pPin);
      if (connections.IsEmpty())
        continue;

      const xiiAnimationControllerNodePin& ctrlPin = xiiStaticCast<const xiiAnimationControllerNodePin&>(*pPin);
      const xiiUInt16                      idx     = pinCounts[(xiiUInt8)ctrlPin.m_DataType].m_uiInputIdx++;
      inputPinIndices[&ctrlPin]                    = idx;

      auto pPinProp = static_cast<xiiAbstractMemberProperty*>(pNewNode->GetDynamicRTTI()->FindPropertyByName(pPin->GetName()));
      XII_ASSERT_DEBUG(pPinProp, "Pin with name '{}' has no equally named property", pPin->GetName());

      xiiReflectionUtils::SetMemberPropertyValue(pIdxProperty, pPinProp->GetPropertyPointer(pNewNode), idx);
      xiiReflectionUtils::SetMemberPropertyValue(pNumProperty, pPinProp->GetPropertyPointer(pNewNode), connections.GetCount());
    }
  }
}

void xiiAnimationControllerAssetDocument::CreateOutputGraphNodes(const xiiDynamicArray<const xiiDocumentObject*>& allNodes, xiiAnimGraph& animController, xiiDynamicArray<xiiAnimGraphNode*>& newNodes) const
{
  // this function is generic and doesn't need to be extended for new types

  for (const xiiDocumentObject* pNode : allNodes)
  {
    animController.m_Nodes.PushBack(pNode->GetType()->GetAllocator()->Allocate<xiiAnimGraphNode>());
    newNodes.PushBack(animController.m_Nodes.PeekBack().Borrow());
    auto pNewNode = animController.m_Nodes.PeekBack().Borrow();

    // copy all the non-hidden properties
    xiiToolsSerializationUtils::CopyProperties(pNode, GetObjectManager(), pNewNode, pNewNode->GetDynamicRTTI(), [](const xiiAbstractProperty* p) { return p->GetAttributeByType<xiiHiddenAttribute>() == nullptr; });
  }
}

void xiiAnimationControllerAssetDocument::CountPinTypes(const xiiDocumentNodeManager* pNodeManager, xiiDynamicArray<const xiiDocumentObject*>& allNodes, xiiMap<xiiUInt8, PinCount>& pinCounts) const
{
  // this function is generic and doesn't need to be extended for new types

  for (auto pNode : pNodeManager->GetRootObject()->GetChildren())
  {
    if (!pNodeManager->IsNode(pNode))
      continue;

    allNodes.PushBack(pNode);

    // input pins
    {
      const auto pins = pNodeManager->GetInputPins(pNode);

      for (auto& pPin : pins)
      {
        if (pNodeManager->HasConnections(*pPin) == false)
          continue;

        const xiiAnimationControllerNodePin& ctrlPin = xiiStaticCast<const xiiAnimationControllerNodePin&>(*pPin);
        pinCounts[(xiiUInt8)ctrlPin.m_DataType].m_uiInputCount++;
      }
    }

    // output pins
    {
      const auto pins = pNodeManager->GetOutputPins(pNode);

      for (auto& pPin : pins)
      {
        if (pNodeManager->HasConnections(*pPin) == false)
          continue;

        const xiiAnimationControllerNodePin& ctrlPin = xiiStaticCast<const xiiAnimationControllerNodePin&>(*pPin);
        pinCounts[(xiiUInt8)ctrlPin.m_DataType].m_uiOutputCount++;
      }
    }
  }
}

void xiiAnimationControllerAssetDocument::InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const
{
  // without this, changing connections only (no property value) may not result in a different asset document hash and therefore no transform

  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void xiiAnimationControllerAssetDocument::AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void xiiAnimationControllerAssetDocument::RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}



void xiiAnimationControllerAssetDocument::GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/xiiEditor.AnimationControllerGraph");
}

bool xiiAnimationControllerAssetDocument::CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const
{
  out_MimeType = "application/xiiEditor.AnimationControllerGraph";

  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool xiiAnimationControllerAssetDocument::Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, xiiQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}

xiiAnimationControllerNodePin::xiiAnimationControllerNodePin(Type type, const char* szName, const xiiColorGammaUB& color, const xiiDocumentObject* pObject) :
  xiiPin(type, szName, color, pObject)
{
}

xiiAnimationControllerNodePin::~xiiAnimationControllerNodePin() = default;
