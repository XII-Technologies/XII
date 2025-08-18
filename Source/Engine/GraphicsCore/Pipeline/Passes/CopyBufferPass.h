#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Copies a structured or raw buffer to be consumed by compute or graphics passes.
class XII_GRAPHICSCORE_DLL xiiCopyBufferPass : public xiiUtilityPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCopyBufferPass, xiiUtilityPipelinePass);

public:
  xiiCopyBufferPass(xiiStringView sName = "CopyBufferPass");

  virtual ~xiiCopyBufferPass();

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

private:
  xiiRenderPipelineNodeInputBufferPin  m_PinInput;
  xiiRenderPipelineNodeOutputBufferPin m_PinOutput;
};
