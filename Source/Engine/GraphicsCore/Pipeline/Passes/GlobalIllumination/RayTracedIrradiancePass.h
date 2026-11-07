/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsFoundation/Resources/Sampler.h>

/// Incident irradiance transported from material emission and analytic lighting through bounded diffuse paths.
/// Environment misses are zero; the ordinary lighting consumer retains its sky contribution.
class XII_GRAPHICSCORE_DLL xiiRayTracedIrradiancePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRayTracedIrradiancePass, xiiRenderPipelinePass);

public:
  xiiRayTracedIrradiancePass();
  xiiRenderPipelineTextureInputPin               m_Depth{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin               m_Normal{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferInputPin                m_Camera{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineAccelerationStructureInputPin m_Scene{true};
  xiiRenderPipelineBufferInputPin                m_Materials{true, xiiGALBindFlags::ShaderResource}, m_Geometry{true, xiiGALBindFlags::ShaderResource};
  /// Optional while the depth gate is absent; every active trace validates all light streams.
  xiiRenderPipelineBufferInputPin   m_Lights{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_LightCounts{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_LightCandidates{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_IESProfiles{true, xiiGALBindFlags::ShaderResource};
  /// Native emissive tables must come from the same RayTracingScenePass as Scene and Geometry.
  xiiRenderPipelineBufferInputPin   m_EmissiveTriangles{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_EmissiveHeader{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_TriangleAliases{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_BlockAliases{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_AliasHeader{true, xiiGALBindFlags::ShaderResource};
  xiiUInt32                         m_uiEmissiveLightSamples = 0;
  xiiRenderPipelineTextureOutputPin m_Irradiance{xiiGALResourceFormat::RGBA32Float, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiUInt32                         m_uiSampleCount = 8, m_uiMaximumHits = 2, m_uiSampleOffset = 0;
  xiiUInt32                         m_uiAnalyticLightSamples = 4;
  bool                              m_bPowerWeightedLights   = true;
  float                             m_fRayLength = 20, m_fSurfaceBias = 0.02f;
  /// Explicitly permit absence so an authored graph can choose its own fallback.
  bool                                        m_bAllowUnsupportedHardware = false, m_bAsyncCompute = true;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiShaderPermutationResourceHandle       m_Shader[2];
  xiiSharedPtr<xiiGALSampler>        m_Sampler;
  xiiSharedPtr<xiiGALComputePipelineState> m_LightScanPipeline, m_BlockScanPipeline;
};
