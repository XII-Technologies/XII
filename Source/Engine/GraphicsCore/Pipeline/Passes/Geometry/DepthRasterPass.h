/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/States/RasterizerState.h>

/// Rasterizes nearest depth with canonical material coverage from explicit draw and constant buffers.
class XII_GRAPHICSCORE_DLL xiiDepthRasterPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDepthRasterPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin    m_Before;
  xiiRenderPipelineOrderOutputPin   m_After;
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
  xiiRenderPipelineTextureInputPin  m_InputDepth{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::DepthStencil, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputDepth{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::DepthStencil, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::ReadWrite};

  /// Zero extent selects the full supplied depth image. Otherwise x/y/width/height are atlas texels.
  xiiVec4U32              m_Viewport        = xiiVec4U32::MakeZero();
  bool                    m_bUseMeshShaders = false;
  xiiEnum<xiiGALFillMode> m_FillMode        = xiiGALFillMode::Solid;

  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return m_bUseMeshShaders ? xiiRenderPipelineDeviceFeature::MeshShaders | xiiRenderPipelineDeviceFeature::BindlessResources : xiiRenderPipelineDeviceFeature::BindlessResources; }

private:
  xiiShaderPermutationResourceHandle m_hShader;
  xiiSharedPtr<xiiGALSampler>        m_Sampler;
  bool                               m_bCachedMeshShaders = false;
  xiiGALFillMode::Enum               m_CachedFill         = xiiGALFillMode::Undefined;
};
