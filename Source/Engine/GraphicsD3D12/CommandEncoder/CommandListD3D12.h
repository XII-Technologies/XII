#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>

class XII_GRAPHICSD3D12_DLL xiiGALCommandListD3D12 final : public xiiGALCommandList
{
public:
  XII_ALWAYS_INLINE const Diligent::ICommandList* GetCommandList() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALCommandListD3D12(const xiiGALCommandListCreationDescription& creationDescription);

  virtual ~xiiGALCommandListD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::RefCntAutoPtr<Diligent::ICommandList> m_pCommandList;
};

#include <GraphicsD3D12/CommandEncoder/Implementation/CommandListD3D12_inl.h>
