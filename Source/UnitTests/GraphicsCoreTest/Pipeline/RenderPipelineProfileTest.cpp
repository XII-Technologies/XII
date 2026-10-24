/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/DdlSerializer.h>
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
  XII_TEST_STRING(config->m_sMainRenderPipeline, xiiRenderPipelineProfileConfig::s_szScenePipeline);
  XII_TEST_INT(config->GetDynamicRTTI()->GetProperties().GetCount(), 1);
  XII_TEST_STRING(config->GetDynamicRTTI()->GetProperties()[0]->GetPropertyName(), "MainRenderPipeline");
  config->m_sMainRenderPipeline = "Project/Simulation.xiiBinRenderPipeline";

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Explicit view resource precedes project policy without graph construction")
  {
    xiiRenderPipelineResourceHandle resource;
    xiiStringBuilder                error;
    XII_TEST_RESULT(xiiRenderPipelineProfileConfig::ResolveResource({}, &profile, resource, &error));
    XII_TEST_STRING(resource.GetResourceID(), config->m_sMainRenderPipeline);
    XII_TEST_RESULT(xiiRenderPipelineProfileConfig::ResolveResource("Cameras/Depth.xiiBinRenderPipeline", &profile, resource, &error));
    XII_TEST_STRING(resource.GetResourceID(), "Cameras/Depth.xiiBinRenderPipeline");
    XII_TEST_STRING(config->m_sMainRenderPipeline, "Project/Simulation.xiiBinRenderPipeline");
    config->m_sMainRenderPipeline.Clear();
    XII_TEST_BOOL(xiiRenderPipelineProfileConfig::ResolveResource({}, &profile, resource, &error).Failed());
    XII_TEST_BOOL(!resource.IsValid() && !error.IsEmpty());
    XII_TEST_BOOL(xiiRenderPipelineProfileConfig::ResolveResource({}, nullptr, resource, &error).Failed());
    XII_TEST_RESULT(xiiRenderPipelineProfileConfig::ResolveResource("Explicit.xiiBinRenderPipeline", nullptr, resource, &error));
    XII_TEST_STRING(resource.GetResourceID(), "Explicit.xiiBinRenderPipeline");
    config->m_sMainRenderPipeline = "Project/Simulation.xiiBinRenderPipeline";
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
      XII_TEST_STRING(copied->m_sMainRenderPipeline, config->m_sMainRenderPipeline);
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
    XII_TEST_STRING(copied.GetResourceReference(), config->GetResourceReference());
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Legacy profiles retain only their main world pipeline")
  {
    for (xiiUInt32 version : {1U, 2U})
      for (bool custom : {false, true})
      {
        const xiiString               expected = custom ? "Custom/Main" : version == 1 ? xiiRenderPipelineProfileConfig::s_szScenePipeline :
                                                                                         "";
        xiiDefaultMemoryStreamStorage storage;
        xiiMemoryStreamWriter         writer(&storage);
        xiiChunkStreamWriter          chunks(writer);
        chunks.BeginStream(1);
        chunks.BeginChunk("RenderPipelineReferences", version);
        chunks << xiiString(custom ? "Custom/Main" : "");
        for (xiiUInt32 index = 0; index < 4; ++index)
          chunks << xiiString("Discarded/UtilityPipeline");
        chunks.EndChunk();
        chunks.EndStream();
        xiiMemoryStreamReader reader(&storage);
        xiiChunkStreamReader  input(reader);
        input.BeginStream();
        xiiRenderPipelineProfileConfig migrated;
        migrated.LoadRuntimeData(input);
        input.EndStream();
        XII_TEST_BOOL(migrated.IsRuntimeDataValid());
        XII_TEST_STRING(migrated.GetResourceReference(), expected);

        xiiAbstractObjectGraph graph;
        auto*                  node = graph.AddNode(xiiUuid::MakeUuid(), "xiiRenderPipelineProfileConfig", version);
        node->AddProperty(version == 1 ? "GamePipeline" : "ScenePipeline", xiiString(custom ? "Custom/Main" : ""));
        for (const auto property : {"EditorScenePipeline", "AssetPreviewPipeline", "TexturePreviewPipeline", "CubeTexturePreviewPipeline"})
          node->AddProperty(property, xiiString("Discarded/UtilityPipeline"));
        xiiAbstractObjectGraph types;
        auto*                  type = types.AddNode(xiiUuid::MakeUuid(), "xiiReflectedTypeDescriptor", 1);
        type->AddProperty("TypeName", xiiString("xiiRenderPipelineProfileConfig"));
        type->AddProperty("ParentTypeName", xiiString("xiiProfileConfigData"));
        type->AddProperty("TypeVersion", version);
        xiiGraphVersioning::GetSingleton()->PatchGraph(&graph, &types);
        if (!XII_TEST_BOOL(node->FindProperty("MainRenderPipeline") != nullptr))
          return;
        XII_TEST_INT(node->GetTypeVersion(), 3);
        XII_TEST_INT(node->GetProperties().GetCount(), 1);
        XII_TEST_STRING(node->FindProperty("MainRenderPipeline")->m_Value.Get<xiiString>(), expected);
      }
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Saved profile DDL migrates without a Types table")
  {
    for (xiiUInt32 version : {1U, 2U})
      for (bool hasMain : {false, true})
      {
        xiiAbstractObjectGraph graph;
        const auto             id   = xiiUuid::MakeUuid();
        auto*                  node = graph.AddNode(id, "xiiRenderPipelineProfileConfig", version);
        node->AddProperty(version == 1 ? "GamePipeline" : "ScenePipeline", xiiString("Custom/OldWorld"));
        if (hasMain)
          node->AddProperty("MainRenderPipeline", xiiString("Custom/ExistingMain"));
        node->AddProperty("AssetPreviewPipeline", xiiString("Discarded/Preview"));
        xiiDefaultMemoryStreamStorage storage;
        xiiMemoryStreamWriter         writer(&storage);
        xiiAbstractGraphDdlSerializer::Write(writer, &graph); // Same Objects-only form as AssetProfiles.ddl.
        xiiMemoryStreamReader  reader(&storage);
        xiiAbstractObjectGraph restored;
        if (!XII_TEST_RESULT(xiiAbstractGraphDdlSerializer::Read(reader, &restored)))
          return;
        const auto* migrated = restored.GetNode(id);
        if (!XII_TEST_BOOL(migrated && migrated->FindProperty("MainRenderPipeline")))
          return;
        XII_TEST_INT(migrated->GetTypeVersion(), 3);
        XII_TEST_INT(migrated->GetProperties().GetCount(), 1);
        XII_TEST_STRING(migrated->FindProperty("MainRenderPipeline")->m_Value.Get<xiiString>(), hasMain ? "Custom/ExistingMain" : "Custom/OldWorld");
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
      chunks.BeginChunk("RenderPipelineReferences", unsupported ? 4 : 3);
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
      XII_TEST_BOOL(invalid.GetResourceReference().IsEmpty());
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
      XII_TEST_BOOL(restored.GetResourceReference().IsEmpty());
    }
  }
}
