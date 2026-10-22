/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
/// Traces one closest-triangle ray against an explicit TLAS; an empty scene is a defined miss.
/// The graph owns private shader records, constants and results for each occurrence/frame.
class XII_GRAPHICSCORE_DLL xiiSceneRayProbePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneRayProbePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin                 m_Before;
  xiiRenderPipelineOrderOutputPin                m_After;
  xiiRenderPipelineAccelerationStructureInputPin m_Scene{true};
  /// Canonical hit records; Metadata.y preserves the full generation-validated world GPU identity.
  xiiRenderPipelineBufferInputPin             m_Materials{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin            m_Result{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiVec3                                     m_Origin = xiiVec3(0, 0, 100), m_Direction = xiiVec3(0, 0, -1);
  float                                       m_fMinimumDistance = 0.001f, m_fMaximumDistance = 1000;
  xiiUInt32                                   m_uiInstanceMask = 255;
  bool                                        m_bAsyncCompute  = true;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::RayTracing; }

private:
  xiiShaderPermutationResourceHandle m_Shader;
};
