/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
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
  xiiRenderPipelineBufferInputPin   m_InputMeshIndirectCommands{true};
  xiiRenderPipelineBufferInputPin   m_InputMeshIndirectCommandCount{true};
  xiiRenderPipelineTextureOutputPin m_OutputSurface{xiiGALResourceFormat::RGBA32UInt, false, {}, xiiGALResourceDimension::Texture2D, 1};

  bool                    m_bOnlyViewWireframe = false;
  bool                    m_bUseMeshShaders    = true;
  xiiEnum<xiiGALFillMode> m_FillMode           = xiiGALFillMode::Solid;

  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::BindlessResources; }
};
