/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GraphicsCore/Pipeline/RenderPipelineProfileConfig.h>
#include <TestFramework/Utilities/TestLogInterface.h>

XII_CREATE_SIMPLE_TEST(Pipeline, ProjectPipelineReferences)
{
  xiiPlatformProfile profile;
  profile.AddMissingConfigs();
  auto* config = profile.GetTypeConfig<xiiRenderPipelineProfileConfig>();
  if (!XII_TEST_BOOL(config != nullptr))
    return;
  XII_TEST_STRING(config->m_sScenePipeline, "RenderPipelines/Scene.xiiBinRenderPipeline");
  XII_TEST_STRING(config->m_sEditorScenePipeline, "RenderPipelines/Editor.xiiBinRenderPipeline");
  XII_TEST_STRING(config->m_sAssetPreviewPipeline, "RenderPipelines/Scene.xiiBinRenderPipeline");
  config->m_sScenePipeline              = "Project/Simulation.xiiBinRenderPipeline";
  config->m_sEditorScenePipeline        = "Project/Scene.xiiBinRenderPipeline";
  config->m_sAssetPreviewPipeline       = "Project/Preview.xiiBinRenderPipeline";
  config->m_sTexturePreviewPipeline     = "Project/Texture.xiiBinRenderPipeline";
  config->m_sCubeTexturePreviewPipeline = "Project/Cube.xiiBinRenderPipeline";

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Explicit view resource precedes project policy without graph construction")
  {
    xiiRenderPipelineResourceHandle resource;
    xiiStringBuilder                error;
    XII_TEST_RESULT(xiiRenderPipelineProfileConfig::ResolveResource({}, &profile, xiiRenderPipelineUsage::Scene, resource, &error));
    XII_TEST_STRING(resource.GetResourceID(), config->m_sScenePipeline);
    XII_TEST_RESULT(xiiRenderPipelineProfileConfig::ResolveResource("Cameras/Depth.xiiBinRenderPipeline", &profile, xiiRenderPipelineUsage::Scene, resource, &error));
    XII_TEST_STRING(resource.GetResourceID(), "Cameras/Depth.xiiBinRenderPipeline");
    XII_TEST_STRING(config->m_sScenePipeline, "Project/Simulation.xiiBinRenderPipeline");
    XII_TEST_RESULT(xiiRenderPipelineProfileConfig::ResolveResource({}, &profile, xiiRenderPipelineUsage::EditorScene, resource, &error));
    XII_TEST_STRING(resource.GetResourceID(), config->m_sEditorScenePipeline);
    config->m_sEditorScenePipeline.Clear();
    XII_TEST_BOOL(xiiRenderPipelineProfileConfig::ResolveResource({}, &profile, xiiRenderPipelineUsage::EditorScene, resource, &error).Failed());
    XII_TEST_BOOL(!resource.IsValid() && !error.IsEmpty());
    XII_TEST_BOOL(xiiRenderPipelineProfileConfig::ResolveResource({}, nullptr, xiiRenderPipelineUsage::Scene, resource, &error).Failed());
    XII_TEST_BOOL(!resource.IsValid());
    XII_TEST_RESULT(xiiRenderPipelineProfileConfig::ResolveResource("Explicit.xiiBinRenderPipeline", nullptr, xiiRenderPipelineUsage::AssetPreview, resource, &error));
    XII_TEST_STRING(resource.GetResourceID(), "Explicit.xiiBinRenderPipeline");
    config->m_sEditorScenePipeline = "Project/Scene.xiiBinRenderPipeline";
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Project configuration uses the existing reflected profile serializer")
  {
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    xiiReflectionSerializer::WriteObjectToBinary(writer, profile.GetDynamicRTTI(), &profile);
    xiiMemoryStreamReader reader(&storage);
    const xiiRTTI*        type    = nullptr;
    auto*                 decoded = static_cast<xiiPlatformProfile*>(xiiReflectionSerializer::ReadObjectFromBinary(reader, type));
    if (!XII_TEST_BOOL(decoded && type == xiiGetStaticRTTI<xiiPlatformProfile>()))
      return;
    const auto* copied = decoded->GetTypeConfig<xiiRenderPipelineProfileConfig>();
    XII_TEST_BOOL(copied != nullptr);
    if (copied)
    {
      XII_TEST_STRING(copied->m_sScenePipeline, config->m_sScenePipeline);
      XII_TEST_STRING(copied->m_sEditorScenePipeline, config->m_sEditorScenePipeline);
      XII_TEST_STRING(copied->m_sCubeTexturePreviewPipeline, config->m_sCubeTexturePreviewPipeline);
    }
    type->GetAllocator()->Deallocate(decoded);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Runtime profile chunks preserve every resource policy reference")
  {
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    xiiChunkStreamWriter          chunks(writer);
    chunks.BeginStream(1);
    config->SaveRuntimeData(chunks);
    chunks.EndStream();
    xiiMemoryStreamReader reader(&storage);
    xiiChunkStreamReader  input(reader);
    input.BeginStream();
    xiiRenderPipelineProfileConfig copied;
    copied.LoadRuntimeData(input);
    input.EndStream();
    for (const auto usage : {xiiRenderPipelineUsage::Scene, xiiRenderPipelineUsage::EditorScene, xiiRenderPipelineUsage::AssetPreview, xiiRenderPipelineUsage::TexturePreview, xiiRenderPipelineUsage::CubeTexturePreview})
      XII_TEST_STRING(copied.GetResourceReference(usage), config->GetResourceReference(usage));
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Version one profile migration retains custom resources")
  {
    for (bool custom : {false, true})
    {
      xiiDefaultMemoryStreamStorage storage;
      xiiMemoryStreamWriter         writer(&storage);
      xiiChunkStreamWriter          chunks(writer);
      chunks.BeginStream(1);
      chunks.BeginChunk("RenderPipelineReferences", 1);
      chunks << xiiString(custom ? "Custom/Scene" : "") << xiiString() << xiiString();
      chunks << xiiString(custom ? "Custom/Texture" : "RenderPipelines/TexturePreview.xiiBinRenderPipeline");
      chunks << xiiString("RenderPipelines/TextureCubePreview.xiiBinRenderPipeline");
      chunks.EndChunk();
      chunks.EndStream();
      xiiMemoryStreamReader reader(&storage);
      xiiChunkStreamReader  input(reader);
      input.BeginStream();
      xiiRenderPipelineProfileConfig migrated;
      migrated.LoadRuntimeData(input);
      input.EndStream();
      XII_TEST_BOOL(migrated.IsRuntimeDataValid());
      XII_TEST_STRING(migrated.m_sScenePipeline, custom ? "Custom/Scene" : "RenderPipelines/Scene.xiiBinRenderPipeline");
      XII_TEST_STRING(migrated.m_sEditorScenePipeline, "RenderPipelines/Editor.xiiBinRenderPipeline");
      XII_TEST_STRING(migrated.m_sAssetPreviewPipeline, "RenderPipelines/Scene.xiiBinRenderPipeline");
      XII_TEST_STRING(migrated.m_sTexturePreviewPipeline, custom ? "Custom/Texture" : "");
      XII_TEST_BOOL(migrated.m_sCubeTexturePreviewPipeline.IsEmpty());

      xiiAbstractObjectGraph graph;
      auto*                  node = graph.AddNode(xiiUuid::MakeUuid(), "xiiRenderPipelineProfileConfig", 1);
      node->AddProperty("GamePipeline", xiiString(custom ? "Custom/Scene" : ""));
      node->AddProperty("EditorScenePipeline", xiiString());
      node->AddProperty("AssetPreviewPipeline", xiiString());
      node->AddProperty("TexturePreviewPipeline", xiiString(custom ? "Custom/Texture" : "RenderPipelines/TexturePreview.xiiBinRenderPipeline"));
      node->AddProperty("CubeTexturePreviewPipeline", xiiString("RenderPipelines/TextureCubePreview.xiiBinRenderPipeline"));
      xiiAbstractObjectGraph types;
      auto*                  version = types.AddNode(xiiUuid::MakeUuid(), "xiiReflectedTypeDescriptor", 1);
      version->AddProperty("TypeName", xiiString("xiiRenderPipelineProfileConfig"));
      version->AddProperty("ParentTypeName", xiiString("xiiProfileConfigData"));
      version->AddProperty("TypeVersion", 1U);
      xiiGraphVersioning::GetSingleton()->PatchGraph(&graph, &types);
      if (!XII_TEST_BOOL(node->FindProperty("ScenePipeline") != nullptr))
        return;
      XII_TEST_BOOL(node->FindProperty("GamePipeline") == nullptr);
      XII_TEST_STRING(node->FindProperty("ScenePipeline")->m_Value.Get<xiiString>(), migrated.m_sScenePipeline);
      XII_TEST_STRING(node->FindProperty("EditorScenePipeline")->m_Value.Get<xiiString>(), migrated.m_sEditorScenePipeline);
      XII_TEST_STRING(node->FindProperty("AssetPreviewPipeline")->m_Value.Get<xiiString>(), migrated.m_sAssetPreviewPipeline);
      XII_TEST_STRING(node->FindProperty("TexturePreviewPipeline")->m_Value.Get<xiiString>(), migrated.m_sTexturePreviewPipeline);
      XII_TEST_STRING(node->FindProperty("CubeTexturePreviewPipeline")->m_Value.Get<xiiString>(), migrated.m_sCubeTexturePreviewPipeline);
    }
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Unsupported and truncated runtime policy never silently select a default pipeline")
  {
    for (bool unsupported : {false, true})
    {
      xiiDefaultMemoryStreamStorage storage;
      xiiMemoryStreamWriter         writer(&storage);
      xiiChunkStreamWriter          chunks(writer);
      chunks.BeginStream(1);
      chunks.BeginChunk("RenderPipelineReferences", unsupported ? 3 : 2);
      chunks << xiiString("Only one reference");
      chunks.EndChunk();
      chunks.EndStream();
      xiiMemoryStreamReader reader(&storage);
      xiiChunkStreamReader  input(reader);
      input.BeginStream();
      xiiTestLogInterface   log;
      xiiTestLogSystemScope scope(&log, true);
      log.ExpectMessage(unsupported ? "Unsupported render pipeline profile version" : "Invalid render pipeline references", xiiLogMsgType::ErrorMsg);
      xiiRenderPipelineProfileConfig invalid;
      invalid.LoadRuntimeData(input);
      input.EndStream();
      XII_TEST_BOOL(!invalid.IsRuntimeDataValid());
      XII_TEST_BOOL(invalid.GetResourceReference(xiiRenderPipelineUsage::Scene).IsEmpty());
      XII_TEST_BOOL(invalid.GetResourceReference(xiiRenderPipelineUsage::AssetPreview).IsEmpty());
      // Re-saving a rejected runtime policy must not revive retained authored defaults.
      xiiDefaultMemoryStreamStorage saved;
      xiiMemoryStreamWriter         savedWriter(&saved);
      xiiChunkStreamWriter          savedChunks(savedWriter);
      savedChunks.BeginStream(1);
      invalid.SaveRuntimeData(savedChunks);
      savedChunks.EndStream();
      xiiMemoryStreamReader savedReader(&saved);
      xiiChunkStreamReader  savedInput(savedReader);
      savedInput.BeginStream();
      xiiRenderPipelineProfileConfig restored;
      restored.LoadRuntimeData(savedInput);
      savedInput.EndStream();
      for (const auto usage : {xiiRenderPipelineUsage::Scene, xiiRenderPipelineUsage::EditorScene, xiiRenderPipelineUsage::AssetPreview, xiiRenderPipelineUsage::TexturePreview, xiiRenderPipelineUsage::CubeTexturePreview})
        XII_TEST_BOOL(restored.GetResourceReference(usage).IsEmpty());
    }
  }
}
