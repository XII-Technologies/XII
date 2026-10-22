/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Publishes existing device page residency through explicit graph outputs.
/// Records coalesced immutable upload deltas; owns no world/light/page lifecycle.
class XII_GRAPHICSCORE_DLL xiiVirtualShadowTablesPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVirtualShadowTablesPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin        m_Before;
  xiiRenderPipelineOrderOutputPin       m_After;
  xiiRenderPipelineBufferOutputPin      m_Physical{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin      m_Virtual{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineTextureOutputPin     m_Atlas{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineDataOutputPin        m_Residency{xiiGetStaticRTTI<xiiVirtualShadowResidencySnapshot>()};
  xiiResult                             Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineFeature> GetRuntimeFeatures() const override { return xiiRenderPipelineFeature::VirtualShadows; }
  xiiUInt64                             GetUploadBytes() const { return m_uiUploadBytes; }
  xiiUInt32                             GetUploadRanges() const { return m_uiUploadRanges; }

private:
  xiiUInt64 m_uiUploadBytes  = 0;
  xiiUInt32 m_uiUploadRanges = 0;
};
