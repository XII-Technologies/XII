#pragma once

#include <GraphicsCore/Pipeline/FrameDataProvider.h>
#include <GraphicsCore/Pipeline/Passes/SourcePass.h>

/// \brief Allows to access data from a previous frame. Always comes in a pair with a xiiHistoryTargetPass.
/// To preserve textures across the next frame you need to create this node to define the type of texture and initial state. This node's output pin will give access to the previous frame's content.
/// Next, create an xiiHistoryTargetPass. It's input pin exposes the same texture as provided by the source node but allows you to write to by connecting the input pin to another pass that produces the image that you want to carry to the next frame. To connect an xiiHistoryTargetPass to its counterpart you need to set it's "SourcePassName" property to the name of the xiiHistorySourcePass you want to match.
/// As both nodes expose the same texture, special care has to be taken that it's not used as input and output of another pass at the same time. In those cases, add a xiiCopyTexturePass to break up invalid state.
class XII_GRAPHICSCORE_DLL xiiHistorySourcePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiHistorySourcePass, xiiRenderPipelinePass);

public:
  xiiHistorySourcePass(xiiStringView sName = "HistorySourcePass");
  ~xiiHistorySourcePass();

  virtual bool                    GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual xiiGALTextureViewHandle QueryTextureProvider(const xiiRenderPipelineNodePin* pPin, const xiiGALTextureCreationDescription& desc) override;
  virtual void                    Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual xiiResult               Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult               Deserialize(xiiStreamReader& inout_stream) override;

protected:
  xiiRenderPipelineNodeOutputProviderPin m_PinOutput;
  xiiEnum<xiiSourceFormat>               m_Format     = xiiSourceFormat::Default;
  xiiEnum<xiiGALMSAASampleCount>         m_MsaaMode   = xiiGALMSAASampleCount::OneSample;
  xiiColor                               m_ClearColor = xiiColor::Black;

  bool m_bFirstExecute = true;
};

class XII_GRAPHICSCORE_DLL xiiHistorySourcePassTextureDataProvider : public xiiFrameDataProviderBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiHistorySourcePassTextureDataProvider, xiiFrameDataProviderBase);

public:
  xiiHistorySourcePassTextureDataProvider();
  ~xiiHistorySourcePassTextureDataProvider();

  void                ResetTexture(xiiStringView sSourcePassName);
  xiiGALTextureHandle GetOrCreateTexture(xiiStringView sSourcePassName, const xiiGALTextureCreationDescription& desc);

public:
  xiiHashTable<xiiString, xiiGALTextureHandle> m_Data;

private:
  // We ignore the frame-based logic for this data provider as we only want to store cross frame data.
  virtual void* UpdateData(const xiiRenderViewContext& renderViewContext, const xiiExtractedRenderData& extractedData) override { return nullptr; }
};
