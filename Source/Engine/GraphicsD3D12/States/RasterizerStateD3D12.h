#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/RasterizerState.h>

class XII_GRAPHICSD3D12_DLL xiiGALRasterizerStateD3D12 : public xiiGALRasterizerState
{
public:
protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALRasterizerStateD3D12(const xiiGALRasterizerStateCreationDescription& creationDescription);

  virtual ~xiiGALRasterizerStateD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;
};

#include <GraphicsD3D12/States/Implementation/RasterizerStateD3D12_inl.h>
