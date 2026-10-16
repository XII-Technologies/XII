/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Per-pixel irradiance from explicitly supplied, independently prepared GI resources.
class XII_GRAPHICSCORE_DLL xiiSparseVoxelGatherPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSparseVoxelGatherPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_Depth{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_Normal{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferInputPin   m_Camera{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin   m_RadiancePool{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_PageTable{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_LevelData{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin   m_VoxelConstants{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineTextureOutputPin m_Output{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  float                             m_fIntensity = 1.0f;
  xiiSparseVoxelGatherPass();
  ~xiiSparseVoxelGatherPass();
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;

private:
  struct State;
  xiiUniquePtr<State> m_pState;
};
