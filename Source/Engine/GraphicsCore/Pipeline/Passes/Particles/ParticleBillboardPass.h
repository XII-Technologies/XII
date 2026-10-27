/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>

/// Appends unlit ballistic billboards to the same weighted transparency layers as world meshes.
class XII_GRAPHICSCORE_DLL xiiParticleBillboardPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBillboardPass, xiiRenderPipelinePass);

public:
  xiiParticleBillboardPass();
  ~xiiParticleBillboardPass() override;
  xiiRenderPipelineBufferInputPin             m_Particles{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Materials{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Camera{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineTextureInputPin            m_Depth{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_InputAccumulation{xiiGALResourceFormat::RGBA32Float, true, xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_InputRevealage{xiiGALResourceFormat::R32Float, true, xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_InputAdditive{xiiGALResourceFormat::RGBA32Float, true, xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_InputModulation{xiiGALResourceFormat::RGBA32Float, true, xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_InputHistoryTrust{xiiGALResourceFormat::R32Float, true, xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin           m_Accumulation{xiiGALResourceFormat::RGBA32Float, true, xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin           m_Revealage{xiiGALResourceFormat::R32Float, true, xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin           m_Additive{xiiGALResourceFormat::RGBA32Float, true, xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin           m_Modulation{xiiGALResourceFormat::RGBA32Float, true, xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin           m_HistoryTrust{xiiGALResourceFormat::R32Float, true, xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  float                                       m_fWeightDepthScale = 5, m_fMaximumWeight = 3000, m_fColorScale = 1;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::BindlessResources; }

private:
  xiiShaderPermutationResourceHandle m_Shader;
  xiiSharedPtr<xiiGALSampler>        m_Sampler;
};
