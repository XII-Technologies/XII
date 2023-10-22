#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>

xiiGALComputeCommandEncoder::xiiGALComputeCommandEncoder(xiiGALDevice& ref_device, xiiGALCommandEncoderState& ref_state, xiiGALCommandEncoderCommonPlatformInterface& ref_commonImpl, xiiGALCommandEncoderComputePlatformInterface& ref_computeImpl) :
  xiiGALCommandEncoder(ref_device, ref_state, ref_commonImpl), m_ComputeImpl(ref_computeImpl)
{
}

xiiGALComputeCommandEncoder::~xiiGALComputeCommandEncoder() = default;

void xiiGALComputeCommandEncoder::Dispatch(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  AssertRenderingThread();

  XII_ASSERT_DEBUG(uiThreadGroupCountX > 0U && uiThreadGroupCountY > 0U && uiThreadGroupCountZ > 0U, "Thread group counts of zero are redundant. Did you mean 1?");

  /// \todo Assert for compute

  m_ComputeImpl.DispatchPlatform(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);

  CountDispatchCall();
}

void xiiGALComputeCommandEncoder::DispatchIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();
  /// \todo Assert for compute
  /// \todo Assert for indirect dispatch
  /// \todo Assert offset < buffer size

  const xiiGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  XII_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

  XII_ASSERT_DEV(pBuffer->GetDescription().m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "The buffer must be created with the xiiGALBindFlags::IndirectDrawArguments bind flag.");

  m_ComputeImpl.DispatchIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);

  CountDispatchCall();
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_ComputeCommandEncoder);
