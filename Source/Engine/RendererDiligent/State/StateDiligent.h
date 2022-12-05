
#pragma once

#include <RendererFoundation/State/State.h>

class XII_RENDERERDILIGENT_DLL xiiGALBlendStateDiligent : public xiiGALBlendState
{
public:
  XII_ALWAYS_INLINE const Diligent::BlendStateDesc* GetBlendStateDesc() const;

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  xiiGALBlendStateDiligent(const xiiGALBlendStateCreationDescription& Description);

  ~xiiGALBlendStateDiligent();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  Diligent::BlendStateDesc m_BlendState{};
};

class XII_RENDERERDILIGENT_DLL xiiGALDepthStencilStateDiligent : public xiiGALDepthStencilState
{
public:
  XII_ALWAYS_INLINE const Diligent::DepthStencilStateDesc* GetDepthStencilStateDesc() const;

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  xiiGALDepthStencilStateDiligent(const xiiGALDepthStencilStateCreationDescription& Description);

  ~xiiGALDepthStencilStateDiligent();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  Diligent::DepthStencilStateDesc m_DepthStencilState{};
};

class XII_RENDERERDILIGENT_DLL xiiGALRasterizerStateDiligent : public xiiGALRasterizerState
{
public:
  XII_ALWAYS_INLINE const Diligent::RasterizerStateDesc* GetRasterizerStateDesc() const;

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  xiiGALRasterizerStateDiligent(const xiiGALRasterizerStateCreationDescription& Description);

  ~xiiGALRasterizerStateDiligent();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  Diligent::RasterizerStateDesc m_RasterizerState{};
};

class XII_RENDERERDILIGENT_DLL xiiGALSamplerStateDiligent : public xiiGALSamplerState
{
public:
  XII_ALWAYS_INLINE Diligent::RefCntAutoPtr<Diligent::ISampler>& GetSamplerState();

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  xiiGALSamplerStateDiligent(const xiiGALSamplerStateCreationDescription& Description);

  ~xiiGALSamplerStateDiligent();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  Diligent::RefCntAutoPtr<Diligent::ISampler> m_pSamplerState;
};


#include <RendererDiligent/State/Implementation/StateDiligent_inl.h>
