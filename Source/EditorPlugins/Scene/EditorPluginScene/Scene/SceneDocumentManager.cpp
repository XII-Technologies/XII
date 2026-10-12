/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Scene/SceneDocumentManager.h>
#include <Foundation/Strings/PathUtils.h>
#include <SharedPluginScene/Scene/SceneCreation.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneDocumentManager, 1, xiiRTTIDefaultAllocator<xiiSceneDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;


xiiSceneDocumentManager::xiiSceneDocumentManager()
{
  // Document type descriptor for a standard XII scene
  {
    auto& docTypeDesc               = m_DocTypeDescs.ExpandAndGetRef();
    docTypeDesc.m_sDocumentTypeName = "Scene";
    docTypeDesc.m_sFileExtension    = "xiiScene";
    docTypeDesc.m_sIcon             = ":/AssetIcons/Scene.svg";
    docTypeDesc.m_sAssetCategory    = "Construction";
    docTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiScene2Document>();
    docTypeDesc.m_pManager          = this;
    docTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Scene");

    docTypeDesc.m_sResourceFileExtension = "xiiBinScene";
    docTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::OnlyTransformManually | xiiAssetDocumentFlags::SupportsThumbnail;
  }

  // Document type descriptor for a prefab
  {
    auto& docTypeDesc = m_DocTypeDescs.ExpandAndGetRef();

    docTypeDesc.m_sDocumentTypeName = "Prefab";
    docTypeDesc.m_sFileExtension    = "xiiPrefab";
    docTypeDesc.m_sIcon             = ":/AssetIcons/Prefab.svg";
    docTypeDesc.m_sAssetCategory    = "Construction";
    docTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiSceneDocument>();
    docTypeDesc.m_pManager          = this;
    docTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Prefab");

    docTypeDesc.m_sResourceFileExtension = "xiiBinPrefab";
    docTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave | xiiAssetDocumentFlags::SupportsThumbnail;
  }

  // Document type descriptor for a layer (similar to a normal scene) as it holds a scene object graph
  {
    auto& docTypeDesc               = m_DocTypeDescs.ExpandAndGetRef();
    docTypeDesc.m_sDocumentTypeName = "Layer";
    docTypeDesc.m_sFileExtension    = "xiiSceneLayer";
    docTypeDesc.m_sIcon             = ":/AssetIcons/Layer.svg";
    docTypeDesc.m_sAssetCategory    = "Construction";
    docTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiLayerDocument>();
    docTypeDesc.m_pManager          = this;
    docTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Scene_Layer");

    docTypeDesc.m_sResourceFileExtension = "";
    // A layer can not be transformed individually (at least at the moment)
    // all layers for a scene are gathered and put into one cohesive runtime scene
    docTypeDesc.m_AssetDocumentFlags = xiiAssetDocumentFlags::DisableTransform; // TODO: Disable creation in "New Document"?
  }
}

xiiStatus xiiSceneDocumentManager::InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  if (sDocumentTypeName.IsEqual("Scene"))
  {
    out_pDocument = new xiiScene2Document(sPath);

    if (bCreateNewDocument)
    {
      XII_SUCCEED_OR_RETURN(xiiCreateDefaultScene(*out_pDocument->GetObjectAccessor()));
    }
  }
  else if (sDocumentTypeName.IsEqual("Prefab"))
  {
    out_pDocument = new xiiSceneDocument(sPath, xiiSceneDocument::DocumentType::Prefab);
  }
  else if (sDocumentTypeName.IsEqual("Layer"))
  {
    if (pOpenContext == nullptr)
    {
      // Opened individually
      out_pDocument = new xiiSceneDocument(sPath, xiiSceneDocument::DocumentType::Layer);
    }
    else
    {
      // Opened via a parent scene document
      xiiScene2Document* pDoc = const_cast<xiiScene2Document*>(xiiDynamicCast<const xiiScene2Document*>(pOpenContext->GetDocumentObjectManager()->GetDocument()));
      if (pDoc == nullptr)
        return xiiStatus("A scene layer requires a parent scene document.");

      out_pDocument           = new xiiLayerDocument(sPath, pDoc);
    }
  }
  else
  {
    return xiiStatus("Unsupported scene document type.");
  }
  return XII_SUCCESS;
}

void xiiSceneDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  for (auto& docTypeDesc : m_DocTypeDescs)
  {
    inout_DocumentTypes.PushBack(&docTypeDesc);
  }
}

void xiiSceneDocumentManager::InternalCloneDocument(xiiStringView sPath, xiiStringView sClonePath, const xiiUuid& documentId, const xiiUuid& seedGuid, const xiiUuid& cloneGuid, xiiAbstractObjectGraph* pHeader, xiiAbstractObjectGraph* pObjects, xiiAbstractObjectGraph* pTypes)
{
  xiiAssetDocumentManager::InternalCloneDocument(sPath, sClonePath, documentId, seedGuid, cloneGuid, pHeader, pObjects, pTypes);

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
              sLayerPath = assetInfo->m_pAssetInfo->m_Path;
            }
            else
            {
              xiiLog::Error("Failed to resolve layer: {}. Cloned Layer will be invalid.");
              pLayer->m_Layer = xiiUuid::MakeInvalid();
            }
          }
          if (!sLayerPath.IsEmpty())
          {
            xiiUuid newLayerGuid = pLayer->m_Layer;
            newLayerGuid.CombineWithSeed(seedGuid);

            xiiStringBuilder sLayerClonePath = sClonePath;
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
