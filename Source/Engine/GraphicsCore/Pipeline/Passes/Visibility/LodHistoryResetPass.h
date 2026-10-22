/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Initializes generation/token-invalid LOD entries for the supplied instance capacity.
class XII_GRAPHICSCORE_DLL xiiLodHistoryResetPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLodHistoryResetPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin              m_Before;
  xiiRenderPipelineOrderOutputPin             m_After;
  xiiRenderPipelineBufferInputPin             m_Instances{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin            m_History{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
