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
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowCascadePass.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowFeedbackPass.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowTablesPass.h>
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

  xiiStatus AuthorVirtualShadowFeedback(xiiStringView outputDirectory)
  {
    const xiiStringView name = "VirtualShadowFeedback";
    xiiStringBuilder    path(outputDirectory, "/", name, ".xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path, name);
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author independent virtual shadow feedback");
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
    const auto* filter   = add("Generate Depth Demand", "xiiVirtualShadowFeedbackPass", {1200, 0});
    const auto* reset    = add("Initialize Append Stream", "xiiVirtualShadowFeedbackResetPass", {600, -200});
    const auto* readback = add("Submit Residency Requests", "xiiVirtualShadowFeedbackReadbackPass", {1800, 0});
    if (!filter || !reset || !readback)
      return xiiStatus("Could not create native shadow feedback nodes.");
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView output, const xiiDocumentObject* target, xiiStringView input) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, output);
      const auto* in  = manager->GetInputPinByName(target, input);
      if (!out || !in)
        return xiiStatus(xiiFmt("Missing pin '{} -> {}'.", output, input));
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };
    XII_SUCCEED_OR_RETURN(connect(reset, "Requests", filter, "InitializedRequests"));
    XII_SUCCEED_OR_RETURN(connect(filter, "Requests", readback, "Requests"));
    xiiVirtualShadowFeedbackPass          prototype;
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    prototype.GetPins(pins);
    xiiUInt32 inputRow = 0, outputRow = 0;
    for (const auto& pin : pins)
    {
      if (pin.m_Type == xiiRenderPipelinePin::Order || pin.m_sName == "InitializedRequests")
        continue;
      const auto* boundary = add(xiiStringBuilder(pin.m_bInput ? "Input/" : "Output/", pin.m_sName), "xiiRenderPipelineInterfacePass", {pin.m_bInput ? 0.0f : 1800.0f, float((pin.m_bInput ? inputRow++ : outputRow++) * 150 + 300)});
      if (!boundary)
        return xiiStatus("Could not create shadow feedback boundary.");
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
      return xiiStatus("Could not import shadow feedback asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/", name, ".xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Shadow feedback asset did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 3);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), 3);
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements().IsNoFlagSet() && loaded->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
    return XII_SUCCESS;
  }

  xiiStatus AuthorShadowCascades(xiiStringView outputDirectory)
  {
    const xiiStringView name = "ShadowCascades";
    xiiStringBuilder    path(outputDirectory, "/", name, ".xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path, name);
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author stabilized directional cascades");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
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
    const auto* cascades = add("Stabilized Directional Cascades", "xiiShadowCascadePass", {600, 0});
    if (!cascades)
      return xiiStatus("Could not create native directional cascade node.");
    const auto* settings = accessor->GetChildObjectByName(cascades, "Settings", {});
    if (!settings)
      return xiiStatus("Missing reflected cascade settings object.");
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(settings, "CascadeCount", 1U));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(settings, "MaximumShadowDistance", 120.0f));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(settings, "DepthPadding", 30.0f));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(settings, "ShadowMapResolution", 2048U));
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView output, const xiiDocumentObject* target, xiiStringView input) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, output);
      const auto* in  = manager->GetInputPinByName(target, input);
      if (!out || !in)
        return xiiStatus(xiiFmt("Missing pin '{} -> {}'.", output, input));
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };
    xiiShadowCascadePass                  prototype;
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    prototype.GetPins(pins);
    xiiUInt32 inputRow = 0, outputRow = 0;
    for (const auto& pin : pins)
    {
      if (pin.m_Type == xiiRenderPipelinePin::Order)
        continue;
      const auto* boundary = add(xiiStringBuilder(pin.m_bInput ? "Input/" : "Output/", pin.m_sName), "xiiRenderPipelineInterfacePass", {pin.m_bInput ? 0.0f : 1800.0f, float((pin.m_bInput ? inputRow++ : outputRow++) * 150 + 300)});
      if (!boundary)
        return xiiStatus("Could not create directional cascades boundary.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Name", pin.m_sName));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Input", pin.m_bInput));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "ResourceType", xiiUInt32(pin.m_Type)));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Format", static_cast<xiiInt64>(pin.m_Format)));
      if (pin.m_pDataType)
        XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "DataType", pin.m_pDataType->GetTypeName()));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(boundary));
      XII_SUCCEED_OR_RETURN(pin.m_bInput ? connect(boundary, "Value", cascades, pin.m_sName) : connect(cascades, pin.m_sName, boundary, "Value"));
    }
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import directional cascades asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/", name, ".xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Directional cascade asset did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 1);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), 2);
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements().IsNoFlagSet() && loaded->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
    return XII_SUCCESS;
  }

  xiiStatus AuthorVirtualShadowTables(xiiStringView outputDirectory)
  {
    const xiiStringView name = "VirtualShadowTables";
    xiiStringBuilder    path(outputDirectory, "/", name, ".xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path, name);
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author device residency table uploads");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
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
    const auto* tables = add("Publish Device Page Tables", "xiiVirtualShadowTablesPass", {600, 0});
    if (!tables)
      return xiiStatus("Could not create native table upload node.");
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView output, const xiiDocumentObject* target, xiiStringView input) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, output);
      const auto* in  = manager->GetInputPinByName(target, input);
      if (!out || !in)
        return xiiStatus(xiiFmt("Missing pin '{} -> {}'.", output, input));
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };
    xiiVirtualShadowTablesPass            prototype;
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    prototype.GetPins(pins);
    xiiUInt32 inputRow = 0, outputRow = 0;
    for (const auto& pin : pins)
    {
      if (pin.m_Type == xiiRenderPipelinePin::Order)
        continue;
      const auto* boundary = add(xiiStringBuilder(pin.m_bInput ? "Input/" : "Output/", pin.m_sName), "xiiRenderPipelineInterfacePass", {pin.m_bInput ? 0.0f : 1800.0f, float((pin.m_bInput ? inputRow++ : outputRow++) * 150 + 300)});
      if (!boundary)
        return xiiStatus("Could not create virtual shadow tables boundary.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Name", pin.m_sName));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Input", pin.m_bInput));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "ResourceType", xiiUInt32(pin.m_Type)));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Format", static_cast<xiiInt64>(pin.m_Format)));
      if (pin.m_pDataType)
        XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "DataType", pin.m_pDataType->GetTypeName()));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(boundary));
      XII_SUCCEED_OR_RETURN(pin.m_bInput ? connect(boundary, "Value", tables, pin.m_sName) : connect(tables, pin.m_sName, boundary, "Value"));
    }
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import virtual shadow tables asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/", name, ".xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Virtual shadow table asset did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 1);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), 4);
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements().IsNoFlagSet() && loaded->GetPipeline()->GetRuntimeFeatures() == xiiRenderPipelineFeature::VirtualShadows);
    return XII_SUCCESS;
  }

  xiiStatus AuthorNativeLeaf(xiiStringView outputDirectory, xiiStringView name, xiiStringView typeName, xiiBitflags<xiiRenderPipelineFeature> features = xiiRenderPipelineFeature::VirtualShadows, bool viewProfile = false)
  {
    xiiStringBuilder path(outputDirectory, "/", name, ".xiiRenderPipelineAsset");
    path.MakeCleanPath();
    NativeVisibilityAuthorDocument document(path, name);
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author an independent native operation");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
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
    const auto* tables = add("Native Shadow Operation", typeName, {600, 0});
    if (!tables)
      return xiiStatus("Could not create native operation.");
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView output, const xiiDocumentObject* target, xiiStringView input) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, output);
      const auto* in  = manager->GetInputPinByName(target, input);
      if (!out || !in)
        return xiiStatus(xiiFmt("Missing pin '{} -> {}'.", output, input));
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };
    if (viewProfile)
    {
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(tables, "UseViewProfile", true));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(tables));
    }
    const auto*            type      = xiiRTTI::FindTypeByName(typeName);
    xiiRenderPipelinePass* prototype = type->GetAllocator()->Allocate<xiiRenderPipelinePass>();
    XII_SCOPE_EXIT(type->GetAllocator()->Deallocate(prototype));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", xiiInt64(prototype->GetRequiredDeviceFeatures().GetValue())));
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    if (viewProfile)
    {
      xiiVariantDictionary configuration;
      configuration.Insert("UseViewProfile", true);
      XII_SUCCEED_OR_RETURN(prototype->Configure(configuration));
    }
    prototype->GetPins(pins);
    xiiUInt32 inputRow = 0, outputRow = 0;
    for (const auto& pin : pins)
    {
      if (pin.m_Type == xiiRenderPipelinePin::Order)
        continue;
      const auto* boundary = add(xiiStringBuilder(pin.m_bInput ? "Input/" : "Output/", pin.m_sName), "xiiRenderPipelineInterfacePass", {pin.m_bInput ? 0.0f : 1800.0f, float((pin.m_bInput ? inputRow++ : outputRow++) * 150 + 300)});
      if (!boundary)
        return xiiStatus("Could not create native operation boundary.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Name", pin.m_sName));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Input", pin.m_bInput));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "ResourceType", xiiUInt32(pin.m_Type)));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Format", static_cast<xiiInt64>(pin.m_Format)));
      if (pin.m_pDataType)
        XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "DataType", pin.m_pDataType->GetTypeName()));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(boundary));
      XII_SUCCEED_OR_RETURN(pin.m_bInput ? connect(boundary, "Value", tables, pin.m_sName) : connect(tables, pin.m_sName, boundary, "Value"));
    }
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import native operation asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/", name, ".xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("Native operation asset did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 1);
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Interface.GetCount(), pins.GetCount() - 2);
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements() == prototype->GetExtractionRequirements() && loaded->GetPipeline()->GetRuntimeFeatures() == features);
    return XII_SUCCESS;
  }

  xiiStatus AuthorViewBoundaryAsset(xiiStringView outputDirectory, xiiStringView name)
  {
    xiiStringBuilder               path(outputDirectory, "/", name, ".xiiRenderPipelineAsset");
    NativeVisibilityAuthorDocument document(path, name);
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author independent view boundary operations");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
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
    const auto boundary = [&](const xiiDocumentObject* operation, xiiStringView pin, xiiStringView label, bool input, xiiRenderPipelinePin::Type kind, xiiGALResourceFormat::Enum format, float row) -> xiiStatus {
      const auto* node = add(label, "xiiRenderPipelineInterfacePass", {input ? 0.0f : 1200.0f, row});
      if (!operation || !node)
        return xiiStatus("Missing view boundary operation.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node, "Name", label));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node, "Input", input));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node, "ResourceType", xiiUInt32(kind)));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node, "Format", xiiInt64(format)));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(node));
      const auto* out = manager->GetOutputPinByName(input ? node : operation, input ? "Value" : pin);
      const auto* in  = manager->GetInputPinByName(input ? operation : node, input ? pin : "Value");
      if (!out || !in)
        return xiiStatus("Missing view boundary pin.");
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };
    if (name == "SceneTargets")
    {
      const auto* color = add("Clear Color", "xiiClearTexturePass", {600, 0});
      const auto* depth = add("Clear Depth", "xiiDepthTargetPass", {600, 300});
      if (!color || !depth)
        return xiiStatus("Missing target initialization nodes.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(color, "Format", xiiInt64(xiiGALResourceFormat::RGBA8UNormalizedSRGB)));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(color, "Color", xiiColor(0.008f, 0.012f, 0.025f)));
      XII_SUCCEED_OR_RETURN(boundary(color, "Texture", "Color", false, xiiRenderPipelinePin::Texture, xiiGALResourceFormat::RGBA8UNormalizedSRGB, 0));
      XII_SUCCEED_OR_RETURN(boundary(depth, "DepthTarget", "Depth", false, xiiRenderPipelinePin::Texture, xiiGALResourceFormat::D32Float, 300));
    }
    else if (name == "ViewPresentation")
    {
      const auto* present = add("Present Display Signal", "xiiPresentPass", {600, 0});
      XII_SUCCEED_OR_RETURN(boundary(present, "GradedColor", "Color", true, xiiRenderPipelinePin::Texture, xiiGALResourceFormat::Unknown, 0));
      XII_SUCCEED_OR_RETURN(boundary(present, "ViewOutput", "ViewOutput", false, xiiRenderPipelinePin::Texture, xiiGALResourceFormat::Unknown, 0));
      XII_SUCCEED_OR_RETURN(boundary(present, "Final Blit Constants", "Constants", false, xiiRenderPipelinePin::Buffer, xiiGALResourceFormat::Unknown, 300));
    }
    else if (name == "BufferExport")
    {
      const auto* operation = add("External Buffer Consumer", "xiiBufferExportPass", {600, 0});
      XII_SUCCEED_OR_RETURN(boundary(operation, "Input", "Input", true, xiiRenderPipelinePin::Buffer, xiiGALResourceFormat::Unknown, 0));
      XII_SUCCEED_OR_RETURN(boundary(operation, "Output", "Output", false, xiiRenderPipelinePin::Buffer, xiiGALResourceFormat::Unknown, 0));
    }
    else
      return xiiStatus("Unknown test fixture.");
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    if (context.Import(document).Failed())
      return xiiStatus("Could not import view boundary asset.");
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(outputDirectory, "/", name, ".xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline() || !loaded->GetAuthoring())
      return xiiStatus("View boundary asset did not compile.");
    XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), name == "SceneTargets" ? 2 : 1);
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements().IsNoFlagSet() && loaded->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
    return XII_SUCCESS;
  }

  xiiStatus AuthorDepthSensorDiagnostic(xiiStringView directory)
  {
    const xiiStringView            name = "DepthSensorDiagnostic";
    NativeVisibilityAuthorDocument document(xiiStringBuilder(directory, "/", name, ".xiiRenderPipelineAsset"), name);
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author an empty-depth sensor diagnostic");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", xiiInt64(xiiRenderPipelineDeviceFeature::Compute)));
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
    const auto* depth  = add("Empty Reversed Depth", "xiiDepthTargetPass", {0, 0});
    const auto* camera = add("Camera Snapshot", "xiiViewConstantsPass", {0, 300});
    const auto* sensor = add("Calibrated Depth", "xiiDepthSensorPass", {600, 0});
    const auto* output = add("SensorOutput", "xiiRenderPipelineInterfacePass", {1200, 0});
    if (!depth || !camera || !sensor || !output)
      return xiiStatus("Missing depth diagnostic node.");
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(sensor, "UseViewProfile", true));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(output, "Name", "SensorOutput"));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(output, "Input", false));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(output, "Format", xiiInt64(xiiGALResourceFormat::RGBA32Float)));
    XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(output));
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView outName, const xiiDocumentObject* target, xiiStringView inName) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, outName);
      const auto* in  = manager->GetInputPinByName(target, inName);
      if (!out || !in)
        return xiiStatus("Missing diagnostic pin.");
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };
    XII_SUCCEED_OR_RETURN(connect(depth, "DepthTarget", sensor, "SceneDepth"));
    XII_SUCCEED_OR_RETURN(connect(camera, "PerFrameCamera", sensor, "PerFrameCamera"));
    XII_SUCCEED_OR_RETURN(connect(sensor, "SensorOutput", output, "Value"));
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    XII_SUCCEED_OR_RETURN(context.Import(document));
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(directory, "/", name, ".xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    XII_SUCCEED_OR_RETURN(context.ExportDocument(&message));
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetPipeline())
      return xiiStatus("Depth diagnostic failed compilation.");
    XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements().IsNoFlagSet() && loaded->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
    return XII_SUCCESS;
  }

  xiiStatus AuthorDirectionalMesh(xiiStringView directory)
  {
    const xiiStringView            name = "DirectionalMesh";
    NativeVisibilityAuthorDocument document(xiiStringBuilder(directory, "/", name, ".xiiRenderPipelineAsset"), name);
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author directional mesh uniforms and rasterization");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", xiiInt64(xiiBitflags<xiiRenderPipelineDeviceFeature>(xiiRenderPipelineDeviceFeature::MeshShaders | xiiRenderPipelineDeviceFeature::BindlessResources).GetValue())));
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
    const auto* constants         = add("Directional Constants", "xiiDirectionalMeshConstantsPass", {600, 0});
    const auto* sampling          = add("Shadow Sampling", "xiiVirtualShadowSamplingConstantsPass", {600, 500});
    const auto* raster            = add("Mesh Raster", "xiiDirectionalMeshRasterPass", {1400, 0});
    const auto* geometryConstants = add("Geometry Constants", "xiiMatrixRasterConstantsPass", {600, -400});
    if (!constants || !sampling || !raster || !geometryConstants)
      return xiiStatus("Missing native directional mesh operation.");
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(constants, "SelectionPulseStrength", 0.04f));
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView output, const xiiDocumentObject* target, xiiStringView input) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, output);
      const auto* in  = manager->GetInputPinByName(target, input);
      if (!out || !in)
        return xiiStatus(xiiFmt("Missing directional mesh pin '{} -> {}'.", output, input));
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };
    XII_SUCCEED_OR_RETURN(connect(geometryConstants, "RasterConstants", raster, "RasterConstants"));
    XII_SUCCEED_OR_RETURN(connect(constants, "Constants", raster, "Constants"));
    XII_SUCCEED_OR_RETURN(connect(sampling, "Constants", raster, "VirtualShadowConstants"));
    xiiHashTable<xiiString, const xiiDocumentObject*> inputs;
    xiiUInt32                                         inputRow = 0, outputRow = 0;
    const auto                                        expose = [&](const xiiDocumentObject* node, const xiiRenderPipelinePin& pin, xiiStringView name) -> xiiStatus {
      const xiiDocumentObject* boundary = nullptr;
      if (pin.m_bInput)
        inputs.TryGetValue(name, boundary);
      if (!boundary)
      {
        boundary = add(xiiStringBuilder(pin.m_bInput ? "Input/" : "Output/", name), "xiiRenderPipelineInterfacePass", {pin.m_bInput ? 0.0f : 2200.0f, float((pin.m_bInput ? inputRow++ : outputRow++) * 160 + 900)});
        if (!boundary)
          return xiiStatus("Could not create native directional mesh interface.");
        XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Name", name));
        XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Input", pin.m_bInput));
        XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "ResourceType", xiiUInt32(pin.m_Type)));
        XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "Format", xiiInt64(pin.m_Format)));
        if (pin.m_pDataType)
          XII_SUCCEED_OR_RETURN(accessor->SetValueByName(boundary, "DataType", pin.m_pDataType->GetTypeName()));
        XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(boundary));
        if (pin.m_bInput)
          inputs.Insert(name, boundary);
      }
      return pin.m_bInput ? connect(boundary, "Value", node, pin.m_sName) : connect(node, pin.m_sName, boundary, "Value");
    };
    for (const auto* node : {constants, sampling, raster})
    {
      xiiRenderPipelinePass* prototype = node->GetType()->GetAllocator()->Allocate<xiiRenderPipelinePass>();
      XII_SCOPE_EXIT(node->GetType()->GetAllocator()->Deallocate(prototype));
      xiiDynamicArray<xiiRenderPipelinePin> pins;
      prototype->GetPins(pins);
      for (const auto& pin : pins)
      {
        if (pin.m_Type == xiiRenderPipelinePin::Order || (node == raster && pin.m_bInput && (pin.m_sName == "Constants" || pin.m_sName == "VirtualShadowConstants" || pin.m_sName == "RasterConstants")))
          continue;
        const xiiStringView external = node == constants && !pin.m_bInput ? xiiStringView("RasterConstants") : node == sampling && !pin.m_bInput ? xiiStringView("SamplingConstants") :
                                                                                                                                                   xiiStringView(pin.m_sName);
        XII_SUCCEED_OR_RETURN(expose(node, pin, external));
      }
    }
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    XII_SUCCEED_OR_RETURN(context.Import(document));
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(directory, "/", name, ".xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    return context.ExportDocument(&message);
  }

  xiiStatus AuthorWorldRayProbe(xiiStringView directory)
  {
    const xiiStringView            name = "WorldRayProbe";
    NativeVisibilityAuthorDocument document(xiiStringBuilder(directory, "/", name, ".xiiRenderPipelineAsset"), name);
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author world acceleration structure and triangle probe");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    const xiiBitflags<xiiRenderPipelineDeviceFeature> features = xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::RayTracing | xiiRenderPipelineDeviceFeature::BindlessResources;
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", xiiInt64(features.GetValue())));
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
    const auto* scene  = add("World Acceleration Structure", "xiiRayTracingScenePass", {0, 0});
    const auto* probe  = add("Closest Triangle Probe", "xiiSceneRayProbePass", {600, 0});
    const auto* retain = add("Retain Result", "xiiBufferExportPass", {1200, 0});
    const auto* output = add("Result", "xiiRenderPipelineInterfacePass", {1800, 0});
    if (!scene || !probe || !retain || !output)
      return xiiStatus("Missing world ray probe node.");
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(scene, "RequireHardwareRayTracing", true));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(scene, "UseViewSelection", false));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(output, "Name", "Result"));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(output, "Input", false));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(output, "ResourceType", xiiUInt32(xiiRenderPipelinePin::Buffer)));
    XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(output));
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView outName, const xiiDocumentObject* target, xiiStringView inName) -> xiiStatus {
      const auto* out = manager->GetOutputPinByName(source, outName);
      const auto* in  = manager->GetInputPinByName(target, inName);
      if (!out || !in)
        return xiiStatus("Missing world ray probe connection.");
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };
    XII_SUCCEED_OR_RETURN(connect(scene, "Scene", probe, "Scene"));
    XII_SUCCEED_OR_RETURN(connect(scene, "Materials", probe, "Materials"));
    XII_SUCCEED_OR_RETURN(connect(probe, "Result", retain, "Input"));
    XII_SUCCEED_OR_RETURN(connect(retain, "Output", output, "Value"));
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    XII_SUCCEED_OR_RETURN(context.Import(document));
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(directory, "/", name, ".xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    return context.ExportDocument(&message);
  }

  xiiStatus AuthorClusterLightLists(xiiStringView directory)
  {
    const xiiStringView            name = "ClusterLightLists";
    NativeVisibilityAuthorDocument document(xiiStringBuilder(directory, "/", name, ".xiiRenderPipelineAsset"), name);
    auto*                          accessor = document.GetObjectAccessor();
    auto*                          manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author independent cluster operations");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(), "RequiredDeviceFeatures", xiiInt64(xiiRenderPipelineDeviceFeature::Compute)));
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
    const auto connect = [&](const xiiDocumentObject* source, xiiStringView outName, const xiiDocumentObject* target, xiiStringView inName) -> xiiStatus {
      if (!source || !target)
        return xiiStatus("Missing cluster operation.");
      const auto* out = manager->GetOutputPinByName(source, outName);
      const auto* in  = manager->GetInputPinByName(target, inName);
      if (!out || !in)
        return xiiStatus("Missing cluster operation pin.");
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *out, *in);
    };
    const auto* layout   = add("Camera And Cluster Layout", "xiiLightClusterViewPass", {0, 0});
    const auto* bounds   = add("Cluster Bounds", "xiiClusterBoundsPass", {600, 0});
    const auto* clear    = add("Initialize Cluster Lists", "xiiClusterListClearPass", {600, 400});
    const auto* assign   = add("Assign Local Lights", "xiiClusterLightAssignmentPass", {1200, 0});
    const auto* sampling = add("Cluster Sampling CDF", "xiiClusterLightSamplingPass", {1500, 400});
    XII_SUCCEED_OR_RETURN(connect(layout, "Layout", bounds, "Layout"));
    XII_SUCCEED_OR_RETURN(connect(layout, "Layout", clear, "Layout"));
    XII_SUCCEED_OR_RETURN(connect(layout, "Layout", assign, "Layout"));
    XII_SUCCEED_OR_RETURN(connect(bounds, "Clusters", assign, "Clusters"));
    XII_SUCCEED_OR_RETURN(connect(clear, "LightGrid", assign, "InitializedGrid"));
    XII_SUCCEED_OR_RETURN(connect(clear, "LightIndices", assign, "InitializedIndices"));
    XII_SUCCEED_OR_RETURN(connect(clear, "Overflow", assign, "InitializedOverflow"));
    XII_SUCCEED_OR_RETURN(connect(layout, "Layout", sampling, "Layout"));
    XII_SUCCEED_OR_RETURN(connect(assign, "LightGrid", sampling, "LightGrid"));
    XII_SUCCEED_OR_RETURN(connect(assign, "LightIndices", sampling, "LightIndices"));
    const auto boundary = [&](xiiStringView label, bool input, const xiiDocumentObject* operation, xiiStringView pin, xiiRenderPipelinePin::Type kind, float row) -> xiiStatus {
      const auto* node = add(label, "xiiRenderPipelineInterfacePass", {input ? 0.0f : 1800.0f, row});
      if (!node)
        return xiiStatus("Missing cluster boundary.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node, "Name", label));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node, "Input", input));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node, "ResourceType", xiiUInt32(kind)));
      if (kind == xiiRenderPipelinePin::FrameData)
        XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node, "DataType", "xiiLightClusterFrameData"));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(node));
      if (input && label == "Lights")
        XII_SUCCEED_OR_RETURN(connect(node, "Value", sampling, "Lights"));
      return input ? connect(node, "Value", operation, pin) : connect(operation, pin, node, "Value");
    };
    XII_SUCCEED_OR_RETURN(boundary("Lights", true, assign, "Lights", xiiRenderPipelinePin::Buffer, 800));
    XII_SUCCEED_OR_RETURN(boundary("Candidates", true, assign, "Candidates", xiiRenderPipelinePin::Buffer, 1000));
    XII_SUCCEED_OR_RETURN(boundary("Layout", false, layout, "Layout", xiiRenderPipelinePin::FrameData, 0));
    XII_SUCCEED_OR_RETURN(boundary("Clusters", false, bounds, "Clusters", xiiRenderPipelinePin::Buffer, 200));
    XII_SUCCEED_OR_RETURN(boundary("LightGrid", false, assign, "LightGrid", xiiRenderPipelinePin::Buffer, 400));
    XII_SUCCEED_OR_RETURN(boundary("LightIndices", false, assign, "LightIndices", xiiRenderPipelinePin::Buffer, 600));
    XII_SUCCEED_OR_RETURN(boundary("Overflow", false, assign, "Overflow", xiiRenderPipelinePin::Buffer, 800));
    XII_SUCCEED_OR_RETURN(boundary("PackedIndices", false, sampling, "PackedIndices", xiiRenderPipelinePin::Buffer, 1000));
    XII_SUCCEED_OR_RETURN(boundary("SamplingTotals", false, sampling, "SamplingTotals", xiiRenderPipelinePin::Buffer, 1200));
    accessor->FinishTransaction();
    finished = true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context;
    XII_SUCCEED_OR_RETURN(context.Import(document));
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(directory, "/", name, ".xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    return context.ExportDocument(&message);
  }

  xiiStatus AuthorDDGIEnvironmentProbes(xiiStringView directory)
  {
    const xiiStringView name="DDGIEnvironmentProbes";
    NativeVisibilityAuthorDocument document(xiiStringBuilder(directory,"/",name,".xiiRenderPipelineAsset"),name);
    auto* accessor=document.GetObjectAccessor();
    auto* manager=static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author independently scheduled environment probes");
    bool finished=false;
    XII_SCOPE_EXIT(if(!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(),"RequiredDeviceFeatures",xiiInt64(xiiRenderPipelineDeviceFeature::Compute)));
    const auto add=[&](xiiStringView label,xiiStringView type,xiiVec2 position)->const xiiDocumentObject*{
      auto id=xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.",name,"/",label));
      if(accessor->AddObject(nullptr,nullptr,-1,xiiRTTI::FindTypeByName(type),id).Failed()) return nullptr;
      xiiMoveNodeCommand move; move.m_Object=id; move.m_NewPos=position;
      if(document.GetCommandHistory()->AddCommand(move).Failed()) return nullptr;
      return accessor->GetObject(id);
    };
    const auto connect=[&](const xiiDocumentObject* a,xiiStringView output,const xiiDocumentObject* b,xiiStringView input)->xiiStatus{
      if(!a || !b) return xiiStatus("Missing DDGI operation.");
      const auto* out=manager->GetOutputPinByName(a,output); const auto* in=manager->GetInputPinByName(b,input);
      if(!out || !in) return xiiStatus("Missing reflected DDGI pin.");
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(),manager->GetConnectionType(),*out,*in);
    };
    const auto* schedule=add("Schedule Camera Probes","xiiDDGIProbeSchedulePass",{0,0});
    const auto* upload=add("Upload Probe Snapshot","xiiDDGIProbeUploadPass",{600,0});
    const auto* prepare=add("Prepare Current Atlases","xiiDDGIAtlasPreparePass",{600,600});
    const auto* seed=add("Seed Environment Radiance","xiiDDGIEnvironmentSeedPass",{1200,0});
    const auto* irradianceHistory=add("Retain Irradiance History","xiiTextureHistoryPass",{1800,400});
    const auto* distanceHistory=add("Retain Distance History","xiiTextureHistoryPass",{1800,800});
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(irradianceHistory,"AsyncCopy",false));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(distanceHistory,"AsyncCopy",false));
    XII_SUCCEED_OR_RETURN(connect(schedule,"Schedule",upload,"Schedule"));
    XII_SUCCEED_OR_RETURN(connect(schedule,"Schedule",prepare,"Schedule"));
    XII_SUCCEED_OR_RETURN(connect(schedule,"Schedule",seed,"Schedule"));
    XII_SUCCEED_OR_RETURN(connect(upload,"ProbeConstants",seed,"ProbeConstants"));
    XII_SUCCEED_OR_RETURN(connect(upload,"ProbeUpdates",seed,"ProbeUpdates"));
    XII_SUCCEED_OR_RETURN(connect(upload,"ProbeStates",seed,"InitialProbeStates"));
    XII_SUCCEED_OR_RETURN(connect(prepare,"Irradiance",seed,"InitialIrradiance"));
    XII_SUCCEED_OR_RETURN(connect(prepare,"Distance",seed,"InitialDistance"));
    XII_SUCCEED_OR_RETURN(connect(seed,"Irradiance",irradianceHistory,"Texture"));
    XII_SUCCEED_OR_RETURN(connect(seed,"Distance",distanceHistory,"Texture"));
    for(const auto* consumer:{schedule,prepare})
    {
      XII_SUCCEED_OR_RETURN(connect(irradianceHistory,"PreviousTexture",consumer,"PreviousIrradiance"));
      XII_SUCCEED_OR_RETURN(connect(distanceHistory,"PreviousTexture",consumer,"PreviousDistance"));
    }
    const auto boundary=[&](xiiStringView label,bool input,const xiiDocumentObject* operation,xiiStringView pin,xiiRenderPipelinePin::Type kind,xiiGALResourceFormat::Enum format,float row)->xiiStatus{
      const auto* node=add(label,"xiiRenderPipelineInterfacePass",{input ? -600.0f : 2400.0f,row});
      if(!node) return xiiStatus("Missing DDGI interface.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node,"Name",label));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node,"Input",input));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node,"ResourceType",xiiUInt32(kind)));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node,"Format",xiiInt64(format)));
      if(kind==xiiRenderPipelinePin::FrameData) XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node,"DataType","xiiDDGIProbeFrameData"));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(node));
      return input ? connect(node,"Value",operation,pin) : connect(operation,pin,node,"Value");
    };
    XII_SUCCEED_OR_RETURN(boundary("EnvironmentLight",true,seed,"EnvironmentLight",xiiRenderPipelinePin::Buffer,xiiGALResourceFormat::Unknown,1000));
    XII_SUCCEED_OR_RETURN(boundary("Irradiance",false,seed,"Irradiance",xiiRenderPipelinePin::Texture,xiiGALResourceFormat::RGBA16Float,0));
    XII_SUCCEED_OR_RETURN(boundary("Distance",false,seed,"Distance",xiiRenderPipelinePin::Texture,xiiGALResourceFormat::RG32Float,200));
    XII_SUCCEED_OR_RETURN(boundary("ProbeStates",false,seed,"ProbeStates",xiiRenderPipelinePin::Buffer,xiiGALResourceFormat::Unknown,400));
    XII_SUCCEED_OR_RETURN(boundary("ProbeConstants",false,upload,"ProbeConstants",xiiRenderPipelinePin::Buffer,xiiGALResourceFormat::Unknown,600));
    XII_SUCCEED_OR_RETURN(boundary("Schedule",false,schedule,"Schedule",xiiRenderPipelinePin::FrameData,xiiGALResourceFormat::Unknown,800));
    accessor->FinishTransaction(); finished=true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context; XII_SUCCEED_OR_RETURN(context.Import(document));
    xiiExportDocumentMsgToEngine message; message.m_sOutputFile=xiiStringBuilder(directory,"/",name,".xiiBinRenderPipeline"); message.m_uiAssetHash=1; message.m_uiVersion=7;
    return context.ExportDocument(&message);
  }
  xiiStatus AuthorSparseVoxelEnvironment(xiiStringView directory)
  {
    const xiiStringView name="SparseVoxelEnvironment";
    NativeVisibilityAuthorDocument document(xiiStringBuilder(directory,"/",name,".xiiRenderPipelineAsset"),name);
    auto* accessor=document.GetObjectAccessor();
    auto* manager=static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
    accessor->StartTransaction("Author independently scheduled environment probes");
    bool finished=false;
    XII_SCOPE_EXIT(if(!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(document.EnsureSettingsObject());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetSettingsObject(),"RequiredDeviceFeatures",xiiInt64(xiiRenderPipelineDeviceFeature::Compute)));
    const auto add=[&](xiiStringView label,xiiStringView type,xiiVec2 position)->const xiiDocumentObject*{
      auto id=xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.",name,"/",label));
      if(accessor->AddObject(nullptr,nullptr,-1,xiiRTTI::FindTypeByName(type),id).Failed()) return nullptr;
      xiiMoveNodeCommand move; move.m_Object=id; move.m_NewPos=position;
      if(document.GetCommandHistory()->AddCommand(move).Failed()) return nullptr;
      return accessor->GetObject(id);
    };
    const auto connect=[&](const xiiDocumentObject* a,xiiStringView output,const xiiDocumentObject* b,xiiStringView input)->xiiStatus{
      if(!a || !b) return xiiStatus("Missing SparseVoxel operation.");
      const auto* out=manager->GetOutputPinByName(a,output); const auto* in=manager->GetInputPinByName(b,input);
      if(!out || !in) return xiiStatus("Missing reflected SparseVoxel pin.");
      return xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(),manager->GetConnectionType(),*out,*in);
    };

    const auto* schedule=add("Schedule Camera Bricks","xiiSparseVoxelSchedulePass",{0,0});
    const auto* upload=add("Upload Clipmap Snapshot","xiiSparseVoxelUploadPass",{600,0});
    const auto* prepare=add("Prepare Current Pool","xiiSparseVoxelPoolPreparePass",{600,600});
    const auto* seed=add("Seed Environment Radiance","xiiSparseVoxelEnvironmentSeedPass",{1200,0});
    const auto* history=add("Retain Radiance History","xiiBufferHistoryPass",{1800,400});
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(history,"AsyncCopy",false));
    XII_SUCCEED_OR_RETURN(connect(schedule,"Schedule",upload,"Schedule"));
    XII_SUCCEED_OR_RETURN(connect(schedule,"Schedule",prepare,"Schedule"));
    XII_SUCCEED_OR_RETURN(connect(schedule,"Schedule",seed,"Schedule"));
    XII_SUCCEED_OR_RETURN(connect(upload,"Constants",seed,"Constants"));
    XII_SUCCEED_OR_RETURN(connect(upload,"BrickUpdates",seed,"BrickUpdates"));
    XII_SUCCEED_OR_RETURN(connect(upload,"LevelData",seed,"LevelData"));
    XII_SUCCEED_OR_RETURN(connect(upload,"PageTable",seed,"InitialPageTable"));
    XII_SUCCEED_OR_RETURN(connect(prepare,"RadiancePool",seed,"InitialPool"));
    XII_SUCCEED_OR_RETURN(connect(seed,"RadiancePool",history,"Buffer"));
    XII_SUCCEED_OR_RETURN(connect(history,"PreviousBuffer",schedule,"PreviousPool"));
    XII_SUCCEED_OR_RETURN(connect(history,"PreviousBuffer",prepare,"PreviousPool"));
    const auto boundary=[&](xiiStringView label,bool input,const xiiDocumentObject* operation,xiiStringView pin,xiiRenderPipelinePin::Type kind,xiiGALResourceFormat::Enum format,float row)->xiiStatus{
      const auto* node=add(label,"xiiRenderPipelineInterfacePass",{input ? -600.0f : 2400.0f,row});
      if(!node) return xiiStatus("Missing SparseVoxel interface.");
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node,"Name",label));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node,"Input",input));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node,"ResourceType",xiiUInt32(kind)));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node,"Format",xiiInt64(format)));
      if(kind==xiiRenderPipelinePin::FrameData) XII_SUCCEED_OR_RETURN(accessor->SetValueByName(node,"DataType","xiiSparseVoxelFrameData"));
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(node));
      return input ? connect(node,"Value",operation,pin) : connect(operation,pin,node,"Value");
    };

    XII_SUCCEED_OR_RETURN(boundary("EnvironmentLight",true,seed,"EnvironmentLight",xiiRenderPipelinePin::Buffer,xiiGALResourceFormat::Unknown,1000));
    XII_SUCCEED_OR_RETURN(boundary("RadiancePool",false,seed,"RadiancePool",xiiRenderPipelinePin::Buffer,xiiGALResourceFormat::Unknown,0));
    XII_SUCCEED_OR_RETURN(boundary("PageTable",false,seed,"PageTable",xiiRenderPipelinePin::Buffer,xiiGALResourceFormat::Unknown,200));
    XII_SUCCEED_OR_RETURN(boundary("LevelData",false,upload,"LevelData",xiiRenderPipelinePin::Buffer,xiiGALResourceFormat::Unknown,400));
    XII_SUCCEED_OR_RETURN(boundary("Constants",false,upload,"Constants",xiiRenderPipelinePin::Buffer,xiiGALResourceFormat::Unknown,600));
    XII_SUCCEED_OR_RETURN(boundary("Schedule",false,schedule,"Schedule",xiiRenderPipelinePin::FrameData,xiiGALResourceFormat::Unknown,800));
    accessor->FinishTransaction(); finished=true;
    XII_SUCCEED_OR_RETURN(document.SaveDocument(true));
    NativeVisibilityAuthorContext context; XII_SUCCEED_OR_RETURN(context.Import(document));
    xiiExportDocumentMsgToEngine message; message.m_sOutputFile=xiiStringBuilder(directory,"/",name,".xiiBinRenderPipeline"); message.m_uiAssetHash=1; message.m_uiVersion=7;
    return context.ExportDocument(&message);
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


XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, VirtualShadowFeedback)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeShadowFeedbackAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeShadowFeedbackAuthoring"));
  XII_TEST_STATUS(AuthorVirtualShadowFeedback(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, ShadowCascades)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeCascadeAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeCascadeAuthoring"));
  XII_TEST_STATUS(AuthorShadowCascades(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, VirtualShadowTables)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeShadowTablesAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeShadowTablesAuthoring"));
  XII_TEST_STATUS(AuthorVirtualShadowTables(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, VirtualShadowRasterWork)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeShadowWorkAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeShadowWorkAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path, "VirtualShadowRasterWork", "xiiVirtualShadowRasterWorkPass"));
}
XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, VirtualShadowPagePublish)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeShadowPublishAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeShadowPublishAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path, "VirtualShadowPagePublish", "xiiVirtualShadowPagePublishPass"));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, SceneTargets)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "ViewBoundaryAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("ViewBoundaryAuthoring"));
  XII_TEST_STATUS(AuthorViewBoundaryAsset(path, "SceneTargets"));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, ViewPresentation)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "ViewBoundaryAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("ViewBoundaryAuthoring"));
  XII_TEST_STATUS(AuthorViewBoundaryAsset(path, "ViewPresentation"));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, BufferExport)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "ViewBoundaryAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("ViewBoundaryAuthoring"));
  XII_TEST_STATUS(AuthorViewBoundaryAsset(path, "BufferExport"));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, DepthSensor)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeSensorAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeSensorAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path, "DepthSensor", "xiiDepthSensorPass", {}, true));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, RgbSensor)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeSensorAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeSensorAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path, "RgbSensor", "xiiRgbSensorPass", {}, true));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, InfraredSensor)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeSensorAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeSensorAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path, "InfraredSensor", "xiiInfraredSensorPass", {}, true));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, LidarSensor)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeSensorAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeSensorAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path, "LidarSensor", "xiiLidarSensorPass", {}, true));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, DepthSensorDiagnostic)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeSensorDiagnosticAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeSensorDiagnosticAuthoring"));
  XII_TEST_STATUS(AuthorDepthSensorDiagnostic(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, DirectionalMesh)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "DirectionalAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("DirectionalAuthoring"));
  XII_TEST_STATUS(AuthorDirectionalMesh(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, DirectionalMeshConstants)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "DirectionalAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("DirectionalAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path, "DirectionalMeshConstants", "xiiDirectionalMeshConstantsPass", {}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, VirtualShadowSamplingConstants)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "DirectionalAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("DirectionalAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path, "VirtualShadowSamplingConstants", "xiiVirtualShadowSamplingConstantsPass", {}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, SceneRayProbe)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "RayProbeAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("RayProbeAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path, "SceneRayProbe", "xiiSceneRayProbePass", {}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, WorldRayProbe)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "RayProbeAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("RayProbeAuthoring"));
  XII_TEST_STATUS(AuthorWorldRayProbe(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, LightClusterView)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "ClusterAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("ClusterAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path, "LightClusterView", "xiiLightClusterViewPass", {}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, ClusterBounds)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "ClusterAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("ClusterAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path, "ClusterBounds", "xiiClusterBoundsPass", {}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, ClusterListClear)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "ClusterAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("ClusterAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path, "ClusterListClear", "xiiClusterListClearPass", {}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, ClusterLightAssignment)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "ClusterAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("ClusterAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path, "ClusterLightAssignment", "xiiClusterLightAssignmentPass", {}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, ClusterLightLists)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "ClusterAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("ClusterAuthoring"));
  XII_TEST_STATUS(AuthorClusterLightLists(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, ClusterLightSampling)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "ClusterAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("ClusterAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path, "ClusterLightSampling", "xiiClusterLightSamplingPass", {}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, WorldLightsUpload)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "WorldLightAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("WorldLightAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path, "WorldLightsUpload", "xiiWorldLightsUploadPass", xiiRenderPipelineFeature::Lighting));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, WorldDirectionalLight)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeDirectionalLightAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeDirectionalLightAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path, "WorldDirectionalLight", "xiiWorldDirectionalLightPass", {}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, EnvironmentLightConstants)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeEnvironmentAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeEnvironmentAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path, "EnvironmentLightConstants", "xiiEnvironmentLightConstantsPass", {}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, DDGIProbeSchedule)
{
  const auto path=xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if(!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path,"NativeDDGIProbeScheduleAuthoring",":",xiiDataDirUsage::AllowWrites))) return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeDDGIProbeScheduleAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path,"DDGIProbeSchedule","xiiDDGIProbeSchedulePass",{}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, DDGIProbeUpload)
{
  const auto path=xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if(!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path,"NativeDDGIProbeUploadAuthoring",":",xiiDataDirUsage::AllowWrites))) return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeDDGIProbeUploadAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path,"DDGIProbeUpload","xiiDDGIProbeUploadPass",{}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, DDGIAtlasPrepare)
{
  const auto path=xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if(!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path,"NativeDDGIAtlasPrepareAuthoring",":",xiiDataDirUsage::AllowWrites))) return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeDDGIAtlasPrepareAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path,"DDGIAtlasPrepare","xiiDDGIAtlasPreparePass",{}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, DDGIEnvironmentSeed)
{
  const auto path=xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if(!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path,"NativeDDGIEnvironmentSeedAuthoring",":",xiiDataDirUsage::AllowWrites))) return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeDDGIEnvironmentSeedAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path,"DDGIEnvironmentSeed","xiiDDGIEnvironmentSeedPass",{}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, DDGIEnvironmentProbes)
{
  const auto path=xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if(!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path,"NativeDDGIEnvironmentAuthoring",":",xiiDataDirUsage::AllowWrites))) return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeDDGIEnvironmentAuthoring"));
  XII_TEST_STATUS(AuthorDDGIEnvironmentProbes(path));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, SparseVoxelSchedule)
{
  const auto path=xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if(!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path,"NativeSparseVoxelScheduleAuthoring",":",xiiDataDirUsage::AllowWrites))) return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeSparseVoxelScheduleAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path,"SparseVoxelSchedule","xiiSparseVoxelSchedulePass",{}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, SparseVoxelUpload)
{
  const auto path=xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if(!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path,"NativeSparseVoxelUploadAuthoring",":",xiiDataDirUsage::AllowWrites))) return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeSparseVoxelUploadAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path,"SparseVoxelUpload","xiiSparseVoxelUploadPass",{}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, SparseVoxelPoolPrepare)
{
  const auto path=xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if(!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path,"NativeSparseVoxelPoolPrepareAuthoring",":",xiiDataDirUsage::AllowWrites))) return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeSparseVoxelPoolPrepareAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path,"SparseVoxelPoolPrepare","xiiSparseVoxelPoolPreparePass",{}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, SparseVoxelEnvironmentSeed)
{
  const auto path=xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if(!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path,"NativeSparseVoxelEnvironmentSeedAuthoring",":",xiiDataDirUsage::AllowWrites))) return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeSparseVoxelEnvironmentSeedAuthoring"));
  XII_TEST_STATUS(AuthorNativeLeaf(path,"SparseVoxelEnvironmentSeed","xiiSparseVoxelEnvironmentSeedPass",{}));
}

XII_CREATE_SIMPLE_TEST(NativeAssetAuthoring, SparseVoxelEnvironment)
{
  const auto path=xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if(!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path,"NativeSparseEnvironmentAuthoring",":",xiiDataDirUsage::AllowWrites))) return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeSparseEnvironmentAuthoring"));
  XII_TEST_STATUS(AuthorSparseVoxelEnvironment(path));
}
