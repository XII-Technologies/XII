/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Allocates and initializes one graph-owned GPU counter, suitable for indirect work.
class XII_GRAPHICSCORE_DLL xiiCounterResetPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCounterResetPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin m_Counter{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments};
  xiiUInt32                        m_uiValue = 0;
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};
