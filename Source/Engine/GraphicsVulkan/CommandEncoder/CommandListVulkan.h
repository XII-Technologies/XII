#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>

class XII_GRAPHICSVULKAN_DLL xiiGALCommandListVulkan final : public xiiGALCommandList
{
public:
  Diligent::ICommandList* GetCommandList() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALCommandListVulkan(const xiiGALCommandListCreationDescription& creationDescription);

  virtual ~xiiGALCommandListVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::ICommandList* m_pCommandList = nullptr;
};

#include <GraphicsVulkan/CommandEncoder/Implementation/CommandListVulkan_inl.h>
