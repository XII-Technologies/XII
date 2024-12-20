#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Query.h>

class XII_GRAPHICSVULKAN_DLL xiiGALQueryVulkan final : public xiiGALQuery
{
public:
  virtual bool GetData(void* pData, xiiUInt32 uiDataSize, bool bAutoInvalidate = true) override final;

  virtual void Invalidate() override final;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALQueryVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALQueryCreationDescription& creationDescription);

  virtual ~xiiGALQueryVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

protected:
};
