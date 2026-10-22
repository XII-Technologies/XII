/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Allocates and initializes a bounded GPU request table, independently of scene extraction.
class XII_GRAPHICSCORE_DLL xiiGeometryLodFeedbackResetPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGeometryLodFeedbackResetPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin              m_Before;
  xiiRenderPipelineOrderOutputPin             m_After;
  xiiRenderPipelineBufferOutputPin            m_Requests{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiUInt32                                   m_uiCapacity = 4096;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
