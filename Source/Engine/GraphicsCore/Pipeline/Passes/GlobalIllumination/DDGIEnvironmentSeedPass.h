/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbeFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>
/// Budgeted analytic sky/sun probe seeding, preserving the existing non-RT baseline.
/// It is an explicit algorithm; it is never selected as a fallback for a missing ray-tracing pipeline.
class XII_GRAPHICSCORE_DLL xiiDDGIEnvironmentSeedPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDDGIEnvironmentSeedPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin               m_Schedule{xiiGetStaticRTTI<xiiDDGIProbeFrameData>()};
  xiiRenderPipelineBufferInputPin             m_Constants{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin             m_Environment{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin             m_Updates{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_StatesInput{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineTextureInputPin            m_IrradianceInput{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_DistanceInput{xiiGALResourceFormat::RG32Float, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferOutputPin            m_States{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineTextureOutputPin           m_Irradiance{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin           m_Distance{xiiGALResourceFormat::RG32Float, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  bool                                        m_bAsyncCompute = false;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
