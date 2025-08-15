#pragma once

#include <GraphicsCore/Pipeline/FrameDataProvider.h>
#include <GraphicsCore/Pipeline/Passes/CreateBufferPass.h>
#include <GraphicsCore/Pipeline/Passes/CreateTexturePass.h>

class XII_GRAPHICSCORE_DLL xiiHistorySourcePassResourceProvider : public xiiFrameDataProviderBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiHistorySourcePassResourceProvider, xiiFrameDataProviderBase);

public:
  xiiHistorySourcePassResourceProvider();

  ~xiiHistorySourcePassResourceProvider();

  void ResetResource(xiiStringView sResourceName);

  xiiSharedPtr<xiiGALDeviceObject> GetOrCreateResource(xiiStringView sResourceName, const xiiRenderPipelineResourceRequest& request);

private:
  xiiHashTable<xiiString, xiiSharedPtr<xiiGALDeviceObject>> m_Data;

private:
  // We ignore the frame-based logic for this data provider as we only want to store cross frame data.
  XII_ALWAYS_INLINE virtual void* UpdateData(const xiiRenderViewContext& renderViewContext, const xiiExtractedRenderData& extractedData) override { return nullptr; }
};

///////////////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiHistoryBufferSourcePass : public xiiCreateBufferPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiHistoryBufferSourcePass, xiiCreateBufferPass);

public:
  xiiHistoryBufferSourcePass(xiiStringView sName = "HistoryBufferSourcePass");

  ~xiiHistoryBufferSourcePass();

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual xiiSharedPtr<xiiGALDeviceObject> QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request) override;
};

///////////////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiHistoryColourAttachmentSourcePass : public xiiCreateColourAttachmentPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiHistoryColourAttachmentSourcePass, xiiCreateColourAttachmentPass);

public:
  xiiHistoryColourAttachmentSourcePass(xiiStringView sName = "HistoryColourAttachmentSourcePass");

  ~xiiHistoryColourAttachmentSourcePass();

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual xiiSharedPtr<xiiGALDeviceObject> QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

private:
  bool m_bFirstExecute = true;
};

///////////////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiHistoryDepthAttachmentSourcePass : public xiiCreateDepthAttachmentPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiHistoryDepthAttachmentSourcePass, xiiCreateDepthAttachmentPass);

public:
  xiiHistoryDepthAttachmentSourcePass(xiiStringView sName = "HistoryDepthAttachmentSourcePass");

  ~xiiHistoryDepthAttachmentSourcePass();

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual xiiSharedPtr<xiiGALDeviceObject> QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

private:
  bool m_bFirstExecute = true;
};
