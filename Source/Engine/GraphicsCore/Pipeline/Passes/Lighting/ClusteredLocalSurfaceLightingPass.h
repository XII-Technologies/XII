/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>
class XII_GRAPHICSCORE_DLL xiiClusteredLocalSurfaceLightingPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClusteredLocalSurfaceLightingPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin            m_Depth{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_Normal{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_Albedo{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_Material{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_Emissive{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferInputPin             m_Camera{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineTextureOutputPin           m_Output{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  float                                       m_fScale        = 1;
  bool                                        m_bAsyncCompute = false;
  xiiRenderPipelineDataInputPin               m_Layout{xiiGetStaticRTTI<xiiLightClusterFrameData>()};
  xiiRenderPipelineBufferInputPin             m_Lights{false, xiiGALBindFlags::ShaderResource}, m_Counts{false, xiiGALBindFlags::ShaderResource}, m_Candidates{false, xiiGALBindFlags::ShaderResource}, m_Profiles{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Grid{false, xiiGALBindFlags::ShaderResource}, m_Indices{false, xiiGALBindFlags::ShaderResource}, m_Overflow{false, xiiGALBindFlags::ShaderResource};
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
