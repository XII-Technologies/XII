#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>

class XII_GRAPHICSD3D12_DLL xiiGALCommandEncoderD3D12 final : public xiiGALCommandEncoderCommonPlatformInterface, public xiiGALCommandEncoderGraphicsPlatformInterface, public xiiGALCommandEncoderComputePlatformInterface
{
public:
  xiiGALCommandEncoderD3D12(xiiGALDeviceD3D12& deviceD3D12);
  ~xiiGALCommandEncoderD3D12();

private:
  friend class xiiGALPassD3D12;

  xiiGALDeviceD3D12&    m_GALDeviceD3D12;
  xiiGALCommandEncoder* m_pOwner = nullptr;
};

#include <GraphicsD3D12/CommandEncoder/Implementation/CommandEncoderD3D12_inl.h>
