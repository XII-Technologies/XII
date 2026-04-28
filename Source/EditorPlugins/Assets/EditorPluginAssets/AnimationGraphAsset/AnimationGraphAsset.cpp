/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAsset.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphQt.h>
#include <Foundation/Math/ColorScheme.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes2/PoseResultAnimNode.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes2/SampleFrameAnimNode.h>
#include <ToolsFoundation/NodeObject/NodeCommandAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationGraphAssetDocument, 5, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationGraphNodePin, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationGraphAssetProperties, 1, xiiRTTIDefaultAllocator<xiiAnimationGraphAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("IncludeGraphs", m_IncludeGraphs)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Keyframe_Graph")),
    XII_ARRAY_MEMBER_PROPERTY("AnimationClipMapping", m_AnimationClipMapping),
  }
    XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, AnimationGraph)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiQtNodeScene::GetNodeFactory().RegisterCreator(xiiGetStaticRTTI<xiiAnimGraphNode>(), [](const xiiRTTI* pRtti)->xiiQtNode* { return new xiiQtAnimationGraphNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiQtNodeScene::GetNodeFactory().UnregisterCreator(xiiGetStaticRTTI<xiiAnimGraphNode>());
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

bool xiiAnimationGraphNodeManager::InternalIsNode(const xiiDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom<xiiAnimGraphNode>();
}

void xiiAnimationGraphNodeManager::InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& ref_node)
{
  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom<xiiAnimGraphNode>())
    return;

  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  const xiiColor triggerPinColor   = xiiColorScheme::DarkUI(xiiColorScheme::Yellow);
  const xiiColor numberPinColor    = xiiColorScheme::DarkUI(xiiColorScheme::Lime);
  const xiiColor boolPinColor      = xiiColorScheme::LightUI(xiiColorScheme::Lime);
  const xiiColor weightPinColor    = xiiColorScheme::DarkUI(xiiColorScheme::Teal);
  const xiiColor localPosePinColor = xiiColorScheme::DarkUI(xiiColorScheme::Blue);
  const xiiColor modelPosePinColor = xiiColorScheme::DarkUI(xiiColorScheme::Grape);
  // EXTEND THIS if a new type is introduced

  xiiHybridArray<xiiString, 16> pinNames;

  for (auto pProp : properties)
  {
    if (!pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphPin>())
      continue;

    pinNames.Clear();

    if (pProp->GetCategory() == xiiPropertyCategory::Array)
    {
      if (const xiiDynamicPinAttribute* pDynPin = pProp->GetAttributeByType<xiiDynamicPinAttribute>())
      {
        GetDynamicPinNames(pObject, pDynPin->GetProperty(), pProp->GetPropertyName(), pinNames);
      }
    }
    else if (pProp->GetCategory() == xiiPropertyCategory::Member)
    {
      pinNames.PushBack(pProp->GetPropertyName());
    }

    for (xiiUInt32 i = 0; i < pinNames.GetCount(); ++i)
    {
      const auto& pinName = pinNames[i];

      if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphTriggerInputPin>())
      {
        auto pPin        = XII_DEFAULT_NEW(xiiAnimationGraphNodePin, xiiPin::Type::Input, pinName, triggerPinColor, pObject);
        pPin->m_DataType = xiiAnimGraphPin::Trigger;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphTriggerOutputPin>())
      {
        auto pPin        = XII_DEFAULT_NEW(xiiAnimationGraphNodePin, xiiPin::Type::Output, pinName, triggerPinColor, pObject);
        pPin->m_DataType = xiiAnimGraphPin::Trigger;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphNumberInputPin>())
      {
        auto pPin        = XII_DEFAULT_NEW(xiiAnimationGraphNodePin, xiiPin::Type::Input, pinName, numberPinColor, pObject);
        pPin->m_DataType = xiiAnimGraphPin::Number;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphNumberOutputPin>())
      {
        auto pPin        = XII_DEFAULT_NEW(xiiAnimationGraphNodePin, xiiPin::Type::Output, pinName, numberPinColor, pObject);
        pPin->m_DataType = xiiAnimGraphPin::Number;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphBoolInputPin>())
      {
        auto pPin        = XII_DEFAULT_NEW(xiiAnimationGraphNodePin, xiiPin::Type::Input, pinName, boolPinColor, pObject);
        pPin->m_DataType = xiiAnimGraphPin::Bool;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphBoolOutputPin>())
      {
        auto pPin        = XII_DEFAULT_NEW(xiiAnimationGraphNodePin, xiiPin::Type::Output, pinName, boolPinColor, pObject);
        pPin->m_DataType = xiiAnimGraphPin::Bool;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphBoneWeightsInputPin>())
      {
        auto pPin        = XII_DEFAULT_NEW(xiiAnimationGraphNodePin, xiiPin::Type::Input, pinName, weightPinColor, pObject);
        pPin->m_DataType = xiiAnimGraphPin::BoneWeights;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphBoneWeightsOutputPin>())
      {
        auto pPin        = XII_DEFAULT_NEW(xiiAnimationGraphNodePin, xiiPin::Type::Output, pinName, weightPinColor, pObject);
        pPin->m_DataType = xiiAnimGraphPin::BoneWeights;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphLocalPoseInputPin>())
      {
        auto pPin        = XII_DEFAULT_NEW(xiiAnimationGraphNodePin, xiiPin::Type::Input, pinName, localPosePinColor, pObject);
        pPin->m_DataType = xiiAnimGraphPin::LocalPose;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphLocalPoseMultiInputPin>())
      {
        auto pPin              = XII_DEFAULT_NEW(xiiAnimationGraphNodePin, xiiPin::Type::Input, pinName, localPosePinColor, pObject);
        pPin->m_DataType       = xiiAnimGraphPin::LocalPose;
        pPin->m_bMultiInputPin = true;
        pPin->m_Shape          = xiiPin::Shape::RoundRect;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphLocalPoseOutputPin>())
      {
        auto pPin        = XII_DEFAULT_NEW(xiiAnimationGraphNodePin, xiiPin::Type::Output, pinName, localPosePinColor, pObject);
        pPin->m_DataType = xiiAnimGraphPin::LocalPose;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphModelPoseInputPin>())
      {
        auto pPin        = XII_DEFAULT_NEW(xiiAnimationGraphNodePin, xiiPin::Type::Input, pinName, modelPosePinColor, pObject);
        pPin->m_DataType = xiiAnimGraphPin::ModelPose;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<xiiAnimGraphModelPoseOutputPin>())
      {
        auto pPin        = XII_DEFAULT_NEW(xiiAnimationGraphNodePin, xiiPin::Type::Output, pinName, modelPosePinColor, pObject);
        pPin->m_DataType = xiiAnimGraphPin::ModelPose;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else
      {
        // EXTEND THIS if a new type is introduced
        XII_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }
}

void xiiAnimationGraphNodeManager::GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& ref_types) const
{
  xiiSet<const xiiRTTI*> typeSet;
  xiiReflectionUtils::GatherTypesDerivedFromClass(xiiGetStaticRTTI<xiiAnimGraphNode>(), typeSet);

  ref_types.Clear();
  for (auto pType : typeSet)
  {
    if (pType->GetTypeFlags().IsAnySet(xiiTypeFlags::Abstract))
      continue;

    ref_types.PushBack(pType);
  }
}

xiiStatus xiiAnimationGraphNodeManager::InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_result) const
{
  const xiiAnimationGraphNodePin& sourcePin = xiiStaticCast<const xiiAnimationGraphNodePin&>(source);
  const xiiAnimationGraphNodePin& targetPin = xiiStaticCast<const xiiAnimationGraphNodePin&>(target);

  out_result = CanConnectResult::ConnectNever;

  if (sourcePin.m_DataType != targetPin.m_DataType)
    return xiiStatus("Can't connect pins of different data types");

  if (sourcePin.GetType() == targetPin.GetType())
    return xiiStatus("Can only connect input pins with output pins.");

  switch (sourcePin.m_DataType)
  {
    case xiiAnimGraphPin::Trigger:
      out_result = CanConnectResult::ConnectNtoN;
      break;

    case xiiAnimGraphPin::Number:
      out_result = CanConnectResult::ConnectNto1;
      break;

    case xiiAnimGraphPin::Bool:
      out_result = CanConnectResult::ConnectNto1;
      break;

    case xiiAnimGraphPin::BoneWeights:
      out_result = CanConnectResult::ConnectNto1;
      break;

    case xiiAnimGraphPin::LocalPose:
      if (targetPin.m_bMultiInputPin)
        out_result = CanConnectResult::ConnectNtoN;
      else
        out_result = CanConnectResult::ConnectNto1;
      break;

    case xiiAnimGraphPin::ModelPose:
      out_result = CanConnectResult::ConnectNto1;
      break;

      // EXTEND THIS if a new type is introduced
      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (out_result != CanConnectResult::ConnectNever && WouldConnectionCreateCircle(source, target))
  {
    out_result = CanConnectResult::ConnectNever;
    return xiiStatus("Connecting these pins would create a circle in the graph.");
  }

  return XII_SUCCESS;
}

bool xiiAnimationGraphNodeManager::InternalIsDynamicPinProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) const
{
  return pProp->GetAttributeByType<xiiDynamicPinAttribute>() != nullptr;
}

xiiAnimationGraphAssetDocument::xiiAnimationGraphAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiAnimationGraphAssetProperties>(XII_DEFAULT_NEW(xiiAnimationGraphNodeManager), sDocumentPath, xiiAssetDocEngineConnection::None)
{
  m_pObjectAccessor = XII_DEFAULT_NEW(xiiNodeCommandAccessor, GetCommandHistory());
}

xiiTransformStatus xiiAnimationGraphAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  const auto* pNodeManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());

  auto pProp = GetProperties();

  {
    stream.WriteVersion(2);
    stream.WriteArray(pProp->m_IncludeGraphs).AssertSuccess();

    const xiiUInt32 uiNum = pProp->m_AnimationClipMapping.GetCount();
    stream << uiNum;

    for (xiiUInt32 i = 0; i < uiNum; ++i)
    {
      stream << pProp->m_AnimationClipMapping[i].m_sClipName;
      stream << pProp->m_AnimationClipMapping[i].m_hClip;
    }
  }

  // find all 'nodes'
  xiiDynamicArray<const xiiDocumentObject*> allNodes;
  for (auto pNode : pNodeManager->GetRootObject()->GetChildren())
  {
    if (!pNodeManager->IsNode(pNode))
      continue;

    allNodes.PushBack(pNode);
  }

  xiiAnimGraph animGraph;

  xiiMap<const xiiDocumentObject*, xiiAnimGraphNode*> docNodeToRuntimeNode;

  // create all nodes in the xiiAnimGraph
  {
    for (const xiiDocumentObject* pNode : allNodes)
    {
      xiiAnimGraphNode* pNewNode = animGraph.AddNode(pNode->GetType()->GetAllocator()->Allocate<xiiAnimGraphNode>());

      // copy all the non-hidden properties
      xiiToolsSerializationUtils::CopyProperties(pNode, GetObjectManager(), pNewNode, pNewNode->GetDynamicRTTI(), [](const xiiAbstractProperty* p) { return p->GetAttributeByType<xiiHiddenAttribute>() == nullptr; });

      docNodeToRuntimeNode[pNode] = pNewNode;
    }
  }

  // add all node connections to the xiiAnimGraph
  {
    for (xiiUInt32 nodeIdx = 0; nodeIdx < allNodes.GetCount(); ++nodeIdx)
    {
      const xiiDocumentObject* pNode = allNodes[nodeIdx];

      const auto outputPins = pNodeManager->GetOutputPins(pNode);

      for (auto& pPin : outputPins)
      {
        for (const xiiConnection* pCon : pNodeManager->GetConnections(*pPin))
        {
          const xiiAnimGraphNode* pSrcNode = docNodeToRuntimeNode[pCon->GetSourcePin().GetParent()];
          xiiAnimGraphNode*       pDstNode = docNodeToRuntimeNode[pCon->GetTargetPin().GetParent()];

          animGraph.AddConnection(pSrcNode, pCon->GetSourcePin().GetName(), pDstNode, pCon->GetTargetPin().GetName());
        }
      }
    }
  }

  XII_SUCCEED_OR_RETURN(animGraph.Serialize(stream));

  return xiiTransformStatus(XII_SUCCESS);
}

void xiiAnimationGraphAssetDocument::InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const
{
  // without this, changing connections only (no property value) may not result in a different asset document hash and therefore no transform

  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void xiiAnimationGraphAssetDocument::AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void xiiAnimationGraphAssetDocument::RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}



void xiiAnimationGraphAssetDocument::GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/xiiEditor.AnimationGraphGraph");
}

bool xiiAnimationGraphAssetDocument::CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const
{
  out_MimeType = "application/xiiEditor.AnimationGraphGraph";

  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool xiiAnimationGraphAssetDocument::Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, xiiStringView sMimeType)
{
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, xiiQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}

xiiAnimationGraphNodePin::xiiAnimationGraphNodePin(Type type, const char* szName, const xiiColorGammaUB& color, const xiiDocumentObject* pObject) :
  xiiPin(type, szName, color, pObject)
{
}

xiiAnimationGraphNodePin::~xiiAnimationGraphNodePin() = default;
