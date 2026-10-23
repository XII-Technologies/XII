/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsFoundation/Resources/Sampler.h>
/// Mesh-shader Lambert rasterization of explicit visible work against supplied color/depth targets.
/// Owns no visibility, scene state, upload, light selection or shadow allocation.
class XII_GRAPHICSCORE_DLL xiiDirectionalMeshRasterPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDirectionalMeshRasterPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin             m_Scene{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Skinning{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Geometry{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Meshlets{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Visible{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Count{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Arguments{false, xiiGALBindFlags::IndirectDrawArguments};
  xiiRenderPipelineBufferInputPin             m_ArgumentCount{false, xiiGALBindFlags::IndirectDrawArguments};
  xiiRenderPipelineBufferInputPin             m_Materials{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Constants{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin             m_ShadowConstants{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin             m_PageTable{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineTextureInputPin            m_ShadowMap{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_ShadowAtlas{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_ColorTarget{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::RenderTarget, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_DepthTarget{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::DepthStencil, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin           m_Color{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::RenderTarget, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin           m_Depth{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::DepthStencil, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferInputPin             m_RasterConstants{false, xiiGALBindFlags::UniformBuffer};
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::MeshShaders | xiiRenderPipelineDeviceFeature::BindlessResources; }

private:
  xiiShaderPermutationResourceHandle m_Shader;
  xiiSharedPtr<xiiGALSampler>        m_Sampler;
};
