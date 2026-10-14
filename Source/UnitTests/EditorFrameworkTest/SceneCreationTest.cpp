/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/WorldRttiConverterContext.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <SharedPluginScene/Scene/SceneCreation.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

namespace
{
  class SceneCreationDocument : public xiiDocument
  {
  public:
    SceneCreationDocument(xiiStringView path = "SceneCreationTest.xiiScene", bool createInfo = true) : xiiDocument(path, XII_DEFAULT_NEW(xiiDocumentObjectManager))
    {
      if (createInfo)
        m_pDocumentInfo = CreateDocumentInfo();
    }

    xiiDocumentInfo* CreateDocumentInfo() override { return XII_DEFAULT_NEW(xiiDocumentInfo); }
  };

  class SceneCreationManager : public xiiDocumentManager
  {
  public:
    SceneCreationManager()
    {
      m_Type.m_sDocumentTypeName = "Scene";
      m_Type.m_sFileExtension    = "xiiScene";
      m_Type.m_pDocumentType     = xiiGetStaticRTTI<xiiDocument>();
      m_Type.m_pManager          = this;
    }

    ~SceneCreationManager() override { CloseAllDocumentsOfManager(); }

    xiiStatus CloneDocument(xiiStringView, xiiStringView, xiiUuid&) override
    {
      ++m_uiTemplateClones;
      return xiiStatus("The native default scene must not be replaced by a bundled legacy template.");
    }

    xiiUInt32 m_uiTemplateClones = 0;

  private:
    xiiStatus InternalCreateDocument(xiiStringView, xiiStringView path, bool create, xiiDocument*& out_document, const xiiDocumentObject*) override
    {
      out_document = new SceneCreationDocument(path, false);
      if (create)
        return xiiCreateDefaultScene(*out_document->GetObjectAccessor());
      return XII_SUCCESS;
    }

    void                      InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& types) const override { types.PushBack(&m_Type); }
    xiiDocumentTypeDescriptor m_Type;
  };

  class RejectLightIntensityAccessor : public xiiObjectCommandAccessor
  {
  public:
    using xiiObjectCommandAccessor::xiiObjectCommandAccessor;

    xiiStatus SetValue(const xiiDocumentObject* object, const xiiAbstractProperty* property, const xiiVariant& value, xiiVariant index = {}) override
    {
      if (property->GetPropertyName() == "Intensity")
        return xiiStatus("Injected light configuration failure.");
      return xiiObjectCommandAccessor::SetValue(object, property, value, index);
    }
  };

  class RejectBoxMeshAccessor : public xiiObjectCommandAccessor
  {
  public:
    using xiiObjectCommandAccessor::xiiObjectCommandAccessor;

    xiiStatus SetValue(const xiiDocumentObject* object, const xiiAbstractProperty* property, const xiiVariant& value, xiiVariant index = {}) override
    {
      if (property->GetPropertyName() == "Mesh")
        return xiiStatus("Injected box mesh assignment failure.");
      return xiiObjectCommandAccessor::SetValue(object, property, value, index);
    }
  };

  const xiiDocumentObject* FindComponent(const xiiDocumentObject* object, const xiiRTTI* type)
  {
    if (object->GetType() == type)
      return object;
    for (const auto* child : object->GetChildren())
    {
      if (auto* found = FindComponent(child, type))
        return found;
    }
    return nullptr;
  }

  void CheckScene(xiiDocument& document)
  {
    const auto* root     = document.GetObjectManager()->GetRootObject();
    auto*       accessor = document.GetObjectAccessor();
    XII_TEST_INT(root->GetChildren().GetCount(), 4);
    const auto* camera = FindComponent(root, xiiGetStaticRTTI<xiiCameraComponent>());
    const auto* mesh   = FindComponent(root, xiiGetStaticRTTI<xiiMeshComponent>());
    const auto* sky    = FindComponent(root, xiiGetStaticRTTI<xiiSkyAtmosphereComponent>());
    const auto* light  = FindComponent(root, xiiGetStaticRTTI<xiiDirectionalLightComponent>());
    if (!XII_TEST_BOOL(camera && mesh && sky && light))
      return;
    XII_TEST_INT(accessor->GetByName<xiiInt64>(camera, "UsageHint"), xiiCameraUsageHint::Thumbnail);
    XII_TEST_INT(accessor->GetByName<xiiInt32>(camera, "EditorShortcut"), 1);
    XII_TEST_STRING(accessor->GetByName<xiiString>(mesh, "Mesh"), "{ 618ee743-ed04-4fac-bf5f-572939db2f1d }");
    XII_TEST_VEC3(accessor->GetByName<xiiVec3>(mesh->GetParent(), "LocalPosition"), xiiVec3(3, 0, 0), 0);
    XII_TEST_INT(accessor->GetCountByName(sky->GetParent(), "Tags"), 1);
    XII_TEST_STRING(accessor->GetByName<xiiString>(sky->GetParent(), "Tags", 0), "SkyLight");
    const auto* atmosphere = accessor->GetObject(accessor->GetByName<xiiUuid>(sky, "Atmosphere"));
    if (XII_TEST_BOOL(atmosphere != nullptr))
    {
      const xiiAtmosphereSettings defaults;
      XII_TEST_FLOAT(accessor->GetByName<float>(atmosphere, "PlanetRadiusKm"), defaults.m_fPlanetRadiusKm, 0);
      XII_TEST_FLOAT(accessor->GetByName<float>(atmosphere, "AtmosphereRadiusKm"), defaults.m_fAtmosphereRadiusKm, 0);
      XII_TEST_VEC3(accessor->GetByName<xiiVec3>(atmosphere, "RayleighScattering"), defaults.m_vRayleighScattering, 0);
      XII_TEST_VEC3(accessor->GetByName<xiiVec3>(atmosphere, "PlanetUpDirection"), defaults.m_vPlanetUpDirection, 0);
      XII_TEST_INT(accessor->GetByName<xiiUInt32>(atmosphere, "SkyViewIntegrationSteps"), defaults.m_uiSkyViewIntegrationSteps);
    }
    XII_TEST_INT(accessor->GetByName<xiiInt64>(light, "IntensityUnit"), xiiPhotometricUnit::Lux);
    XII_TEST_FLOAT(accessor->GetByName<float>(light, "Intensity"), 10.0f, 0.0f);
  }
} // namespace

XII_CREATE_SIMPLE_TEST(EditorDocuments, SceneBoxCreation)
{
  const auto            bounds   = xiiBoundingBox::MakeFromMinMax(xiiVec3(-2, -1, 3), xiiVec3(4, 5, 7));
  const auto            material = xiiConversionUtils::ConvertStringToUuid("{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");
  SceneCreationDocument document;
  xiiUuid               object;
  XII_TEST_STATUS(xiiCreateSceneBox(*document.GetObjectAccessor(), bounds, material, object));
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native box dimensions, material and stable undo identity")
  {
    auto*       accessor = document.GetObjectAccessor();
    const auto* authored = accessor->GetObject(object);
    if (!XII_TEST_BOOL(authored != nullptr))
      return;
    const auto* mesh = FindComponent(authored, xiiGetStaticRTTI<xiiMeshComponent>());
    if (!XII_TEST_BOOL(mesh != nullptr))
      return;
    XII_TEST_VEC3(accessor->GetByName<xiiVec3>(authored, "LocalPosition"), xiiVec3(1, 2, 5), 0);
    XII_TEST_VEC3(accessor->GetByName<xiiVec3>(authored, "LocalScaling"), xiiVec3(6, 6, 4), 0);
    XII_TEST_INT(accessor->GetByName<xiiUInt32>(mesh, "SectionIndex"), 0);
    XII_TEST_INT(accessor->GetCountByName(mesh, "MaterialOverrides"), 1);
    XII_TEST_STRING(accessor->GetByName<xiiString>(mesh, "MaterialOverrides", 0), "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");
    XII_TEST_INT(document.GetCommandHistory()->GetUndoStackSize(), 1);
    XII_TEST_STATUS(document.GetCommandHistory()->Undo());
    XII_TEST_BOOL(accessor->GetObject(object) == nullptr);
    XII_TEST_STATUS(document.GetCommandHistory()->Redo());
    XII_TEST_BOOL(accessor->GetObject(object) != nullptr);

    XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "SceneBoxCreation", "SceneBoxBase", xiiDataDirUsage::ReadOnly));
    XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("SceneBoxCreation"));
    xiiUniquePtr<xiiAbstractObjectGraph> header, objects, types;
    XII_TEST_STATUS(xiiDocument::ReadDocument(":SceneBoxBase/Meshes/Box.xiiMeshAsset", header, objects, types));
    if (!XII_TEST_BOOL(header && objects))
      return;
    const auto* info = header->GetNodeByName("Header");
    if (!XII_TEST_BOOL(info != nullptr && info->FindProperty("DocumentID") != nullptr))
      return;
    const auto asset = xiiConversionUtils::ConvertStringToUuid(accessor->GetByName<xiiString>(mesh, "Mesh"));
    XII_TEST_BOOL(info->FindProperty("DocumentID")->m_Value.Get<xiiUuid>() == asset);
    bool found = false;
    for (auto it = objects->GetAllNodes().GetIterator(); it.IsValid(); ++it)
    {
      const auto* node = it.Value();
      if (node->GetType() != "xiiMeshAssetProperties")
        continue;
      found                 = true;
      const auto* primitive = node->FindProperty("PrimitiveType");
      const auto* scale     = node->FindProperty("UniformScaling");
      if (XII_TEST_BOOL(primitive && scale))
      {
        XII_TEST_STRING(primitive->m_Value.Get<xiiString>(), "xiiMeshPrimitive::Box");
        XII_TEST_FLOAT(scale->m_Value.Get<float>(), 1.0f, 0);
      }
    }
    XII_TEST_BOOL(found);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid dimensions and partial command failures preserve existing content")
  {
    xiiUuid failedObject = object;
    XII_TEST_BOOL(xiiCreateSceneBox(*document.GetObjectAccessor(), xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3::MakeZero(), xiiVec3::MakeZero()), {}, failedObject).Failed());
    XII_TEST_BOOL(!failedObject.IsValid());
    XII_TEST_INT(document.GetCommandHistory()->GetUndoStackSize(), 1);
    RejectBoxMeshAccessor rejected(document.GetCommandHistory());
    XII_TEST_BOOL(xiiCreateSceneBox(rejected, bounds, {}, failedObject).Failed());
    XII_TEST_BOOL(!failedObject.IsValid());
    XII_TEST_INT(document.GetObjectManager()->GetRootObject()->GetChildren().GetCount(), 1);
    XII_TEST_BOOL(document.GetObjectAccessor()->GetObject(object) != nullptr);
    XII_TEST_INT(document.GetCommandHistory()->GetUndoStackSize(), 1);
  }
}

XII_CREATE_SIMPLE_TEST(EditorDocuments, SceneCreation)
{
  SceneCreationDocument document;
  XII_TEST_STATUS(xiiCreateDefaultScene(*document.GetObjectAccessor()));

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Default document creation uses native content with the shipped data mounted")
  {
    xiiStringBuilder output = xiiTestFramework::GetInstance()->GetAbsOutputPath();
    output.AppendPath("NativeSceneCreation");
    XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(output));
    XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "SceneCreation", "SceneCreationBase", xiiDataDirUsage::ReadOnly));
    XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(output, "SceneCreation", ":", xiiDataDirUsage::AllowWrites));
    XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("SceneCreation"));
    output.AppendPath("Native.xiiScene");

    SceneCreationManager manager;
    xiiDocument*         created = nullptr;
    // Exercise the normal template-aware path, without EmptyDocument bypassing template lookup.
    XII_TEST_STATUS(manager.CreateDocument("Scene", output, created, xiiDocumentFlags::None));
    XII_TEST_INT(manager.m_uiTemplateClones, 0);
    if (created != nullptr)
    {
      CheckScene(*static_cast<SceneCreationDocument*>(created));
      const auto id = created->GetGuid();
      manager.CloseDocument(created);
      XII_TEST_STATUS(manager.OpenDocument("Scene", output, created, xiiDocumentFlags::None));
      if (created != nullptr)
      {
        XII_TEST_BOOL(created->GetGuid() == id);
        XII_TEST_INT(created->GetUnknownObjectTypeInstances(), 0);
        CheckScene(*static_cast<SceneCreationDocument*>(created));
        manager.CloseDocument(created);
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native component definitions and single-transaction undo / redo")
  {
    CheckScene(document);
    XII_TEST_INT(document.GetCommandHistory()->GetUndoStackSize(), 1);
    XII_TEST_STATUS(document.GetCommandHistory()->Undo());
    XII_TEST_INT(document.GetObjectManager()->GetRootObject()->GetChildren().GetCount(), 0);
    XII_TEST_STATUS(document.GetCommandHistory()->Redo());
    CheckScene(document);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Document graph serialization retains native types and settings")
  {
    xiiAbstractObjectGraph           graph;
    xiiDocumentObjectConverterWriter writer(&graph, document.GetObjectManager());
    writer.AddObjectToGraph(document.GetObjectManager()->GetRootObject(), "ObjectTree");
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         stream(&storage);
    xiiAbstractGraphBinarySerializer::Write(stream, &graph);
    xiiAbstractObjectGraph loadedGraph;
    xiiMemoryStreamReader  reader(&storage);
    xiiAbstractGraphBinarySerializer::Read(reader, &loadedGraph);
    SceneCreationDocument            loaded;
    xiiDocumentObjectConverterReader converter(&loadedGraph, loaded.GetObjectManager(), xiiDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
    const auto*                      tree = loadedGraph.GetNodeByName("ObjectTree");
    if (XII_TEST_BOOL(tree != nullptr))
    {
      converter.ApplyPropertiesToObject(tree, loaded.GetObjectManager()->GetRootObject());
      XII_TEST_INT(converter.GetNumUnknownObjectCreations(), 0);
      CheckScene(loaded);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Document mirroring creates the actual native World components")
  {
    xiiWorldDescription description("Default scene mirror");
    // Match the engine-process document's editable World configuration.
    description.m_bReportErrorWhenStaticObjectMoves = false;
    xiiWorld world(description);
    XII_LOCK(world.GetWriteMarker());
    xiiWorldRttiConverterContext context;
    context.m_pWorld = &world;
    xiiDocumentObjectMirror mirror;
    mirror.InitSender(document.GetObjectManager());
    mirror.InitReceiver(&context);
    XII_SCOPE_EXIT(mirror.Clear(); mirror.DeInit(); context.Clear());
    mirror.SendDocument();
    XII_TEST_INT(world.GetObjectCount(), 4);
    XII_TEST_BOOL(context.m_UnknownTypes.IsEmpty());
    const auto* root        = document.GetObjectManager()->GetRootObject();
    const auto* skyObject   = FindComponent(root, xiiGetStaticRTTI<xiiSkyAtmosphereComponent>());
    const auto* lightObject = FindComponent(root, xiiGetStaticRTTI<xiiDirectionalLightComponent>());
    auto        sky         = context.GetObjectByGUID(skyObject->GetGuid());
    auto        light       = context.GetObjectByGUID(lightObject->GetGuid());
    if (XII_TEST_BOOL(sky.m_pObject && light.m_pObject))
    {
      XII_TEST_BOOL(sky.m_pType == xiiGetStaticRTTI<xiiSkyAtmosphereComponent>());
      XII_TEST_BOOL(light.m_pType == xiiGetStaticRTTI<xiiDirectionalLightComponent>());
      const auto* nativeSky   = static_cast<xiiSkyAtmosphereComponent*>(sky.m_pObject);
      const auto* nativeLight = static_cast<xiiDirectionalLightComponent*>(light.m_pObject);
      XII_TEST_BOOL(nativeSky->GetOwner() != nullptr);
      XII_TEST_BOOL(nativeLight->GetOwner() != nullptr);
      XII_TEST_BOOL(nativeLight->GetIntensityUnit() == xiiPhotometricUnit::Lux);
      XII_TEST_FLOAT(nativeLight->GetIntensity(), 10.0f, 0.0f);
    }
    XII_TEST_STATUS(document.GetCommandHistory()->Undo());
    XII_TEST_INT(world.GetObjectCount(), 0);
    XII_TEST_STATUS(document.GetCommandHistory()->Redo());
    XII_TEST_INT(world.GetObjectCount(), 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Failure cancels all new content and preserves existing document objects")
  {
    SceneCreationDocument failedDocument;
    auto*                 accessor = failedDocument.GetObjectAccessor();
    accessor->StartTransaction("Existing content");
    xiiUuid existing;
    XII_TEST_STATUS(accessor->AddObjectByName(failedDocument.GetObjectManager()->GetRootObject(), "Children", -1, xiiGetStaticRTTI<xiiGameObject>(), existing));
    accessor->FinishTransaction();
    RejectLightIntensityAccessor rejecting(failedDocument.GetCommandHistory());
    const auto                   result = xiiCreateDefaultScene(rejecting);
    XII_TEST_BOOL(result.Failed());
    XII_TEST_STRING(result.GetMessageString(), "Injected light configuration failure.");
    XII_TEST_INT(failedDocument.GetObjectManager()->GetRootObject()->GetChildren().GetCount(), 1);
    XII_TEST_BOOL(accessor->GetObject(existing) != nullptr);
    XII_TEST_INT(failedDocument.GetCommandHistory()->GetUndoStackSize(), 1);
  }
}
