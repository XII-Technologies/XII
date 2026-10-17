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
#include <GraphicsCore/Pipeline/Passes/Geometry/DepthRasterPass.h>
#include <GraphicsCore/Pipeline/Passes/Streaming/GeometryLodFeedbackPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/InstanceOcclusionPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/MeshletOcclusionPass.h>
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


  xiiStatus AuthorGeometryLodFeedback(xiiStringView outputDirectory);
  xiiStatus AuthorInstanceVisibility(xiiStringView outputDirectory, bool matrix = false)
  {
    const xiiStringView name = matrix ? "MatrixInstanceVisibility" : "InstanceVisibility";
    xiiStringBuilder    path(outputDirectory, "/", name, ".xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path, name);
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author native world instance visibility");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", static_cast<xiiInt64>(xiiRenderPipelineDeviceFeature::Compute)));
    const auto add = [&](xiiStringView label, xiiStringView type, xiiVec2 position) -> const xiiDocumentObject* {
      auto id = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.", name, "/", label));
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
    const auto* view     = add("Culling View", matrix ? "xiiMatrixVisibilityViewPass" : "xiiVisibilityViewPass", {0, 600});
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
    if (matrix)
    {
      XII_SUCCEED_OR_RETURN(expose(view, "Candidates"));
    }
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import visibility asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/", name, ".xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Authored visibility did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 7);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), matrix ? 10 : 9);
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements() == xiiRenderExtractionCategory::Geometry);
    XII_TEST_BOOL(loaded->GetPipeline()->GetRuntimeFeatures() == xiiRenderPipelineFeature::GpuScene);
    return XII_SUCCESS;
  }

  xiiStatus AuthorMeshletVisibility(xiiStringView outputDirectory, bool matrix = false)
  {
    const xiiStringBuilder library(outputDirectory, "/RenderPipelines");
    XII_SUCCEED_OR_RETURN(xiiOSFile::CreateDirectoryStructure(library));
    XII_SUCCEED_OR_RETURN(AuthorInstanceVisibility(library, matrix));
    const xiiStringView name = matrix ? "MatrixMeshletVisibility" : "MeshletVisibility";
    xiiStringBuilder    path(outputDirectory, "/", name, ".xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path, name);
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author native meshlet visibility");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", static_cast<xiiInt64>(xiiRenderPipelineDeviceFeature::Compute)));
    const auto add = [&](xiiStringView label, xiiStringView type, xiiVec2 position) -> const xiiDocumentObject* {
      auto id = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.", name, "/", label));
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
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(instances, "Resource", matrix ? "RenderPipelines/MatrixInstanceVisibility.xiiBinRenderPipeline" : "RenderPipelines/InstanceVisibility.xiiBinRenderPipeline"));
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
    if (matrix)
    {
      XII_SUCCEED_OR_RETURN(expose(instances, "Candidates"));
    }
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import visibility asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/", name, ".xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Authored visibility did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 7);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), matrix ? 14 : 13);
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

  xiiStatus AuthorWorldMeshDraws(xiiStringView outputDirectory)
  {
    const xiiStringBuilder library(outputDirectory, "/RenderPipelines");
    XII_SUCCEED_OR_RETURN(xiiOSFile::CreateDirectoryStructure(library));
    XII_SUCCEED_OR_RETURN(AuthorInstanceVisibility(library, true));
    XII_SUCCEED_OR_RETURN(AuthorMeshletVisibility(library, true));
    XII_SUCCEED_OR_RETURN(AuthorGeometryLodFeedback(library));
    xiiStringBuilder path(outputDirectory, "/WorldMeshDraws.xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path, "WorldMeshDraws");
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author world mesh draws");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", static_cast<xiiInt64>((xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::MeshShaders).GetValue())));
    const auto add = [&](xiiStringView label, xiiStringView type, xiiVec2 position) -> const xiiDocumentObject* {
      auto id = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.WorldMeshDraws/", label));
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
    const auto* draw      = add("Mesh Draw Commands", "xiiMeshDrawCommandsPass", {600, 0});
    const auto* feedback  = add("LOD Demand", "xiiRenderPipelineSubgraphPass", {600, 600});
    if (!instances || !draw || !feedback)
      return xiiStatus("Could not create world mesh draw nodes.");
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(instances, "Resource", "RenderPipelines/MatrixMeshletVisibility.xiiBinRenderPipeline"));
    XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(instances));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(feedback, "Resource", "RenderPipelines/GeometryLodFeedback.xiiBinRenderPipeline"));
    XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(feedback));
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
    for (auto pin : {"VisibleMeshlets", "VisibleMeshletCount"})
      XII_SUCCEED_OR_RETURN(connect(instances, pin, draw, pin));
    for (auto pin : {"SceneInstances", "VisibilityView", "GeometryMetadata", "Candidates"})
    {
      XII_SUCCEED_OR_RETURN(connect(instances, pin, feedback, pin));
    }
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
    XII_SUCCEED_OR_RETURN(expose(instances, "Candidates"));
    XII_SUCCEED_OR_RETURN(expose(draw, "DrawCommands"));
    XII_SUCCEED_OR_RETURN(expose(draw, "DrawCommandCount"));
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import visibility asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/WorldMeshDraws.xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Authored visibility did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 3);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), 16);
    XII_TEST_INT(loaded->GetPipeline()->GetDescriptor().m_Passes.GetCount(), 17);
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

  xiiStatus AuthorDepthOnly(xiiStringView outputDirectory, bool materialTargets = false)
  {
    const xiiStringBuilder library(outputDirectory, "/RenderPipelines");
    XII_SUCCEED_OR_RETURN(xiiOSFile::CreateDirectoryStructure(library));
    XII_SUCCEED_OR_RETURN(AuthorInstanceVisibility(library));
    XII_SUCCEED_OR_RETURN(AuthorMeshletVisibility(library));
    XII_SUCCEED_OR_RETURN(AuthorVertexPullDraws(library));
    const xiiStringView assetName = materialTargets ? "GBuffer" : "DepthOnly";
    xiiStringBuilder    path(outputDirectory, "/", assetName, ".xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path, assetName);
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author independent depth raster graph");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", static_cast<xiiInt64>((xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::BindlessResources).GetValue())));
    const auto add = [&](xiiStringView label, xiiStringView type, xiiVec2 position) -> const xiiDocumentObject* {
      auto id = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.", assetName, "/", label));
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
    const auto* surface   = add("Depth Raster", "xiiDepthRasterPass", {1200, 0});
    const auto* target    = add("Depth Target", "xiiDepthTargetPass", {600, 1500});
    if (!instances || !materials || !constants || !surface || !target)
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
    XII_SUCCEED_OR_RETURN(connect(target, "DepthTarget", surface, "DepthTarget"));
    const xiiDocumentObject* gbuffer = nullptr;
    if (materialTargets)
    {
      gbuffer = add("Material Raster", "xiiGBufferRasterPass", {1800, 0});
      if (!gbuffer)
        return xiiStatus("Could not create native material raster node.");
      for (auto pin : {"SceneInstances", "SkinningMatrices", "GeometryMetadata", "MeshletMetadata", "VisibleMeshlets", "VisibleMeshletCount", "DrawCommands", "DrawCommandCount"})
        XII_SUCCEED_OR_RETURN(connect(instances, pin, gbuffer, pin));
      XII_SUCCEED_OR_RETURN(connect(materials, "SurfaceMaterials", gbuffer, "SurfaceMaterials"));
      XII_SUCCEED_OR_RETURN(connect(constants, "RasterConstants", gbuffer, "RasterConstants"));
      XII_SUCCEED_OR_RETURN(connect(surface, "SceneDepth", gbuffer, "SceneDepth"));
    }
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
    XII_SUCCEED_OR_RETURN(expose(surface, "SceneDepth", xiiRenderPipelinePin::Texture));
    if (gbuffer)
      for (auto pin : {"Albedo", "Normal", "Material", "Emissive", "NormalRoughness", "Velocity", "ObjectId"})
        XII_SUCCEED_OR_RETURN(expose(gbuffer, pin, xiiRenderPipelinePin::Texture));
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import visibility asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/", assetName, ".xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Authored visibility did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), materialTargets ? 6 : 5);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), materialTargets ? 23 : 16);
    XII_TEST_INT(loaded->GetPipeline()->GetDescriptor().m_Passes.GetCount(), materialTargets ? 19 : 18);
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements() == xiiRenderExtractionCategory::Geometry);
    XII_TEST_BOOL(loaded->GetPipeline()->GetRuntimeFeatures() == xiiRenderPipelineFeature::GpuScene);
    return XII_SUCCESS;
  }

  xiiStatus AuthorDepthPyramid(xiiStringView outputDirectory)
  {
    xiiStringBuilder path(outputDirectory, "/DepthPyramid.xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path, "DepthPyramid");
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author separate depth reduction and texture history");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", static_cast<xiiInt64>(xiiRenderPipelineDeviceFeature::Compute)));
    const auto add = [&](xiiStringView label, xiiStringView type, xiiVec2 position) -> const xiiDocumentObject* {
      auto id = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.DepthPyramid/", label));
      if (accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName(type), id).Failed())
        return nullptr;
      xiiMoveNodeCommand move;
      move.m_Object = id;
      move.m_NewPos = position;
      if (document.GetCommandHistory()->AddCommand(move).Failed())
        return nullptr;
      return accessor->GetObject(id);
    };
    const auto* reduce  = add("Depth Reduction", "xiiDepthPyramidPass", {600, 0});
    const auto* history = add("Previous Depth", "xiiTextureHistoryPass", {1200, 0});
    if (!reduce || !history)
      return xiiStatus("Could not create native depth/history nodes.");
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView output, const xiiDocumentObject* target, xiiStringView input) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, output);
      const auto* in  = manager->GetInputPinByName(target, input);
      if (!out || !in)
        return xiiStatus(xiiFmt("Missing pin '{} -> {}'.", output, input));
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };
    const auto expose = [&](xiiStringView name, bool input, const xiiDocumentObject* target, xiiStringView pin, xiiVec2 position) -> xiiStatus {
      const auto* boundary = add(name, "xiiRenderPipelineInterfacePass", position);
      if (!boundary)
        return xiiStatus("Could not create depth/history boundary.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Name", name));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Input", input));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(boundary));
      return input ? connect(boundary, "Value", target, pin) : connect(target, pin, boundary, "Value");
    };
    XII_SUCCEED_OR_RETURN(connect(reduce, "DepthPyramid", history, "Texture"));
    XII_SUCCEED_OR_RETURN(expose("SceneDepth", true, reduce, "SceneDepth", {0, 0}));
    XII_SUCCEED_OR_RETURN(expose("DepthPyramid", false, reduce, "DepthPyramid", {1800, 0}));
    XII_SUCCEED_OR_RETURN(expose("PreviousDepthPyramid", false, history, "PreviousTexture", {1800, 400}));
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import depth/history asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/DepthPyramid.xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Depth/history asset did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 2);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), 3);
    XII_TEST_INT(loaded->GetPipeline()->GetDescriptor().m_Passes.GetCount(), 2);
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements().IsNoFlagSet() && loaded->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
    xiiRenderPipelineSubgraphPass prototype;
    prototype.m_sResource = message.m_sOutputFile;
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    prototype.GetPins(pins);
    bool temporal = false;
    for (const auto& pin : pins)
      if (pin.m_sName == "PreviousDepthPyramid")
        temporal = pin.m_bTemporalHistoryOutput && pin.m_bOptional;
    XII_TEST_BOOL(temporal);
    return XII_SUCCESS;
  }

  xiiStatus AuthorDepthOcclusion(xiiStringView outputDirectory, bool meshlets = false)
  {
    const xiiStringView name = meshlets ? "MeshletOcclusion" : "InstanceOcclusion";
    xiiStringBuilder    path(outputDirectory, "/", name, ".xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path, name);
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author independent depth occlusion");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", static_cast<xiiInt64>(xiiRenderPipelineDeviceFeature::Compute)));
    const auto add = [&](xiiStringView label, xiiStringView type, xiiVec2 position) -> const xiiDocumentObject* {
      auto id = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.", name, "/", label));
      if (accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName(type), id).Failed())
        return nullptr;
      xiiMoveNodeCommand move;
      move.m_Object = id;
      move.m_NewPos = position;
      if (document.GetCommandHistory()->AddCommand(move).Failed())
        return nullptr;
      return accessor->GetObject(id);
    };
    const auto* filter = add("Filter Candidates", meshlets ? "xiiMeshletOcclusionPass" : "xiiInstanceOcclusionPass", {1200, 0});
    const auto* reset  = add("Reset Visible Count", "xiiCounterResetPass", {600, -200});
    if (!filter || !reset)
      return xiiStatus("Could not create native occlusion nodes.");
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView output, const xiiDocumentObject* target, xiiStringView input) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, output);
      const auto* in  = manager->GetInputPinByName(target, input);
      if (!out || !in)
        return xiiStatus(xiiFmt("Missing pin '{} -> {}'.", output, input));
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };
    XII_SUCCEED_OR_RETURN(connect(reset, "Counter", filter, "InitialCount"));
    xiiInstanceOcclusionPass              instancePrototype;
    xiiMeshletOcclusionPass               meshletPrototype;
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    (meshlets ? static_cast<xiiRenderPipelinePass*>(&meshletPrototype) : static_cast<xiiRenderPipelinePass*>(&instancePrototype))->GetPins(pins);
    xiiUInt32 inputRow = 0, outputRow = 0;
    for (const auto& pin : pins)
    {
      if (pin.m_Type == xiiRenderPipelinePin::Order || pin.m_sName == "InitialCount")
        continue;
      const auto* boundary = add(xiiStringBuilder(pin.m_bInput ? "Input/" : "Output/", pin.m_sName), "xiiRenderPipelineInterfacePass", {pin.m_bInput ? 0.0f : 1800.0f, float((pin.m_bInput ? inputRow++ : outputRow++) * 150)});
      if (!boundary)
        return xiiStatus("Could not create occlusion boundary.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Name", pin.m_sName));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Input", pin.m_bInput));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "ResourceType", xiiUInt32(pin.m_Type)));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Format", static_cast<xiiInt64>(pin.m_Format)));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(boundary));
      XII_SUCCEED_OR_RETURN(pin.m_bInput ? connect(boundary, "Value", filter, pin.m_sName) : connect(filter, pin.m_sName, boundary, "Value"));
    }
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import occlusion asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/", name, ".xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Occlusion asset did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 2);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), meshlets ? 8 : 7);
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements().IsNoFlagSet() && loaded->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
    return XII_SUCCESS;
  }

  xiiStatus AuthorShadowDepth(xiiStringView outputDirectory)
  {
    const xiiStringView name = "ShadowDepth";
    xiiStringBuilder    path(outputDirectory, "/", name, ".xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path, name);
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author independent depth shadow depth");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", static_cast<xiiInt64>(xiiRenderPipelineDeviceFeature::BindlessResources)));
    const auto add = [&](xiiStringView label, xiiStringView type, xiiVec2 position) -> const xiiDocumentObject* {
      auto id = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.", name, "/", label));
      if (accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName(type), id).Failed())
        return nullptr;
      xiiMoveNodeCommand move;
      move.m_Object = id;
      move.m_NewPos = position;
      if (document.GetCommandHistory()->AddCommand(move).Failed())
        return nullptr;
      return accessor->GetObject(id);
    };
    const auto* filter    = add("Depth Raster", "xiiDepthRasterPass", {1200, 0});
    const auto* reset     = add("Atlas Region Clear", "xiiDepthRegionClearPass", {600, -200});
    const auto* constants = add("Projection Constants", "xiiMatrixRasterConstantsPass", {600, 1000});
    if (!filter || !reset || !constants)
      return xiiStatus("Could not create native shadow depth nodes.");
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView output, const xiiDocumentObject* target, xiiStringView input) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, output);
      const auto* in  = manager->GetInputPinByName(target, input);
      if (!out || !in)
        return xiiStatus(xiiFmt("Missing pin '{} -> {}'.", output, input));
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };
    XII_SUCCEED_OR_RETURN(connect(reset, "Depth", filter, "DepthTarget"));
    XII_SUCCEED_OR_RETURN(connect(constants, "RasterConstants", filter, "RasterConstants"));
    xiiDepthRasterPass                    prototype;
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    prototype.GetPins(pins);
    xiiUInt32 inputRow = 0, outputRow = 0;
    for (const auto& pin : pins)
    {
      if (pin.m_Type == xiiRenderPipelinePin::Order || pin.m_sName == "RasterConstants")
        continue;
      const auto* boundary = add(xiiStringBuilder(pin.m_bInput ? "Input/" : "Output/", pin.m_sName), "xiiRenderPipelineInterfacePass", {pin.m_bInput ? 0.0f : 1800.0f, float((pin.m_bInput ? inputRow++ : outputRow++) * 150)});
      if (!boundary)
        return xiiStatus("Could not create shadow depth boundary.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Name", pin.m_bInput ? pin.m_sName : xiiString("ShadowDepth")));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Input", pin.m_bInput));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "ResourceType", xiiUInt32(pin.m_Type)));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Format", static_cast<xiiInt64>(pin.m_Format)));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(boundary));
      XII_SUCCEED_OR_RETURN(pin.m_bInput ? connect(boundary, "Value", pin.m_sName == "DepthTarget" ? reset : filter, pin.m_sName) : connect(filter, pin.m_sName, boundary, "Value"));
    }
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import shadow depth asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/", name, ".xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Occlusion asset did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 3);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), 11);
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements().IsNoFlagSet() && loaded->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
    return XII_SUCCESS;
  }

  xiiStatus AuthorShadowMap(xiiStringView outputDirectory, bool externalTarget = false)
  {
    const xiiStringView    name = externalTarget ? "ShadowMeshDepth" : "ShadowMap";
    const xiiStringBuilder library(outputDirectory, "/RenderPipelines");
    XII_SUCCEED_OR_RETURN(xiiOSFile::CreateDirectoryStructure(library));
    XII_SUCCEED_OR_RETURN(AuthorShadowDepth(library));
    xiiStringBuilder               path(outputDirectory, "/", name, ".xiiRenderPipelineAsset");
    NativeVisibilityAuthorDocument document(path, name);
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author explicit shadow target and reusable depth subgraph");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", static_cast<xiiInt64>((xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::MeshShaders).GetValue())));
    const auto add = [&](xiiStringView label, xiiStringView type, xiiVec2 position) -> const xiiDocumentObject* {
      auto id = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.", name, "/", label));
      if (accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName(type), id).Failed())
        return nullptr;
      xiiMoveNodeCommand move;
      move.m_Object = id;
      move.m_NewPos = position;
      if (document.GetCommandHistory()->AddCommand(move).Failed())
        return nullptr;
      return accessor->GetObject(id);
    };
    const auto* target = externalTarget ? nullptr : add("Depth Allocation", "xiiDepthTargetPass", {0, -300});
    const auto* depth  = add("Authored Depth", "xiiRenderPipelineSubgraphPass", {600, 0});
    if ((!externalTarget && !target) || !depth)
      return xiiStatus("Could not author shadow map nodes.");
    if (target)
    {
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(target, "Width", 2048U));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(target, "Height", 2048U));
    }
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(depth, "Resource", "RenderPipelines/ShadowDepth.xiiBinRenderPipeline"));
    auto overrideId = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.", name, "/Mesh Shader Override"));
    XII_SUCCEED_OR_RETURN(accessor->AddObject(depth, depth->GetType()->FindPropertyByName("Overrides"), -1, xiiGetStaticRTTI<xiiGraphPropertyOverride>(), overrideId));
    const auto* overrideObject = accessor->GetObject(overrideId);
    XII_SUCCEED_OR_RETURN(accessor->InsertValueByName(overrideObject, "Path", xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.ShadowDepth/Depth Raster"), -1));
    XII_SUCCEED_OR_RETURN(accessor->InsertValueByName(overrideObject, "Properties", true, "UseMeshShaders"));
    XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(depth));
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView output, const xiiDocumentObject* targetNode, xiiStringView input) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, output);
      const auto* in  = manager->GetInputPinByName(targetNode, input);
      if (!out || !in)
        return xiiStatus(xiiFmt("Missing shadow map pin '{} -> {}'.", output, input));
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };
    if (target)
      XII_SUCCEED_OR_RETURN(connect(target, "DepthTarget", depth, "DepthTarget"));
    xiiUInt32 row = 0;
    for (const auto& pin : manager->GetInputPins(depth))
    {
      const auto* configuration = manager->GetPinPropertyObject(*pin);
      if (!configuration)
        return xiiStatus("Shadow map input has no reflected pin configuration.");
      const auto* metadata = accessor->GetChildObjectByName(configuration, "Metadata", {});
      if (!metadata)
        return xiiStatus("Shadow map pin configuration has no reflected metadata.");
      xiiVariant resourceType, format;
      XII_SUCCEED_OR_RETURN(accessor->GetValueByName(metadata, "ResourceType", resourceType));
      XII_SUCCEED_OR_RETURN(accessor->GetValueByName(metadata, "DeclaredFormat", format));
      if ((!externalTarget && pin->GetName() == "DepthTarget") || resourceType.ConvertTo<xiiUInt32>() == xiiRenderPipelinePin::Order)
        continue;
      const auto* boundary = add(xiiStringBuilder("Input/", pin->GetName()), "xiiRenderPipelineInterfacePass", {-600, float(row++ * 150)});
      if (!boundary)
        return xiiStatus("Could not author shadow map interface.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Name", pin->GetName()));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Input", true));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "ResourceType", resourceType.ConvertTo<xiiUInt32>()));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Format", format));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(boundary));
      XII_SUCCEED_OR_RETURN(connect(boundary, "Value", depth, pin->GetName()));
    }
    const auto* output = add("Output/ShadowDepth", "xiiRenderPipelineInterfacePass", {1800, 0});
    if (!output)
      return xiiStatus("Could not author shadow map output.");
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(output, "Name", "ShadowDepth"));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(output, "Input", false));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(output, "ResourceType", xiiUInt32(xiiRenderPipelinePin::Texture)));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(output, "Format", static_cast<xiiInt64>(xiiGALResourceFormat::D32Float)));
    XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(output));
    XII_SUCCEED_OR_RETURN(connect(depth, "ShadowDepth", output, "Value"));
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import shadow map asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/", name, ".xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Shadow map asset did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), externalTarget ? 1 : 2);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), externalTarget ? 11 : 10);
    XII_TEST_INT(loaded->GetPipeline()->GetDescriptor().m_Passes.GetCount(), externalTarget ? 3 : 4);
    return XII_SUCCESS;
  }

  xiiStatus AuthorGeometryLodFeedback(xiiStringView outputDirectory)
  {
    const xiiStringView name = "GeometryLodFeedback";
    xiiStringBuilder    path(outputDirectory, "/", name, ".xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path, name);
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author independent geometry streaming feedback");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", static_cast<xiiInt64>(xiiRenderPipelineDeviceFeature::Compute)));
    const auto add = [&](xiiStringView label, xiiStringView type, xiiVec2 position) -> const xiiDocumentObject* {
      auto id = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.", name, "/", label));
      if (accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName(type), id).Failed())
        return nullptr;
      xiiMoveNodeCommand move;
      move.m_Object = id;
      move.m_NewPos = position;
      if (document.GetCommandHistory()->AddCommand(move).Failed())
        return nullptr;
      return accessor->GetObject(id);
    };
    const auto* filter   = add("Filter Candidates", "xiiGeometryLodFeedbackPass", {1200, 0});
    const auto* reset    = add("Initialize Request Table", "xiiGeometryLodFeedbackResetPass", {600, -200});
    const auto* readback = add("Submit Residency Requests", "xiiGeometryLodFeedbackReadbackPass", {1800, 0});
    if (!filter || !reset || !readback)
      return xiiStatus("Could not create native geometry feedback nodes.");
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView output, const xiiDocumentObject* target, xiiStringView input) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, output);
      const auto* in  = manager->GetInputPinByName(target, input);
      if (!out || !in)
        return xiiStatus(xiiFmt("Missing pin '{} -> {}'.", output, input));
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };
    XII_SUCCEED_OR_RETURN(connect(reset, "Requests", filter, "InitializedRequests"));
    XII_SUCCEED_OR_RETURN(connect(filter, "Requests", readback, "Requests"));
    xiiGeometryLodFeedbackPass            prototype;
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    prototype.GetPins(pins);
    xiiUInt32 inputRow = 0, outputRow = 0;
    for (const auto& pin : pins)
    {
      if (pin.m_Type == xiiRenderPipelinePin::Order || pin.m_sName == "InitializedRequests")
        continue;
      const auto* boundary = add(xiiStringBuilder(pin.m_bInput ? "Input/" : "Output/", pin.m_sName), "xiiRenderPipelineInterfacePass", {pin.m_bInput ? 0.0f : 1800.0f, float((pin.m_bInput ? inputRow++ : outputRow++) * 150 + 300)});
      if (!boundary)
        return xiiStatus("Could not create geometry feedback boundary.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Name", pin.m_sName));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Input", pin.m_bInput));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "ResourceType", xiiUInt32(pin.m_Type)));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Format", static_cast<xiiInt64>(pin.m_Format)));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(boundary));
      XII_SUCCEED_OR_RETURN(pin.m_bInput ? connect(boundary, "Value", filter, pin.m_sName) : connect(filter, pin.m_sName, boundary, "Value"));
    }
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import geometry feedback asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/", name, ".xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Occlusion asset did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 3);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), 5);
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements().IsNoFlagSet() && loaded->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
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

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, DepthOnly)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "NativeDepthAuthoring", "NativeDepthBase", xiiDataDirUsage::ReadOnly)) ||
      !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeDepthAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeDepthAuthoring"));
  XII_TEST_STATUS(AuthorDepthOnly(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, GBuffer)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "NativeGBufferAuthoring", "NativeGBufferBase", xiiDataDirUsage::ReadOnly)) ||
      !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeGBufferAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeGBufferAuthoring"));
  XII_TEST_STATUS(AuthorDepthOnly(path, true));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, DepthPyramid)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativePyramidAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativePyramidAuthoring"));
  XII_TEST_STATUS(AuthorDepthPyramid(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, InstanceOcclusion)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeOcclusionAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeOcclusionAuthoring"));
  XII_TEST_STATUS(AuthorDepthOcclusion(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, MeshletOcclusion)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeMeshletOcclusionAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeMeshletOcclusionAuthoring"));
  XII_TEST_STATUS(AuthorDepthOcclusion(path, true));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, GeometryLodFeedback)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeGeometryFeedbackAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeGeometryFeedbackAuthoring"));
  XII_TEST_STATUS(AuthorGeometryLodFeedback(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, ShadowDepth)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeShadowAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeShadowAuthoring"));
  XII_TEST_STATUS(AuthorShadowDepth(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, MatrixInstanceVisibility)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "MatrixInstanceAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("MatrixInstanceAuthoring"));
  XII_TEST_STATUS(AuthorInstanceVisibility(path, true));
}
XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, MatrixMeshletVisibility)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "MatrixMeshletAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("MatrixMeshletAuthoring"));
  XII_TEST_STATUS(AuthorMeshletVisibility(path, true));
}
XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, WorldMeshDraws)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "WorldMeshAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("WorldMeshAuthoring"));
  XII_TEST_STATUS(AuthorWorldMeshDraws(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, ShadowMap)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeShadowMapAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeShadowMapAuthoring"));
  XII_TEST_STATUS(AuthorShadowMap(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, ShadowMeshDepth)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeShadowMeshAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeShadowMeshAuthoring"));
  XII_TEST_STATUS(AuthorShadowMap(path, true));
}
