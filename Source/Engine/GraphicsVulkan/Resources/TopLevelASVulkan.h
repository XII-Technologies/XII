#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/TopLevelAS.h>

class XII_GRAPHICSVULKAN_DLL xiiGALTopLevelASVulkan final : public xiiGALTopLevelAS
{
public:
  virtual xiiGALTopLevelASInstanceDescription GetInstanceDescription(xiiStringView sName) const override;

  virtual xiiGALTopLevelASBuildDescription GetBuildDescription() const override;

  virtual xiiGALScratchBufferSizeDescription GetScratchBufferSizeDescription() const override;

  virtual void SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) override;

  virtual xiiBitflags<xiiGALResourceStateFlags> GetState() const override;

  XII_ALWAYS_INLINE const Diligent::ITopLevelAS* GetTopLevelAS() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALTopLevelASVulkan(const xiiGALTopLevelASCreationDescription& creationDescription);

  virtual ~xiiGALTopLevelASVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::RefCntAutoPtr<Diligent::ITopLevelAS> m_pTopLevelAS;
};

#include <GraphicsVulkan/Resources/Implementation/TopLevelASVulkan_inl.h>
