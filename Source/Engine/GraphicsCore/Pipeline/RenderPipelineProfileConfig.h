/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

struct xiiRenderPipelineUsage
{
  using StorageType = xiiUInt8;
  enum Enum
  {
    Game,
    EditorScene,
    AssetPreview,
    TexturePreview,
    CubeTexturePreview,
    Default = Game
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineUsage);

/// Project/platform policy selects resource references, never embeds a rendering stack.
/// The existing platform profile owns, edits and persists this configuration.
class XII_GRAPHICSCORE_DLL xiiRenderPipelineProfileConfig : public xiiProfileConfigData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineProfileConfig, xiiProfileConfigData);

public:
  xiiString m_sGamePipeline;
  xiiString m_sEditorScenePipeline;
  xiiString m_sAssetPreviewPipeline;
  xiiString m_sTexturePreviewPipeline     = "RenderPipelines/TexturePreview.xiiBinRenderPipeline";
  xiiString m_sCubeTexturePreviewPipeline = "RenderPipelines/TextureCubePreview.xiiBinRenderPipeline";

  void          SaveRuntimeData(xiiChunkStreamWriter& stream) const override;
  void          LoadRuntimeData(xiiChunkStreamReader& stream) override;
  xiiStringView GetResourceReference(xiiRenderPipelineUsage::Enum usage) const;

  /// Explicit view references take precedence over the selected project policy. Empty
  /// selection fails and clears the handle; loading/compilation validates the chosen resource.
  /// No default implementation or fallback graph is created by this operation.
  static xiiResult ResolveResource(xiiStringView explicitReference, const xiiPlatformProfile* profile,
                                   xiiRenderPipelineUsage::Enum usage, xiiRenderPipelineResourceHandle& out_resource,
                                   xiiStringBuilder* out_error = nullptr);

private:
  bool m_bRuntimeDataValid = true;
};
