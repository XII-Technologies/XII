#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/TopLevelAS.h>

class XII_GRAPHICSVULKAN_DLL xiiGALTopLevelASVulkan final : public xiiGALTopLevelAS
{
public:
  virtual xiiGALTopLevelASInstanceDescription GetInstanceDescription(xiiStringView sName) const override final;

  virtual xiiGALTopLevelASBuildDescription GetBuildDescription() const override final;

  virtual xiiGALScratchBufferSizeDescription GetScratchBufferSizeDescription() const override final;

  virtual void SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) override final;

  virtual xiiBitflags<xiiGALResourceStateFlags> GetState() const override final;

  Diligent::ITopLevelAS* GetTopLevelAS() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALTopLevelASVulkan(const xiiGALTopLevelASCreationDescription& creationDescription);

  virtual ~xiiGALTopLevelASVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::ITopLevelAS* m_pTopLevelAS = nullptr;
};

#include <GraphicsVulkan/Resources/Implementation/TopLevelASVulkan_inl.h>
