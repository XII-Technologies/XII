/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <EnginePluginAssets/RenderPipelineAsset/RenderPipelineContext.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/ViewRenderModePass.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

namespace
{
  class NativeVisibilityAuthorDocument : public xiiRenderPipelineAssetDocument
  {
  public:
    explicit NativeVisibilityAuthorDocument(xiiStringView path, xiiStringView name = "InstanceVisibility") : xiiRenderPipelineAssetDocument(path, xiiAssetDocEngineConnection::None)
    {
      m_pDocumentInfo               = CreateDocumentInfo();
      m_pDocumentInfo->m_DocumentID = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.", name));
      static xiiAssetDocumentTypeDescriptor descriptor;
      descriptor.m_sDocumentTypeName  = "RenderPipeline";
      descriptor.m_AssetDocumentFlags = xiiAssetDocumentFlags::DisableTransform;
      m_pTypeDescriptor               = &descriptor;
      static_cast<xiiAssetDocumentInfo*>(m_pDocumentInfo)->SetAssetsDocumentTypeName("RenderPipeline");
    }

  protected:
    xiiTaskGroupID InternalSaveDocument(AfterSaveCallback callback) override { return xiiDocument::InternalSaveDocument(callback); }
    void           InternalAfterSaveDocument() override {}
  };
  class NativeVisibilityAuthorContext : public xiiRenderPipelineContext
  {
  public:
    ~NativeVisibilityAuthorContext()
    {
      for (const auto& id : m_Objects)
        m_Context.DeleteObject(id);
    }
    xiiResult Import(const xiiDocument& document)
    {
      m_DocumentGuid = document.GetGuid();
      xiiAbstractObjectGraph           graph;
      xiiDocumentObjectConverterWriter writer(&graph, document.GetObjectManager());
      for (const auto* object : document.GetObjectManager()->GetRootObject()->GetChildren())
        writer.AddObjectToGraph(object);
      xiiRttiConverterReader reader(&graph, &m_Context);
      for (const auto* object : document.GetObjectManager()->GetRootObject()->GetChildren())
      {
        if (!reader.CreateObjectFromNode(graph.GetNode(object->GetGuid())))
          return XII_FAILURE;
        m_Objects.PushBack(object->GetGuid());
      }
      return XII_SUCCESS;
    }
    using xiiRenderPipelineContext::ExportDocument;

  private:
    xiiDynamicArray<xiiUuid> m_Objects;
  };


  xiiStatus AuthorInstanceVisibility(xiiStringView outputDirectory)
  {
    xiiStringBuilder path(outputDirectory, "/InstanceVisibility.xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path);
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author native world instance visibility");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", static_cast<xiiInt64>(xiiRenderPipelineDeviceFeature::Compute)));
    const auto add = [&](xiiStringView label, xiiStringView type, xiiVec2 position) -> const xiiDocumentObject* {
      auto id = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.InstanceVisibility/", label));
      if (accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName(type), id).Failed())
        return nullptr;
      xiiMoveNodeCommand move;
      move.m_Object = id;
      move.m_NewPos = position;
      if (document.GetCommandHistory()->AddCommand(move).Failed())
        return nullptr;
      return accessor->GetObject(id);
    };
    const auto* world    = add("World Instances", "xiiWorldInstancesUploadPass", {0, 0});
    const auto* geometry = add("Geometry Residency", "xiiGeometryUploadPass", {0, 300});
    const auto* view     = add("Culling View", "xiiVisibilityViewPass", {0, 600});
    const auto* counter  = add("Visible Count Reset", "xiiCounterResetPass", {600, 600});
    const auto* reset    = add("LOD Initialization", "xiiLodHistoryResetPass", {600, 0});
    const auto* cull     = add("Instance Culling", "xiiInstanceCullPass", {1200, 0});
    const auto* history  = add("Previous LOD Buffer", "xiiBufferHistoryPass", {1800, 600});
    if (!world || !geometry || !view || !counter || !reset || !cull || !history)
      return xiiStatus("Could not create native visibility nodes.");
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView output, const xiiDocumentObject* target, xiiStringView input) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, output);
      const auto* in  = manager->GetInputPinByName(target, input);
      if (!out || !in)
        return xiiStatus(xiiFmt("Missing pin '{} -> {}'.", output, input));
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };

    const auto* candidates = add("Input/Filtered Candidates", "xiiRenderPipelineInterfacePass", {-600, 600});
    if (!candidates)
      return xiiStatus("Could not create candidate interface.");
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(candidates, "Name", "InputCandidates"));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(candidates, "Input", true));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(candidates, "ResourceType", xiiUInt32(xiiRenderPipelinePin::Buffer)));
    XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(candidates));
    XII_SUCCEED_OR_RETURN(connect(candidates, "Value", view, "InputCandidates"));
    XII_SUCCEED_OR_RETURN(connect(world, "SceneInstances", reset, "SceneInstances"));
    XII_SUCCEED_OR_RETURN(connect(world, "SceneInstances", cull, "SceneInstances"));
    XII_SUCCEED_OR_RETURN(connect(geometry, "GeometryMetadata", cull, "GeometryMetadata"));
    XII_SUCCEED_OR_RETURN(connect(view, "VisibilityView", cull, "VisibilityView"));
    XII_SUCCEED_OR_RETURN(connect(view, "Candidates", cull, "Candidates"));
    XII_SUCCEED_OR_RETURN(connect(counter, "Counter", cull, "Counter"));
    XII_SUCCEED_OR_RETURN(connect(reset, "LodHistory", cull, "InitializedLodHistory"));
    XII_SUCCEED_OR_RETURN(connect(cull, "LodHistory", history, "Buffer"));
    XII_SUCCEED_OR_RETURN(connect(history, "PreviousBuffer", cull, "PreviousLodHistory"));
    xiiUInt32  row    = 0;
    const auto expose = [&](const xiiDocumentObject* source, xiiStringView name) -> xiiStatus {
      const auto* boundary = add(xiiStringBuilder("Output/", name), "xiiRenderPipelineInterfacePass", {2400, float(row++ * 150)});
      if (!boundary)
        return xiiStatus("Could not create a visibility interface.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Name", name));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Input", false));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "ResourceType", xiiUInt32(xiiRenderPipelinePin::Buffer)));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(boundary));
      return connect(source, name, boundary, "Value");
    };
    XII_SUCCEED_OR_RETURN(expose(world, "SceneInstances"));
    XII_SUCCEED_OR_RETURN(expose(world, "SkinningMatrices"));
    XII_SUCCEED_OR_RETURN(expose(geometry, "GeometryMetadata"));
    XII_SUCCEED_OR_RETURN(expose(geometry, "MeshletMetadata"));
    XII_SUCCEED_OR_RETURN(expose(view, "VisibilityView"));
    XII_SUCCEED_OR_RETURN(expose(cull, "VisibleInstances"));
    XII_SUCCEED_OR_RETURN(expose(cull, "VisibleInstanceCount"));
    XII_SUCCEED_OR_RETURN(expose(cull, "LodHistory"));
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import visibility asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/InstanceVisibility.xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Authored visibility did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 7);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), 9);
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements() == xiiRenderExtractionCategory::Geometry);
    XII_TEST_BOOL(loaded->GetPipeline()->GetRuntimeFeatures() == xiiRenderPipelineFeature::GpuScene);
    return XII_SUCCESS;
  }

  xiiStatus AuthorMeshletVisibility(xiiStringView outputDirectory)
  {
    const xiiStringBuilder library(outputDirectory, "/RenderPipelines");
    XII_SUCCEED_OR_RETURN(xiiOSFile::CreateDirectoryStructure(library));
    XII_SUCCEED_OR_RETURN(AuthorInstanceVisibility(library));
    xiiStringBuilder path(outputDirectory, "/MeshletVisibility.xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path, "MeshletVisibility");
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author native meshlet visibility");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", static_cast<xiiInt64>(xiiRenderPipelineDeviceFeature::Compute)));
    const auto add = [&](xiiStringView label, xiiStringView type, xiiVec2 position) -> const xiiDocumentObject* {
      auto id = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.MeshletVisibility/", label));
      if (accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName(type), id).Failed())
        return nullptr;
      xiiMoveNodeCommand move;
      move.m_Object = id;
      move.m_NewPos = position;
      if (document.GetCommandHistory()->AddCommand(move).Failed())
        return nullptr;
      return accessor->GetObject(id);
    };
    const auto* instances = add("Instance Visibility", "xiiRenderPipelineSubgraphPass", {0, 0});
    const auto* maximum   = add("Maximum Reset", "xiiCounterResetPass", {600, 600});
    const auto* bounds    = add("Dispatch Bounds", "xiiMeshletDispatchBoundsPass", {1200, 600});
    const auto* dispatch  = add("Indirect Dispatch", "xiiMeshletDispatchPass", {1800, 600});
    const auto* count     = add("Meshlet Count Reset", "xiiCounterResetPass", {1800, 1000});
    const auto* overflow  = add("Overflow Reset", "xiiCounterResetPass", {1800, 1400});
    const auto* cull      = add("Meshlet Culling", "xiiMeshletCullPass", {2400, 0});
    if (!instances || !maximum || !bounds || !dispatch || !count || !overflow || !cull)
      return xiiStatus("Could not create native meshlet visibility nodes.");
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(instances, "Resource", "RenderPipelines/InstanceVisibility.xiiBinRenderPipeline"));
    XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(instances));
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView output, const xiiDocumentObject* target, xiiStringView input) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, output);
      const auto* in  = manager->GetInputPinByName(target, input);
      if (!out || !in)
        return xiiStatus(xiiFmt("Missing pin '{} -> {}'.", output, input));
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };

    const auto* candidates = add("Input/Filtered Candidates", "xiiRenderPipelineInterfacePass", {-600, 600});
    if (!candidates)
      return xiiStatus("Could not create candidate interface.");
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(candidates, "Name", "InputCandidates"));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(candidates, "Input", true));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(candidates, "ResourceType", xiiUInt32(xiiRenderPipelinePin::Buffer)));
    XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(candidates));
    XII_SUCCEED_OR_RETURN(connect(candidates, "Value", instances, "InputCandidates"));
    for (auto pin : {"SceneInstances", "GeometryMetadata", "MeshletMetadata", "VisibilityView", "VisibleInstances", "VisibleInstanceCount", "LodHistory"})
    {
      XII_SUCCEED_OR_RETURN(connect(instances, pin, bounds, pin));
      XII_SUCCEED_OR_RETURN(connect(instances, pin, cull, pin));
    }
    XII_SUCCEED_OR_RETURN(connect(maximum, "Counter", bounds, "InitializedMaximum"));
    XII_SUCCEED_OR_RETURN(connect(bounds, "MaximumMeshletGroups", dispatch, "MaximumMeshletGroups"));
    XII_SUCCEED_OR_RETURN(connect(instances, "MeshletMetadata", dispatch, "MeshletMetadata"));
    XII_SUCCEED_OR_RETURN(connect(instances, "VisibleInstances", dispatch, "VisibleInstances"));
    XII_SUCCEED_OR_RETURN(connect(instances, "VisibleInstanceCount", dispatch, "VisibleInstanceCount"));
    XII_SUCCEED_OR_RETURN(connect(dispatch, "DispatchArguments", cull, "DispatchArguments"));
    XII_SUCCEED_OR_RETURN(connect(count, "Counter", cull, "InitialCount"));
    XII_SUCCEED_OR_RETURN(connect(overflow, "Counter", cull, "InitialOverflow"));
    xiiUInt32  row    = 0;
    const auto expose = [&](const xiiDocumentObject* source, xiiStringView name) -> xiiStatus {
      const auto* boundary = add(xiiStringBuilder("Output/", name), "xiiRenderPipelineInterfacePass", {3000, float(row++ * 150)});
      if (!boundary)
        return xiiStatus("Could not create a visibility interface.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Name", name));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Input", false));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "ResourceType", xiiUInt32(xiiRenderPipelinePin::Buffer)));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(boundary));
      return connect(source, name, boundary, "Value");
    };
    for (auto pin : {"SceneInstances", "SkinningMatrices", "GeometryMetadata", "MeshletMetadata", "VisibilityView", "VisibleInstances", "VisibleInstanceCount", "LodHistory"})
      XII_SUCCEED_OR_RETURN(expose(instances, pin));
    XII_SUCCEED_OR_RETURN(expose(dispatch, "DispatchArguments"));
    XII_SUCCEED_OR_RETURN(expose(cull, "VisibleMeshlets"));
    XII_SUCCEED_OR_RETURN(expose(cull, "VisibleMeshletCount"));
    XII_SUCCEED_OR_RETURN(expose(cull, "VisibleMeshletOverflow"));
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import visibility asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/MeshletVisibility.xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Authored visibility did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 7);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), 13);
    XII_TEST_INT(loaded->GetPipeline()->GetDescriptor().m_Passes.GetCount(), 13);
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements() == xiiRenderExtractionCategory::Geometry);
    XII_TEST_BOOL(loaded->GetPipeline()->GetRuntimeFeatures() == xiiRenderPipelineFeature::GpuScene);
    return XII_SUCCESS;
  }

  xiiStatus AuthorVertexPullDraws(xiiStringView outputDirectory)
  {
    const xiiStringBuilder library(outputDirectory, "/RenderPipelines");
    XII_SUCCEED_OR_RETURN(xiiOSFile::CreateDirectoryStructure(library));
    XII_SUCCEED_OR_RETURN(AuthorInstanceVisibility(library));
    XII_SUCCEED_OR_RETURN(AuthorMeshletVisibility(library));
    xiiStringBuilder path(outputDirectory, "/VertexPullDraws.xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path, "VertexPullDraws");
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author native vertex-pull draws");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", static_cast<xiiInt64>(xiiRenderPipelineDeviceFeature::Compute)));
    const auto add = [&](xiiStringView label, xiiStringView type, xiiVec2 position) -> const xiiDocumentObject* {
      auto id = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.VertexPullDraws/", label));
      if (accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName(type), id).Failed())
        return nullptr;
      xiiMoveNodeCommand move;
      move.m_Object = id;
      move.m_NewPos = position;
      if (document.GetCommandHistory()->AddCommand(move).Failed())
        return nullptr;
      return accessor->GetObject(id);
    };
    const auto* instances = add("Meshlet Visibility", "xiiRenderPipelineSubgraphPass", {0, 0});
    const auto* draw      = add("Vertex-pull Draw Commands", "xiiVertexPullDrawCommandsPass", {600, 0});
    if (!instances || !draw)
      return xiiStatus("Could not create native vertex-pull nodes.");
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(instances, "Resource", "RenderPipelines/MeshletVisibility.xiiBinRenderPipeline"));
    XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(instances));
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView output, const xiiDocumentObject* target, xiiStringView input) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, output);
      const auto* in  = manager->GetInputPinByName(target, input);
      if (!out || !in)
        return xiiStatus(xiiFmt("Missing pin '{} -> {}'.", output, input));
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };

    const auto* candidates = add("Input/Filtered Candidates", "xiiRenderPipelineInterfacePass", {-600, 600});
    if (!candidates)
      return xiiStatus("Could not create candidate interface.");
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(candidates, "Name", "InputCandidates"));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(candidates, "Input", true));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(candidates, "ResourceType", xiiUInt32(xiiRenderPipelinePin::Buffer)));
    XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(candidates));
    XII_SUCCEED_OR_RETURN(connect(candidates, "Value", instances, "InputCandidates"));
    for (auto pin : {"SceneInstances", "VisibilityView", "GeometryMetadata", "MeshletMetadata", "VisibleMeshlets", "VisibleMeshletCount"})
      XII_SUCCEED_OR_RETURN(connect(instances, pin, draw, pin));
    xiiUInt32  row    = 0;
    const auto expose = [&](const xiiDocumentObject* source, xiiStringView name) -> xiiStatus {
      const auto* boundary = add(xiiStringBuilder("Output/", name), "xiiRenderPipelineInterfacePass", {3000, float(row++ * 150)});
      if (!boundary)
        return xiiStatus("Could not create a visibility interface.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Name", name));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Input", false));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "ResourceType", xiiUInt32(xiiRenderPipelinePin::Buffer)));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(boundary));
      return connect(source, name, boundary, "Value");
    };
    for (auto pin : {"SceneInstances", "SkinningMatrices", "GeometryMetadata", "MeshletMetadata", "VisibilityView", "VisibleInstances", "VisibleInstanceCount", "LodHistory", "DispatchArguments", "VisibleMeshlets", "VisibleMeshletCount", "VisibleMeshletOverflow"})
      XII_SUCCEED_OR_RETURN(expose(instances, pin));
    XII_SUCCEED_OR_RETURN(expose(draw, "DrawCommands"));
    XII_SUCCEED_OR_RETURN(expose(draw, "DrawCommandCount"));
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import visibility asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/VertexPullDraws.xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Authored visibility did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 2);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), 15);
    XII_TEST_INT(loaded->GetPipeline()->GetDescriptor().m_Passes.GetCount(), 14);
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements() == xiiRenderExtractionCategory::Geometry);
    XII_TEST_BOOL(loaded->GetPipeline()->GetRuntimeFeatures() == xiiRenderPipelineFeature::GpuScene);
    return XII_SUCCESS;
  }

  xiiStatus AuthorSurfaceQuery(xiiStringView outputDirectory)
  {
    const xiiStringBuilder library(outputDirectory, "/RenderPipelines");
    XII_SUCCEED_OR_RETURN(xiiOSFile::CreateDirectoryStructure(library));
    XII_SUCCEED_OR_RETURN(AuthorInstanceVisibility(library));
    XII_SUCCEED_OR_RETURN(AuthorMeshletVisibility(library));
    XII_SUCCEED_OR_RETURN(AuthorVertexPullDraws(library));
    xiiStringBuilder path(outputDirectory, "/SurfaceQuery.xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path, "SurfaceQuery");
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author native surface raster graph");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", static_cast<xiiInt64>((xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::BindlessResources).GetValue())));
    const auto add = [&](xiiStringView label, xiiStringView type, xiiVec2 position) -> const xiiDocumentObject* {
      auto id = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.SurfaceQuery/", label));
      if (accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName(type), id).Failed())
        return nullptr;
      xiiMoveNodeCommand move;
      move.m_Object = id;
      move.m_NewPos = position;
      if (document.GetCommandHistory()->AddCommand(move).Failed())
        return nullptr;
      return accessor->GetObject(id);
    };
    const auto* instances = add("Vertex-pull Draws", "xiiRenderPipelineSubgraphPass", {0, 0});
    const auto* materials = add("Material Residency", "xiiMaterialUploadPass", {0, 1200});
    const auto* constants = add("Raster Constants", "xiiSurfaceRasterConstantsPass", {600, 1200});
    const auto* surface   = add("Surface Raster", "xiiSurfaceQueryPass", {1200, 0});
    if (!instances || !materials || !constants || !surface)
      return xiiStatus("Could not create native surface nodes.");
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(instances, "Resource", "RenderPipelines/VertexPullDraws.xiiBinRenderPipeline"));
    XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(instances));
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView output, const xiiDocumentObject* target, xiiStringView input) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, output);
      const auto* in  = manager->GetInputPinByName(target, input);
      if (!out || !in)
        return xiiStatus(xiiFmt("Missing pin '{} -> {}'.", output, input));
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };

    const auto* candidates = add("Input/Filtered Candidates", "xiiRenderPipelineInterfacePass", {-600, 600});
    if (!candidates)
      return xiiStatus("Could not create candidate interface.");
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(candidates, "Name", "InputCandidates"));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(candidates, "Input", true));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(candidates, "ResourceType", xiiUInt32(xiiRenderPipelinePin::Buffer)));
    XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(candidates));
    XII_SUCCEED_OR_RETURN(connect(candidates, "Value", instances, "InputCandidates"));
    for (auto pin : {"SceneInstances", "SkinningMatrices", "GeometryMetadata", "MeshletMetadata", "VisibleMeshlets", "VisibleMeshletCount", "DrawCommands", "DrawCommandCount"})
      XII_SUCCEED_OR_RETURN(connect(instances, pin, surface, pin));
    XII_SUCCEED_OR_RETURN(connect(materials, "SurfaceMaterials", surface, "SurfaceMaterials"));
    XII_SUCCEED_OR_RETURN(connect(constants, "RasterConstants", surface, "RasterConstants"));
    xiiUInt32  row    = 0;
    const auto expose = [&](const xiiDocumentObject* source, xiiStringView name, xiiUInt32 resourceType = xiiRenderPipelinePin::Buffer) -> xiiStatus {
      const auto* boundary = add(xiiStringBuilder("Output/", name), "xiiRenderPipelineInterfacePass", {3000, float(row++ * 150)});
      if (!boundary)
        return xiiStatus("Could not create a visibility interface.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Name", name));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Input", false));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "ResourceType", resourceType));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(boundary));
      return connect(source, name, boundary, "Value");
    };
    for (auto pin : {"SceneInstances", "SkinningMatrices", "GeometryMetadata", "MeshletMetadata", "VisibilityView", "VisibleInstances", "VisibleInstanceCount", "LodHistory", "DispatchArguments", "VisibleMeshlets", "VisibleMeshletCount", "VisibleMeshletOverflow", "DrawCommands", "DrawCommandCount"})
      XII_SUCCEED_OR_RETURN(expose(instances, pin));
    XII_SUCCEED_OR_RETURN(expose(surface, "Surface", xiiRenderPipelinePin::Texture));
    XII_SUCCEED_OR_RETURN(expose(surface, "SceneDepth", xiiRenderPipelinePin::Texture));
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import visibility asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/SurfaceQuery.xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Authored visibility did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 4);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), 17);
    XII_TEST_INT(loaded->GetPipeline()->GetDescriptor().m_Passes.GetCount(), 17);
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements() == xiiRenderExtractionCategory::Geometry);
    XII_TEST_BOOL(loaded->GetPipeline()->GetRuntimeFeatures() == xiiRenderPipelineFeature::GpuScene);
    return XII_SUCCESS;
  }

  xiiStatus AuthorDebugVisualization(xiiStringView outputDirectory)
  {
    xiiStringBuilder path(outputDirectory, "/DebugVisualization.xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path, "DebugVisualization");
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author explicit debug composition");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", static_cast<xiiInt64>(xiiRenderPipelineDeviceFeature::Compute)));
    const auto add = [&](xiiStringView label, xiiStringView type, xiiVec2 position) -> const xiiDocumentObject* {
      auto id = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.DebugVisualization/", label));
      if (accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName(type), id).Failed())
        return nullptr;
      xiiMoveNodeCommand move;
      move.m_Object = id;
      move.m_NewPos = position;
      if (document.GetCommandHistory()->AddCommand(move).Failed())
        return nullptr;
      return accessor->GetObject(id);
    };
    const auto* mode    = add("View Render Mode", "xiiViewRenderModePass", {600, 0});
    const auto* overlay = add("Debug Overlay", "xiiDebugOverlayPass", {1200, 0});
    if (!mode || !overlay)
      return xiiStatus("Could not create debug nodes.");
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(mode, "UseViewMode", true));
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView output, const xiiDocumentObject* target, xiiStringView input) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, output);
      const auto* in  = manager->GetInputPinByName(target, input);
      if (!out || !in)
        return xiiStatus(xiiFmt("Missing pin '{} -> {}'.", output, input));
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };
    xiiViewRenderModePass prototype;
    prototype.m_bUseViewMode = true;
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    prototype.GetPins(pins);
    xiiUInt32 row = 0;
    for (const auto& pin : pins)
    {
      if (!pin.m_bInput || pin.m_Type == xiiRenderPipelinePin::Order)
        continue;
      const auto* boundary = add(xiiStringBuilder("Input/", pin.m_sName), "xiiRenderPipelineInterfacePass", {0, float(row++ * 150)});
      if (!boundary)
        return xiiStatus("Could not create debug interface.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Name", pin.m_sName));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Input", true));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "ResourceType", xiiUInt32(pin.m_Type)));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(boundary));
      XII_SUCCEED_OR_RETURN(connect(boundary, "Value", mode, pin.m_sName));
      if (pin.m_sName == "SceneDepth")
        XII_SUCCEED_OR_RETURN(connect(boundary, "Value", overlay, "SceneDepth"));
    }
    XII_SUCCEED_OR_RETURN(connect(mode, "DisplayLinearColor", overlay, "DisplayLinearColor"));
    for (auto name : {"DisplayLinearColor", "SceneDepth"})
    {
      const auto* boundary = add(xiiStringBuilder("Output/", name), "xiiRenderPipelineInterfacePass", {1800, 0});
      if (!boundary)
        return xiiStatus("Could not create debug output.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Name", name));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Input", false));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(boundary));
      XII_SUCCEED_OR_RETURN(connect(overlay, name, boundary, "Value"));
    }
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import visibility asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/DebugVisualization.xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Authored visibility did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 2);

    XII_TEST_INT(loaded->GetPipeline()->GetDescriptor().m_Passes.GetCount(), 2);
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements() == xiiRenderExtractionCategory::Custom);
    XII_TEST_BOOL(loaded->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
    return XII_SUCCESS;
  }
  xiiStatus AuthorEditorMeshPicking(xiiStringView outputDirectory)
  {
    const xiiStringBuilder library(outputDirectory, "/RenderPipelines");
    XII_SUCCEED_OR_RETURN(xiiOSFile::CreateDirectoryStructure(library));
    XII_SUCCEED_OR_RETURN(AuthorInstanceVisibility(library));
    XII_SUCCEED_OR_RETURN(AuthorMeshletVisibility(library));
    XII_SUCCEED_OR_RETURN(AuthorVertexPullDraws(library));
    XII_SUCCEED_OR_RETURN(AuthorSurfaceQuery(library));
    xiiStringBuilder path(outputDirectory, "/EditorMeshPicking.xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path, "EditorMeshPicking");
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author explicit editor mesh picking");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", static_cast<xiiInt64>((xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::BindlessResources).GetValue())));
    const auto add = [&](xiiStringView label, xiiStringView type, xiiVec2 position) -> const xiiDocumentObject* {
      auto id = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.EditorMeshPicking/", label));
      if (accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName(type), id).Failed())
        return nullptr;
      xiiMoveNodeCommand move;
      move.m_Object = id;
      move.m_NewPos = position;
      if (document.GetCommandHistory()->AddCommand(move).Failed())
        return nullptr;
      return accessor->GetObject(id);
    };
    const auto* instances = add("Authored Surface", "xiiRenderPipelineSubgraphPass", {600, 0});
    const auto* selection = add("World Candidate Selection", "xiiSceneInstanceSelectionPass", {0, 0});
    const auto* capture   = add("Surface Capture", "xiiSceneMeshPickingPass", {1200, 0});
    if (!instances || !selection || !capture)
      return xiiStatus("Could not create editor picking nodes.");
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(instances, "Resource", "RenderPipelines/SurfaceQuery.xiiBinRenderPipeline"));
    XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(instances));
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView output, const xiiDocumentObject* target, xiiStringView input) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, output);
      const auto* in  = manager->GetInputPinByName(target, input);
      if (!out || !in)
        return xiiStatus(xiiFmt("Missing pin '{} -> {}'.", output, input));
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };

    XII_SUCCEED_OR_RETURN(connect(selection, "Candidates", instances, "InputCandidates"));
    XII_SUCCEED_OR_RETURN(connect(instances, "Surface", capture, "Surface"));
    xiiUInt32  row    = 0;
    const auto expose = [&](const xiiDocumentObject* source, xiiStringView name, xiiUInt32 resourceType = xiiRenderPipelinePin::Buffer) -> xiiStatus {
      const auto* boundary = add(xiiStringBuilder("Output/", name), "xiiRenderPipelineInterfacePass", {3000, float(row++ * 150)});
      if (!boundary)
        return xiiStatus("Could not create a visibility interface.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Name", name));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Input", false));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "ResourceType", resourceType));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(boundary));
      return connect(source, name, boundary, "Value");
    };
    XII_SUCCEED_OR_RETURN(expose(capture, "Surface", xiiRenderPipelinePin::Texture));
    XII_SUCCEED_OR_RETURN(expose(instances, "SceneDepth", xiiRenderPipelinePin::Texture));
    XII_SUCCEED_OR_RETURN(expose(instances, "VisibleMeshletOverflow"));
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import visibility asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/EditorMeshPicking.xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Authored visibility did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 3);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), 3);
    XII_TEST_INT(loaded->GetPipeline()->GetDescriptor().m_Passes.GetCount(), 19);
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements() == xiiRenderExtractionCategory::Geometry);
    XII_TEST_BOOL(loaded->GetPipeline()->GetRuntimeFeatures() == xiiRenderPipelineFeature::GpuScene);
    return XII_SUCCESS;
  }

} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(NativeAssetAuthoring);
XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, InstanceVisibility)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeVisibilityAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeVisibilityAuthoring"));
  XII_TEST_STATUS(AuthorInstanceVisibility(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, MeshletVisibility)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "NativeMeshletAuthoring", "NativeMeshletBase", xiiDataDirUsage::ReadOnly)) ||
      !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeMeshletAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeMeshletAuthoring"));
  XII_TEST_STATUS(AuthorMeshletVisibility(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, VertexPullDraws)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "NativeDrawAuthoring", "NativeDrawBase", xiiDataDirUsage::ReadOnly)) ||
      !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeDrawAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeDrawAuthoring"));
  XII_TEST_STATUS(AuthorVertexPullDraws(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, SurfaceQuery)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "NativeSurfaceAuthoring", "NativeSurfaceBase", xiiDataDirUsage::ReadOnly)) ||
      !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeSurfaceAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeSurfaceAuthoring"));
  XII_TEST_STATUS(AuthorSurfaceQuery(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, DebugVisualization)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeDebugAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeDebugAuthoring"));
  XII_TEST_STATUS(AuthorDebugVisualization(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, EditorMeshPicking)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "NativeEditorPickingAuthoring", "NativeEditorPickingBase", xiiDataDirUsage::ReadOnly)) ||
      !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeEditorPickingAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeEditorPickingAuthoring"));
  XII_TEST_STATUS(AuthorEditorMeshPicking(path));
}
