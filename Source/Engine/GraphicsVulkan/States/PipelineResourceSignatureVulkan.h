#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/PipelineResourceSignature.h>

class XII_GRAPHICSVULKAN_DLL xiiGALPipelineResourceSignatureVulkan final : public xiiGALPipelineResourceSignature
{
public:
  virtual bool IsCompatibleWith(const xiiGALPipelineResourceSignature* pPipelineResourceSignature) const override final;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALPipelineResourceSignatureVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALPipelineResourceSignatureCreationDescription& creationDescription);

  virtual ~xiiGALPipelineResourceSignatureVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
};

#include <GraphicsVulkan/States/Implementation/PipelineResourceSignatureVulkan_inl.h>
