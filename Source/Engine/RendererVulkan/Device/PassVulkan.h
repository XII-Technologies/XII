
#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Pass.h>

struct xiiGALCommandEncoderRenderState;
class xiiGALRenderCommandEncoder;
class xiiGALComputeCommandEncoder;
class xiiGALCommandEncoderImplVulkan;

class xiiGALPassVulkan : public xiiGALPass
{
protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;
  void Reset();
  void MarkDirty();
  void SetCurrentCommandBuffer(vk::CommandBuffer* commandBuffer, xiiPipelineBarrierVulkan* pipelineBarrier);

  virtual xiiGALRenderCommandEncoder* BeginRenderingPlatform(const xiiGALRenderingSetup& renderingSetup, const char* szName) override;
  virtual void                        EndRenderingPlatform(xiiGALRenderCommandEncoder* pCommandEncoder) override;

  virtual xiiGALComputeCommandEncoder* BeginComputePlatform(const char* szName) override;
  virtual void                         EndComputePlatform(xiiGALComputeCommandEncoder* pCommandEncoder) override;

  xiiGALPassVulkan(xiiGALDevice& device);
  virtual ~xiiGALPassVulkan();

  void BeginPass(const char* szName);
  void EndPass();

private:
  xiiUniquePtr<xiiGALCommandEncoderRenderState> m_pCommandEncoderState;
  xiiUniquePtr<xiiGALCommandEncoderImplVulkan>  m_pCommandEncoderImpl;

  xiiUniquePtr<xiiGALRenderCommandEncoder>  m_pRenderCommandEncoder;
  xiiUniquePtr<xiiGALComputeCommandEncoder> m_pComputeCommandEncoder;
};
