#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

void xiiDocument::UpdatePrefabs()
{
  GetCommandHistory()->StartTransaction("Update Prefabs");

  UpdatePrefabsRecursive(GetObjectManager()->GetRootObject());

  GetCommandHistory()->FinishTransaction();

  ShowDocumentStatus("Prefabs have been updated");
  SetModified(true);
}

void xiiDocument::RevertPrefabs(const xiiDeque<const xiiDocumentObject*>& selection)
{
  if (selection.IsEmpty())
    return;

  auto pHistory = GetCommandHistory();

  pHistory->StartTransaction("Revert Prefab");

  for (auto pItem : selection)
  {
    RevertPrefab(pItem);
  }

  pHistory->FinishTransaction();
}

void xiiDocument::UnlinkPrefabs(const xiiDeque<const xiiDocumentObject*>& selection)
{
  if (selection.IsEmpty())
    return;

  auto pHistory = GetCommandHistory();
  pHistory->StartTransaction("Unlink Prefab");

  for (auto pObject : selection)
  {
    xiiUnlinkPrefabCommand cmd;
    cmd.m_Object = pObject->GetGuid();

    pHistory->AddCommand(cmd);
  }

  pHistory->FinishTransaction();
}

xiiStatus xiiDocument::CreatePrefabDocumentFromSelection(const char* szFile, const xiiRTTI* pRootType, xiiDelegate<void(xiiAbstractObjectNode*)> adjustGraphNodeCB, xiiDelegate<void(xiiDocumentObject*)> adjustNewNodesCB, xiiDelegate<void(xiiAbstractObjectGraph& graph, xiiDynamicArray<xiiAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB)
{
  auto Selection = GetSelectionManager()->GetTopLevelSelection(pRootType);

  if (Selection.IsEmpty())
    return xiiStatus("To create a prefab, the selection must not be empty");

  xiiHybridArray<const xiiDocumentObject*, 32> nodes;
  nodes.Reserve(Selection.GetCount());
  for (auto pNode : Selection)
  {
    nodes.PushBack(pNode);
  }

  xiiUuid PrefabGuid, SeedGuid;
  SeedGuid.CreateNewUuid();
  xiiStatus res = CreatePrefabDocument(szFile, nodes, SeedGuid, PrefabGuid, adjustGraphNodeCB, true);

  if (res.m_Result.Succeeded())
  {
    GetCommandHistory()->StartTransaction("Replace all by Prefab");

    // this replaces ONE object by the new prefab (we pick the last one in the selection)
    xiiUuid newObj = ReplaceByPrefab(nodes.PeekBack(), szFile, PrefabGuid, SeedGuid, true);

    // if we had more than one selected objects, remove the others as well
    if (nodes.GetCount() > 1)
    {
      nodes.PopBack();

      for (auto pNode : nodes)
      {
        xiiRemoveObjectCommand remCmd;
        remCmd.m_Object = pNode->GetGuid();

        GetCommandHistory()->AddCommand(remCmd);
      }
    }

    auto pObject = GetObjectManager()->GetObject(newObj);

    if (adjustNewNodesCB.IsValid())
    {
      adjustNewNodesCB(pObject);
    }

    GetCommandHistory()->FinishTransaction();
    GetSelectionManager()->SetSelection(pObject);
  }

  return res;
}

xiiStatus xiiDocument::CreatePrefabDocument(const char* szFile, xiiArrayPtr<const xiiDocumentObject*> rootObjects, const xiiUuid& invPrefabSeed, xiiUuid& out_newDocumentGuid, xiiDelegate<void(xiiAbstractObjectNode*)> adjustGraphNodeCB, bool bKeepOpen, xiiDelegate<void(xiiAbstractObjectGraph& graph, xiiDynamicArray<xiiAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB)
{
  const xiiDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (xiiDocumentManager::FindDocumentTypeFromPath(szFile, true, pTypeDesc).Failed())
    return xiiStatus(xiiFmt("Document type is unknown: '{0}'", szFile));

  pTypeDesc->m_pManager->EnsureDocumentIsClosed(szFile);

  // prepare the current state as a graph
  xiiAbstractObjectGraph           PrefabGraph;
  xiiDocumentObjectConverterWriter writer(&PrefabGraph, GetObjectManager());

  xiiHybridArray<xiiAbstractObjectNode*, 32> graphRootNodes;
  graphRootNodes.Reserve(rootObjects.GetCount() + 1);

  for (xiiUInt32 i = 0; i < rootObjects.GetCount(); ++i)
  {
    auto pSaveAsPrefab = rootObjects[i];

    XII_ASSERT_DEV(pSaveAsPrefab != nullptr, "CreatePrefabDocument: pSaveAsPrefab must be a valid object!");
    const xiiRTTI* pRootType = pSaveAsPrefab->GetTypeAccessor().GetType();

    auto pPrefabGraphMainNode = writer.AddObjectToGraph(pSaveAsPrefab);
    graphRootNodes.PushBack(pPrefabGraphMainNode);

    // allow external adjustments
    if (adjustGraphNodeCB.IsValid())
    {
      adjustGraphNodeCB(pPrefabGraphMainNode);
    }
  }

  if (finalizeGraphCB.IsValid())
  {
    finalizeGraphCB(PrefabGraph, graphRootNodes);
  }

  PrefabGraph.ReMapNodeGuids(invPrefabSeed, true);

  xiiDocument* pSceneDocument = nullptr;

  XII_SUCCEED_OR_RETURN(pTypeDesc->m_pManager->CreateDocument("Prefab", szFile, pSceneDocument, xiiDocumentFlags::RequestWindow | xiiDocumentFlags::AddToRecentFilesList | xiiDocumentFlags::EmptyDocument));

  out_newDocumentGuid   = pSceneDocument->GetGuid();
  auto pPrefabSceneRoot = pSceneDocument->GetObjectManager()->GetRootObject();

  xiiDocumentObjectConverterReader reader(&PrefabGraph, pSceneDocument->GetObjectManager(), xiiDocumentObjectConverterReader::Mode::CreateAndAddToDocument);

  for (xiiUInt32 i = 0; i < graphRootNodes.GetCount(); ++i)
  {
    const xiiRTTI* pRootType = xiiRTTI::FindTypeByName(graphRootNodes[i]->GetType());

    xiiUuid rootGuid = graphRootNodes[i]->GetGuid();
    rootGuid.RevertCombinationWithSeed(invPrefabSeed);

    xiiDocumentObject* pPrefabSceneMainObject = pSceneDocument->GetObjectManager()->CreateObject(pRootType, rootGuid);
    pSceneDocument->GetObjectManager()->AddObject(pPrefabSceneMainObject, pPrefabSceneRoot, "Children", -1);

    reader.ApplyPropertiesToObject(graphRootNodes[i], pPrefabSceneMainObject);
  }

  pSceneDocument->SetModified(true);
  auto res = pSceneDocument->SaveDocument();

  if (!bKeepOpen)
  {
    pTypeDesc->m_pManager->CloseDocument(pSceneDocument);
  }

  return res;
}


xiiUuid xiiDocument::ReplaceByPrefab(const xiiDocumentObject* pRootObject, const char* szPrefabFile, const xiiUuid& prefabAsset, const xiiUuid& prefabSeed, bool bEnginePrefab)
{
  GetCommandHistory()->StartTransaction("Replace by Prefab");

  xiiUuid instantiatedRoot;

  if (!bEnginePrefab) // create editor prefab
  {
    xiiInstantiatePrefabCommand instCmd;
    instCmd.m_Index                = pRootObject->GetPropertyIndex().ConvertTo<xiiInt32>();
    instCmd.m_bAllowPickedPosition = false;
    instCmd.m_CreateFromPrefab     = prefabAsset;
    instCmd.m_Parent               = pRootObject->GetParent() == GetObjectManager()->GetRootObject() ? xiiUuid() : pRootObject->GetParent()->GetGuid();
    instCmd.m_sBasePrefabGraph     = xiiPrefabUtils::ReadDocumentAsString(
      szPrefabFile); // since the prefab might have been created just now, going through the cache (via GUID) will most likely fail
    instCmd.m_RemapGuid = prefabSeed;

    GetCommandHistory()->AddCommand(instCmd);

    instantiatedRoot = instCmd.m_CreatedRootObject;
  }
  else // create an object with the reference prefab component
  {
    auto pHistory = GetCommandHistory();

    xiiStringBuilder tmp;
    xiiUuid          CmpGuid;
    instantiatedRoot.CreateNewUuid();
    CmpGuid.CreateNewUuid();

    xiiAddObjectCommand cmd;
    cmd.m_Parent = (pRootObject->GetParent() == GetObjectManager()->GetRootObject()) ? xiiUuid() : pRootObject->GetParent()->GetGuid();
    cmd.m_Index  = pRootObject->GetPropertyIndex();
    cmd.SetType("xiiGameObject");
    cmd.m_NewObjectGuid   = instantiatedRoot;
    cmd.m_sParentProperty = "Children";

    XII_VERIFY(pHistory->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    cmd.SetType("xiiPrefabReferenceComponent");
    cmd.m_sParentProperty = "Components";
    cmd.m_Index           = -1;
    cmd.m_NewObjectGuid   = CmpGuid;
    cmd.m_Parent          = instantiatedRoot;
    XII_VERIFY(pHistory->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    xiiSetObjectPropertyCommand cmd2;
    cmd2.m_Object    = CmpGuid;
    cmd2.m_sProperty = "Prefab";
    cmd2.m_NewValue  = xiiConversionUtils::ToString(prefabAsset, tmp).GetData();
    XII_VERIFY(pHistory->AddCommand(cmd2).m_Result.Succeeded(), "AddCommand failed");
  }

  {
    xiiRemoveObjectCommand remCmd;
    remCmd.m_Object = pRootObject->GetGuid();

    GetCommandHistory()->AddCommand(remCmd);
  }

  GetCommandHistory()->FinishTransaction();

  return instantiatedRoot;
}

xiiUuid xiiDocument::RevertPrefab(const xiiDocumentObject* pObject)
{
  auto pHistory = GetCommandHistory();
  auto pMeta    = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid());

  const xiiUuid PrefabAsset = pMeta->m_CreateFromPrefab;

  if (!PrefabAsset.IsValid())
  {
    m_DocumentObjectMetaData->EndReadMetaData();
    return xiiUuid();
  }

  xiiRemoveObjectCommand remCmd;
  remCmd.m_Object = pObject->GetGuid();

  xiiInstantiatePrefabCommand instCmd;
  instCmd.m_Index                = pObject->GetPropertyIndex().ConvertTo<xiiInt32>();
  instCmd.m_bAllowPickedPosition = false;
  instCmd.m_CreateFromPrefab     = PrefabAsset;
  instCmd.m_Parent               = pObject->GetParent() == GetObjectManager()->GetRootObject() ? xiiUuid() : pObject->GetParent()->GetGuid();
  instCmd.m_RemapGuid            = pMeta->m_PrefabSeedGuid;
  instCmd.m_sBasePrefabGraph     = xiiPrefabCache::GetSingleton()->GetCachedPrefabDocument(pMeta->m_CreateFromPrefab);

  m_DocumentObjectMetaData->EndReadMetaData();

  pHistory->AddCommand(remCmd);
  pHistory->AddCommand(instCmd);

  return instCmd.m_CreatedRootObject;
}


void xiiDocument::UpdatePrefabsRecursive(xiiDocumentObject* pObject)
{
  // Deliberately copy the array as the UpdatePrefabObject function will add / remove elements from the array.
  auto ChildArray = pObject->GetChildren();

  xiiStringBuilder sPrefabBase;

  for (auto pChild : ChildArray)
  {
    auto          pMeta       = m_DocumentObjectMetaData->BeginReadMetaData(pChild->GetGuid());
    const xiiUuid PrefabAsset = pMeta->m_CreateFromPrefab;
    const xiiUuid PrefabSeed  = pMeta->m_PrefabSeedGuid;
    sPrefabBase               = pMeta->m_sBasePrefab;

    m_DocumentObjectMetaData->EndReadMetaData();

    // if this is a prefab instance, update it
    if (PrefabAsset.IsValid())
    {
      UpdatePrefabObject(pChild, PrefabAsset, PrefabSeed, sPrefabBase);
    }
    else
    {
      // only recurse if no prefab was found
      // nested prefabs are not allowed
      UpdatePrefabsRecursive(pChild);
    }
  }
}

void xiiDocument::UpdatePrefabObject(xiiDocumentObject* pObject, const xiiUuid& PrefabAsset, const xiiUuid& PrefabSeed, const char* szBasePrefab)
{
  const xiiStringBuilder& sNewBasePrefab = xiiPrefabCache::GetSingleton()->GetCachedPrefabDocument(PrefabAsset);

  xiiStringBuilder sNewMergedGraph;
  xiiPrefabUtils::Merge(szBasePrefab, sNewBasePrefab, pObject, true, PrefabSeed, sNewMergedGraph);

  // remove current object
  xiiRemoveObjectCommand rm;
  rm.m_Object = pObject->GetGuid();

  // instantiate prefab again
  xiiInstantiatePrefabCommand inst;
  inst.m_Index                = pObject->GetPropertyIndex().ConvertTo<xiiInt32>();
  inst.m_bAllowPickedPosition = false;
  inst.m_CreateFromPrefab     = PrefabAsset;
  inst.m_Parent               = pObject->GetParent() == GetObjectManager()->GetRootObject() ? xiiUuid() : pObject->GetParent()->GetGuid();
  inst.m_RemapGuid            = PrefabSeed;
  inst.m_sBasePrefabGraph     = sNewBasePrefab;
  inst.m_sObjectGraph         = sNewMergedGraph;

  GetCommandHistory()->AddCommand(rm);
  GetCommandHistory()->AddCommand(inst);
}
