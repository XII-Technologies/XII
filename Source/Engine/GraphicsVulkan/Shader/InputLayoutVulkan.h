#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>

class XII_GRAPHICSVULKAN_DLL xiiGALInputLayoutVulkan final : public xiiGALInputLayout
{
public:
  const Diligent::InputLayoutDesc* GetLayout() const;

  xiiArrayPtr<Diligent::LayoutElement> GetElements();

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALInputLayoutVulkan(const xiiGALInputLayoutCreationDescription& creationDescription);

  virtual ~xiiGALInputLayoutVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);

protected:
  Diligent::InputLayoutDesc                   m_InputLayout = {};
  xiiHybridArray<Diligent::LayoutElement, 8U> m_InputElements;
};

#include <GraphicsVulkan/Shader/Implementation/InputLayoutVulkan_inl.h>
