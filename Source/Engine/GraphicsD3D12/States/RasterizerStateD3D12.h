#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/RasterizerState.h>

class XII_GRAPHICSD3D12_DLL xiiGALRasterizerStateD3D12 : public xiiGALRasterizerState
{
public:
  XII_ALWAYS_INLINE const Diligent::RasterizerStateDesc* GetRasterizerState() const;

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALRasterizerStateD3D12(const xiiGALRasterizerStateCreationDescription& creationDescription);

  virtual ~xiiGALRasterizerStateD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);

protected:
  Diligent::RasterizerStateDesc m_RasterizerState = {};
};

#include <GraphicsD3D12/States/Implementation/RasterizerStateD3D12_inl.h>
