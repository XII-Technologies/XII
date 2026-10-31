/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/MaterialRasterBatches.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterFrameData.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/States/RasterizerState.h>

/// Lit transparent rasterization. Culling, indirect commands and HDR composition remain separate operations.
class XII_GRAPHICSCORE_DLL xiiTransparentRasterPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTransparentRasterPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin   m_VisibilityView{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_SceneInstances{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_SkinningMatrices{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_VisibleMeshlets{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_GeometryMetadata{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_MeshletMetadata{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_SurfaceMaterials{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_RasterConstants{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin   m_DrawCommands{true, xiiGALBindFlags::IndirectDrawArguments};
  xiiRenderPipelineBufferInputPin   m_DrawCommandCount{true, xiiGALBindFlags::IndirectDrawArguments};
  xiiRenderPipelineBufferInputPin   m_Camera{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin   m_Environment{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin   m_Lights{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_LightCounts{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_LightCandidates{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_IESProfiles{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_LightGrid{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_LightIndices{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_LightOverflow{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineTextureInputPin  m_Depth{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::DepthStencil, xiiGALResourceDimension::Texture2D, 1};
  /// Completed opaque/atmosphere/reflection color, before any transparent layers are resolved.
  xiiRenderPipelineTextureInputPin  m_BackgroundRadiance{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineDataInputPin     m_LightLayout{xiiGetStaticRTTI<xiiLightClusterFrameData>()};
  xiiRenderPipelineDataInputPin     m_Cascades{xiiGetStaticRTTI<xiiShadowCascadeSnapshot>(), true};
  xiiRenderPipelineTextureInputPin  m_ShadowAtlas{xiiGALResourceFormat::D32Float, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineDataInputPin     m_Spots{xiiGetStaticRTTI<xiiSpotShadowSnapshot>(), true};
  xiiRenderPipelineTextureInputPin  m_SpotAtlas{xiiGALResourceFormat::D32Float, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferInputPin   m_Atmosphere{true, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineTextureInputPin  m_Transmittance{xiiGALResourceFormat::RGBA32Float, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_MultiScatter{xiiGALResourceFormat::RGBA32Float, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_Accumulation{xiiGALResourceFormat::RGBA32Float, true, xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_Revealage{xiiGALResourceFormat::R32Float, true, xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_Additive{xiiGALResourceFormat::RGBA32Float, true, xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_Modulation{xiiGALResourceFormat::RGBA32Float, true, xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_HistoryTrust{xiiGALResourceFormat::R32Float, true, xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};

  float                   m_fLightingScale = 1, m_fWeightDepthScale = 5, m_fMaximumWeight = 3000;
  float                   m_fDepthBias = 0.0005f, m_fNormalBias = 0.02f, m_fNormalBiasTexels = 1.5f;
  xiiEnum<xiiGALFillMode> m_FillMode = xiiGALFillMode::Solid;
  bool                    m_bEnableRefraction             = false;
  float                   m_fRefractionDistance           = 100.0f;
  float                   m_fRefractionThicknessTolerance = 0.1f;
  xiiUInt32               m_uiRefractionSteps             = 32;

  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::BindlessResources; }
  xiiBitflags<xiiRenderExtractionCategory>    GetExtractionRequirements() const override { return xiiRenderExtractionCategory::Geometry; }
  xiiBitflags<xiiRenderPipelineFeature>       GetRuntimeFeatures() const override { return xiiRenderPipelineFeature::GpuScene; }

private:
  xiiMaterialRasterBatches            m_MaterialBatches;
  xiiShaderPermutationResourceHandle  m_hShaders[16];
  xiiSharedPtr<xiiGALSampler>         m_MaterialSampler, m_AtmosphereSampler;
  xiiSharedPtr<xiiGALSampler>         m_RefractionSampler;
  xiiSharedPtr<xiiGALRasterizerState> m_Rasterizer;
};
