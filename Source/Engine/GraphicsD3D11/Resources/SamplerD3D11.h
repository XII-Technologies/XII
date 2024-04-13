#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Resources/Sampler.h>

struct ID3D11SamplerState;

class XII_GRAPHICSD3D11_DLL xiiGALSamplerD3D11 final : public xiiGALSampler
{
public:
  ID3D11SamplerState* GetSampler() const;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALSamplerD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALSamplerCreationDescription& creationDescription);

  virtual ~xiiGALSamplerD3D11();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
  ID3D11SamplerState* m_pSampler = nullptr;
};

#include <GraphicsD3D11/Resources/Implementation/SamplerD3D11_inl.h>
