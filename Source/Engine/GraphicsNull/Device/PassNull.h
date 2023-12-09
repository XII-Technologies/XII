#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <Foundation/Types/UniquePtr.h>

#include <GraphicsFoundation/Device/Pass.h>

class XII_GRAPHICSNULL_DLL xiiGALPassNull final : public xiiGALPass
{
public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALPassNull(xiiGALDevice& device);
  virtual ~xiiGALPassNull();

  virtual xiiGALGraphicsCommandEncoder* BeginRenderingPlatform(const xiiGALRenderingSetup& renderingSetup, xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiStringView sName = {}) override final;
  virtual void                          EndRenderingPlatform(xiiGALGraphicsCommandEncoder* pCommandEncoder) override final;

  virtual xiiGALComputeCommandEncoder* BeginComputePlatform(xiiStringView sName = {}) override final;
  virtual void                         EndComputePlatform(xiiGALComputeCommandEncoder* pCommandEncoder) override final;

private:
  xiiUniquePtr<xiiGALCommandEncoderGraphicsState> m_pCommandEncoderState;
  xiiUniquePtr<xiiGALCommandEncoderNull>         m_pCommandEncoderImpl;

  xiiUniquePtr<xiiGALGraphicsCommandEncoder> m_pGraphicsCommandEncoder;
  xiiUniquePtr<xiiGALComputeCommandEncoder>  m_pComputeCommandEncoder;

  xiiGALDeviceNull& m_GALDeviceNull;
};

#include <GraphicsNull/Device/Implementation/PassNull_inl.h>
