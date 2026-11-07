/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbeFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Updates scheduled directional irradiance probes from world or camera-visible geometry.
class XII_GRAPHICSCORE_DLL xiiDDGISceneUpdatePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDDGISceneUpdatePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin               m_Schedule{xiiGetStaticRTTI<xiiDDGIProbeFrameData>(), true};
  xiiRenderPipelineBufferInputPin             m_Constants{true, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin             m_Camera{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin             m_Updates{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Instances{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Geometry{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Meshlets{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Materials{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Lights{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_LightCounts{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_LightCandidates{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_IESProfiles{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_StatesInput{true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineTextureInputPin            m_Depth{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_Normal{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_Radiance{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_IrradianceInput{xiiGALResourceFormat::RGBA16Float, true, xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_DistanceInput{xiiGALResourceFormat::RG32Float, true, xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferOutputPin            m_States{true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineTextureOutputPin           m_Irradiance{xiiGALResourceFormat::RGBA16Float, true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin           m_Distance{xiiGALResourceFormat::RG32Float, true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  bool                                        m_bAllowAbsentSchedule = false;
  bool                                        m_bUseWorldGeometry    = false;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALComputePipelineState> m_WorldPipeline;
  xiiSharedPtr<xiiGALComputePipelineState> m_WorldMeshletPipeline, m_WorldLitPipeline;
  xiiSharedPtr<xiiGALSampler>              m_ProbeSampler;
};
