
#pragma once

#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

class XII_RENDERERFOUNDATION_DLL xiiGALComputeCommandEncoder : public xiiGALCommandEncoder
{
public:
  xiiGALComputeCommandEncoder(xiiGALDevice& ref_device, xiiGALCommandEncoderState& ref_state, xiiGALCommandEncoderCommonPlatformInterface& ref_commonImpl, xiiGALCommandEncoderComputePlatformInterface& ref_computeImpl);
  virtual ~xiiGALComputeCommandEncoder();

  // Dispatch

  void Dispatch(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ);
  void DispatchIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);

  virtual void ClearStatisticsCounters() override;

private:
  void CountDispatchCall() { m_uiDispatchCalls++; }

  // Statistic variables
  xiiUInt32 m_uiDispatchCalls = 0;

  xiiGALCommandEncoderComputePlatformInterface& m_ComputeImpl;
};
