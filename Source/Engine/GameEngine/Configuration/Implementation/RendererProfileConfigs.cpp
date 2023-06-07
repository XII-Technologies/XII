#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Configuration/RendererProfileConfigs.h>

#include <Foundation/IO/ChunkStream.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineProfileConfig, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineProfileConfig>)
{
  XII_BEGIN_PROPERTIES
  {
    // MainRenderPipeline.xiiRenderPipelineAsset
    XII_MEMBER_PROPERTY("MainRenderPipeline", m_sMainRenderPipeline)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_RenderPipeline"), new xiiDefaultValueAttribute(xiiStringView("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }"))),
    // EditorRenderPipeline.xiiRenderPipelineAsset
    //XII_MEMBER_PROPERTY("EditorRenderPipeline", m_sEditorRenderPipeline)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_RenderPipeline"), new xiiDefaultValueAttribute(xiiStringView("{ da463c4d-c984-4910-b0b7-a0b3891d0448 }"))),
    // DebugRenderPipeline.xiiRenderPipelineAsset
    //XII_MEMBER_PROPERTY("DebugRenderPipeline", m_sDebugRenderPipeline)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_RenderPipeline"), new xiiDefaultValueAttribute(xiiStringView("{ 0416eb3e-69c0-4640-be5b-77354e0e37d7 }"))),

    XII_MAP_MEMBER_PROPERTY("CameraPipelines", m_CameraPipelines)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_RenderPipeline")),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiRenderPipelineProfileConfig::SaveRuntimeData(xiiChunkStreamWriter& ref_stream) const
{
  ref_stream.BeginChunk("xiiRenderPipelineProfileConfig", 2);

  ref_stream << m_sMainRenderPipeline;

  ref_stream << m_CameraPipelines.GetCount();
  for (auto it = m_CameraPipelines.GetIterator(); it.IsValid(); ++it)
  {
    ref_stream << it.Key();
    ref_stream << it.Value();
  }

  ref_stream.EndChunk();
}

void xiiRenderPipelineProfileConfig::LoadRuntimeData(xiiChunkStreamReader& ref_stream)
{
  const auto& chunk = ref_stream.GetCurrentChunk();

  if (chunk.m_sChunkName == "xiiRenderPipelineProfileConfig" && chunk.m_uiChunkVersion == 2)
  {
    xiiRenderWorld::BeginModifyCameraConfigs();
    xiiRenderWorld::ClearCameraConfigs();

    ref_stream >> m_sMainRenderPipeline;

    m_CameraPipelines.Clear();

    xiiUInt32 uiNumCamPipes = 0;
    ref_stream >> uiNumCamPipes;
    for (xiiUInt32 i = 0; i < uiNumCamPipes; ++i)
    {
      xiiString sPipeName, sPipeAsset;

      ref_stream >> sPipeName;
      ref_stream >> sPipeAsset;

      m_CameraPipelines[sPipeName] = sPipeAsset;

      xiiRenderWorld::CameraConfig cfg;
      cfg.m_hRenderPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(sPipeAsset);

      xiiRenderWorld::SetCameraConfig(sPipeName, cfg);
    }

    xiiRenderWorld::EndModifyCameraConfigs();
  }
}



XII_STATICLINK_FILE(GameEngine, GameEngine_Configuration_Implementation_RendererProfileConfigs);
