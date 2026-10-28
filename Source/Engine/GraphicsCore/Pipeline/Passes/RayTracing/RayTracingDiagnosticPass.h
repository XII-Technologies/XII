/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsFoundation/Resources/Sampler.h>

/// One primary camera ray and one reflected probe per primary hit. Counters
/// describe this explicit workload, not unrelated ray-traced lighting passes.
class XII_GRAPHICSCORE_DLL xiiRayTracingDiagnosticPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRayTracingDiagnosticPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineAccelerationStructureInputPin m_Scene{true};
  xiiRenderPipelineBufferInputPin                m_Materials{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin                m_Geometry{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin                m_Camera{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineTextureOutputPin              m_Output{xiiGALResourceFormat::RGBA16Float, true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::RenderTarget, xiiGALResourceDimension::Texture2D, 1};

  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiShaderPermutationResourceHandle m_Shader;
  xiiSharedPtr<xiiGALSampler>        m_Sampler;
};
