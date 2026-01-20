#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

struct xiiGlobalConstants;

class XII_GRAPHICSCORE_DLL xiiFrameConstantsPass : public xiiUtilityPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiFrameConstantsPass, xiiUtilityPipelinePass);

public:
  xiiFrameConstantsPass(xiiStringView sName = "FrameConstantsPass");

  virtual ~xiiFrameConstantsPass();

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual xiiResult InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

private:
  xiiRenderPipelineNodeOutputBufferPin m_PinOutput;

  xiiBlobPtr<xiiGlobalConstants> m_pGlobalConstants;
};
