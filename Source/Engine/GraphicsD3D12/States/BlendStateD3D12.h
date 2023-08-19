#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/BlendState.h>

class XII_GRAPHICSD3D12_DLL xiiGALBlendStateD3D12 : public xiiGALBlendState
{
public:
protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALBlendStateD3D12(const xiiGALBlendStateCreationDescription& creationDescription);

  virtual ~xiiGALBlendStateD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;
};

#include <GraphicsD3D12/States/Implementation/BlendStateD3D12_inl.h>
