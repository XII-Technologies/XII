#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/PipelineResourceSignature.h>

class XII_GRAPHICSVULKAN_DLL xiiGALPipelineResourceSignatureVulkan final : public xiiGALPipelineResourceSignature
{
public:
  virtual bool IsCompatibleWith(const xiiGALPipelineResourceSignature* pPipelineResourceSignature) const override final;

  Diligent::IPipelineResourceSignature* GetPipelineResourceSignature() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALPipelineResourceSignatureVulkan(const xiiGALPipelineResourceSignatureCreationDescription& creationDescription);

  virtual ~xiiGALPipelineResourceSignatureVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  xiiHybridArray<Diligent::ImmutableSamplerDesc, 2U> m_ImmutableSamplers;
  xiiHybridArray<Diligent::PipelineResourceDesc, 2U> m_PipelineResourceDescriptions;

  Diligent::IPipelineResourceSignature* m_pPipelineResourceSignature = nullptr;
};

#include <GraphicsVulkan/States/Implementation/PipelineResourceSignatureVulkan_inl.h>
