#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>

class XII_GRAPHICSD3D12_DLL xiiGALInputLayoutD3D12 final : public xiiGALInputLayout
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALInputLayoutD3D12, xiiGALInputLayout);

public:
protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALInputLayoutD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALInputLayoutCreationDescription& creationDescription);

  virtual ~xiiGALInputLayoutD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
};

#include <GraphicsD3D12/Shader/Implementation/InputLayoutD3D12_inl.h>
