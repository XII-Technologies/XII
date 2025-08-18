#pragma once

#include <GraphicsCore/Pipeline/Passes/HistorySourcePass.h>

class XII_GRAPHICSCORE_DLL xiiHistoryBufferTargetPass : public xiiUtilityPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiHistoryBufferTargetPass, xiiUtilityPipelinePass);

public:
  xiiHistoryBufferTargetPass(xiiStringView sName = "HistoryBufferTargetPass");

  ~xiiHistoryBufferTargetPass();

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual xiiSharedPtr<xiiGALDeviceObject> QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

private:
  xiiRenderPipelineNodeInputBufferProviderPin m_PinInput;

  xiiString m_sSourcePassName = "HistoryBufferSourcePass";
};

///////////////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiHistoryColourAttachmentTargetPass : public xiiUtilityPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiHistoryColourAttachmentTargetPass, xiiUtilityPipelinePass);

public:
  xiiHistoryColourAttachmentTargetPass(xiiStringView sName = "HistoryColourAttachmentTargetPass");

  ~xiiHistoryColourAttachmentTargetPass();

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual xiiSharedPtr<xiiGALDeviceObject> QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

private:
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinInput;

  xiiString m_sSourcePassName = "HistoryColourAttachmentSourcePass";
};

///////////////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiHistoryDepthAttachmentTargetPass : public xiiUtilityPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiHistoryDepthAttachmentTargetPass, xiiUtilityPipelinePass);

public:
  xiiHistoryDepthAttachmentTargetPass(xiiStringView sName = "HistoryDepthAttachmentTargetPass");

  ~xiiHistoryDepthAttachmentTargetPass();

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual xiiSharedPtr<xiiGALDeviceObject> QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

private:
  xiiRenderPipelineNodeInputDepthAttachmentProviderPin m_PinInput;

  xiiString m_sSourcePassName = "HistoryDepthAttachmentSourcePass";
};
