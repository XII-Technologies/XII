#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Allocates a structured or raw buffer to be consumed by compute or graphics passes.
class xiiCreateBufferPass : public xiiUtilityPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCreateBufferPass, xiiUtilityPipelinePass);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiCreateBufferPass);

public:
  xiiCreateBufferPass(xiiStringView sName = "CreateBufferPass");

  virtual ~xiiCreateBufferPass();

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

public:
  /// \brief Returns the total buffer size in bytes.
  XII_ALWAYS_INLINE xiiUInt64 GetSize() const { return m_uiSize; }

  /// \brief Sets the total buffer size in bytes.
  XII_ALWAYS_INLINE void SetSize(xiiUInt64 uiSize) { m_uiSize = uiSize; }


  /// \brief Returns the stride in bytes for each buffer element.
  XII_ALWAYS_INLINE xiiUInt32 GetElementByteStride() const { return m_uiElementByteStride; }

  /// \brief Sets the stride in bytes for each buffer element.
  XII_ALWAYS_INLINE void SetElementByteStride(xiiUInt32 uiStride) { m_uiElementByteStride = uiStride; }


  /// \brief Returns the bind flags used for buffer creation.
  XII_ALWAYS_INLINE xiiBitflags<xiiGALBindFlags> GetBindFlags() const { return m_BindFlags; }

  /// \brief Sets the bind flags used for buffer creation.
  XII_ALWAYS_INLINE void SetBindFlags(xiiBitflags<xiiGALBindFlags> bindFlags) { m_BindFlags = bindFlags; }


  /// \brief Returns the resource usage type.
  XII_ALWAYS_INLINE xiiEnum<xiiGALResourceUsage> GetUsage() const { return m_Usage; }

  /// \brief Sets the resource usage type.
  XII_ALWAYS_INLINE void SetUsage(xiiEnum<xiiGALResourceUsage> usage) { m_Usage = usage; }


  /// \brief Returns the CPU access flags.
  XII_ALWAYS_INLINE xiiBitflags<xiiGALCPUAccessFlag> GetAccessFlags() const { return m_AccessFlags; }

  /// \brief Sets the CPU access flags.
  XII_ALWAYS_INLINE void SetAccessFlags(xiiBitflags<xiiGALCPUAccessFlag> accessFlags) { m_AccessFlags = accessFlags; }


  /// \brief Returns the buffer mode (e.g., structured, raw).
  XII_ALWAYS_INLINE xiiEnum<xiiGALBufferMode> GetMode() const { return m_Mode; }

  /// \brief Sets the buffer mode (e.g., structured, raw).
  XII_ALWAYS_INLINE void SetMode(xiiEnum<xiiGALBufferMode> mode) { m_Mode = mode; }


  /// \brief Returns miscellaneous buffer flags.
  XII_ALWAYS_INLINE xiiBitflags<xiiGALMiscBufferFlags> GetMiscFlags() const { return m_MiscFlags; }

  /// \brief Sets miscellaneous buffer flags.
  XII_ALWAYS_INLINE void SetMiscFlags(xiiBitflags<xiiGALMiscBufferFlags> miscFlags) { m_MiscFlags = miscFlags; }

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
