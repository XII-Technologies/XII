#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Allocates a colour attachment to be consumed by compute or graphics passes.
class XII_GRAPHICSCORE_DLL xiiCreateColourAttachmentPass : public xiiUtilityPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCreateColourAttachmentPass, xiiUtilityPipelinePass);

public:
  xiiCreateColourAttachmentPass(xiiStringView sName = "CreateColourAttachmentPass");

  virtual ~xiiCreateColourAttachmentPass();

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual xiiResult InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

public:
  /// \brief Gets the resource dimension type (e.g., Texture2D).
  XII_ALWAYS_INLINE xiiEnum<xiiGALResourceDimension> GetType() const { return m_Type; }

  /// \brief Sets the resource dimension type.
  XII_ALWAYS_INLINE void SetType(xiiEnum<xiiGALResourceDimension> type) { m_Type = type; }


  /// \brief Gets the source format of the attachment.
  XII_ALWAYS_INLINE xiiEnum<xiiSourceFormat> GetFormat() const { return m_Format; }

  /// \brief Sets the source format of the attachment.
  XII_ALWAYS_INLINE void SetFormat(xiiEnum<xiiSourceFormat> format) { m_Format = format; }


  /// \brief Gets the array size or depth of the texture.
  XII_ALWAYS_INLINE xiiUInt32 GetArraySizeOrDepth() const { return m_uiArraySizeOrDepth; }

  /// \brief Sets the array size or depth of the texture.
  XII_ALWAYS_INLINE void SetArraySizeOrDepth(xiiUInt32 sizeOrDepth) { m_uiArraySizeOrDepth = sizeOrDepth; }


  /// \brief Gets the number of mipmap levels.
  XII_ALWAYS_INLINE xiiUInt32 GetMipLevels() const { return m_uiMipLevels; }

  /// \brief Sets the number of mipmap levels.
  XII_ALWAYS_INLINE void SetMipLevels(xiiUInt32 mipLevels) { m_uiMipLevels = mipLevels; }


  /// \brief Gets the sample count for multisampling.
  XII_ALWAYS_INLINE xiiUInt32 GetSampleCount() const { return m_uiSampleCount; }

  /// \brief Sets the sample count for multisampling.
  XII_ALWAYS_INLINE void SetSampleCount(xiiUInt32 sampleCount) { m_uiSampleCount = sampleCount; }


  /// \brief Gets the bind flags used for the attachment.
  XII_ALWAYS_INLINE xiiBitflags<xiiGALBindFlags> GetBindFlags() const { return m_BindFlags; }

  /// \brief Sets the bind flags used for the attachment.
  XII_ALWAYS_INLINE void SetBindFlags(xiiBitflags<xiiGALBindFlags> bindFlags) { m_BindFlags = bindFlags; }


  /// \brief Gets the resource usage type.
  XII_ALWAYS_INLINE xiiEnum<xiiGALResourceUsage> GetUsage() const { return m_Usage; }

  /// \brief Sets the resource usage type.
  XII_ALWAYS_INLINE void SetUsage(xiiEnum<xiiGALResourceUsage> usage) { m_Usage = usage; }


  /// \brief Gets the CPU access flags.
  XII_ALWAYS_INLINE xiiBitflags<xiiGALCPUAccessFlag> GetAccessFlags() const { return m_AccessFlags; }

  /// \brief Sets the CPU access flags.
  XII_ALWAYS_INLINE void SetAccessFlags(xiiBitflags<xiiGALCPUAccessFlag> accessFlags) { m_AccessFlags = accessFlags; }


  /// \brief Gets miscellaneous texture flags.
  XII_ALWAYS_INLINE xiiBitflags<xiiGALMiscTextureFlags> GetMiscFlags() const { return m_MiscFlags; }

  /// \brief Sets miscellaneous texture flags.
  XII_ALWAYS_INLINE void SetMiscFlags(xiiBitflags<xiiGALMiscTextureFlags> miscFlags) { m_MiscFlags = miscFlags; }


  /// \brief Returns whether the attachment should be cleared before use.
  XII_ALWAYS_INLINE bool GetClear() const { return m_bClear; }

  /// \brief Sets whether the attachment should be cleared before use.
  XII_ALWAYS_INLINE void SetClear(bool bClear) { m_bClear = bClear; }


  /// \brief Gets the clear colour used when clearing the attachment.
  XII_ALWAYS_INLINE xiiColor GetClearColour() const { return m_ClearColour; }

  /// \brief Sets the clear colour used when clearing the attachment.
  XII_ALWAYS_INLINE void SetClearColour(const xiiColor& color) { m_ClearColour = color; }

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

  bool     m_bClear      = false;
  xiiColor m_ClearColour = xiiColor::Black;

  xiiSharedPtr<xiiGALRenderPass>                      m_pRenderPass;
  xiiHybridArray<xiiSharedPtr<xiiGALFramebuffer>, 2U> m_FramebufferCache;
};

/// \brief Allocates a depth attachment to be consumed by compute or graphics passes.
class XII_GRAPHICSCORE_DLL xiiCreateDepthAttachmentPass : public xiiUtilityPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCreateDepthAttachmentPass, xiiUtilityPipelinePass);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiCreateDepthAttachmentPass);

public:
  xiiCreateDepthAttachmentPass(xiiStringView sName = "CreateDepthAttachmentPass");

  virtual ~xiiCreateDepthAttachmentPass();

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual xiiResult InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

private:
  xiiRenderPipelineNodeOutputDepthAttachmentPin m_PinOutput;

  xiiEnum<xiiGALResourceDimension>    m_Type = xiiGALResourceDimension::Buffer;
  xiiEnum<xiiSourceFormat>            m_Format;
  xiiUInt32                           m_uiArraySizeOrDepth = 0U;
  xiiUInt32                           m_uiMipLevels        = 1U;
  xiiUInt32                           m_uiSampleCount      = 1U;
  xiiBitflags<xiiGALBindFlags>        m_BindFlags;
  xiiEnum<xiiGALResourceUsage>        m_Usage;
  xiiBitflags<xiiGALCPUAccessFlag>    m_AccessFlags;
  xiiBitflags<xiiGALMiscTextureFlags> m_MiscFlags;

  bool     m_bClear              = false;
  float    m_fDepthClearValue    = 1.0f;
  xiiUInt8 m_uiStencilClearValue = 0U;

  xiiSharedPtr<xiiGALRenderPass>                      m_pRenderPass;
  xiiHybridArray<xiiSharedPtr<xiiGALFramebuffer>, 2U> m_FramebufferCache;
};
