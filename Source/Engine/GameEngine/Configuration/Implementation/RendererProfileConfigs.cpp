#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Configuration/RendererProfileConfigs.h>

#include <Foundation/IO/ChunkStream.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

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

void xiiRenderPipelineProfileConfig::SaveRuntimeData(xiiChunkStreamWriter& inout_stream) const
{
  inout_stream.BeginChunk("xiiRenderPipelineProfileConfig", 2);

  inout_stream << m_sMainRenderPipeline;

  inout_stream << m_CameraPipelines.GetCount();
  for (auto it = m_CameraPipelines.GetIterator(); it.IsValid(); ++it)
  {
    inout_stream << it.Key();
    inout_stream << it.Value();
  }

  inout_stream.EndChunk();
}

void xiiRenderPipelineProfileConfig::LoadRuntimeData(xiiChunkStreamReader& inout_stream)
{
  const auto& chunk = inout_stream.GetCurrentChunk();

  if (chunk.m_sChunkName == "xiiRenderPipelineProfileConfig" && chunk.m_uiChunkVersion == 2)
  {
    xiiRenderWorld::BeginModifyCameraConfigs();
    xiiRenderWorld::ClearCameraConfigs();

    inout_stream >> m_sMainRenderPipeline;

    m_CameraPipelines.Clear();

    xiiUInt32 uiNumCamPipes = 0;
    inout_stream >> uiNumCamPipes;
    for (xiiUInt32 i = 0; i < uiNumCamPipes; ++i)
    {
      xiiString sPipeName, sPipeAsset;

      inout_stream >> sPipeName;
      inout_stream >> sPipeAsset;

      m_CameraPipelines[sPipeName] = sPipeAsset;

      xiiRenderWorld::CameraConfig cfg;
      cfg.m_hRenderPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(sPipeAsset);

      xiiRenderWorld::SetCameraConfig(sPipeName, cfg);
    }

    xiiRenderWorld::EndModifyCameraConfigs();
  }
}

XII_STATICLINK_FILE(GameEngine, GameEngine_Configuration_Implementation_RendererProfileConfigs);
