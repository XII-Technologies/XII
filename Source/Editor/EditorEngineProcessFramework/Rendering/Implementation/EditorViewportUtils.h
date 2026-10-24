/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/States/PipelineState.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

namespace xiiEditorViewportUtils
{
  xiiResult LoadPipeline(xiiStringView shader, xiiGALGraphicsPipelineStateCreationDescription& out_pipeline);
  xiiResult ValidateTargets(const xiiRenderGraph& graph, xiiRenderGraphTextureHandle color, xiiRenderGraphTextureHandle depth);

  template <typename T>
  bool Upload(xiiRenderGraphPassContext& execution, xiiRenderGraphBufferHandle buffer, xiiArrayPtr<const T> values, xiiGALResourceStateFlags::Enum state)
  {
    auto& commands = execution.GetCommandList();
    {
      xiiGALMapHelper<T> mapped(commands, execution.GetBuffer(buffer), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      if (!mapped.GetMappedData())
      {
        xiiLog::Error("Failed to upload Editor viewport data.");
        return false;
      }
      xiiMemoryUtils::Copy(mapped.GetMappedData(), values.GetPtr(), values.GetCount());
    }
    xiiGALStateTransitionDescription transition;
    transition.m_pResource       = execution.GetBuffer(buffer);
    transition.m_NewState        = state;
    transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
    return true;
  }
} // namespace xiiEditorViewportUtils
