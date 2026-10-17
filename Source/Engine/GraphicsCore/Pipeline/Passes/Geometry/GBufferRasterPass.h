/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/States/RasterizerState.h>

/// Rasterizes canonical material targets against supplied depth. Visibility, draws and temporal constants are explicit inputs.
class XII_GRAPHICSCORE_DLL xiiGBufferRasterPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGBufferRasterPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin   m_InputSceneInstances{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_InputSkinningMatrices{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_InputVisibleMeshlets{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_InputVisibleMeshletCount{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_InputGeometryMetadata{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_InputMeshletMetadata{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_InputSurfaceMaterials{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_InputRasterConstants{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin   m_InputDrawCommands{false, xiiGALBindFlags::IndirectDrawArguments};
  xiiRenderPipelineBufferInputPin   m_InputDrawCommandCount{false, xiiGALBindFlags::IndirectDrawArguments};
  xiiRenderPipelineTextureInputPin  m_InputDepth{xiiGALResourceFormat::D32Float, false, {}, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputAlbedo{xiiGALResourceFormat::RGBA8UNormalized};
  xiiRenderPipelineTextureOutputPin m_OutputNormal{xiiGALResourceFormat::RG16SNormalized};
  xiiRenderPipelineTextureOutputPin m_OutputMaterial{xiiGALResourceFormat::RGBA8UNormalized};
  xiiRenderPipelineTextureOutputPin m_OutputEmissive{xiiGALResourceFormat::RGBA16Float};
  xiiRenderPipelineTextureOutputPin m_OutputNormalRoughness{xiiGALResourceFormat::RGBA8UNormalized};
  xiiRenderPipelineTextureOutputPin m_OutputVelocity{xiiGALResourceFormat::RG16Float};
  xiiRenderPipelineTextureOutputPin m_OutputObjectId{xiiGALResourceFormat::R32UInt};

  bool                    m_bUseMeshShaders = false;
  xiiEnum<xiiGALFillMode> m_FillMode        = xiiGALFillMode::Solid;

  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return m_bUseMeshShaders ? xiiRenderPipelineDeviceFeature::MeshShaders | xiiRenderPipelineDeviceFeature::BindlessResources : xiiRenderPipelineDeviceFeature::BindlessResources; }

private:
  xiiShaderPermutationResourceHandle  m_hShader;
  xiiSharedPtr<xiiGALSampler>         m_Sampler;
  xiiSharedPtr<xiiGALRasterizerState> m_Rasterizer;
  bool                                m_bCachedMeshShaders = false;
  xiiGALFillMode::Enum                m_CachedFill         = xiiGALFillMode::Undefined;
};
