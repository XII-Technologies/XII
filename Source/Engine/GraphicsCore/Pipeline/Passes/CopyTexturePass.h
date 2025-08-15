#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Copies a colour attachment to be consumed by compute or graphics passes.
class XII_GRAPHICSCORE_DLL xiiCopyColourAttachmentPass : public xiiUtilityPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCopyColourAttachmentPass, xiiUtilityPipelinePass);

public:
  xiiCopyColourAttachmentPass(xiiStringView sName = "CopyColourAttachmentPass");

  virtual ~xiiCopyColourAttachmentPass();

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

private:
  xiiRenderPipelineNodeInputColourAttachmentPin  m_PinInput;
  xiiRenderPipelineNodeOutputColourAttachmentPin m_PinOutput;
};

///////////////////////////////////////////////////////////////////////////////////

/// \brief Copies a depth attachment to be consumed by compute or graphics passes.
class XII_GRAPHICSCORE_DLL xiiCopyDepthAttachmentPass : public xiiUtilityPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCopyDepthAttachmentPass, xiiUtilityPipelinePass);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiCopyDepthAttachmentPass);

public:
  xiiCopyDepthAttachmentPass(xiiStringView sName = "CopyDepthAttachmentPass");

  virtual ~xiiCopyDepthAttachmentPass();

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

private:
  xiiRenderPipelineNodeInputDepthAttachmentPin  m_PinInput;
  xiiRenderPipelineNodeOutputDepthAttachmentPin m_PinOutput;
};
