/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/States/RasterizerState.h>

/// Queries nearest surfaces from supplied canonical GPU visibility and residency buffers.
class XII_GRAPHICSCORE_DLL xiiSurfaceQueryPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSurfaceQueryPass, xiiRenderPipelinePass);

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
  xiiRenderPipelineTextureOutputPin m_OutputSurface{xiiGALResourceFormat::RGBA32UInt, false, {}, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputDepth{xiiGALResourceFormat::D32Float, true, {}, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputOverdraw{xiiGALResourceFormat::RGBA32Float, true, {}, xiiGALResourceDimension::Texture2D, 1};

  bool                             m_bOnlyViewWireframe = false;
  xiiRenderPipelineBufferInputPin  m_InputDiagnosticMeshlets{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin  m_InputDiagnosticCount{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin  m_InputDiagnosticCommands{true, xiiGALBindFlags::IndirectDrawArguments};
  xiiRenderPipelineBufferInputPin  m_InputDiagnosticCommandCount{true, xiiGALBindFlags::IndirectDrawArguments};
  xiiRenderPipelineTextureInputPin m_InputDiagnosticDepth{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  /// Also produce geometry diagnostics in the conditional wireframe occurrence.
  bool                    m_bVisualizeView  = false;
  bool                    m_bUseMeshShaders = false;
  xiiEnum<xiiGALFillMode> m_FillMode        = xiiGALFillMode::Solid;

  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return m_bUseMeshShaders ? xiiRenderPipelineDeviceFeature::MeshShaders | xiiRenderPipelineDeviceFeature::BindlessResources : xiiRenderPipelineDeviceFeature::BindlessResources; }

private:
  xiiShaderPermutationResourceHandle m_hShader;
  xiiSharedPtr<xiiGALSampler>        m_Sampler;
  bool                               m_bCachedMeshShaders = false;
  bool                               m_bCachedDiagnostics = false;
  bool                               m_bCachedOverdraw    = false;
  bool                               m_bCachedCulling     = false;
  xiiGALFillMode::Enum               m_CachedFill         = xiiGALFillMode::Undefined;
};
