#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Allocates a structured or raw buffer to be consumed by compute or graphics passes.
class xiiCreateBufferPass : public xiiUtilityPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCreateBufferPass, xiiUtilityPipelinePass);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiCreateBufferPass);

public:
  xiiCreateBufferPass(xiiStringView sName);

  virtual ~xiiCreateBufferPass();

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual xiiResult InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

private:
  xiiRenderPipelineNodeOutputBufferPin m_PinOutput;

  xiiUInt64                          m_uiSize              = 0U;
  xiiUInt32                          m_uiElementByteStride = 0U;
  xiiBitflags<xiiGALBindFlags>       m_BindFlags;
  xiiEnum<xiiGALResourceUsage>       m_Usage;
  xiiBitflags<xiiGALCPUAccessFlag>   m_AccessFlags;
  xiiEnum<xiiGALBufferMode>          m_Mode;
  xiiBitflags<xiiGALMiscBufferFlags> m_MiscFlags;
};
