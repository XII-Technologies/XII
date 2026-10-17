/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Publishes clean page records after an explicitly connected final raster atlas.
/// Completion fences retire device metadata; no scene or pass lifecycle is owned here.
class XII_GRAPHICSCORE_DLL xiiVirtualShadowPagePublishPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVirtualShadowPagePublishPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin         m_Pages{xiiGetStaticRTTI<xiiVirtualShadowRasterBatch>()};
  xiiRenderPipelineTextureInputPin      m_Atlas{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferInputPin       m_Physical{false, xiiGALBindFlags::ShaderResource}, m_Virtual{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineTextureOutputPin     m_ReadyAtlas{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineBufferOutputPin      m_ReadyPhysical{false, xiiGALBindFlags::ShaderResource}, m_ReadyVirtual{false, xiiGALBindFlags::ShaderResource};
  xiiResult                             Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineFeature> GetRuntimeFeatures() const override { return xiiRenderPipelineFeature::VirtualShadows; }
};
