#pragma once

#include <RendererFoundation/Resources/Query.h>

struct ID3D11Query;

class xiiGALQueryDX11 : public xiiGALQuery
{
public:
  XII_ALWAYS_INLINE ID3D11Query* GetDXQuery() const;

protected:
  friend class xiiGALDeviceDX11;
  friend class xiiMemoryUtils;

  xiiGALQueryDX11(const xiiGALQueryCreationDescription& Description);
  ~xiiGALQueryDX11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  ID3D11Query* m_pDXQuery;
};

#include <RendererDX11/Resources/Implementation/QueryDX11_inl.h>
