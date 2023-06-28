#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAsset.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraph.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptTypeRegistry.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>

//////////////////////////////////////////////////////////////////////////
// xiiVisualScriptAssetDocument
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptParameter, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("Expose", m_bExpose),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptParameterBool, 1, xiiRTTIDefaultAllocator<xiiVisualScriptParameterBool>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptParameterNumber, 1, xiiRTTIDefaultAllocator<xiiVisualScriptParameterNumber>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptParameterString, 1, xiiRTTIDefaultAllocator<xiiVisualScriptParameterString>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptAssetProperties, 1, xiiRTTIDefaultAllocator<xiiVisualScriptAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("BoolParameters", m_BoolParameters),
    XII_ARRAY_MEMBER_PROPERTY("NumberParameters", m_NumberParameters),
    XII_ARRAY_MEMBER_PROPERTY("StringParameters", m_StringParameters)
  }
    XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////
// xiiVisualScriptAssetDocument
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptAssetDocument, 6, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiVisualScriptAssetDocument::xiiVisualScriptAssetDocument(const char* szDocumentPath) :
  xiiSimpleAssetDocument<xiiVisualScriptAssetProperties>(XII_DEFAULT_NEW(xiiVisualScriptNodeManager_Legacy), szDocumentPath, xiiAssetDocEngineConnection::None)
{
  xiiVisualScriptTypeRegistry::GetSingleton()->UpdateNodeTypes();
}

void xiiVisualScriptAssetDocument::OnInterDocumentMessage(xiiReflectedClass* pMessage, xiiDocument* pSender)
{
  if (pMessage->GetDynamicRTTI()->IsDerivedFrom<xiiVisualScriptActivityMsgToEditor>())
  {
    HandleVsActivityMsg(static_cast<xiiVisualScriptActivityMsgToEditor*>(pMessage));
    return;
  }

  if (pMessage->GetDynamicRTTI()->IsDerivedFrom<xiiGatherObjectsForDebugVisMsgInterDoc>())
  {
    m_InterDocumentMessages.Broadcast(pMessage);
  }
}

void xiiVisualScriptAssetDocument::HandleVsActivityMsg(const xiiVisualScriptActivityMsgToEditor* pActivityMsg)
{
  const auto&     db    = pActivityMsg->m_Activity;
  const xiiUInt8* pData = db.GetData();

  xiiUInt32* pNumExecCon = (xiiUInt32*)pData;
  xiiUInt32* pNumDataCon = (xiiUInt32*)xiiMemoryUtils::AddByteOffset(pData, 4);

  xiiUInt32* pExecCon = (xiiUInt32*)xiiMemoryUtils::AddByteOffset(pData, 8);
  xiiUInt32* pDataCon = pExecCon + (*pNumExecCon);

  xiiVisualScriptInstanceActivity act;
  act.m_ActiveExecutionConnections.SetCountUninitialized(*pNumExecCon);
  act.m_ActiveDataConnections.SetCountUninitialized(*pNumDataCon);

  xiiMemoryUtils::Copy<xiiUInt32>(act.m_ActiveExecutionConnections.GetData(), pExecCon, act.m_ActiveExecutionConnections.GetCount());
  xiiMemoryUtils::Copy<xiiUInt32>(act.m_ActiveDataConnections.GetData(), pDataCon, act.m_ActiveDataConnections.GetCount());

  xiiVisualScriptActivityEvent ae;
  ae.m_pActivityData = &act;
  ae.m_ObjectGuid    = pActivityMsg->m_ComponentGuid;

  m_ActivityEvents.Broadcast(ae);
}

xiiTransformStatus xiiVisualScriptAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  xiiVisualScriptResourceDescriptor desc;
  if (GenerateVisualScriptDescriptor(desc).Failed())
  {
    xiiLog::Warning("Couldn't generate visual script descriptor!");
    return xiiStatus(XII_FAILURE);
  }

  desc.Save(stream);

  return xiiStatus(XII_SUCCESS);
}

void xiiVisualScriptAssetDocument::InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const
{
  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void xiiVisualScriptAssetDocument::AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void xiiVisualScriptAssetDocument::RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}

xiiResult xiiVisualScriptAssetDocument::GenerateVisualScriptDescriptor(xiiVisualScriptResourceDescriptor& desc)
{
  xiiVisualScriptNodeManager_Legacy* pNodeManager  = static_cast<xiiVisualScriptNodeManager_Legacy*>(GetObjectManager());
  xiiVisualScriptTypeRegistry*       pTypeRegistry = xiiVisualScriptTypeRegistry::GetSingleton();

  xiiDynamicArray<const xiiDocumentObject*> allNodes;
  GetAllVsNodes(allNodes);

  xiiMap<const xiiDocumentObject*, xiiUInt16> ObjectToIndex;
  desc.m_Nodes.Reserve(allNodes.GetCount());

  for (xiiUInt32 i = 0; i < allNodes.GetCount(); ++i)
  {
    const xiiDocumentObject*             pObject = allNodes[i];
    const xiiVisualScriptNodeDescriptor* pDesc   = pTypeRegistry->GetDescriptorForType(pObject->GetType());

    if (pDesc == nullptr)
    {
      xiiLog::Error("Couldn't get descriptor from type registry. Are all required plugins loaded?");
      return XII_FAILURE;
    }

    auto& node             = desc.m_Nodes.ExpandAndGetRef();
    node.m_pType           = nullptr;
    node.m_sTypeName       = pDesc->m_sTypeName;
    node.m_uiFirstProperty = desc.m_Properties.GetCount();
    node.m_uiNumProperties = 0;

    ObjectToIndex[pObject] = i;

    xiiHybridArray<xiiAbstractProperty*, 32> properties;
    pObject->GetType()->GetAllProperties(properties);

    for (const xiiAbstractProperty* pProp : properties)
    {
      if (pProp->GetCategory() == xiiPropertyCategory::Member)
      {
        auto& ref   = desc.m_Properties.ExpandAndGetRef();
        ref.m_sName = pProp->GetPropertyName();
        ref.m_Value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName());

        if (const xiiVisScriptMappingAttribute* pMappingAttr = pProp->GetAttributeByType<xiiVisScriptMappingAttribute>())
        {
          ref.m_iMappingIndex = pMappingAttr->m_iMapping;
        }

        node.m_uiNumProperties++;
      }
    }
  }

  for (xiiUInt32 srcNodeIdx = 0; srcNodeIdx < allNodes.GetCount(); ++srcNodeIdx)
  {
    const xiiDocumentObject* pSrcObject = allNodes[srcNodeIdx];

    auto outputPins = pNodeManager->GetOutputPins(pSrcObject);

    for (auto& pPin : outputPins)
    {
      const auto connections = pNodeManager->GetConnections(*pPin);

      for (const xiiConnection* pCon : connections)
      {
        const xiiVisualScriptPin_Legacy& vsPinSource = static_cast<const xiiVisualScriptPin_Legacy&>(pCon->GetSourcePin());
        const xiiVisualScriptPin_Legacy& vsPinTarget = static_cast<const xiiVisualScriptPin_Legacy&>(pCon->GetTargetPin());

        if (vsPinSource.GetDescriptor()->m_PinType == xiiVisualScriptPinDescriptor::PinType::Execution)
        {
          auto& path          = desc.m_ExecutionPaths.ExpandAndGetRef();
          path.m_uiSourceNode = srcNodeIdx;
          path.m_uiOutputPin  = vsPinSource.GetDescriptor()->m_uiPinIndex;

          path.m_uiTargetNode = ObjectToIndex[vsPinTarget.GetParent()];
          path.m_uiInputPin   = vsPinTarget.GetDescriptor()->m_uiPinIndex;
        }
        else if (vsPinSource.GetDescriptor()->m_PinType == xiiVisualScriptPinDescriptor::PinType::Data)
        {
          auto& path             = desc.m_DataPaths.ExpandAndGetRef();
          path.m_uiSourceNode    = srcNodeIdx;
          path.m_uiOutputPin     = vsPinSource.GetDescriptor()->m_uiPinIndex;
          path.m_uiOutputPinType = vsPinSource.GetDescriptor()->m_DataType;
          path.m_uiTargetNode    = ObjectToIndex[vsPinTarget.GetParent()];
          path.m_uiInputPin      = vsPinTarget.GetDescriptor()->m_uiPinIndex;
          path.m_uiInputPinType  = vsPinTarget.GetDescriptor()->m_DataType;
        }
      }
    }
  }

  // local variables
  {
    desc.m_BoolParameters.Clear();
    desc.m_BoolParameters.Reserve(GetProperties()->m_BoolParameters.GetCount());

    for (const auto& p : GetProperties()->m_BoolParameters)
    {
      if (p.m_sName.IsEmpty())
      {
        xiiLog::Warning("Visual script declared an unnamed bool variable. Variable is ignored.");
        continue;
      }

      auto& outP = desc.m_BoolParameters.ExpandAndGetRef();
      outP.m_sName.Assign(p.m_sName.GetData());
      outP.m_Value = p.m_DefaultValue;
    }

    desc.m_NumberParameters.Clear();
    desc.m_NumberParameters.Reserve(GetProperties()->m_NumberParameters.GetCount());

    for (const auto& p : GetProperties()->m_NumberParameters)
    {
      if (p.m_sName.IsEmpty())
      {
        xiiLog::Warning("Visual script declared an unnamed number variable. Variable is ignored.");
        continue;
      }

      auto& outP = desc.m_NumberParameters.ExpandAndGetRef();
      outP.m_sName.Assign(p.m_sName.GetData());
      outP.m_Value = p.m_DefaultValue;
    }

    desc.m_StringParameters.Clear();
    desc.m_StringParameters.Reserve(GetProperties()->m_StringParameters.GetCount());

    for (const auto& p : GetProperties()->m_StringParameters)
    {
      if (p.m_sName.IsEmpty())
      {
        xiiLog::Warning("Visual script declared an unnamed number variable. Variable is ignored.");
        continue;
      }

      auto& outP = desc.m_StringParameters.ExpandAndGetRef();
      outP.m_sName.Assign(p.m_sName.GetData());
      outP.m_sValue = p.m_DefaultValue;
    }
  }

  // verify used local variables
  {
    for (xiiUInt32 i = 0; i < allNodes.GetCount(); ++i)
    {
      const xiiDocumentObject*             pObject = allNodes[i];
      const xiiVisualScriptNodeDescriptor* pDesc   = pTypeRegistry->GetDescriptorForType(pObject->GetType());

      if (pDesc->m_sTypeName == "xiiVisualScriptNode_Bool" || pDesc->m_sTypeName == "xiiVisualScriptNode_Number" || pDesc->m_sTypeName == "xiiVisualScriptNode_String" || pDesc->m_sTypeName == "xiiVisualScriptNode_StoreNumber" || pDesc->m_sTypeName == "xiiVisualScriptNode_StoreBool" || pDesc->m_sTypeName == "xiiVisualScriptNode_ToggleBool" || pDesc->m_sTypeName == "xiiVisualScriptNode_StoreString")
      {
        const xiiVariant varName = pObject->GetTypeAccessor().GetValue("Name");
        XII_ASSERT_DEBUG(varName.IsA<xiiString>(), "Missing or invalid property");
        const xiiString name = varName.ConvertTo<xiiString>();

        auto findVarName = [&](auto parameters) {
          for (const auto& p : parameters)
          {
            if (p.m_sName == name)
            {
              return true;
            }
          }

          return false;
        };

        bool        found = false;
        const char* szValueType;

        if (pDesc->m_sTypeName == "xiiVisualScriptNode_Bool" || pDesc->m_sTypeName == "xiiVisualScriptNode_StoreBool" || pDesc->m_sTypeName == "xiiVisualScriptNode_ToggleBool")
        {
          found       = findVarName(desc.m_BoolParameters);
          szValueType = "bool";
        }
        else if (pDesc->m_sTypeName == "xiiVisualScriptNode_String" || pDesc->m_sTypeName == "xiiVisualScriptNode_StoreString")
        {
          found       = findVarName(desc.m_StringParameters);
          szValueType = "string";
        }
        else
        {
          found       = findVarName(desc.m_NumberParameters);
          szValueType = "number";
        }

        if (!found)
        {
          xiiLog::Error("Visual Script uses undeclared {0} variable '{1}'.", szValueType, name);
        }
      }
    }
  }

  return XII_SUCCESS;
}


void xiiVisualScriptAssetDocument::GetAllVsNodes(xiiDynamicArray<const xiiDocumentObject*>& allNodes) const
{
  xiiVisualScriptTypeRegistry* pTypeRegistry = xiiVisualScriptTypeRegistry::GetSingleton();
  const xiiRTTI*               pNodeBaseRtti = pTypeRegistry->GetNodeBaseType();

  allNodes.Clear();
  allNodes.Reserve(64);

  const auto& children = GetObjectManager()->GetRootObject()->GetChildren();
  for (const xiiDocumentObject* pObject : children)
  {
    auto pType = pObject->GetTypeAccessor().GetType();
    if (!pType->IsDerivedFrom(pNodeBaseRtti))
      continue;

    allNodes.PushBack(pObject);
  }
}

void xiiVisualScriptAssetDocument::UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  xiiExposedParameters* pExposedParams = XII_DEFAULT_NEW(xiiExposedParameters);

  {
    for (const auto& p : GetProperties()->m_BoolParameters)
    {
      if (p.m_bExpose)
      {
        xiiExposedParameter* param = XII_DEFAULT_NEW(xiiExposedParameter);
        pExposedParams->m_Parameters.PushBack(param);
        param->m_sName        = p.m_sName;
        param->m_DefaultValue = p.m_DefaultValue;
      }
    }

    for (const auto& p : GetProperties()->m_NumberParameters)
    {
      if (p.m_bExpose)
      {
        xiiExposedParameter* param = XII_DEFAULT_NEW(xiiExposedParameter);
        pExposedParams->m_Parameters.PushBack(param);
        param->m_sName        = p.m_sName;
        param->m_DefaultValue = p.m_DefaultValue;
      }
    }

    for (const auto& p : GetProperties()->m_StringParameters)
    {
      if (p.m_bExpose)
      {
        xiiExposedParameter* param = XII_DEFAULT_NEW(xiiExposedParameter);
        pExposedParams->m_Parameters.PushBack(param);
        param->m_sName        = p.m_sName;
        param->m_DefaultValue = p.m_DefaultValue;
      }
    }
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

void xiiVisualScriptAssetDocument::GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/xiiEditor.VisualScriptGraph");
}

bool xiiVisualScriptAssetDocument::CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const
{
  out_MimeType = "application/xiiEditor.VisualScriptGraph";

  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool xiiVisualScriptAssetDocument::Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, xiiQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}

//////////////////////////////////////////////////////////////////////////

// don't think this will work

//#include <Foundation/Serialization/GraphPatch.h>
//
// class xiiVisScriptAsset_3_4 : public xiiGraphPatch
//{
// public:
//  xiiVisScriptAsset_3_4()
//    : xiiGraphPatch("xiiVisualScriptAssetDocument", 4) {}
//
//  virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
//  {
//    xiiVersionKey bases[] = { { "xiiSimpleAssetDocument<xiiVisualScriptAssetProperties>", 1 } };
//    context.ChangeBaseClass(bases);
//  }
//};
//
// xiiVisScriptAsset_3_4 g_xiiVisScriptAsset_3_4;
