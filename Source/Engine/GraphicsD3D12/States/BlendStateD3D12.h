#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/BlendState.h>

struct D3D12_BLEND_DESC;

class XII_GRAPHICSD3D12_DLL xiiGALBlendStateD3D12 : public xiiGALBlendState
{
public:
  XII_ALWAYS_INLINE const D3D12_BLEND_DESC* GetBlendState() const;

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALBlendStateD3D12(const xiiGALBlendStateCreationDescription& creationDescription);

  virtual ~xiiGALBlendStateD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);

protected:
  D3D12_BLEND_DESC m_BlendState = {};
};

#include <GraphicsD3D12/States/Implementation/BlendStateD3D12_inl.h>
