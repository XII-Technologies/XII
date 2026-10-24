/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/Shader/Types.h>
#include <GraphicsFoundation/States/PipelineState.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

class xiiView;

namespace xiiEditorViewportUtils
{
  inline bool IsFinite(const xiiShaderMat4& matrix)
  {
    float elements[16];
    xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(elements), reinterpret_cast<const xiiUInt8*>(&matrix), sizeof(elements));
    for (float element : elements)
      if (!xiiMath::IsFinite(element))
        return false;
    return true;
  }
  xiiResult PixelRatio(const xiiView& view, float& out_ratio);
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
