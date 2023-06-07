#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

xiiGALComputeCommandEncoder::xiiGALComputeCommandEncoder(xiiGALDevice& ref_device, xiiGALCommandEncoderState& ref_state, xiiGALCommandEncoderCommonPlatformInterface& ref_commonImpl, xiiGALCommandEncoderComputePlatformInterface& ref_computeImpl) :
  xiiGALCommandEncoder(ref_device, ref_state, ref_commonImpl), m_ComputeImpl(ref_computeImpl)
{
}

xiiGALComputeCommandEncoder::~xiiGALComputeCommandEncoder() = default;

void xiiGALComputeCommandEncoder::Dispatch(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  AssertRenderingThread();

  XII_ASSERT_DEBUG(uiThreadGroupCountX > 0 && uiThreadGroupCountY > 0 && uiThreadGroupCountZ > 0, "Thread group counts of zero are not meaningful. Did you mean 1?");

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

  /// \todo Assert that the buffer can be used for indirect arguments (flag in desc)
  m_ComputeImpl.DispatchIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);

  CountDispatchCall();
}

void xiiGALComputeCommandEncoder::ClearStatisticsCounters()
{
  xiiGALCommandEncoder::ClearStatisticsCounters();

  m_uiDispatchCalls = 0;
}


XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_CommandEncoder_Implementation_ComputeCommandEncoder);
