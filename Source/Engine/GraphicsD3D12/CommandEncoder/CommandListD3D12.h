#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>

class XII_GRAPHICSD3D12_DLL xiiGALCommandListD3D12 final : public xiiGALCommandList
{
public:
  Diligent::ICommandList* GetCommandList() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALCommandListD3D12(const xiiGALCommandListCreationDescription& creationDescription);

  virtual ~xiiGALCommandListD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::ICommandList* m_pCommandList = nullptr;
};

#include <GraphicsD3D12/CommandEncoder/Implementation/CommandListD3D12_inl.h>
