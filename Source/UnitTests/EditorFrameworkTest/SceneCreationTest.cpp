/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/WorldRttiConverterContext.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <SharedPluginScene/Scene/SceneCreation.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

namespace
{
  class SceneCreationDocument : public xiiDocument
  {
  public:
    SceneCreationDocument() : xiiDocument("SceneCreationTest.xiiScene", XII_DEFAULT_NEW(xiiDocumentObjectManager))
    {
      m_pDocumentInfo = CreateDocumentInfo();
    }

    xiiDocumentInfo* CreateDocumentInfo() override { return XII_DEFAULT_NEW(xiiDocumentInfo); }
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
    XII_TEST_INT(accessor->GetByName<xiiInt64>(light, "IntensityUnit"), xiiPhotometricUnit::Lux);
    XII_TEST_FLOAT(accessor->GetByName<float>(light, "Intensity"), 10.0f, 0.0f);
  }
} // namespace

XII_CREATE_SIMPLE_TEST(EditorDocuments, SceneCreation)
{
  SceneCreationDocument document;
  XII_TEST_STATUS(xiiCreateDefaultScene(*document.GetObjectAccessor()));

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
    xiiWorld            world(description);
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
