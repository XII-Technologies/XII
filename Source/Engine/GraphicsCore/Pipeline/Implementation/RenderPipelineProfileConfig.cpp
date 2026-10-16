/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/ChunkStream.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/RenderPipelineProfileConfig.h>

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRenderPipelineUsage, 1)
  XII_ENUM_CONSTANTS(xiiRenderPipelineUsage::Game, xiiRenderPipelineUsage::EditorScene, xiiRenderPipelineUsage::AssetPreview, xiiRenderPipelineUsage::TexturePreview, xiiRenderPipelineUsage::CubeTexturePreview)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineProfileConfig, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineProfileConfig>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("GamePipeline", m_sGamePipeline)->AddAttributes(new xiiDefaultValueAttribute("RenderPipelines/Deferred.xiiBinRenderPipeline"), new xiiAssetBrowserAttribute("CompatibleAsset_RenderPipeline")),
      XII_MEMBER_PROPERTY("EditorScenePipeline", m_sEditorScenePipeline)->AddAttributes(new xiiDefaultValueAttribute("RenderPipelines/EditorScene.xiiBinRenderPipeline"), new xiiAssetBrowserAttribute("CompatibleAsset_RenderPipeline")),
      XII_MEMBER_PROPERTY("AssetPreviewPipeline", m_sAssetPreviewPipeline)->AddAttributes(new xiiDefaultValueAttribute("RenderPipelines/Deferred.xiiBinRenderPipeline"), new xiiAssetBrowserAttribute("CompatibleAsset_RenderPipeline")),
      XII_MEMBER_PROPERTY("TexturePreviewPipeline", m_sTexturePreviewPipeline)->AddAttributes(new xiiDefaultValueAttribute("RenderPipelines/TexturePreview.xiiBinRenderPipeline"), new xiiAssetBrowserAttribute("CompatibleAsset_RenderPipeline")),
      XII_MEMBER_PROPERTY("CubeTexturePreviewPipeline", m_sCubeTexturePreviewPipeline)->AddAttributes(new xiiDefaultValueAttribute("RenderPipelines/TextureCubePreview.xiiBinRenderPipeline"), new xiiAssetBrowserAttribute("CompatibleAsset_RenderPipeline")),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiRenderPipelineProfileConfig::SaveRuntimeData(xiiChunkStreamWriter& stream) const
{
  stream.BeginChunk("RenderPipelineReferences", 1);
  stream << m_sGamePipeline << m_sEditorScenePipeline << m_sAssetPreviewPipeline << m_sTexturePreviewPipeline << m_sCubeTexturePreviewPipeline;
  stream.EndChunk();
}

void xiiRenderPipelineProfileConfig::LoadRuntimeData(xiiChunkStreamReader& stream)
{
  const auto& chunk = stream.GetCurrentChunk();
  if (chunk.m_sChunkName != "RenderPipelineReferences")
    return;
  m_bRuntimeDataValid = false;
  if (chunk.m_uiChunkVersion != 1)
  {
    xiiLog::Error("Unsupported render pipeline profile version {}.", chunk.m_uiChunkVersion);
    return;
  }
  xiiString references[5];
  for (auto& reference : references)
    if (stream.ReadString(reference).Failed())
    {
      xiiLog::Error("Invalid render pipeline references in the platform profile.");
      return;
    }
  m_sGamePipeline               = references[0];
  m_sEditorScenePipeline        = references[1];
  m_sAssetPreviewPipeline       = references[2];
  m_sTexturePreviewPipeline     = references[3];
  m_sCubeTexturePreviewPipeline = references[4];
  m_bRuntimeDataValid           = true;
}

xiiStringView xiiRenderPipelineProfileConfig::GetResourceReference(xiiRenderPipelineUsage::Enum usage) const
{
  if (!m_bRuntimeDataValid)
    return {};
  switch (usage)
  {
    case xiiRenderPipelineUsage::Game:
      return m_sGamePipeline;
    case xiiRenderPipelineUsage::EditorScene:
      return m_sEditorScenePipeline;
    case xiiRenderPipelineUsage::AssetPreview:
      return m_sAssetPreviewPipeline;
    case xiiRenderPipelineUsage::TexturePreview:
      return m_sTexturePreviewPipeline;
    case xiiRenderPipelineUsage::CubeTexturePreview:
      return m_sCubeTexturePreviewPipeline;
    default:
      return {};
  }
}

xiiResult xiiRenderPipelineProfileConfig::ResolveResource(xiiStringView explicitReference, const xiiPlatformProfile* profile,
                                                          xiiRenderPipelineUsage::Enum usage, xiiRenderPipelineResourceHandle& out_resource,
                                                          xiiStringBuilder* out_error)
{
  if (out_error)
    out_error->Clear();
  xiiStringView reference = explicitReference;
  if (reference.IsEmpty() && profile)
    if (const auto* config = profile->GetTypeConfig<xiiRenderPipelineProfileConfig>())
      reference = config->GetResourceReference(usage);
  if (reference.IsEmpty())
  {
    out_resource.Invalidate();
    if (out_error)
      out_error->Set("No render pipeline resource is configured for this view's project policy.");
    return XII_FAILURE;
  }
  out_resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(reference);
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelineProfileConfig);
