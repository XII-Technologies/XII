#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandEncoder.h>

class XII_GRAPHICSFOUNDATION_DLL xiiGALComputeCommandEncoder : public xiiGALCommandEncoder
{
public:
  xiiGALComputeCommandEncoder(xiiGALDevice& ref_device, xiiGALCommandEncoderState& ref_state, xiiGALCommandEncoderCommonPlatformInterface& ref_commonImpl, xiiGALCommandEncoderComputePlatformInterface& ref_computeImpl);

  virtual ~xiiGALComputeCommandEncoder();

  // Dispatch

  void Dispatch(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ);
  void DispatchIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);

  virtual void ClearStatisticsCounters() override;

private:
  void CountDispatchCall();

  // Statistic variables
  xiiUInt32 m_uiDispatchCalls = 0U;

  xiiGALCommandEncoderComputePlatformInterface& m_ComputeImpl;
};

#include <GraphicsFoundation/CommandEncoder/Implementation/ComputeCommandEncoder_inl.h>
