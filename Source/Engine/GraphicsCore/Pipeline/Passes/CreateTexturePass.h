#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Allocates a colour attachment to be consumed by compute or graphics passes.
class xiiCreateColourAttachmentPass : public xiiUtilityPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCreateColourAttachmentPass, xiiUtilityPipelinePass);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiCreateColourAttachmentPass);

public:
  xiiCreateColourAttachmentPass(xiiStringView sName);

  virtual ~xiiCreateColourAttachmentPass();

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual xiiResult InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

private:
  xiiRenderPipelineNodeOutputColourAttachmentPin m_PinOutput;

  xiiEnum<xiiGALResourceDimension>    m_Type = xiiGALResourceDimension::Texture2D;
  xiiEnum<xiiSourceFormat>            m_Format;
  xiiUInt32                           m_uiArraySizeOrDepth = 1U;
  xiiUInt32                           m_uiMipLevels        = 1U;
  xiiUInt32                           m_uiSampleCount      = 1U;
  xiiBitflags<xiiGALBindFlags>        m_BindFlags;
  xiiEnum<xiiGALResourceUsage>        m_Usage;
  xiiBitflags<xiiGALCPUAccessFlag>    m_AccessFlags;
  xiiBitflags<xiiGALMiscTextureFlags> m_MiscFlags;
};

/// \brief Allocates a depth attachment to be consumed by compute or graphics passes.
class xiiCreateDepthAttachmentPass : public xiiUtilityPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCreateDepthAttachmentPass, xiiUtilityPipelinePass);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiCreateDepthAttachmentPass);

public:
  xiiCreateDepthAttachmentPass(xiiStringView sName);

  virtual ~xiiCreateDepthAttachmentPass();

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual xiiResult InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

private:
  xiiRenderPipelineNodeOutputDepthAttachmentPin m_PinOutput;

  xiiEnum<xiiGALResourceDimension>    m_Type = xiiGALResourceDimension::Buffer;
  xiiEnum<xiiSourceFormat>       m_Format;
  xiiUInt32                           m_uiArraySizeOrDepth = 0U;
  xiiUInt32                           m_uiMipLevels        = 1U;
  xiiUInt32                           m_uiSampleCount      = 1U;
  xiiBitflags<xiiGALBindFlags>        m_BindFlags;
  xiiEnum<xiiGALResourceUsage>        m_Usage;
  xiiBitflags<xiiGALCPUAccessFlag>    m_AccessFlags;
  xiiBitflags<xiiGALMiscTextureFlags> m_MiscFlags;
};
