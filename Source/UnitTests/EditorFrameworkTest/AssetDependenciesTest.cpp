/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorFrameworkTest/AssetDependenciesTest.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

class AssetDependencyProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(AssetDependencyProperties, xiiReflectedClass);

public:
  xiiString                    m_sMember;
  xiiDynamicArray<xiiString>   m_Array;
  xiiSet<xiiString>            m_Set;
  xiiMap<xiiString, xiiString> m_Map;
  xiiVariantDictionary         m_Variants;
  xiiString                    m_sTemporary;
  xiiString                    m_sUnannotated;
  AssetDependencyProperties*   m_pChild          = nullptr;
  AssetDependencyProperties*   m_pTemporaryChild = nullptr;
};

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(AssetDependencyProperties, 1, xiiRTTIDefaultAllocator<AssetDependencyProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Member", m_sMember)->AddAttributes(new xiiAssetBrowserAttribute("Mesh", xiiDependencyFlags::Package)),
    XII_ARRAY_MEMBER_PROPERTY("Array", m_Array)->AddAttributes(new xiiAssetBrowserAttribute("Mesh", xiiDependencyFlags::Transform | xiiDependencyFlags::Thumbnail)),
    XII_SET_MEMBER_PROPERTY("Set", m_Set)->AddAttributes(new xiiFileBrowserAttribute("Source", "*", "", "", xiiDependencyFlags::Transform)),
    XII_MAP_MEMBER_PROPERTY("Map", m_Map)->AddAttributes(new xiiAssetBrowserAttribute("Mesh", xiiDependencyFlags::Transform | xiiDependencyFlags::Thumbnail | xiiDependencyFlags::Package)),
    XII_MAP_MEMBER_PROPERTY("Variants", m_Variants)->AddAttributes(new xiiAssetBrowserAttribute("Mesh", xiiDependencyFlags::Package)),
    XII_MEMBER_PROPERTY("Temporary", m_sTemporary)->AddAttributes(new xiiTemporaryAttribute(), new xiiAssetBrowserAttribute("Mesh")),
    XII_MEMBER_PROPERTY("Unannotated", m_sUnannotated),
    XII_MEMBER_PROPERTY("Child", m_pChild)->AddFlags(xiiPropertyFlags::PointerOwner),
    XII_MEMBER_PROPERTY("TemporaryChild", m_pTemporaryChild)->AddFlags(xiiPropertyFlags::PointerOwner)->AddAttributes(new xiiTemporaryAttribute()),
  }
  XII_END_PROPERTIES;
}

XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  class DependencyDocument : public xiiAssetDocument
  {
  public:
    DependencyDocument() : xiiAssetDocument("Dependencies.xiiAsset", XII_DEFAULT_NEW(xiiDocumentObjectManager), xiiAssetDocEngineConnection::None)
    {
      m_pDocumentInfo = CreateDocumentInfo();
    }
    void               Gather(xiiAssetDocumentInfo& info) const { UpdateAssetDocumentInfo(&info); }
    xiiTransformStatus InternalTransformAsset(xiiStreamWriter&, xiiStringView, const xiiPlatformProfile*, const xiiAssetFileHeader&, xiiBitflags<xiiTransformFlags>) override
    {
      return xiiTransformStatus("This document tests dependency metadata only.");
    }
  };
} // namespace

void TestExposedAssetDependencies(xiiQtEditorApp& editor, xiiStringView projectDirectory)
{
  xiiStringBuilder path = projectDirectory;
  path.AppendPath("DependencySource.xiiPrefab");
  auto* source = xiiDynamicCast<xiiSceneDocument*>(editor.CreateDocument(path, xiiDocumentFlags::EmptyDocument));
  if (!XII_TEST_BOOL(source != nullptr))
    return;
  XII_SCOPE_EXIT(source->GetDocumentManager()->CloseDocument(source));
  auto* accessor = source->GetObjectAccessor();
  accessor->StartTransaction("Expose native resource properties");
  xiiUuid ownerId, meshId;
  XII_TEST_STATUS(accessor->AddObjectByName(source->GetObjectManager()->GetRootObject(), "Children", -1, xiiGetStaticRTTI<xiiGameObject>(), ownerId));
  const auto* owner = accessor->GetObject(ownerId);
  XII_TEST_STATUS(accessor->AddObjectByName(owner, "Components", -1, xiiGetStaticRTTI<xiiMeshComponent>(), meshId));
  const auto* mesh = accessor->GetObject(meshId);
  XII_TEST_STATUS(accessor->SetValueByName(mesh, "Mesh", "{ 9a013e54-324d-47d0-899c-1ec97551bf06 }"));
  XII_TEST_STATUS(source->AddExposedParameter("Geometry", mesh, mesh->GetType()->FindPropertyByName("Mesh"), xiiVariant()));
  XII_TEST_STATUS(source->AddExposedParameter("Materials", mesh, mesh->GetType()->FindPropertyByName("MaterialOverrides"), xiiVariant()));
  XII_TEST_STATUS(source->AddExposedParameter("Label", owner, owner->GetType()->FindPropertyByName("Name"), xiiVariant()));
  accessor->FinishTransaction();
  XII_TEST_STATUS(source->SaveDocument());
  xiiStringBuilder sourceId;
  xiiConversionUtils::ToString(source->GetGuid(), sourceId);
  auto* curator = xiiAssetCurator::GetSingleton();
  curator->NotifyOfFileChange(path);
  curator->MainThreadTick(false);
  {
    const auto asset = curator->FindSubAsset(sourceId);
    if (!XII_TEST_BOOL(static_cast<bool>(asset)))
      return;
    const auto* metadata = asset->m_pAssetInfo->m_Info->GetMetaInfo<xiiExposedParameters>();
    if (!XII_TEST_BOOL(metadata != nullptr))
      return;
    XII_TEST_BOOL(metadata->Find("Geometry") != nullptr);
    XII_TEST_BOOL(metadata->Find("Materials") != nullptr);
    XII_TEST_BOOL(metadata->Find("Label") != nullptr);
  }

  DependencyDocument consumer;
  accessor = consumer.GetObjectAccessor();
  accessor->StartTransaction("Override prefab resource parameters");
  xiiUuid consumerOwnerId, consumerId;
  XII_TEST_STATUS(accessor->AddObjectByName(consumer.GetObjectManager()->GetRootObject(), "Children", -1, xiiGetStaticRTTI<xiiGameObject>(), consumerOwnerId));
  XII_TEST_STATUS(accessor->AddObjectByName(accessor->GetObject(consumerOwnerId), "Components", -1, xiiGetStaticRTTI<xiiPrefabReferenceComponent>(), consumerId));
  const auto* object = accessor->GetObject(consumerId);
  XII_TEST_STATUS(accessor->SetValueByName(object, "Prefab", sourceId.GetView()));
  XII_TEST_STATUS(accessor->InsertValueByName(object, "Parameters", "Override.xiiMeshAsset", "Geometry"));
  xiiVariantArray materials;
  materials.PushBack("Override.xiiMaterialAsset");
  materials.PushBack("");
  XII_TEST_STATUS(accessor->InsertValueByName(object, "Parameters", materials, "Materials"));
  XII_TEST_STATUS(accessor->InsertValueByName(object, "Parameters", "Label.xiiMeshAsset", "Label"));
  XII_TEST_STATUS(accessor->InsertValueByName(object, "Parameters", "Unknown.xiiMeshAsset", "Unknown"));
  accessor->FinishTransaction();
  auto verify = [&](bool hasGeometry) {
    xiiAssetDocumentInfo info;
    consumer.Gather(info);
    XII_TEST_BOOL(info.m_PackageDependencies.Contains(sourceId));
    XII_TEST_BOOL(info.m_PackageDependencies.Contains("Override.xiiMeshAsset") == hasGeometry);
    XII_TEST_BOOL(info.m_PackageDependencies.Contains("Override.xiiMaterialAsset"));
    // Native mesh and material overrides deliberately request packaging only.
    XII_TEST_BOOL(!info.m_ThumbnailDependencies.Contains("Override.xiiMeshAsset"));
    XII_TEST_BOOL(!info.m_ThumbnailDependencies.Contains("Override.xiiMaterialAsset"));
    for (const auto* dependencies : {&info.m_TransformDependencies, &info.m_ThumbnailDependencies, &info.m_PackageDependencies})
    {
      XII_TEST_BOOL(!dependencies->Contains("Label.xiiMeshAsset"));
      XII_TEST_BOOL(!dependencies->Contains("Unknown.xiiMeshAsset"));
      XII_TEST_BOOL(!dependencies->Contains(""));
    }
    XII_TEST_INT(info.m_TransformDependencies.GetCount(), 0);
    XII_TEST_BOOL(info.m_ThumbnailDependencies.Contains(sourceId));
    XII_TEST_INT(info.m_ThumbnailDependencies.GetCount(), 1);
  };
  verify(true);
  accessor->StartTransaction("Remove resource override");
  XII_TEST_STATUS(accessor->RemoveValueByName(object, "Parameters", "Geometry"));
  accessor->FinishTransaction();
  verify(false);
  XII_TEST_STATUS(consumer.GetCommandHistory()->Undo());
  verify(true);
  XII_TEST_STATUS(consumer.GetCommandHistory()->Redo());
  verify(false);

  // Instantiate the saved prefab through the production command and default-state provider.
  // Its unchanged resources belong to the source prefab; authored changes belong to the scene.
  xiiStringBuilder scenePath = projectDirectory;
  scenePath.AppendPath("DependencyConsumer.xiiScene");
  auto* scene = xiiDynamicCast<xiiSceneDocument*>(editor.CreateDocument(scenePath, xiiDocumentFlags::EmptyDocument));
  if (!XII_TEST_BOOL(scene != nullptr))
    return;
  XII_SCOPE_EXIT(scene->GetDocumentManager()->CloseDocument(scene));
  xiiInstantiatePrefabCommand instantiate;
  instantiate.m_CreateFromPrefab     = source->GetGuid();
  instantiate.m_RemapGuid            = xiiUuid::MakeUuid();
  instantiate.m_sBasePrefabGraph     = xiiPrefabCache::GetSingleton()->GetCachedPrefabDocument(source->GetGuid());
  instantiate.m_bAllowPickedPosition = false;
  auto* history                      = scene->GetCommandHistory();
  history->StartTransaction("Instantiate resource dependency source");
  const auto status = history->AddCommand(instantiate);
  XII_TEST_STATUS(status);
  if (status.Failed())
  {
    history->CancelTransaction();
    return;
  }
  history->FinishTransaction();
  XII_TEST_STATUS(scene->SaveDocument());
  XII_TEST_BOOL(scene->GetAssetDocumentInfo()->m_TransformDependencies.Contains(sourceId));
  XII_TEST_BOOL(scene->GetAssetDocumentInfo()->m_ThumbnailDependencies.Contains(sourceId));
  XII_TEST_BOOL(!scene->GetAssetDocumentInfo()->m_PackageDependencies.Contains("{ 9a013e54-324d-47d0-899c-1ec97551bf06 }"));
  auto instanceMeshId = meshId;
  instanceMeshId.CombineWithSeed(instantiate.m_RemapGuid);
  const auto* instanceMesh = scene->GetObjectManager()->GetObject(instanceMeshId);
  if (!XII_TEST_BOOL(instanceMesh != nullptr))
    return;
  accessor = scene->GetObjectAccessor();
  accessor->StartTransaction("Override inherited mesh");
  XII_TEST_STATUS(accessor->SetValueByName(instanceMesh, "Mesh", "InstanceOverride.xiiMeshAsset"));
  accessor->FinishTransaction();
  XII_TEST_STATUS(scene->SaveDocument());
  XII_TEST_BOOL(scene->GetAssetDocumentInfo()->m_PackageDependencies.Contains("InstanceOverride.xiiMeshAsset"));
  XII_TEST_STATUS(history->Undo());
  XII_TEST_STATUS(scene->SaveDocument());
  XII_TEST_BOOL(!scene->GetAssetDocumentInfo()->m_PackageDependencies.Contains("InstanceOverride.xiiMeshAsset"));

  // A source metadata revision must immediately affect subsequent collection, without retaining
  // pointers to old curator metadata or treating removed/unknown parameter names as resources.
  XII_TEST_STATUS(consumer.GetCommandHistory()->Undo());
  verify(true);
  accessor = source->GetObjectAccessor();
  accessor->StartTransaction("Remove exposed resource metadata");
  XII_TEST_STATUS(source->RemoveExposedParameter(source->FindExposedParameter(mesh, mesh->GetType()->FindPropertyByName("Mesh"), xiiVariant())));
  accessor->FinishTransaction();
  XII_TEST_STATUS(source->SaveDocument());
  curator->NotifyOfFileChange(path);
  curator->MainThreadTick(false);
  verify(false);

  accessor = consumer.GetObjectAccessor();
  accessor->StartTransaction("Unresolved parameter source");
  XII_TEST_STATUS(accessor->SetValueByName(object, "Prefab", "Missing.xiiPrefab"));
  accessor->FinishTransaction();
  xiiAssetDocumentInfo unresolved;
  consumer.Gather(unresolved);
  XII_TEST_BOOL(unresolved.m_PackageDependencies.Contains("Missing.xiiPrefab"));
  XII_TEST_INT(unresolved.m_PackageDependencies.GetCount(), 1);
  XII_TEST_INT(unresolved.m_ThumbnailDependencies.GetCount(), 1);
  XII_TEST_INT(unresolved.m_TransformDependencies.GetCount(), 0);
  XII_TEST_STATUS(consumer.GetCommandHistory()->Undo());
  verify(false);
}

XII_CREATE_SIMPLE_TEST(EditorDocuments, AssetDependencies)
{
  DependencyDocument document;
  auto*              accessor = document.GetObjectAccessor();
  accessor->StartTransaction("Author references");
  xiiUuid id;
  XII_TEST_STATUS(accessor->AddObjectByName(document.GetObjectManager()->GetRootObject(), "Children", -1, xiiGetStaticRTTI<AssetDependencyProperties>(), id));
  const auto* object = accessor->GetObject(id);
  XII_TEST_STATUS(accessor->SetValueByName(object, "Member", "Member.xiiMeshAsset"));
  XII_TEST_STATUS(accessor->InsertValueByName(object, "Array", "Array.xiiMeshAsset", -1));
  XII_TEST_STATUS(accessor->InsertValueByName(object, "Set", "Source.fbx", -1));
  XII_TEST_STATUS(accessor->InsertValueByName(object, "Map", "Map.xiiMeshAsset", "entry"));
  XII_TEST_STATUS(accessor->InsertValueByName(object, "Map", "", "empty"));
  XII_TEST_STATUS(accessor->InsertValueByName(object, "Variants", "Variant.xiiMeshAsset", "resource"));
  XII_TEST_STATUS(accessor->InsertValueByName(object, "Variants", 17, "number"));
  XII_TEST_STATUS(accessor->InsertValueByName(object, "Variants", xiiStringView("View.xiiMeshAsset"), "view"));
  xiiVariantDictionary nested;
  nested.Insert("resource", "NestedVariant.xiiMeshAsset");
  nested.Insert("ignored", false);
  xiiVariantArray array;
  array.PushBack(nested);
  array.PushBack("Member.xiiMeshAsset"); // Duplicate references remain one dependency.
  XII_TEST_STATUS(accessor->InsertValueByName(object, "Variants", array, "nested"));
  xiiUuid childId, temporaryChildId;
  XII_TEST_STATUS(accessor->AddObjectByName(object, "Child", xiiVariant(), xiiGetStaticRTTI<AssetDependencyProperties>(), childId));
  XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(childId), "Member", "Child.xiiMeshAsset"));
  XII_TEST_STATUS(accessor->AddObjectByName(object, "TemporaryChild", xiiVariant(), xiiGetStaticRTTI<AssetDependencyProperties>(), temporaryChildId));
  XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(temporaryChildId), "Member", "TemporaryChild.xiiMeshAsset"));
  XII_TEST_STATUS(accessor->SetValueByName(object, "Temporary", "Temporary.xiiMeshAsset"));
  XII_TEST_STATUS(accessor->SetValueByName(object, "Unannotated", "Unannotated.xiiMeshAsset"));
  accessor->FinishTransaction();
  auto verify = [&](xiiStringView mapReference) {
    xiiAssetDocumentInfo info;
    document.Gather(info);
    XII_TEST_BOOL(info.m_TransformDependencies.Contains("Source.fbx"));
    XII_TEST_BOOL(info.m_TransformDependencies.Contains("Array.xiiMeshAsset"));
    XII_TEST_BOOL(info.m_ThumbnailDependencies.Contains("Array.xiiMeshAsset"));
    XII_TEST_BOOL(info.m_PackageDependencies.Contains("Member.xiiMeshAsset"));
    XII_TEST_BOOL(info.m_PackageDependencies.Contains("Variant.xiiMeshAsset"));
    XII_TEST_BOOL(info.m_PackageDependencies.Contains("View.xiiMeshAsset"));
    XII_TEST_BOOL(info.m_PackageDependencies.Contains("NestedVariant.xiiMeshAsset"));
    XII_TEST_BOOL(info.m_PackageDependencies.Contains("Child.xiiMeshAsset"));
    for (const auto* dependencies : {&info.m_TransformDependencies, &info.m_ThumbnailDependencies, &info.m_PackageDependencies})
    {
      XII_TEST_BOOL(dependencies->Contains(mapReference));
      XII_TEST_BOOL(!dependencies->Contains(""));
      XII_TEST_BOOL(!dependencies->Contains("Temporary.xiiMeshAsset"));
      XII_TEST_BOOL(!dependencies->Contains("TemporaryChild.xiiMeshAsset"));
      XII_TEST_BOOL(!dependencies->Contains("Unannotated.xiiMeshAsset"));
      XII_TEST_BOOL(!dependencies->Contains("17"));
    }
    XII_TEST_INT(info.m_TransformDependencies.GetCount(), 3);
    XII_TEST_INT(info.m_ThumbnailDependencies.GetCount(), 2);
    XII_TEST_INT(info.m_PackageDependencies.GetCount(), 6);
  };
  verify("Map.xiiMeshAsset");
  accessor->StartTransaction("Replace mapped resource");
  XII_TEST_STATUS(accessor->SetValueByName(object, "Map", "Replacement.xiiMeshAsset", "entry"));
  accessor->FinishTransaction();
  verify("Replacement.xiiMeshAsset");
  XII_TEST_STATUS(document.GetCommandHistory()->Undo());
  verify("Map.xiiMeshAsset");
  XII_TEST_STATUS(document.GetCommandHistory()->Redo());
  verify("Replacement.xiiMeshAsset");
}
