#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Query.h>

class XII_GRAPHICSVULKAN_DLL xiiGALQueryVulkan final : public xiiGALQuery
{
public:
  virtual bool GetData(void* pData, xiiUInt32 uiDataSize, bool bAutoInvalidate = true) override;

  virtual void Invalidate() override;

  Diligent::IQuery* GetQuery() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALQueryVulkan(const xiiGALQueryCreationDescription& creationDescription);

  virtual ~xiiGALQueryVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::IQuery* m_pQuery = nullptr;
};

#include <GraphicsVulkan/Resources/Implementation/QueryVulkan_inl.h>
