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

  virtual xiiGALGraphicsCommandEncoder* BeginRenderingPlatform(const xiiGALRenderingSetup& renderingSetup, xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiStringView sName = {}) override;
  virtual void                          EndRenderingPlatform(xiiGALGraphicsCommandEncoder* pCommandEncoder) override;

  virtual xiiGALComputeCommandEncoder* BeginComputePlatform(xiiStringView sName = {}) override;
  virtual void                         EndComputePlatform(xiiGALComputeCommandEncoder* pCommandEncoder) override;

private:
  xiiUniquePtr<xiiGALCommandEncoderGraphicsState> m_pCommandEncoderState;
  xiiUniquePtr<xiiGALCommandEncoderVulkan>         m_pCommandEncoderImpl;

  xiiUniquePtr<xiiGALGraphicsCommandEncoder> m_pGraphicsCommandEncoder;
  xiiUniquePtr<xiiGALComputeCommandEncoder>  m_pComputeCommandEncoder;

  xiiGALDeviceVulkan& m_GALDeviceVulkan;
};

#include <GraphicsVulkan/Device/Implementation/PassVulkan_inl.h>
