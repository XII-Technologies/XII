/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Resolves a published cascade atlas into per-surface directional-light visibility.
class XII_GRAPHICSCORE_DLL xiiDirectionalShadowVisibilityPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDirectionalShadowVisibilityPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin               m_Cascades{xiiGetStaticRTTI<xiiShadowCascadeSnapshot>(), true};
  xiiRenderPipelineTextureInputPin            m_Atlas{xiiGALResourceFormat::D32Float, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_Depth{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_Normal{xiiGALResourceFormat::RG16SNormalized, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_ObjectId{xiiGALResourceFormat::R32UInt, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferInputPin             m_Camera{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineTextureOutputPin           m_Visibility{xiiGALResourceFormat::R32Float, true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  float                                       m_fDepthBias  = 0.0005f;
  float                                       m_fNormalBias = 0.02f;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderExtractionCategory>    GetExtractionRequirements() const override { return xiiRenderExtractionCategory::Geometry; }
  xiiBitflags<xiiRenderPipelineFeature>       GetRuntimeFeatures() const override { return xiiRenderPipelineFeature::GpuScene; }
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
