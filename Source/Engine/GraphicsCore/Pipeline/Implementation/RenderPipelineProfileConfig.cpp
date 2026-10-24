/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

#include <Foundation/IO/ChunkStream.h>
#include <GraphicsCore/Pipeline/RenderPipelineProfileConfig.h>

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRenderPipelineUsage, 1)
  XII_ENUM_CONSTANTS(xiiRenderPipelineUsage::Scene, xiiRenderPipelineUsage::EditorScene, xiiRenderPipelineUsage::AssetPreview, xiiRenderPipelineUsage::TexturePreview, xiiRenderPipelineUsage::CubeTexturePreview)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineProfileConfig, 2, xiiRTTIDefaultAllocator<xiiRenderPipelineProfileConfig>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("ScenePipeline", m_sScenePipeline)->AddAttributes(new xiiDefaultValueAttribute("RenderPipelines/Scene.xiiBinRenderPipeline"), new xiiAssetBrowserAttribute("CompatibleAsset_RenderPipeline")),
      XII_MEMBER_PROPERTY("EditorScenePipeline", m_sEditorScenePipeline)->AddAttributes(new xiiDefaultValueAttribute("RenderPipelines/Editor.xiiBinRenderPipeline"), new xiiAssetBrowserAttribute("CompatibleAsset_RenderPipeline")),
      XII_MEMBER_PROPERTY("AssetPreviewPipeline", m_sAssetPreviewPipeline)->AddAttributes(new xiiDefaultValueAttribute("RenderPipelines/Scene.xiiBinRenderPipeline"), new xiiAssetBrowserAttribute("CompatibleAsset_RenderPipeline")),
      XII_MEMBER_PROPERTY("TexturePreviewPipeline", m_sTexturePreviewPipeline)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_RenderPipeline")),
      XII_MEMBER_PROPERTY("CubeTexturePreviewPipeline", m_sCubeTexturePreviewPipeline)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_RenderPipeline")),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiRenderPipelineProfileConfig::SaveRuntimeData(xiiChunkStreamWriter& stream) const
{
  stream.BeginChunk("RenderPipelineReferences", 2);
  for (const auto usage : {xiiRenderPipelineUsage::Scene, xiiRenderPipelineUsage::EditorScene, xiiRenderPipelineUsage::AssetPreview, xiiRenderPipelineUsage::TexturePreview, xiiRenderPipelineUsage::CubeTexturePreview})
    stream << GetResourceReference(usage);
  stream.EndChunk();
}

void xiiRenderPipelineProfileConfig::LoadRuntimeData(xiiChunkStreamReader& stream)
{
  const auto& chunk = stream.GetCurrentChunk();
  if (chunk.m_sChunkName != "RenderPipelineReferences")
    return;
  m_bRuntimeDataValid = false;
  if (chunk.m_uiChunkVersion != 1 && chunk.m_uiChunkVersion != 2)
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
  if (chunk.m_uiChunkVersion == 1)
  {
    if (references[0].IsEmpty())
      references[0] = "RenderPipelines/Scene.xiiBinRenderPipeline";
    if (references[1].IsEmpty())
      references[1] = "RenderPipelines/Editor.xiiBinRenderPipeline";
    if (references[2].IsEmpty())
      references[2] = "RenderPipelines/Scene.xiiBinRenderPipeline";
    if (references[3] == "RenderPipelines/TexturePreview.xiiBinRenderPipeline")
      references[3].Clear();
    if (references[4] == "RenderPipelines/TextureCubePreview.xiiBinRenderPipeline")
      references[4].Clear();
  }
  m_sScenePipeline              = references[0];
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
    case xiiRenderPipelineUsage::Scene:
      return m_sScenePipeline;
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

xiiResult xiiRenderPipelineProfileConfig::ResolveResource(xiiStringView explicitReference, const xiiPlatformProfile* profile, xiiRenderPipelineUsage::Enum usage, xiiRenderPipelineResourceHandle& out_resource, xiiStringBuilder* out_error)
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

namespace
{
  class xiiRenderPipelineProfileConfigPatch_1_2 : public xiiGraphPatch
  {
  public:
    xiiRenderPipelineProfileConfigPatch_1_2() : xiiGraphPatch("xiiRenderPipelineProfileConfig", 2) {}
    void Patch(xiiGraphPatchContext&, xiiAbstractObjectGraph*, xiiAbstractObjectNode* node) const override
    {
      node->RenameProperty("GamePipeline", "ScenePipeline");
      for (const auto property : {"ScenePipeline", "EditorScenePipeline", "AssetPreviewPipeline"})
        if (const auto* value = node->FindProperty(property))
          if (value->m_Value.IsA<xiiString>() && value->m_Value.Get<xiiString>().IsEmpty())
            node->ChangeProperty(property, xiiString(property == xiiStringView("EditorScenePipeline") ? "RenderPipelines/Editor.xiiBinRenderPipeline" : "RenderPipelines/Scene.xiiBinRenderPipeline"));
      for (const auto property : {"TexturePreviewPipeline", "CubeTexturePreviewPipeline"})
        if (const auto* value = node->FindProperty(property))
          if (value->m_Value == xiiString("RenderPipelines/TexturePreview.xiiBinRenderPipeline") || value->m_Value == xiiString("RenderPipelines/TextureCubePreview.xiiBinRenderPipeline"))
            node->ChangeProperty(property, xiiString());
    }
  } g_RenderPipelineProfileConfigPatch_1_2;
} // namespace
