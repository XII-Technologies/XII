/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

#include <Foundation/IO/ChunkStream.h>
#include <GraphicsCore/Pipeline/RenderPipelineProfileConfig.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineProfileConfig, 3, xiiRTTIDefaultAllocator<xiiRenderPipelineProfileConfig>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("MainRenderPipeline", m_sMainRenderPipeline)->AddAttributes(new xiiDefaultValueAttribute(xiiRenderPipelineProfileConfig::s_szScenePipeline), new xiiAssetBrowserAttribute("CompatibleAsset_RenderPipeline")),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiRenderPipelineProfileConfig::SaveRuntimeData(xiiChunkStreamWriter& stream) const
{
  stream.BeginChunk("RenderPipelineReferences", 3);
  stream << GetResourceReference();
  stream.EndChunk();
}

void xiiRenderPipelineProfileConfig::LoadRuntimeData(xiiChunkStreamReader& stream)
{
  const auto& chunk = stream.GetCurrentChunk();
  if (chunk.m_sChunkName != "RenderPipelineReferences")
    return;
  m_bRuntimeDataValid = false;
  if (chunk.m_uiChunkVersion < 1 || chunk.m_uiChunkVersion > 3)
  {
    xiiLog::Error("Unsupported render pipeline profile version {}.", chunk.m_uiChunkVersion);
    return;
  }
  xiiString references[5];
  for (xiiUInt32 index = 0; index < (chunk.m_uiChunkVersion == 3 ? 1U : 5U); ++index)
    if (stream.ReadString(references[index]).Failed())
    {
      xiiLog::Error("Invalid render pipeline references in the platform profile.");
      return;
    }
  // Version one had an empty default world selection. Later versions preserve an explicit empty choice.
  if (chunk.m_uiChunkVersion == 1 && references[0].IsEmpty())
    references[0] = s_szScenePipeline;
  m_sMainRenderPipeline = references[0];
  m_bRuntimeDataValid   = true;
}

xiiStringView xiiRenderPipelineProfileConfig::GetResourceReference() const
{
  return m_bRuntimeDataValid ? m_sMainRenderPipeline.GetView() : xiiStringView();
}

xiiResult xiiRenderPipelineProfileConfig::ResolveResource(xiiStringView explicitReference, const xiiPlatformProfile* profile, xiiRenderPipelineResourceHandle& out_resource, xiiStringBuilder* out_error)
{
  if (out_error)
    out_error->Clear();
  xiiStringView reference = explicitReference;
  if (reference.IsEmpty() && profile)
    if (const auto* config = profile->GetTypeConfig<xiiRenderPipelineProfileConfig>())
      reference = config->GetResourceReference();
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
  class xiiRenderPipelineProfileConfigPatch_3 : public xiiGraphPatch
  {
  public:
    xiiRenderPipelineProfileConfigPatch_3() : xiiGraphPatch("xiiRenderPipelineProfileConfig", 3) {}
    void Patch(xiiGraphPatchContext&, xiiAbstractObjectGraph*, xiiAbstractObjectNode* node) const override
    {
      node->RenameProperty(node->FindProperty("ScenePipeline") ? "ScenePipeline" : "GamePipeline", "MainRenderPipeline");
      if (node->GetTypeVersion() == 1)
        if (const auto* value = node->FindProperty("MainRenderPipeline"))
          if (value->m_Value.IsA<xiiString>() && value->m_Value.Get<xiiString>().IsEmpty())
            node->ChangeProperty("MainRenderPipeline", xiiString(xiiRenderPipelineProfileConfig::s_szScenePipeline));
      for (const auto property : {"EditorScenePipeline", "AssetPreviewPipeline", "TexturePreviewPipeline", "CubeTexturePreviewPipeline"})
        node->RemoveProperty(property);
    }
  } g_RenderPipelineProfileConfigPatch_3;
} // namespace
