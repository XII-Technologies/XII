#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/BottomLevelAS.h>

class XII_GRAPHICSVULKAN_DLL xiiGALBottomLevelASVulkan final : public xiiGALBottomLevelAS
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALBottomLevelASVulkan, xiiGALBottomLevelAS);

public:
protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBottomLevelASVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALBottomLevelASCreationDescription& creationDescription);

  virtual ~xiiGALBottomLevelASVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
};
