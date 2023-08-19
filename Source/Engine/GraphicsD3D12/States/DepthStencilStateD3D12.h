#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/DepthStencilState.h>

class XII_GRAPHICSD3D12_DLL xiiGALDepthStencilStateD3D12 : public xiiGALDepthStencilState
{
public:
protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALDepthStencilStateD3D12(const xiiGALDepthStencilStateCreationDescription& creationDescription);

  virtual ~xiiGALDepthStencilStateD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;
};

#include <GraphicsD3D12/States/Implementation/DepthStencilStateD3D12_inl.h>
