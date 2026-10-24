/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/Configuration/PlatformProfile.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

struct xiiRenderPipelineUsage
{
  using StorageType = xiiUInt8;
  enum Enum
  {
    Scene,
    EditorScene,
    AssetPreview,
    TexturePreview,
    CubeTexturePreview,
    Default = Scene
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineUsage);

/// Project/platform policy selects resource references, never embeds a rendering stack.
/// The existing platform profile owns, edits and persists this configuration.
class XII_GRAPHICSCORE_DLL xiiRenderPipelineProfileConfig : public xiiProfileConfigData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineProfileConfig, xiiProfileConfigData);

public:
  static constexpr const char* s_szScenePipeline  = "{ d83bb009-7ef8-315a-1f96-12e7df0c90f7 }";
  static constexpr const char* s_szEditorPipeline = "{ 7e79df72-9ebd-674a-bcd5-ca4405549c82 }";

  xiiString m_sScenePipeline        = s_szScenePipeline;
  xiiString m_sEditorScenePipeline  = s_szEditorPipeline;
  xiiString m_sAssetPreviewPipeline = s_szScenePipeline;
  xiiString m_sTexturePreviewPipeline;
  xiiString m_sCubeTexturePreviewPipeline;

  void          SaveRuntimeData(xiiChunkStreamWriter& stream) const override;
  void          LoadRuntimeData(xiiChunkStreamReader& stream) override;
  bool          IsRuntimeDataValid() const { return m_bRuntimeDataValid; }
  xiiStringView GetResourceReference(xiiRenderPipelineUsage::Enum usage) const;

  /// Explicit view references take precedence over the selected project policy. Empty
  /// selection fails and clears the handle; loading/compilation validates the chosen resource.
  /// No default implementation or fallback graph is created by this operation.
  static xiiResult ResolveResource(xiiStringView explicitReference, const xiiPlatformProfile* profile, xiiRenderPipelineUsage::Enum usage, xiiRenderPipelineResourceHandle& out_resource, xiiStringBuilder* out_error = nullptr);

private:
  bool m_bRuntimeDataValid = true;
};
