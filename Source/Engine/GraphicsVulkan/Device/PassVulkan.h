#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <Foundation/Types/UniquePtr.h>

#include <GraphicsFoundation/Device/Pass.h>

class XII_GRAPHICSVULKAN_DLL xiiGALPassVulkan final : public xiiGALPass
{
public:
protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALPassVulkan(xiiGALDevice& device);
  virtual ~xiiGALPassVulkan();

  virtual xiiGALGraphicsCommandEncoder* BeginRenderingPlatform(const xiiGALRenderingSetup& renderingSetup, xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiStringView sName = {}) override final;
  virtual void                          EndRenderingPlatform(xiiGALGraphicsCommandEncoder* pCommandEncoder) override final;

  virtual xiiGALComputeCommandEncoder* BeginComputePlatform(xiiStringView sName = {}) override final;
  virtual void                         EndComputePlatform(xiiGALComputeCommandEncoder* pCommandEncoder) override final;

private:
  xiiUniquePtr<xiiGALCommandEncoderGraphicsState> m_pCommandEncoderState;
  xiiUniquePtr<xiiGALCommandEncoderVulkan>        m_pCommandEncoderImpl;

  xiiUniquePtr<xiiGALGraphicsCommandEncoder> m_pGraphicsCommandEncoder;
  xiiUniquePtr<xiiGALComputeCommandEncoder>  m_pComputeCommandEncoder;

  xiiGALDeviceVulkan& m_GALDeviceVulkan;
};

#include <GraphicsVulkan/Device/Implementation/PassVulkan_inl.h>
