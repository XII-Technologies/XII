#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/BufferView.h>

class XII_GRAPHICSD3D12_DLL xiiGALBufferViewD3D12 final : public xiiGALBufferView
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALBufferViewD3D12, xiiGALBufferView);

public:
protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALBufferViewD3D12(xiiGALDeviceD3D12* pDeviceD3D12, xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& creationDescription);

  virtual ~xiiGALBufferViewD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
};
