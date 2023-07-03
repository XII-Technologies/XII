#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <ToolsFoundation/Command/TreeCommands.h>


void xiiSceneDocument::UnlinkPrefabs(const xiiDeque<const xiiDocumentObject*>& Selection)
{
  SUPER::UnlinkPrefabs(Selection);

  // Clear cached names.
  for (auto pObject : Selection)
  {
    auto pMetaScene = m_GameObjectMetaData->BeginModifyMetaData(pObject->GetGuid());
    pMetaScene->m_CachedNodeName.Clear();
    m_GameObjectMetaData->EndModifyMetaData(xiiGameObjectMetaData::CachedName);
  }
}


bool xiiSceneDocument::IsObjectEditorPrefab(const xiiUuid& object, xiiUuid* out_PrefabAssetGuid) const
{
  auto       pMeta     = m_DocumentObjectMetaData->BeginReadMetaData(object);
  const bool bIsPrefab = pMeta->m_CreateFromPrefab.IsValid();

  if (out_PrefabAssetGuid)
  {
    *out_PrefabAssetGuid = pMeta->m_CreateFromPrefab;
  }

  m_DocumentObjectMetaData->EndReadMetaData();

  return bIsPrefab;
}


bool xiiSceneDocument::IsObjectEnginePrefab(const xiiUuid& object, xiiUuid* out_PrefabAssetGuid) const
{
  const xiiDocumentObject* pObject = GetObjectManager()->GetObject(object);

  xiiHybridArray<xiiVariant, 16> values;
  pObject->GetTypeAccessor().GetValues("Components", values);

  for (xiiVariant& value : values)
  {
    auto pChild = GetObjectManager()->GetObject(value.Get<xiiUuid>());

    // search for prefab components
    if (pChild->GetTypeAccessor().GetType()->IsDerivedFrom<xiiPrefabReferenceComponent>())
    {
      xiiVariant varPrefab = pChild->GetTypeAccessor().GetValue("Prefab");

      if (varPrefab.IsA<xiiString>())
      {
        if (out_PrefabAssetGuid)
        {
          const xiiString sAsset = varPrefab.Get<xiiString>();

          const auto info = xiiAssetCurator::GetSingleton()->FindSubAsset(sAsset);

          if (info.isValid())
          {
            *out_PrefabAssetGuid = info->m_Data.m_Guid;
          }
        }

        return true;
      }
    }
  }

  return false;
}

void xiiSceneDocument::UpdatePrefabs()
{
  XII_LOCK(m_GameObjectMetaData->GetMutex());
  SUPER::UpdatePrefabs();
}


xiiUuid xiiSceneDocument::ReplaceByPrefab(const xiiDocumentObject* pRootObject, const char* szPrefabFile, const xiiUuid& PrefabAsset, const xiiUuid& PrefabSeed, bool bEnginePrefab)
{
  xiiUuid newGuid = SUPER::ReplaceByPrefab(pRootObject, szPrefabFile, PrefabAsset, PrefabSeed, bEnginePrefab);
  if (newGuid.IsValid())
  {
    auto pMeta = m_GameObjectMetaData->BeginModifyMetaData(newGuid);
    pMeta->m_CachedNodeName.Clear();
    m_GameObjectMetaData->EndModifyMetaData(xiiGameObjectMetaData::CachedName);
  }
  return newGuid;
}

xiiUuid xiiSceneDocument::RevertPrefab(const xiiDocumentObject* pObject)
{
  auto          pHistory           = GetCommandHistory();
  const xiiVec3 vLocalPos          = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<xiiVec3>();
  const xiiQuat vLocalRot          = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<xiiQuat>();
  const xiiVec3 vLocalScale        = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<xiiVec3>();
  const float   fLocalUniformScale = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  xiiUuid newGuid = SUPER::RevertPrefab(pObject);

  if (newGuid.IsValid())
  {
    xiiSetObjectPropertyCommand setCmd;
    setCmd.m_Object = newGuid;

    setCmd.m_sProperty = "LocalPosition";
    setCmd.m_NewValue  = vLocalPos;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalRotation";
    setCmd.m_NewValue  = vLocalRot;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalScaling";
    setCmd.m_NewValue  = vLocalScale;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalUniformScaling";
    setCmd.m_NewValue  = fLocalUniformScale;
    pHistory->AddCommand(setCmd).AssertSuccess();
  }
  return newGuid;
}

void xiiSceneDocument::UpdatePrefabObject(xiiDocumentObject* pObject, const xiiUuid& PrefabAsset, const xiiUuid& PrefabSeed, const char* szBasePrefab)
{
  auto          pHistory           = GetCommandHistory();
  const xiiVec3 vLocalPos          = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<xiiVec3>();
  const xiiQuat vLocalRot          = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<xiiQuat>();
  const xiiVec3 vLocalScale        = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<xiiVec3>();
  const float   fLocalUniformScale = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  SUPER::UpdatePrefabObject(pObject, PrefabAsset, PrefabSeed, szBasePrefab);

  // the root object has the same GUID as the PrefabSeed
  if (PrefabSeed.IsValid())
  {
    xiiSetObjectPropertyCommand setCmd;
    setCmd.m_Object = PrefabSeed;

    setCmd.m_sProperty = "LocalPosition";
    setCmd.m_NewValue  = vLocalPos;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalRotation";
    setCmd.m_NewValue  = vLocalRot;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalScaling";
    setCmd.m_NewValue  = vLocalScale;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalUniformScaling";
    setCmd.m_NewValue  = fLocalUniformScale;
    pHistory->AddCommand(setCmd).AssertSuccess();
  }
}

void xiiSceneDocument::ConvertToEditorPrefab(const xiiDeque<const xiiDocumentObject*>& Selection)
{
  xiiDeque<const xiiDocumentObject*> newSelection;

  auto pHistory = GetCommandHistory();
  pHistory->StartTransaction("Convert to Editor Prefab");

  for (const xiiDocumentObject* pObject : Selection)
  {
    xiiUuid assetGuid;
    if (!IsObjectEnginePrefab(pObject->GetGuid(), &assetGuid))
      continue;

    auto pAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(assetGuid);

    if (!pAsset.isValid())
      continue;

    const xiiTransform transform = GetGlobalTransform(pObject);

    xiiUuid newGuid;
    newGuid.CreateNewUuid();
    xiiUuid newObject = ReplaceByPrefab(pObject, pAsset->m_pAssetInfo->m_sAbsolutePath, assetGuid, newGuid, false);

    if (newObject.IsValid())
    {
      const xiiDocumentObject* pNewObject = GetObjectManager()->GetObject(newObject);
      SetGlobalTransform(pNewObject, transform, TransformationChanges::All);

      newSelection.PushBack(pNewObject);
    }
  }

  pHistory->FinishTransaction();

  GetSelectionManager()->SetSelection(newSelection);
}

void xiiSceneDocument::ConvertToEnginePrefab(const xiiDeque<const xiiDocumentObject*>& Selection)
{
  xiiDeque<const xiiDocumentObject*> newSelection;

  auto pHistory = GetCommandHistory();
  pHistory->StartTransaction("Convert to Engine Prefab");

  xiiStringBuilder tmp;

  for (const xiiDocumentObject* pObject : Selection)
  {
    xiiUuid assetGuid;
    if (!IsObjectEditorPrefab(pObject->GetGuid(), &assetGuid))
      continue;

    auto pAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(assetGuid);

    if (!pAsset.isValid())
      continue;

    const xiiTransform transform = ComputeGlobalTransform(pObject);

    const xiiDocumentObject* pNewObject = nullptr;

    // create an object with the reference prefab component
    {
      xiiUuid ObjectGuid, CmpGuid;
      ObjectGuid.CreateNewUuid();
      CmpGuid.CreateNewUuid();

      xiiAddObjectCommand cmd;
      cmd.m_Parent = (pObject->GetParent() == GetObjectManager()->GetRootObject()) ? xiiUuid() : pObject->GetParent()->GetGuid();
      cmd.m_Index  = pObject->GetPropertyIndex();
      cmd.SetType("xiiGameObject");
      cmd.m_NewObjectGuid   = ObjectGuid;
      cmd.m_sParentProperty = "Children";

      XII_VERIFY(pHistory->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

      cmd.SetType("xiiPrefabReferenceComponent");
      cmd.m_sParentProperty = "Components";
      cmd.m_Index           = -1;
      cmd.m_NewObjectGuid   = CmpGuid;
      cmd.m_Parent          = ObjectGuid;
      XII_VERIFY(pHistory->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

      xiiSetObjectPropertyCommand cmd2;
      cmd2.m_Object    = CmpGuid;
      cmd2.m_sProperty = "Prefab";
      cmd2.m_NewValue  = xiiConversionUtils::ToString(assetGuid, tmp).GetData();
      XII_VERIFY(pHistory->AddCommand(cmd2).m_Result.Succeeded(), "AddCommand failed");


      pNewObject = GetObjectManager()->GetObject(ObjectGuid);
    }

    // set same position
    SetGlobalTransform(pNewObject, transform, TransformationChanges::All);

    newSelection.PushBack(pNewObject);

    // delete old object
    {
      xiiRemoveObjectCommand rem;
      rem.m_Object = pObject->GetGuid();

      XII_VERIFY(pHistory->AddCommand(rem).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  pHistory->FinishTransaction();

  GetSelectionManager()->SetSelection(newSelection);
}
