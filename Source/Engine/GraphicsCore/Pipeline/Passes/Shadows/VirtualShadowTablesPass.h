/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Frame-local GPU bindings and addressing, never a scene manipulation interface.
struct xiiVirtualShadowTableBindings : xiiVirtualShadowGpuLayout
{
  xiiRenderGraphBufferHandle  m_hPhysicalPageTable, m_hVirtualPageTable;
  xiiRenderGraphTextureHandle m_hPhysicalAtlas;
};

/// Publishes existing device page residency through explicit graph outputs.
/// Records coalesced immutable upload deltas; owns no world/light/page lifecycle.
class XII_GRAPHICSCORE_DLL xiiVirtualShadowTablesPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVirtualShadowTablesPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin      m_Physical{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin      m_Virtual{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineTextureOutputPin     m_Atlas{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiResult                             Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineFeature> GetRuntimeFeatures() const override { return xiiRenderPipelineFeature::VirtualShadows; }
  /// Setup-scoped addressing for explicit CPU sparse-page clients.
  const xiiVirtualShadowTableBindings* GetSnapshot(const xiiRenderGraph& graph) const;
  xiiUInt64                            GetUploadBytes() const { return m_uiUploadBytes; }
  xiiUInt32                            GetUploadRanges() const { return m_uiUploadRanges; }

private:
  xiiVirtualShadowTableBindings m_Snapshot;
  xiiUInt64                     m_uiSetupSerial = 0, m_uiConfigurationRevision = 0, m_uiUploadBytes = 0;
  xiiUInt32                     m_uiUploadRanges = 0;
};
