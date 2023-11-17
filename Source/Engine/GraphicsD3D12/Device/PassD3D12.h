#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <Foundation/Types/UniquePtr.h>

#include <GraphicsFoundation/Device/Pass.h>

class XII_GRAPHICSD3D12_DLL xiiGALPassD3D12 final : public xiiGALPass
{
public:
protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALPassD3D12(xiiGALDevice& device);
  virtual ~xiiGALPassD3D12();

  virtual xiiGALGraphicsCommandEncoder* BeginRenderingPlatform(xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiStringView sName = {}) override;
  virtual void                          EndRenderingPlatform(xiiGALGraphicsCommandEncoder* pCommandEncoder) override;

  virtual xiiGALComputeCommandEncoder* BeginComputePlatform(xiiStringView sName = {}) override;
  virtual void                         EndComputePlatform(xiiGALComputeCommandEncoder* pCommandEncoder) override;

private:
  xiiUniquePtr<xiiGALCommandEncoderGraphicsState> m_pCommandEncoderState;
  xiiUniquePtr<xiiGALCommandEncoderD3D12>         m_pCommandEncoderImpl;

  xiiUniquePtr<xiiGALGraphicsCommandEncoder> m_pGraphicsCommandEncoder;
  xiiUniquePtr<xiiGALComputeCommandEncoder>  m_pComputeCommandEncoder;

  xiiGALDeviceD3D12& m_GALDeviceD3D12;
};

#include <GraphicsD3D12/Device/Implementation/PassD3D12_inl.h>
