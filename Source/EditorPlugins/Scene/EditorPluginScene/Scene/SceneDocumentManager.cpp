#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Scene/SceneDocumentManager.h>
#include <Foundation/Strings/PathUtils.h>
#include <ToolsFoundation/Command/TreeCommands.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneDocumentManager, 1, xiiRTTIDefaultAllocator<xiiSceneDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;


xiiSceneDocumentManager::xiiSceneDocumentManager()
{
  // Document type descriptor for a standard XII scene
  {
    auto& docTypeDesc               = m_DocTypeDescs.ExpandAndGetRef();
    docTypeDesc.m_sDocumentTypeName = "Scene";
    docTypeDesc.m_sFileExtension    = "xiiScene";
    docTypeDesc.m_sIcon             = ":/AssetIcons/Scene.png";
    docTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiScene2Document>();
    docTypeDesc.m_pManager          = this;
    docTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Scene");

    docTypeDesc.m_sResourceFileExtension = "xiiObjectGraph";
    docTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::OnlyTransformManually | xiiAssetDocumentFlags::SupportsThumbnail;
  }

  // Document type descriptor for a prefab
  {
    auto& docTypeDesc = m_DocTypeDescs.ExpandAndGetRef();

    docTypeDesc.m_sDocumentTypeName = "Prefab";
    docTypeDesc.m_sFileExtension    = "xiiPrefab";
    docTypeDesc.m_sIcon             = ":/AssetIcons/Prefab.png";
    docTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiSceneDocument>();
    docTypeDesc.m_pManager          = this;
    docTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Prefab");

    docTypeDesc.m_sResourceFileExtension = "xiiObjectGraph";
    docTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave | xiiAssetDocumentFlags::SupportsThumbnail;
  }

  // Document type descriptor for a layer (similar to a normal scene) as it holds a scene object graph
  {
    auto& docTypeDesc               = m_DocTypeDescs.ExpandAndGetRef();
    docTypeDesc.m_sDocumentTypeName = "Layer";
    docTypeDesc.m_sFileExtension    = "xiiSceneLayer";
    docTypeDesc.m_sIcon             = ":/AssetIcons/Layer.png";
    docTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiLayerDocument>();
    docTypeDesc.m_pManager          = this;
    docTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Scene_Layer");

    docTypeDesc.m_sResourceFileExtension = "";
    // A layer can not be transformed individually (at least at the moment)
    // all layers for a scene are gathered and put into one cohesive runtime scene
    docTypeDesc.m_AssetDocumentFlags = xiiAssetDocumentFlags::DisableTransform; // TODO: Disable creation in "New Document"?
  }
}

void xiiSceneDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  if (xiiStringUtils::IsEqual(szDocumentTypeName, "Scene"))
  {
    out_pDocument = new xiiScene2Document(szPath);

    if (bCreateNewDocument)
    {
      SetupDefaultScene(out_pDocument);
    }
  }
  else if (xiiStringUtils::IsEqual(szDocumentTypeName, "Prefab"))
  {
    out_pDocument = new xiiSceneDocument(szPath, xiiSceneDocument::DocumentType::Prefab);
  }
  else if (xiiStringUtils::IsEqual(szDocumentTypeName, "Layer"))
  {
    if (pOpenContext == nullptr)
    {
      // Opened individually
      out_pDocument = new xiiSceneDocument(szPath, xiiSceneDocument::DocumentType::Layer);
    }
    else
    {
      // Opened via a parent scene document
      xiiScene2Document* pDoc = const_cast<xiiScene2Document*>(xiiDynamicCast<const xiiScene2Document*>(pOpenContext->GetDocumentObjectManager()->GetDocument()));
      out_pDocument           = new xiiLayerDocument(szPath, pDoc);
    }
  }
}

void xiiSceneDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  for (auto& docTypeDesc : m_DocTypeDescs)
  {
    inout_DocumentTypes.PushBack(&docTypeDesc);
  }
}

void xiiSceneDocumentManager::InternalCloneDocument(const char* szPath, const char* szClonePath, const xiiUuid& documentId, const xiiUuid& seedGuid, const xiiUuid& cloneGuid, xiiAbstractObjectGraph* pHeader, xiiAbstractObjectGraph* pObjects, xiiAbstractObjectGraph* pTypes)
{
  xiiAssetDocumentManager::InternalCloneDocument(szPath, szClonePath, documentId, seedGuid, cloneGuid, pHeader, pObjects, pTypes);

  auto    pRoot        = pObjects->GetNodeByName("ObjectTree");
  xiiUuid settingsGuid = pRoot->FindProperty("Settings")->m_Value.Get<xiiUuid>();
  auto    pSettings    = pObjects->GetNode(settingsGuid);
  if (xiiRTTI::FindTypeByName(pSettings->GetType()) != xiiGetStaticRTTI<xiiSceneDocumentSettings>())
    return;

  // Fix up scene layers during cloning
  pObjects->ModifyNodeViaNativeCounterpart(pSettings, [&](void* pNativeObject, const xiiRTTI* pType) {
    xiiSceneDocumentSettings* pObject = static_cast<xiiSceneDocumentSettings*>(pNativeObject);

    for (xiiSceneLayerBase* pLayerBase : pObject->m_Layers)
    {
      if (auto pLayer = xiiDynamicCast<xiiSceneLayer*>(pLayerBase))
      {
        if (pLayer->m_Layer == documentId)
        {
          // Fix up main layer reference in layer list
          pLayer->m_Layer = cloneGuid;
        }
        else
        {
          // Clone layer.
          xiiStringBuilder sLayerPath;
          {
            auto assetInfo = xiiAssetCurator::GetSingleton()->GetSubAsset(pLayer->m_Layer);
            if (assetInfo.isValid())
            {
              sLayerPath = assetInfo->m_pAssetInfo->m_sAbsolutePath;
            }
            else
            {
              xiiLog::Error("Failed to resolve layer: {}. Cloned Layer will be invalid.");
              pLayer->m_Layer.SetInvalid();
            }
          }
          if (!sLayerPath.IsEmpty())
          {
            xiiUuid newLayerGuid = pLayer->m_Layer;
            newLayerGuid.CombineWithSeed(seedGuid);

            xiiStringBuilder sLayerClonePath = szClonePath;
            sLayerClonePath.RemoveFileExtension();
            sLayerClonePath.Append("_data");
            xiiStringBuilder sCloneFleName = xiiPathUtils::GetFileNameAndExtension(sLayerPath.GetData());
            sLayerClonePath.AppendPath(sCloneFleName);
            // We assume that all layers are handled by the same document manager, i.e. this.
            CloneDocument(sLayerPath, sLayerClonePath, newLayerGuid).LogFailure();
            pLayer->m_Layer = newLayerGuid;
          }
        }
      }
    }
  });
}

void xiiSceneDocumentManager::SetupDefaultScene(xiiDocument* pDocument)
{
  auto history = pDocument->GetCommandHistory();
  history->StartTransaction("Initial Scene Setup");

  xiiUuid skyObjectGuid;
  skyObjectGuid.CreateNewUuid();
  xiiUuid lightObjectGuid;
  lightObjectGuid.CreateNewUuid();
  xiiUuid meshObjectGuid;
  meshObjectGuid.CreateNewUuid();

  // Thumbnail Camera
  {
    xiiUuid objectGuid;
    objectGuid.CreateNewUuid();

    xiiAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("xiiGameObject");
    cmd.m_NewObjectGuid   = objectGuid;
    cmd.m_sParentProperty = "Children";
    XII_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    // object name
    {
      xiiSetObjectPropertyCommand propCmd;
      propCmd.m_Object    = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Name";
      propCmd.m_NewValue  = "Scene Thumbnail Camera";
      XII_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    // camera position
    {
      xiiSetObjectPropertyCommand propCmd;
      propCmd.m_Object    = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue  = xiiVec3(0, 0, 0);
      XII_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    // camera component
    {
      xiiAddObjectCommand cmd;
      cmd.m_Index = -1;
      cmd.SetType("xiiCameraComponent");
      cmd.m_Parent          = objectGuid;
      cmd.m_sParentProperty = "Components";
      XII_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

      // camera shortcut
      {
        xiiSetObjectPropertyCommand propCmd;
        propCmd.m_Object    = cmd.m_NewObjectGuid;
        propCmd.m_sProperty = "EditorShortcut";
        propCmd.m_NewValue  = 1;
        XII_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
      }

      // camera usage hint
      {
        xiiSetObjectPropertyCommand propCmd;
        propCmd.m_Object    = cmd.m_NewObjectGuid;
        propCmd.m_sProperty = "UsageHint";
        propCmd.m_NewValue  = (int)xiiCameraUsageHint::Thumbnail;
        XII_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
      }
    }
  }

  {
    xiiAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("xiiGameObject");
    cmd.m_NewObjectGuid   = meshObjectGuid;
    cmd.m_sParentProperty = "Children";
    XII_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    {
      xiiSetObjectPropertyCommand propCmd;
      propCmd.m_Object    = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue  = xiiVec3(3, 0, 0);
      XII_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  {
    xiiAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("xiiGameObject");
    cmd.m_NewObjectGuid   = skyObjectGuid;
    cmd.m_sParentProperty = "Children";
    XII_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    {
      xiiSetObjectPropertyCommand propCmd;
      propCmd.m_Object    = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue  = xiiVec3(0, 0, 1);
      XII_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    {
      xiiRemoveObjectPropertyCommand propCmd;
      propCmd.m_Object    = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Tags";
      propCmd.m_Index     = 0; // There is only one value in the set, CastShadow.
      XII_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    {
      xiiInsertObjectPropertyCommand propCmd;
      propCmd.m_Object    = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Tags";
      propCmd.m_Index     = 0;
      propCmd.m_NewValue  = "SkyLight";
      XII_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  {
    xiiAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("xiiGameObject");
    cmd.m_NewObjectGuid   = lightObjectGuid;
    cmd.m_sParentProperty = "Children";
    XII_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    {
      xiiSetObjectPropertyCommand propCmd;
      propCmd.m_Object    = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue  = xiiVec3(0, 0, 2);
      XII_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    {
      xiiQuat qRot;
      qRot.SetFromEulerAngles(xiiAngle::Degree(0), xiiAngle::Degree(55), xiiAngle::Degree(90));

      xiiSetObjectPropertyCommand propCmd;
      propCmd.m_Object    = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalRotation";
      propCmd.m_NewValue  = qRot;
      XII_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  {
    xiiAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("xiiSkyBoxComponent");
    cmd.m_Parent          = skyObjectGuid;
    cmd.m_sParentProperty = "Components";
    XII_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    {
      xiiSetObjectPropertyCommand propCmd;
      propCmd.m_Object    = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "CubeMap";
      propCmd.m_NewValue  = "{ 0b202e08-a64f-465d-b38e-15b81d161822 }";
      XII_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    {
      xiiSetObjectPropertyCommand propCmd;
      propCmd.m_Object    = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "ExposureBias";
      propCmd.m_NewValue  = 1.0f;
      XII_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  {
    xiiAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("xiiSkyLightComponent");
    cmd.m_Parent          = lightObjectGuid;
    cmd.m_sParentProperty = "Components";
    XII_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");
  }

  {
    xiiAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("xiiDirectionalLightComponent");
    cmd.m_Parent          = lightObjectGuid;
    cmd.m_sParentProperty = "Components";
    XII_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");
  }

  {
    xiiAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("xiiMeshComponent");
    cmd.m_Parent          = meshObjectGuid;
    cmd.m_sParentProperty = "Components";
    XII_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    {
      xiiSetObjectPropertyCommand propCmd;
      propCmd.m_Object    = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Mesh";
      propCmd.m_NewValue  = "{ 618ee743-ed04-4fac-bf5f-572939db2f1d }"; // Base/Meshes/Sphere.xiiMeshAsset
      XII_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  history->FinishTransaction();
}
