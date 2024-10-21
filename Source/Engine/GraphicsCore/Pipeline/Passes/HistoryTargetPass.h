#pragma once

#include <GraphicsCore/Pipeline/Passes/HistorySourcePass.h>

/// \brief Allows to write data to be accessible in the next frame. See xiiHistorySourcePass for usage.
class XII_GRAPHICSCORE_DLL xiiHistoryTargetPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiHistoryTargetPass, xiiRenderPipelinePass);

public:
  xiiHistoryTargetPass(xiiStringView sName = "HistoryTargetPass");
  ~xiiHistoryTargetPass();

  virtual bool                    GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual xiiGALTextureViewHandle QueryTextureProvider(const xiiRenderPipelineNodePin* pPin, const xiiGALTextureCreationDescription& desc) override;
  virtual void                    Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual xiiResult               Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult               Deserialize(xiiStreamReader& inout_stream) override;

protected:
  xiiRenderPipelineNodeInputProviderPin m_PinInput;
  xiiString                             m_sSourcePassName = "HistorySourcePass";
};
