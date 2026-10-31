/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/MaterialRasterBatches.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/States/RasterizerState.h>

/// Rasterizes nearest depth with canonical material coverage from explicit draw and constant buffers.
class XII_GRAPHICSCORE_DLL xiiDepthRasterPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDepthRasterPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin   m_InputSceneInstances{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_InputSkinningMatrices{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_InputVisibleMeshlets{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_InputVisibleMeshletCount{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_InputGeometryMetadata{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_InputMeshletMetadata{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_InputSurfaceMaterials{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_InputRasterConstants{true, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin   m_InputDrawCommands{true, xiiGALBindFlags::IndirectDrawArguments};
  xiiRenderPipelineBufferInputPin   m_InputDrawCommandCount{true, xiiGALBindFlags::IndirectDrawArguments};
  xiiRenderPipelineTextureInputPin  m_InputDepth{xiiGALResourceFormat::D32Float, true, xiiGALBindFlags::DepthStencil, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputDepth{xiiGALResourceFormat::D32Float, true, xiiGALBindFlags::DepthStencil, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::ReadWrite};
  xiiRenderPipelineDataInputPin     m_Cascades{xiiGetStaticRTTI<xiiShadowCascadeSnapshot>(), true};
  xiiRenderPipelineDataInputPin     m_Spots{xiiGetStaticRTTI<xiiSpotShadowSnapshot>(), true};
  bool                              m_bShadowCaster  = false;
  xiiUInt32                         m_uiCascadeIndex = 0;

  /// Zero extent selects the full supplied depth image. Otherwise x/y/width/height are atlas texels.
  xiiVec4U32              m_Viewport        = xiiVec4U32::MakeZero();
  bool                    m_bUseMeshShaders = false;
  xiiEnum<xiiGALFillMode> m_FillMode        = xiiGALFillMode::Solid;

  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  void                                        GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return m_bUseMeshShaders ? xiiRenderPipelineDeviceFeature::MeshShaders | xiiRenderPipelineDeviceFeature::BindlessResources : xiiRenderPipelineDeviceFeature::BindlessResources; }

private:
  xiiMaterialRasterBatches           m_MaterialBatches;
  xiiShaderPermutationResourceHandle m_hShader;
  xiiSharedPtr<xiiGALSampler>        m_Sampler;
  bool                               m_bCachedMeshShaders = false;
  bool                               m_bCachedShadowCaster = false;
  xiiGALFillMode::Enum               m_CachedFill         = xiiGALFillMode::Undefined;
};
